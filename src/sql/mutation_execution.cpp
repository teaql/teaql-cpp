#include <teaql/sql/detail/mutation_execution.hpp>

namespace teaql::detail {
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

ExecutionMetadata mutation_metadata(Memory r,
                                    const SqlDialect& dialect,
                                    const CompiledQuery& query,
                                    const MutationRequest& request,
                                    DataServiceOperation operation) {
    auto now = ExecutionMetadata::SystemTime::clock::now();
    ExecutionMetadata m(std::allocator_arg, Allocator(r), operation, now, now);
    m.sql_log = clone_in(r, query.log_context);
    switch (dialect.kind()) {
    case DatabaseKind::Sqlite:
        m.backend = "sqlite";
        break;
    case DatabaseKind::PostgreSql:
        m.backend = "postgresql";
        break;
    case DatabaseKind::MySql:
        m.backend = "mysql";
        break;
    }
    for (const auto& node : request.trace_chain()) {
        m.trace_chain.emplace_back(node);
    }
    if (auto comment = request.comment()) {
        m.comment.emplace(*comment, r);
    }
    m.parameterized_query.emplace(query.sql, r);
    for (const auto& param : query.params) {
        m.params.emplace_back(param);
    }
    return m;
}
}
