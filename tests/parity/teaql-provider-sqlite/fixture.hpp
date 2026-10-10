#pragma once
#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/sql/executor.hpp>

namespace original_provider {
using namespace teaql;
using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;

inline void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory resource = user->memory_resource();
    SqliteConnection& connection = user->make_resource<SqliteConnection>();
    SqliteMutationExecutor& transport = user->make<SqliteMutationExecutor>(connection);
    SqliteDialect& dialect = user->make_resource<SqliteDialect>();
    InMemoryMetadataStore& metadata = user->make<InMemoryMetadataStore>();
    SqlExecutorState& state = user->make<SqlExecutorState>();
    Executor& executor = user->make<Executor>(dialect, transport, metadata, state);

    Fixture() {
        user->disable_sql_log();
        user->set_metadata(*user, metadata);
    }

    void exec(std::string_view statement) {
        CompiledQuery sql(std::allocator_arg, Allocator(resource));
        sql.sql = statement;
        check(bool(transport.execute(resource, sql)), "native fixture SQL succeeds");
    }

    Vector<CompactRow> rows(std::string_view statement) {
        CompiledQuery sql(std::allocator_arg, Allocator(resource));
        sql.sql = statement;
        auto result = transport.fetch_all_compact(resource, sql);
        check(result.is_ok(), "native fixture reread succeeds");
        return std::move(result).value();
    }

    void seed(const EntityDescriptor& entity, const InsertCommand& command) {
        auto sql = dialect.compile_insert(resource, entity, command);
        check(bool(transport.execute(resource, sql)), "compiled seed insert succeeds");
    }
};

inline EntityDescriptor order_entity(Memory r) {
    EntityDescriptor entity(std::allocator_arg, Allocator(r), "Order");
    entity.table("orders");
    entity.property("id", DataType::I64).id().not_null();
    entity.property("version", DataType::I64).version().not_null();
    entity.property("tenant_id", DataType::I64).not_null();
    entity.property("name", DataType::Text).not_null();
    return entity;
}
}
