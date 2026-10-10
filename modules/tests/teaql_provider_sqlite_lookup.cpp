// Isolated lookup of the production package export catalog.
#include <cstdint>
import teaql_provider_sqlite;

namespace isolated_teaql_provider_sqlite_teaql {
using ::teaql::DEFAULT_BUSINESS_ID_SPACE_TABLE;
using ::teaql::DEFAULT_COLUMN_LAYOUT_CACHE_CAPACITY;
using ::teaql::DEFAULT_ID_SPACE_TABLE;
using ::teaql::DEFAULT_PREPARED_STATEMENT_CACHE_CAPACITY;
using ::teaql::MutationExecutorError;
using ::teaql::SqliteBoxedError;
using ::teaql::SqliteBusinessIdAllocator;
using ::teaql::SqliteConnection;
using ::teaql::SqliteDialect;
using ::teaql::SqliteDriverError;
using ::teaql::SqliteErrorCauseView;
using ::teaql::SqliteFromSqlError;
using ::teaql::SqliteIdSpaceGenerator;
using ::teaql::SqliteMutationExecutor;
using ::teaql::SqliteNativeError;
using ::teaql::SqliteNulError;
using ::teaql::SqlitePath;
using ::teaql::SqliteSchemaProvider;
using ::teaql::SqliteUtf8Error;
using ::teaql::SqliteValueType;
using ::teaql::UserContext;
using ::teaql::use_sqlite_provider;
using ::teaql::write_debug;
}

namespace isolated_teaql_provider_sqlite_teaql_detail {
using ::teaql::detail::SqliteBindings;
using ::teaql::detail::SqliteBoundValue;
using ::teaql::detail::SqliteConnectionState;
using ::teaql::detail::SqliteDecodeKind;
using ::teaql::detail::SqliteForeignKey;
using ::teaql::detail::SqliteProviderState;
using ::teaql::detail::SqliteRowLayout;
using ::teaql::detail::SqliteSchemaCall;
using ::teaql::detail::SqliteSimpleError;
using ::teaql::detail::SqliteTableColumns;
using ::teaql::detail::bind_sqlite_value;
using ::teaql::detail::bind_statement;
using ::teaql::detail::bind_values;
using ::teaql::detail::compile_sqlite_create_table;
using ::teaql::detail::decode_sqlite_row;
using ::teaql::detail::decode_sqlite_text;
using ::teaql::detail::ensure_sqlite_column_compatibility;
using ::teaql::detail::ensure_sqlite_physical_schema;
using ::teaql::detail::parse_fixed_sqlite_timestamp;
using ::teaql::detail::sqlite_compatible_soundex;
using ::teaql::detail::sqlite_declared_type_args;
using ::teaql::detail::sqlite_decode_kind;
using ::teaql::detail::sqlite_foreign_keys_for;
using ::teaql::detail::sqlite_identifier_lower;
using ::teaql::detail::sqlite_schema_utf8_valid;
using ::teaql::detail::sqlite_table_columns;
using ::teaql::detail::sqlite_table_exists;
using ::teaql::detail::sqlite_type_family;
using ::teaql::detail::sqlite_utf8_error;
using ::teaql::detail::strip_sqlite_identifier_quotes;
using ::teaql::detail::write_debug;
}
