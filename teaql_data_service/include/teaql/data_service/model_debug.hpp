#pragma once
#include <teaql/data_service/types.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include <teaql/core/mutation_debug.hpp>
#include <teaql/core/system_time_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter&, const ExecutionMetadata&);
inline void write_debug(DebugWriter&, const MutationRequest&);

inline void write_debug(DebugWriter& writer, DataServiceOperation operation) {
    constexpr std::string_view names[]{
        "Query", "Insert", "Update", "Delete", "Recover", "Batch", "Schema"};
    auto index = static_cast<std::size_t>(operation);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown data-service operation diagnostic variant");
    }
    writer.raw(names[index]);
}

namespace detail {
template <class Fields, class T>
void data_service_debug_field(Fields& fields, std::string_view name, const T& value) {
    fields.field(name, [&](auto& out) {
        mutation_debug_element(out, value);
    });
}
}

inline void write_debug(DebugWriter& writer, const DataServiceCapabilities& value) {
    writer.structure("DataServiceCapabilities", [&](auto& fields) {
        detail::data_service_debug_field(fields, "query", value.query);
        detail::data_service_debug_field(fields, "mutation", value.mutation);
        detail::data_service_debug_field(fields, "transaction", value.transaction);
        detail::data_service_debug_field(fields, "schema", value.schema);
        detail::data_service_debug_field(fields, "id_generation", value.id_generation);
        detail::data_service_debug_field(fields, "batch_mutation", value.batch_mutation);
        detail::data_service_debug_field(fields, "returning", value.returning);
        detail::data_service_debug_field(
            fields, "small_parent_relation_probes", value.small_parent_relation_probes);
    });
}

inline void write_debug(DebugWriter& writer, const QueryRequest& value) {
    writer.structure("QueryRequest", [&](auto& fields) {
        detail::data_service_debug_field(fields, "query", value.query);
        detail::data_service_debug_field(fields, "trace_chain", value.trace_chain);
        detail::data_service_debug_field(fields, "comment", value.comment);
        detail::data_service_debug_field(fields, "capture_debug_query", value.capture_debug_query);
        detail::data_service_debug_field(
            fields, "capture_execution_metadata", value.capture_execution_metadata);
    });
}

inline void write_debug(DebugWriter& writer, const MutationRequest& value) {
    constexpr std::string_view names[]{"Insert", "Update", "Delete", "Recover", "Batch"};
    if (value.data.valueless_by_exception()) {
        throw std::invalid_argument("Valueless mutation diagnostic");
    }
    writer.tuple(names[value.data.index()], [&](auto& out) {
        std::visit(
            [&](const auto& payload) {
                detail::mutation_debug_element(out, payload);
            },
            value.data);
    });
}

inline void write_debug(DebugWriter& writer, const GuardedMutationRequest& value) {
    writer.structure("GuardedMutationRequest", [&](auto& fields) {
        detail::data_service_debug_field(fields, "mutation", value.mutation);
        detail::data_service_debug_field(fields, "guard", value.guard);
    });
}

inline void write_debug(DebugWriter& writer, const ExecutionMetadata& value) {
    writer.structure("ExecutionMetadata", [&](auto& fields) {
        detail::data_service_debug_field(fields, "statements", value.statements);
        detail::data_service_debug_field(fields, "sql_log", value.sql_log);
        detail::data_service_debug_field(fields, "backend", value.backend);
        detail::data_service_debug_field(fields, "operation", value.operation);
        detail::data_service_debug_field(fields, "started_at", value.started_at);
        detail::data_service_debug_field(fields, "ended_at", value.ended_at);
        detail::data_service_debug_field(fields, "affected_rows", value.affected_rows);
        detail::data_service_debug_field(fields, "result_count", value.result_count);
        detail::data_service_debug_field(fields, "trace_chain", value.trace_chain);
        detail::data_service_debug_field(fields, "comment", value.comment);
        detail::data_service_debug_field(fields, "backend_request_id", value.backend_request_id);
        detail::data_service_debug_field(fields, "parameterized_query", value.parameterized_query);
        detail::data_service_debug_field(fields, "params", value.params);
        detail::data_service_debug_field(fields, "debug_query", value.debug_query);
    });
}

inline void write_debug(DebugWriter& writer, const QueryResult& value) {
    writer.structure("QueryResult", [&](auto& fields) {
        detail::data_service_debug_field(fields, "rows", value.rows);
        detail::data_service_debug_field(fields, "metadata", value.metadata);
    });
}

inline void write_debug(DebugWriter& writer, const MutationResult& value) {
    writer.structure("MutationResult", [&](auto& fields) {
        detail::data_service_debug_field(fields, "affected_rows", value.affected_rows);
        detail::data_service_debug_field(fields, "generated_values", value.generated_values);
        detail::data_service_debug_field(fields, "persisted_snapshot", value.persisted_snapshot);
        detail::data_service_debug_field(fields, "metadata", value.metadata);
    });
}

inline void write_debug(DebugWriter& writer, const StreamChunk& value) {
    writer.structure("StreamChunk", [&](auto& fields) {
        detail::data_service_debug_field(fields, "rows", value.rows);
        detail::data_service_debug_field(fields, "chunk_index", value.chunk_index);
        detail::data_service_debug_field(fields, "is_last", value.is_last);
    });
}

inline void write_debug(DebugWriter& writer, const SchemaRequest& value) {
    writer.structure("SchemaRequest", [&](auto& fields) {
        detail::data_service_debug_field(fields, "entity_name", value.entity_name);
    });
}

inline void write_debug(DebugWriter& writer, const SchemaResult& value) {
    writer.structure("SchemaResult", [&](auto& fields) {
        detail::data_service_debug_field(fields, "changed", value.changed);
    });
}
}
