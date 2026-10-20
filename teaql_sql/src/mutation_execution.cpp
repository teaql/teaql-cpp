#include <teaql/sql/detail/mutation_execution.hpp>
#include <teaql/sql/bindings.hpp>

namespace teaql::detail {
TraceChain
mutation_statement_trace(Memory r, const MutationRequest& request, const BatchIntentScope* scope) {
    auto trace = request.execution_trace_chain();
    if (!scope) {
        return trace;
    }
    Vector<const BatchIntentScope*> scopes(r);
    for (auto* current = scope; current; current = current->parent) {
        scopes.push_back(current);
    }
    std::reverse(scopes.begin(), scopes.end());
    auto source = std::find_if(trace.begin(), trace.end(), [](const TraceNode& node) {
        return node.kind == TraceKind::AuditReason;
    });
    TraceChain prefix(r);
    for (auto* node : scopes) {
        prefix.emplace_back(TraceKind::AuditReason, source->entity_type, node->intent.comment());
    }
    std::optional<std::size_t> repeated;
    if (source->comment == prefix.front().comment) {
        repeated = 0;
    } else if (source->comment == prefix.back().comment) {
        repeated = prefix.size() - 1;
    }
    if (repeated) {
        prefix[*repeated] = clone_in(r, *source);
        trace.erase(source);
    }
    for (const auto& node : trace) {
        prefix.emplace_back(node);
    }
    return prefix;
}

SqlIntentRedactions batch_mutation_redactions(Memory r,
                                              const MutationRequest& request,
                                              const SchemaProvider& schema,
                                              const SqlExecutorState& cache) {
    SqlIntentRedactions redactions(r);
    if (auto* own = request.diagnostic_redactions()) {
        redactions.extend(*own);
    }
    if (!std::holds_alternative<MutationRequest::Batch>(request.data)) {
        return redactions;
    }
    auto capture = [&](const EntityDescriptor* entity, const Record& values) {
        Vector<Value> params(r);
        SqlLogContext log(r);
        log.generated_sql = true;
        for (const auto& [field, value] : values) {
            params.emplace_back(value);
            log.parameter_policies.push_back(entity ? sql_field_policy(*entity, field)
                                                    : SqlParameterLogPolicy::Unknown);
        }
        auto classified = SqlIntentRedactions::from_bindings(r, log, params, "");
        redactions.extend(classified);
    };
    Vector<const MutationRequest*> pending(r);
    pending.push_back(&request);
    while (!pending.empty()) {
        auto* item = pending.back();
        pending.pop_back();
        if (auto* own = item->diagnostic_redactions()) {
            redactions.extend(*own);
        }
        std::visit(
            [&](const auto& command) {
                using T = std::decay_t<decltype(command)>;
                if constexpr (std::same_as<T, MutationRequest::Batch>) {
                    for (const auto& child : command) {
                        pending.push_back(&child);
                    }
                } else if constexpr (std::same_as<T, InsertCommand> ||
                                     std::same_as<T, UpdateCommand>) {
                    auto* entity = cache.entity_descriptor(schema, command.entity);
                    capture(entity, command.values.fields);
                    if constexpr (std::same_as<T, UpdateCommand>) {
                        if (command.old_values) {
                            capture(entity, command.old_values->fields);
                        }
                    }
                    if (auto* id = mutation_target_id(*item)) {
                        redactions.capture_target_id(*id);
                    }
                } else {
                    redactions.capture_target_id(command.id);
                }
            },
            item->data);
    }
    return redactions;
}

std::string_view mutation_entity_name(const MutationRequest& request) {
    return std::visit(
        [](const auto& command) -> std::string_view {
            if constexpr (std::same_as<std::decay_t<decltype(command)>, MutationRequest::Batch>) {
                throw std::logic_error("batch has no single entity");
            } else {
                return command.entity;
            }
        },
        request.data);
}

DataServiceOperation mutation_operation(const MutationRequest& request) {
    return DataServiceOperation(request.data.index() + 1);
}

const Value* mutation_target_id(const MutationRequest& request) {
    return std::visit(
        [](const auto& command) -> const Value* {
            using T = std::decay_t<decltype(command)>;
            if constexpr (std::same_as<T, MutationRequest::Batch>) {
                return nullptr;
            } else if constexpr (std::same_as<T, InsertCommand>) {
                auto p = command.values.fields.find("id");
                return p == command.values.fields.end() ? nullptr : &p->second;
            } else {
                return &command.id;
            }
        },
        request.data);
}

const Value* mutation_persisted_id(const MutationRequest& request) {
    if (auto* command = std::get_if<DeleteCommand>(&request.data);
        command && !command->soft_delete) {
        return nullptr;
    }
    return mutation_target_id(request);
}

CompiledQuery compile_mutation(Memory r,
                               const SqlDialect& dialect,
                               const EntityDescriptor& entity,
                               const MutationRequest& request,
                               const std::optional<Expr>& guard) {
    return std::visit(
        [&](const auto& command) -> CompiledQuery {
            using T = std::decay_t<decltype(command)>;
            if constexpr (std::same_as<T, InsertCommand>) {
                return dialect.compile_insert(r, entity, command);
            } else if constexpr (std::same_as<T, UpdateCommand>) {
                return guard ? dialect.compile_guarded_update(r, entity, command, *guard)
                             : dialect.compile_update(r, entity, command);
            } else if constexpr (std::same_as<T, DeleteCommand>) {
                return guard ? dialect.compile_guarded_delete(r, entity, command, *guard)
                             : dialect.compile_delete(r, entity, command);
            } else if constexpr (std::same_as<T, RecoverCommand>) {
                return guard ? dialect.compile_guarded_recover(r, entity, command, *guard)
                             : dialect.compile_recover(r, entity, command);
            } else {
                throw std::logic_error("batch must compile each leaf separately");
            }
        },
        request.data);
}

std::string_view mutation_backend_name(const SqlDialect& dialect) {
    switch (dialect.kind()) {
    case DatabaseKind::Sqlite:
        return "sqlite";
    case DatabaseKind::PostgreSql:
        return "postgresql";
    case DatabaseKind::MySql:
        return "mysql";
    }
    throw std::invalid_argument("Invalid mutation database kind");
}

ExecutionMetadata mutation_metadata(Memory r,
                                    std::string_view backend,
                                    const CompiledQuery& query,
                                    const MutationRequest& request,
                                    DataServiceOperation operation) {
    auto now = ExecutionMetadata::SystemTime::clock::now();
    ExecutionMetadata m(std::allocator_arg, Allocator(r), operation, now, now);
    m.sql_log = clone_in(r, query.log_context);
    m.backend = backend;
    for (const auto& node : request.execution_trace_chain()) {
        m.trace_chain.emplace_back(node);
    }
    m.comment.emplace(request.comment(), r);
    m.parameterized_query.emplace(query.sql, r);
    for (const auto& param : query.params) {
        m.params.emplace_back(param);
    }
    return m;
}
}
