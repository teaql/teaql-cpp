#pragma once
#include <teaql/runtime/detail/data_service/data_service.hpp>
#include <teaql/core/detail/debug.hpp>
#include <charconv>

namespace teaql::detail {
inline std::uint64_t mutation_count_add(std::uint64_t total, std::uint64_t value) {
#ifndef NDEBUG
    if (value > UINT64_MAX - total) {
        throw std::overflow_error("attempt to add with overflow");
    }
#endif
    return total + value;
}

inline String mutation_conflict_id(Memory r, const Value& id, bool debug_only = false) {
    if (!debug_only) {
        if (auto number = id.try_u64()) {
            char buffer[32];
            auto end = std::to_chars(buffer, buffer + sizeof(buffer), *number).ptr;
            return String(buffer, end, r);
        }
    }
    return debug_value(r, id);
}

template <class C>
struct MutationSequence {
    using allocator_type = Allocator;
    Vector<C> commands;

    MutationSequence(std::allocator_arg_t, Allocator a, std::span<const C> values)
        : commands(values.begin(), values.end(), a.resource()) {}

    MutationSequence(std::allocator_arg_t, Allocator a, const MutationSequence& other)
        : commands(other.commands, a.resource()) {}
};
template <class C>
inline constexpr bool mutation_batch =
    std::same_as<C, BatchInsertCommand> || std::same_as<C, BatchUpdateCommand>;

template <class C>
std::size_t mutation_size(const C& command) {
    if constexpr (mutation_batch<C>) {
        return command.batch_values.size();
    } else if constexpr (requires { command.commands; }) {
        return command.commands.size();
    } else {
        return 1;
    }
}

template <class C>
MutationRequest mutation_item(Memory r, const C& command, std::size_t index) {
    auto result = [&] {
        if constexpr (std::same_as<C, BatchInsertCommand>) {
            InsertCommand out(std::allocator_arg, Allocator(r), command.entity);
            out.values = clone_in(r, command.batch_values.at(index));
            if (index < command.trace_chains.size()) {
                TraceChain trace(command.trace_chains[index], r);
                out.trace_chain.swap(trace);
            }
            return MutationRequest(std::allocator_arg, Allocator(r), std::move(out));
        } else if constexpr (std::same_as<C, BatchUpdateCommand>) {
            UpdateCommand out(
                std::allocator_arg, Allocator(r), command.entity, command.batch_ids.at(index));
            const auto& values = command.batch_values.at(index).fields;
            for (const auto& field : command.update_fields) {
                auto found = values.find(field);
                if (found != values.end()) {
                    out.values.fields.insert_or_assign(String(field, r), Value(r, found->second));
                }
            }
            if (index < command.batch_expected_versions.size()) {
                out.expected_version_value = command.batch_expected_versions[index];
            }
            if (index < command.batch_old_values.size()) {
                out.old_values = clone_in(r, command.batch_old_values[index]);
            }
            if (index < command.trace_chains.size()) {
                TraceChain trace(command.trace_chains[index], r);
                out.trace_chain.swap(trace);
            }
            return MutationRequest(std::allocator_arg, Allocator(r), std::move(out));
        } else if constexpr (requires { command.commands; }) {
            return MutationRequest(std::allocator_arg, Allocator(r), command.commands.at(index));
        } else {
            return MutationRequest(std::allocator_arg, Allocator(r), command);
        }
    }();
    std::visit(
        [](auto& cmd) {
            using T = std::decay_t<decltype(cmd)>;
            if constexpr (!std::same_as<T, MutationRequest::Batch>) {
                if (std::none_of(
                        cmd.trace_chain.begin(), cmd.trace_chain.end(), [&](const TraceNode& node) {
                            return node.kind == TraceKind::Entity && node.entity_type == cmd.entity;
                        })) {
                    cmd.trace_chain.emplace_back(TraceKind::Entity, cmd.entity, "");
                }
            }
        },
        result.data);
    return result;
}

template <class M, class E, class C>
class BaseMutationOperation {
    using Error = typename E::Error;
    using R = Result<std::uint64_t, DataServiceError<Error>>;

public:
    using allocator_type = Allocator;

    BaseMutationOperation(std::allocator_arg_t,
                          Allocator a,
                          const ResourceBorrow<const M>& metadata,
                          const ResourceBorrow<E>& executor,
                          const C& command)
        : resource_(a.resource()), metadata_(std::allocator_arg, a, metadata),
          executor_(std::allocator_arg, a, executor), command_(std::allocator_arg, a, command) {}

    std::optional<R> poll(WakeSignal& wake) {
        while (index_ < mutation_size(command_)) {
            if (!child_) {
                request_.emplace(mutation_item(resource_, command_, index_));
                auto& work = executor_.get().mutate_observed(
                    resource_,
                    clone_in(resource_, *request_),
                    metadata_.get().mutation_diagnostic_observer(resource_));
                if (work.memory_resource() != resource_) {
                    work.cancel();
                    throw std::invalid_argument(
                        "Runtime mutation must belong to its result Context");
                }
                child_ = &work;
            }
            forwarding_.attach(child_->wake_signal(), wake);
            auto* result = child_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            auto* completed = std::exchange(child_, nullptr);
            if (!*result) {
                return R::err(resource_,
                              DataServiceError<Error>::executor(
                                  resource_, copy_or_take_error(*completed, *result)));
            }
            metadata_.get().record_metadata_log(result->value().metadata);
            auto affected = result->value().affected_rows;
            auto conflict = std::visit(
                [&](const auto& cmd) -> std::optional<RuntimeError> {
                    using T = std::decay_t<decltype(cmd)>;
                    if constexpr (std::same_as<T, UpdateCommand> ||
                                  std::same_as<T, DeleteCommand>) {
                        if (cmd.expected_version_value && affected == 0) {
                            return RuntimeError::optimistic_lock_conflict(
                                resource_, cmd.entity, mutation_conflict_id(resource_, cmd.id));
                        }
                    } else if constexpr (std::same_as<T, RecoverCommand>) {
                        if (affected == 0) {
                            return RuntimeError::optimistic_lock_conflict(
                                resource_,
                                cmd.entity,
                                mutation_conflict_id(resource_, cmd.id, true));
                        }
                    }
                    return {};
                },
                request_->data);
            if (conflict) {
                return R::err(resource_,
                              DataServiceError<Error>::runtime(resource_, std::move(*conflict)));
            }
            total_ = mutation_count_add(total_, affected);
            request_.reset();
            ++index_;
        }
        if constexpr (std::same_as<C, BatchUpdateCommand>) {
            if (std::any_of(command_.batch_expected_versions.begin(),
                            command_.batch_expected_versions.end(),
                            [](auto version) {
                                return version.has_value();
                            }) &&
                total_ != command_.batch_ids.size()) {
                String message("batch update for ", resource_);
                message += command_.entity;
                message += " affected ";
                append_number(message, total_);
                message += " rows for ";
                append_number(message, command_.batch_ids.size());
                message += " IDs despite version checks";
                return R::err(resource_,
                              DataServiceError<Error>::runtime(
                                  resource_, RuntimeError::graph(resource_, message)));
            }
        }
        return R::ok(resource_, total_);
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        child_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
    }

private:
    static void append_number(String& out, std::uint64_t n) {
        char buffer[32];
        auto end = std::to_chars(buffer, buffer + sizeof(buffer), n).ptr;
        out.append(buffer, end);
    }

    Memory resource_;
    ResourceBorrow<const M> metadata_;
    ResourceBorrow<E> executor_;
    C command_;
    std::optional<MutationRequest> request_;
    AsyncOperation<Result<MutationResult, Error>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
    std::size_t index_ = 0;
    std::uint64_t total_ = 0;
};

#define TEAQL_BASE_MUTATION_METHOD(Name, Command)                                                  \
    template <class M, class E>                                                                    \
    auto RuntimeDataService<M, E>::Name(const Command& command)                                    \
        const -> DataServiceOperationResult<std::uint64_t, DataServiceError<Error>>& {             \
        using R = Result<std::uint64_t, DataServiceError<Error>>;                                  \
        return make_operation<R, BaseMutationOperation<M, E, Command>>(                            \
            memory_resource(), metadata_, executor_, command);                                     \
    }
TEAQL_BASE_MUTATION_METHOD(insert, InsertCommand)
TEAQL_BASE_MUTATION_METHOD(update, UpdateCommand)
TEAQL_BASE_MUTATION_METHOD(delete_command, DeleteCommand)
TEAQL_BASE_MUTATION_METHOD(recover, RecoverCommand)
TEAQL_BASE_MUTATION_METHOD(batch_insert, BatchInsertCommand)
TEAQL_BASE_MUTATION_METHOD(batch_update, BatchUpdateCommand)
#undef TEAQL_BASE_MUTATION_METHOD
#define TEAQL_BASE_MUTATION_MANY(Name, Command)                                                    \
    template <class M, class E>                                                                    \
    auto RuntimeDataService<M, E>::Name(std::span<const Command> commands)                         \
        const -> DataServiceOperationResult<std::uint64_t, DataServiceError<Error>>& {             \
        using R = Result<std::uint64_t, DataServiceError<Error>>;                                  \
        MutationSequence<Command> snapshot(                                                        \
            std::allocator_arg, Allocator(memory_resource()), commands);                           \
        return make_operation<R, BaseMutationOperation<M, E, MutationSequence<Command>>>(          \
            memory_resource(), metadata_, executor_, snapshot);                                    \
    }
TEAQL_BASE_MUTATION_MANY(insert_many, InsertCommand)
TEAQL_BASE_MUTATION_MANY(update_many, UpdateCommand)
TEAQL_BASE_MUTATION_MANY(delete_many, DeleteCommand)
TEAQL_BASE_MUTATION_MANY(recover_many, RecoverCommand)
#undef TEAQL_BASE_MUTATION_MANY
}
