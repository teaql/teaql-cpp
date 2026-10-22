// Verify this package independently exposes its complete reviewed catalog.
#include <cstdint>
import teaql_data_service;

namespace isolated_teaql_data_service_teaql {
using ::teaql::DataServiceCapabilities;
using ::teaql::DynamicFieldStore;
using ::teaql::DynamicFieldStreamPlan;
using ::teaql::DynamicFieldWrite;
using ::teaql::DynamicStorageRows;
using ::teaql::DynamicStorageFuture;

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
using ::teaql::MutationCommand;
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
using ::teaql::TransactionResourceScope;
using ::teaql::TransactionExecutor;
using ::teaql::TransactionHandle;
using ::teaql::as_str;
using ::teaql::is_credential_log_name;
using ::teaql::write_debug;
}

namespace isolated_teaql_data_service_teaql_detail {
using ::teaql::detail::data_service_debug_field;
using ::teaql::detail::write_sql_log_optional;
}

namespace isolated_teaql_data_service_teaql_data_service {
using ::teaql::data_service::DataServiceCapabilities;
using ::teaql::data_service::DynamicFieldStore;
using ::teaql::data_service::DynamicFieldStreamPlan;
using ::teaql::data_service::DynamicFieldWrite;
using ::teaql::data_service::DynamicStorageRows;
using ::teaql::data_service::DynamicStorageFuture;

using ::teaql::data_service::DataServiceExecutor;
using ::teaql::data_service::DataServiceOperation;
using ::teaql::data_service::ExecutionMetadata;
using ::teaql::data_service::ExecutionObserver;
using ::teaql::data_service::GuardedMutationExecutor;
using ::teaql::data_service::GuardedMutationRequest;
using ::teaql::data_service::IdGeneratorExecutor;
using ::teaql::data_service::MutationExecutor;
using ::teaql::data_service::MutationCommand;
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
using ::teaql::data_service::TransactionResourceScope;
using ::teaql::data_service::TransactionExecutor;
using ::teaql::data_service::is_credential_log_name;
using ::teaql::data_service::write_debug;
}

namespace teaql::dynamic_fields {
using ::teaql::dynamic_fields::DynamicFieldStore;
using ::teaql::dynamic_fields::DynamicFieldStreamPlan;
using ::teaql::dynamic_fields::DynamicFieldWrite;
using ::teaql::dynamic_fields::DynamicStorageRows;
using ::teaql::dynamic_fields::DynamicStorageFuture;
}

namespace teaql::data_service::dynamic_fields {
using ::teaql::data_service::dynamic_fields::DynamicFieldStore;
using ::teaql::data_service::dynamic_fields::DynamicFieldStreamPlan;
using ::teaql::data_service::dynamic_fields::DynamicFieldWrite;
using ::teaql::data_service::dynamic_fields::DynamicStorageRows;
using ::teaql::data_service::dynamic_fields::DynamicStorageFuture;
}
