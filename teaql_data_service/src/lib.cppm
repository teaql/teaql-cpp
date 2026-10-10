// Publish canonical global declarations without moving implementation bodies.
module;
#include <teaql/data_service.hpp>
export module teaql_data_service;

export import teaql_core;
export import teaql_async;

export namespace teaql {
using ::teaql::DataServiceCapabilities;
using ::teaql::DataServiceExecutor;
using ::teaql::DataServiceOperation;
using ::teaql::DataServiceOperationResult;
using ::teaql::DataServiceTransaction;
using ::teaql::DebugWriter;
using ::teaql::ExecutionMetadata;
using ::teaql::ExecutionObserver;
using ::teaql::GuardedMutationExecutor;
using ::teaql::GuardedMutationRequest;
using ::teaql::IdGeneratorExecutor;
using ::teaql::MutationExecutor;
using ::teaql::MutationRequest;
using ::teaql::MutationResult;
using ::teaql::QueryExecutor;
using ::teaql::QueryRequest;
using ::teaql::QueryResult;
using ::teaql::QueryStream;
using ::teaql::SchemaExecutor;
using ::teaql::SchemaProvider;
using ::teaql::SchemaRequest;
using ::teaql::SchemaResult;
using ::teaql::SqlExecutionOutcome;
using ::teaql::SqlIntentRedactions;
using ::teaql::SqlLogContext;
using ::teaql::SqlParameterLogPolicy;
using ::teaql::SqlProjectionState;
using ::teaql::StreamChunk;
using ::teaql::StreamQueryExecutor;
using ::teaql::Transaction;
using ::teaql::TransactionExecutor;
using ::teaql::TransactionHandle;
using ::teaql::as_str;
using ::teaql::is_credential_log_name;
using ::teaql::write_debug;
}

export namespace teaql::detail {
using ::teaql::detail::data_service_debug_field;
using ::teaql::detail::write_sql_log_optional;
}

export namespace teaql::data_service {
using ::teaql::data_service::DataServiceCapabilities;
using ::teaql::data_service::DataServiceExecutor;
using ::teaql::data_service::DataServiceOperation;
using ::teaql::data_service::ExecutionMetadata;
using ::teaql::data_service::ExecutionObserver;
using ::teaql::data_service::GuardedMutationExecutor;
using ::teaql::data_service::GuardedMutationRequest;
using ::teaql::data_service::IdGeneratorExecutor;
using ::teaql::data_service::MutationExecutor;
using ::teaql::data_service::MutationRequest;
using ::teaql::data_service::MutationResult;
using ::teaql::data_service::QueryExecutor;
using ::teaql::data_service::QueryRequest;
using ::teaql::data_service::QueryResult;
using ::teaql::data_service::QueryStream;
using ::teaql::data_service::SchemaExecutor;
using ::teaql::data_service::SchemaProvider;
using ::teaql::data_service::SchemaRequest;
using ::teaql::data_service::SchemaResult;
using ::teaql::data_service::SqlExecutionOutcome;
using ::teaql::data_service::SqlIntentRedactions;
using ::teaql::data_service::SqlLogContext;
using ::teaql::data_service::SqlParameterLogPolicy;
using ::teaql::data_service::SqlProjectionState;
using ::teaql::data_service::StreamChunk;
using ::teaql::data_service::StreamQueryExecutor;
using ::teaql::data_service::Transaction;
using ::teaql::data_service::TransactionExecutor;
using ::teaql::data_service::is_credential_log_name;
using ::teaql::data_service::write_debug;
}
