// Publish canonical global declarations without moving implementation bodies.
module;
#include <teaql/sql.hpp>
export module teaql_sql;

export import teaql_data_service;

export namespace teaql {
using ::teaql::CompiledQuery;
using ::teaql::DatabaseKind;
using ::teaql::DebugWriter;
using ::teaql::SchemaIndexSpec;
using ::teaql::SqlBindings;
using ::teaql::SqlCompileError;
using ::teaql::SqlDataServiceExecutor;
using ::teaql::SqlDataServiceTransaction;
using ::teaql::SqlDialect;
using ::teaql::SqlExecutorError;
using ::teaql::SqlExecutorState;
using ::teaql::SqlRenderResult;
using ::teaql::SqlTransaction;
using ::teaql::SqlTransactionHandle;
using ::teaql::SqlTransactionTransport;
using ::teaql::SqlTransport;
using ::teaql::SqlTransportTransaction;
using ::teaql::StreamingSqlTransport;
using ::teaql::bounded_sql_identifier;
using ::teaql::quote_identifier_if_needed;
using ::teaql::render_sql_value;
using ::teaql::render_sql_with;
using ::teaql::schema_foreign_key_name;
using ::teaql::schema_index_specs;
using ::teaql::storage_length_covers;
using ::teaql::storage_numeric_covers;
using ::teaql::validate_schema_identifier_lengths;
using ::teaql::write_debug;
}

export namespace teaql::detail {
using ::teaql::detail::BufferedTransactionStream;
using ::teaql::detail::CachedSelectPlan;
using ::teaql::detail::DiagnosticStream;
using ::teaql::detail::FailureJournal;
using ::teaql::detail::MappedSqlStream;
using ::teaql::detail::MutationExecution;
using ::teaql::detail::MutationTreeOperation;
using ::teaql::detail::ObservedSqlCall;
using ::teaql::detail::ObservedSqlTransport;
using ::teaql::detail::RepeatedSqlFetch;
using ::teaql::detail::SqlBeginOperation;
using ::teaql::detail::SqlBindingScope;
using ::teaql::detail::SqlErrorStream;
using ::teaql::detail::SqlExecutorState;
using ::teaql::detail::SqlQueryOperation;
using ::teaql::detail::SqlStreamExecutorSupport;
using ::teaql::detail::SqlTransactionExecutorSupport;
using ::teaql::detail::SqlTransactionTerminal;
using ::teaql::detail::StatementDiagnostic;
using ::teaql::detail::buffered_transaction_stream;
using ::teaql::detail::collect_select_params;
using ::teaql::detail::combine_sql_policy;
using ::teaql::detail::compile_mutation;
using ::teaql::detail::diagnostic_stream;
using ::teaql::detail::execute_mutation;
using ::teaql::detail::mutation_backend_name;
using ::teaql::detail::mutation_entity_name;
using ::teaql::detail::mutation_metadata;
using ::teaql::detail::mutation_operation;
using ::teaql::detail::mutation_persisted_id;
using ::teaql::detail::mutation_target_id;
using ::teaql::detail::native_sql_stream;
using ::teaql::detail::partition_probe_values;
using ::teaql::detail::query_diagnostic_metadata;
using ::teaql::detail::scalar_partition_probe_query;
using ::teaql::detail::select_plan_key;
using ::teaql::detail::select_plan_matches;
using ::teaql::detail::sql_error_stream;
using ::teaql::detail::sql_expression_policy;
using ::teaql::detail::sql_field_policy;
}

export namespace teaql::sql {
using ::teaql::sql::CompiledQuery;
using ::teaql::sql::DatabaseKind;
using ::teaql::sql::SchemaIndexSpec;
using ::teaql::sql::SqlBindings;
using ::teaql::sql::SqlCompileError;
using ::teaql::sql::SqlDataServiceExecutor;
using ::teaql::sql::SqlDataServiceTransaction;
using ::teaql::sql::SqlDialect;
using ::teaql::sql::SqlExecutorError;
using ::teaql::sql::SqlTransaction;
using ::teaql::sql::SqlTransactionTransport;
using ::teaql::sql::SqlTransport;
using ::teaql::sql::StreamingSqlTransport;
using ::teaql::sql::bounded_sql_identifier;
using ::teaql::sql::quote_identifier_if_needed;
using ::teaql::sql::render_sql_value;
using ::teaql::sql::render_sql_with;
using ::teaql::sql::schema_foreign_key_name;
using ::teaql::sql::schema_index_specs;
using ::teaql::sql::storage_length_covers;
using ::teaql::sql::storage_numeric_covers;
using ::teaql::sql::validate_schema_identifier_lengths;
}
