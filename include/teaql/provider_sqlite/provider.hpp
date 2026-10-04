#pragma once
#include <teaql/provider_sqlite/error.hpp>
#include <teaql/sql/transport.hpp>

struct sqlite3;
namespace teaql {
namespace detail { struct SqliteConnectionState; struct SqliteProviderState; struct SqliteSchemaCall; }
// Allocate with the owning Context's make_resource(). Native SQLite allocations
// and cached statements are released before that arena is reclaimed. Borrowed
// providers/operations must not outlive this explicitly scoped connection.
class SqliteConnection final {
public:
    using allocator_type=Allocator;
    SqliteConnection(std::allocator_arg_t,Allocator,std::string_view filename=":memory:");
    SqliteConnection(const SqliteConnection&)=delete;
    SqliteConnection& operator=(const SqliteConnection&)=delete;
    ~SqliteConnection()=default;
    Memory memory_resource() const noexcept;
    sqlite3* native_handle() const;
    std::size_t cached_statement_count() const;
private:
    friend class SqliteMutationExecutor;
    detail::SqliteConnectionState& state_;
};
// Source provider identity is retained. Clones in its owning Context borrow the
// same connection/cache; cloning cannot extend either lifetime.
class SqliteMutationExecutor final : public StreamingSqlTransport<MutationExecutorError>,public SqlTransactionTransport<MutationExecutorError>,public StreamQueryExecutor<MutationExecutorError> {
public:
    using allocator_type=Allocator;
    using Rows=Vector<CompactRow>;
    using TableColumns=std::pmr::map<String,std::pair<String,bool>,std::less<>>;
    SqliteMutationExecutor(std::allocator_arg_t,Allocator,SqliteConnection&);
    SqliteMutationExecutor(std::allocator_arg_t,Allocator,const SqliteMutationExecutor&);
    SqliteMutationExecutor(const SqliteMutationExecutor&)=delete;
    Memory memory_resource() const noexcept;
    SqliteConnection& connection() const noexcept {return connection_;}
    DataServiceCapabilities capabilities() const override;
    // Synchronous entry points require an idle connection. Use pollable methods
    // when another stream holds its native cursor; they wait via owner-thread wakes.
    Result<std::uint64_t,MutationExecutorError> execute(Memory,const CompiledQuery&);
    Result<Rows,MutationExecutorError> fetch_all_compact(Memory,const CompiledQuery&);
    Result<Vector<StreamChunk>,MutationExecutorError> fetch_stream(Memory,const CompiledQuery&,std::size_t);
    Result<std::monostate,MutationExecutorError> begin_transaction(Memory);
    Result<std::monostate,MutationExecutorError> commit_transaction(Memory);
    Result<std::monostate,MutationExecutorError> rollback_transaction(Memory);
    Result<std::monostate,MutationExecutorError> ensure_id_space_table(Memory,std::string_view table_name);
    Result<bool,MutationExecutorError> table_exists(Memory,std::string_view table_name) const;
    Result<TableColumns,MutationExecutorError> table_columns(Memory,std::string_view table_name) const;
    DataServiceOperationResult<SqlTransactionHandle<MutationExecutorError>,MutationExecutorError>& begin_sql(Memory) override;
    DataServiceOperationResult<std::uint64_t,MutationExecutorError>& execute_sql(Memory,const CompiledQuery&) override;
    DataServiceOperationResult<Rows,MutationExecutorError>& fetch_all_compact_sql(Memory,const CompiledQuery&) override;
    DataServiceOperationResult<Rows,MutationExecutorError>& fetch_repeated_compact_sql(Memory,const CompiledQuery&,std::size_t,std::span<const Value>) override;
    QueryStream<MutationExecutorError>& stream_sql(Memory,CompiledQuery,std::size_t) override;
    QueryStream<MutationExecutorError>& query_stream(Memory,QueryRequest,std::size_t) override;
    void clear_query_caches();
    std::size_t cached_column_layout_count() const;
private:
    friend class SqliteIdSpaceGenerator;
    friend class SqliteBusinessIdAllocator;
    friend struct detail::SqliteSchemaCall;
    SqliteConnection& connection_;
    detail::SqliteProviderState& state_;
};
}
