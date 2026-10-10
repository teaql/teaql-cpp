#pragma once
#include <teaql/runtime/detail/entity_graph_plan.hpp>
#include <bit>
#include <teaql/runtime/event.hpp>

namespace teaql {
template <class E>
Result<InsertCommand, RuntimeError>
EntityDataService<E>::prepare_insert_command(const InsertCommand& source) const {
    auto r = memory_resource();
    using R = Result<InsertCommand, RuntimeError>;
    Arena::Quiescence active;
    active.attach(detail::resource_arena(r), nullptr, [](void*) noexcept {
        std::terminate();
    });
    auto command = detail::clone_in(r, source);
    if (auto hook = behavior()) {
        auto result = hook->get().before_insert(r, context(), command);
        if (!result) {
            return R::err(r, std::move(result).error());
        }
    }
    if (auto* policy = detail::ContextEntityAccess::request_policy(context())) {
        auto result = policy->enforce_insert(r, context(), command);
        if (!result) {
            return R::err(r, std::move(result).error());
        }
    }
    auto descriptor = context().require_entity(command.entity);
    if (!descriptor) {
        return R::err(r, std::move(descriptor).error());
    }
    auto field = [&](std::string_view name) -> const Value* {
        auto it = command.values.fields.find(name);
        return it == command.values.fields.end() ? nullptr : &it->second;
    };
    if (auto* id = descriptor.value().get().id_property();
        id && detail::graph_unassigned(field(id->name))) {
        auto generated = context().next_id(command.entity);
        if (!generated) {
            return R::err(r, std::move(generated).error());
        }
        command.value(id->name, generated.value());
    }
    if (auto* version = descriptor.value().get().version_property();
        version && detail::graph_unassigned(field(version->name))) {
        command.value(version->name, std::int64_t(1));
    }
    EntityValues checked(std::allocator_arg, Allocator(r), command.values);
    mark_entity_status(checked, CheckObjectStatus::Create);
    auto result = context().check_and_fix_values(command.entity, checked);
    clear_entity_status(checked);
    if (!result) {
        return R::err(r, std::move(result).error());
    }
    command.values.fields = std::move(checked.fields);
    return R::ok(r, std::move(command));
}

}

namespace teaql::detail {
inline std::int64_t mutation_next_version(std::int64_t version) {
#ifndef NDEBUG
    if (version == INT64_MAX) {
        throw std::overflow_error("attempt to add with overflow");
    }
#endif
    return std::bit_cast<std::int64_t>(std::uint64_t(version) + 1);
}

template <class E, class C>
class EntityMutationOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<std::uint64_t, Error>;

public:
    using allocator_type = Allocator;

    EntityMutationOperation(std::allocator_arg_t,
                            Allocator a,
                            const EntityDataService<E>& service,
                            const C& command,
                            bool prepared,
                            std::span<const TraceNode> lineage)
        : service_(std::allocator_arg, a, service), command_(std::allocator_arg, a, command),
          lineage_(lineage.begin(), lineage.end(), a.resource()), prepared_(prepared) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& context = EntityDataServiceAccess::context(service_);
        if (!child_) {
            if constexpr (mutation_batch<C>) {
                if (command_.batch_values.empty()) {
                    return R::ok(r, 0);
                }
            }
            if (!prepared_) {
                if constexpr (std::same_as<C, InsertCommand>) {
                    auto prepared = service_.prepare_insert_command(command_);
                    if (!prepared) {
                        return fail(std::move(prepared).error());
                    }
                    command_ = std::move(prepared).value();
                } else if constexpr (std::same_as<C, UpdateCommand>) {
                    auto prepared = service_.prepare_update_command(command_);
                    if (!prepared) {
                        return fail(std::move(prepared).error());
                    }
                    command_ = std::move(prepared).value();
                }
            }
            if constexpr (!mutation_batch<C>) {
                TraceChain trace(lineage_, r);
                command_.trace_chain.swap(trace);
                if constexpr (std::same_as<C, DeleteCommand> || std::same_as<C, RecoverCommand>) {
                    if (auto hook = EntityDataServiceAccess::behavior(service_)) {
                        auto result = [&] {
                            if constexpr (std::same_as<C, DeleteCommand>) {
                                return hook->get().before_delete(r, context, command_);
                            } else {
                                return hook->get().before_recover(r, context, command_);
                            }
                        }();
                        if (!result) {
                            return fail(std::move(result).error());
                        }
                    }
                    if (auto* policy = ContextEntityAccess::request_policy(context)) {
                        auto result = [&] {
                            if constexpr (std::same_as<C, DeleteCommand>) {
                                return policy->enforce_delete(r, context, command_);
                            } else {
                                return policy->enforce_recover(r, context, command_);
                            }
                        }();
                        if (!result) {
                            return fail(std::move(result).error());
                        }
                    }
                }
                if constexpr (std::same_as<C, UpdateCommand>) {
                    bool complete =
                        command_.old_values &&
                        std::all_of(command_.values.fields.begin(),
                                    command_.values.fields.end(),
                                    [&](const auto& pair) {
                                        return command_.old_values->fields.contains(pair.first);
                                    });
                    // The pinned audit helper deliberately performs no database read.
                    if (complete) {
                        old_.emplace(clone_record(r, command_.old_values->fields));
                    }
                }
            }
            const auto& base = EntityDataServiceAccess::context_service(service_);
            if constexpr (std::same_as<C, InsertCommand>) {
                child_ = &base.insert(command_);
            } else if constexpr (std::same_as<C, UpdateCommand>) {
                child_ = &base.update(command_);
            } else if constexpr (std::same_as<C, DeleteCommand>) {
                child_ = &base.delete_command(command_);
            } else if constexpr (std::same_as<C, RecoverCommand>) {
                child_ = &base.recover(command_);
            } else if constexpr (std::same_as<C, BatchInsertCommand>) {
                child_ = &base.batch_insert(command_);
            } else {
                child_ = &base.batch_update(command_);
            }
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(r, copy_or_take_error(*child_, *result));
        }
        if constexpr (std::same_as<C, InsertCommand>) {
            auto event = RawAuditEvent::created(r, command_.entity, command_.values.fields);
            TraceChain trace(lineage_, r);
            event.trace_chain.swap(trace);
            auto sent = context.send_event(event);
            if (!sent) {
                return fail(std::move(sent).error());
            }
        } else if constexpr (std::same_as<C, UpdateCommand>) {
            Vector<String> fields(r);
            for (const auto& [name, v] : command_.values.fields) {
                fields.emplace_back(name);
            }
            auto event = updated_event(command_.values,
                                       command_.id,
                                       command_.expected_version_value,
                                       old_,
                                       fields,
                                       lineage_);
            auto sent = context.send_event(event);
            if (!sent) {
                return fail(std::move(sent).error());
            }
        } else if constexpr (std::same_as<C, DeleteCommand>) {
            auto event = RawAuditEvent::deleted_with_old_values(
                r, command_.entity, command_.id, command_.expected_version_value, {});
            TraceChain trace(lineage_, r);
            event.trace_chain.swap(trace);
            auto sent = context.send_event(event);
            if (!sent) {
                return fail(std::move(sent).error());
            }
        } else if constexpr (std::same_as<C, RecoverCommand>) {
            // Source recover does not attach the command lineage to its event.
            auto event = RawAuditEvent::recovered_with_old_values(
                r, command_.entity, command_.id, command_.expected_version, {});
            auto sent = context.send_event(event);
            if (!sent) {
                return fail(std::move(sent).error());
            }
        } else {
            for (std::size_t i = 0; i < command_.batch_values.size(); ++i) {
                std::span<const TraceNode> trace =
                    i < command_.trace_chains.size()
                        ? std::span<const TraceNode>(command_.trace_chains[i])
                        : std::span<const TraceNode>{};
                auto event = [&] {
                    if constexpr (std::same_as<C, BatchInsertCommand>) {
                        auto e = RawAuditEvent::created(
                            r, command_.entity, command_.batch_values[i].fields);
                        TraceChain copy(trace.begin(), trace.end(), r);
                        e.trace_chain.swap(copy);
                        return e;
                    } else {
                        std::optional<Record> old;
                        if (i < command_.batch_old_values.size() && command_.batch_old_values[i]) {
                            old.emplace(clone_record(r, command_.batch_old_values[i]->fields));
                        }
                        auto version = i < command_.batch_expected_versions.size()
                                           ? command_.batch_expected_versions[i]
                                           : std::optional<std::int64_t>{};
                        return updated_event(command_.batch_values[i],
                                             command_.batch_ids.at(i),
                                             version,
                                             old,
                                             command_.update_fields,
                                             trace);
                    }
                }();
                auto sent = context.send_event(event);
                if (!sent) {
                    return fail(std::move(sent).error());
                }
            }
        }
        return R::ok(r, result->value());
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
    RawAuditEvent updated_event(const MutationValues& input,
                                const Value& id,
                                std::optional<std::int64_t> version,
                                const std::optional<Record>& old,
                                std::span<const String> fields,
                                std::span<const TraceNode> trace) {
        auto r = service_.memory_resource();
        Record values(input.fields, r);
        values.insert_or_assign(String("id", r), Value(r, id));
        if (version) {
            values.insert_or_assign(String("version", r),
                                    Value(r, mutation_next_version(*version)));
        }
        Record current = old ? clone_record(r, *old) : Record(r);
        for (const auto& [name, v] : values) {
            current.insert_or_assign(String(name, r), Value(r, v));
        }
        auto event = RawAuditEvent::updated_with_old_values(
            r, command_.entity, values, old, current, fields);
        TraceChain copy(trace.begin(), trace.end(), r);
        event.trace_chain.swap(copy);
        return event;
    }

    R fail(RuntimeError error) {
        auto r = service_.memory_resource();
        return R::err(r, Error::runtime(r, std::move(error)));
    }

    EntityDataService<E> service_;
    C command_;
    TraceChain lineage_;
    std::optional<Record> old_;
    bool prepared_;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

#define TEAQL_ENTITY_MUTATION_ENTRY(Name, Command, Prepared, Trace)                                \
    template <class E>                                                                             \
    auto EntityDataServiceAccess::Name(const EntityDataService<E>& service,                        \
                                       const Command& command)                                     \
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>& {           \
        using R = Result<std::uint64_t, DataServiceError<typename E::Error>>;                      \
        return make_operation<R, EntityMutationOperation<E, Command>>(                             \
            service.memory_resource(), service, command, Prepared, Trace);                         \
    }
TEAQL_ENTITY_MUTATION_ENTRY(insert_internal, InsertCommand, false, trace(service))
TEAQL_ENTITY_MUTATION_ENTRY(update_internal, UpdateCommand, false, trace(service))
TEAQL_ENTITY_MUTATION_ENTRY(delete_internal,
                            DeleteCommand,
                            false,
                            command.trace_chain.empty()
                                ? trace(service)
                                : std::span<const TraceNode>(command.trace_chain))
TEAQL_ENTITY_MUTATION_ENTRY(recover_internal, RecoverCommand, false, trace(service))
TEAQL_ENTITY_MUTATION_ENTRY(execute_prepared_batch_insert,
                            BatchInsertCommand,
                            true,
                            std::span<const TraceNode>{})
TEAQL_ENTITY_MUTATION_ENTRY(execute_prepared_batch_update,
                            BatchUpdateCommand,
                            true,
                            std::span<const TraceNode>{})
#undef TEAQL_ENTITY_MUTATION_ENTRY
#define TEAQL_ENTITY_MUTATION_TRACE_ENTRY(Name, Command, Prepared)                                 \
    template <class E>                                                                             \
    auto EntityDataServiceAccess::Name(const EntityDataService<E>& service,                        \
                                       const Command& command,                                     \
                                       std::span<const TraceNode> trace)                           \
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>& {           \
        using R = Result<std::uint64_t, DataServiceError<typename E::Error>>;                      \
        return make_operation<R, EntityMutationOperation<E, Command>>(                             \
            service.memory_resource(), service, command, Prepared, trace);                         \
    }
TEAQL_ENTITY_MUTATION_TRACE_ENTRY(delete_scoped_internal, DeleteCommand, false)
TEAQL_ENTITY_MUTATION_TRACE_ENTRY(execute_prepared_insert, InsertCommand, true)
TEAQL_ENTITY_MUTATION_TRACE_ENTRY(execute_prepared_update, UpdateCommand, true)
#undef TEAQL_ENTITY_MUTATION_TRACE_ENTRY
}
