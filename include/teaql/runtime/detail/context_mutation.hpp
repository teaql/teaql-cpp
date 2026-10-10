#pragma once
#include <teaql/runtime/detail/data_service.hpp>
#include <teaql/runtime/detail/query_keys.hpp>
#include <teaql/runtime/cache.hpp>

namespace teaql::detail {
struct MutationOperationName {
    using allocator_type = Allocator;
    String entity;
    std::string_view action;

    MutationOperationName(std::allocator_arg_t,
                          Allocator a,
                          std::string_view entity,
                          std::string_view action)
        : entity(entity, a.resource()), action(action) {}

    MutationOperationName(std::allocator_arg_t, Allocator a, const MutationOperationName& other)
        : MutationOperationName(std::allocator_arg, a, other.entity, other.action) {}

    String operator()() const {
        String name(entity, entity.get_allocator());
        name += '.';
        name += action;
        return name;
    }
};

template <class E, class C>
class ContextMutationOperation {
    using R = Result<std::uint64_t, DataServiceError<typename E::Error>>;

public:
    using allocator_type = Allocator;

    ContextMutationOperation(std::allocator_arg_t,
                             Allocator a,
                             const ContextDataService<E>& service,
                             const C& command,
                             std::string_view action)
        : service_(std::allocator_arg, a, service), command_(std::allocator_arg, a, command),
          action_(action) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_) {
            auto base = service_.data_service();
            auto& work = [&]() -> AsyncOperation<R>& {
                if constexpr (std::same_as<C, InsertCommand>) {
                    return base.insert(command_);
                } else if constexpr (std::same_as<C, UpdateCommand>) {
                    return base.update(command_);
                } else if constexpr (std::same_as<C, DeleteCommand>) {
                    return base.delete_command(command_);
                } else if constexpr (std::same_as<C, RecoverCommand>) {
                    return base.recover(command_);
                } else if constexpr (std::same_as<C, BatchInsertCommand>) {
                    return base.batch_insert(command_);
                } else {
                    return base.batch_update(command_);
                }
            }();
            child_ = &service_.observe(
                "mutation",
                MutationOperationName(std::allocator_arg, Allocator(r), command_.entity, action_),
                command_.entity,
                work);
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (*result) {
            auto& context = const_cast<UserContext&>(service_.metadata.context());
            auto invalidate = [&](AggregationCacheBackend& cache) {
                String name(cache.name_space(), r);
                name += "::";
                name += aggregation_cache_namespace(r, command_.entity);
                cache.invalidate_namespace(name);
            };
            if (auto* cache = context.template get_resource<AggregationCacheBackend>()) {
                invalidate(*cache);
            }
            if (auto* cache = context.template get_resource<InMemoryAggregationCache>()) {
                invalidate(*cache);
            }
        }
        return copy_or_take_completed(*child_, *result);
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
    ContextDataService<E> service_;
    C command_;
    std::string_view action_;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

#define TEAQL_CONTEXT_MUTATION_METHOD(Name, Command, Action)                                       \
    template <class E>                                                                             \
    auto ContextDataService<E>::Name(const Command& command)                                       \
        const -> DataServiceOperationResult<std::uint64_t, DataServiceError<Error>>& {             \
        using R = Result<std::uint64_t, DataServiceError<Error>>;                                  \
        return make_operation<R, ContextMutationOperation<E, Command>>(                            \
            memory_resource(), *this, command, Action);                                            \
    }
TEAQL_CONTEXT_MUTATION_METHOD(insert, InsertCommand, "insert")
TEAQL_CONTEXT_MUTATION_METHOD(update, UpdateCommand, "update")
TEAQL_CONTEXT_MUTATION_METHOD(delete_command, DeleteCommand, "delete")
TEAQL_CONTEXT_MUTATION_METHOD(recover, RecoverCommand, "recover")
TEAQL_CONTEXT_MUTATION_METHOD(batch_insert, BatchInsertCommand, "batch_insert")
TEAQL_CONTEXT_MUTATION_METHOD(batch_update, BatchUpdateCommand, "batch_update")
#undef TEAQL_CONTEXT_MUTATION_METHOD
}
