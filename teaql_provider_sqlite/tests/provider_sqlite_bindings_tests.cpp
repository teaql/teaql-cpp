#include <teaql/provider_sqlite/detail/bindings.hpp>
#include <teaql/runtime/context.hpp>
#include "../../tests/support/provider_sqlite_fixture.hpp"
#include <iostream>
#include <limits>
using namespace teaql;
using namespace sqlite_fixture;

static void binding_lifetimes() {
    GlobalContext global;
    auto& connection = global.make_resource<Connection>();
    auto& statement = global.make_resource<Statement>(connection, "SELECT ?, ?, ?");
    constexpr char bytes[] = "long bound text must survive source Context close\0尾";
    const std::string_view text(bytes, sizeof(bytes) - 1);
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        Vector<Value> inputs(r);
        inputs.emplace_back(text);
        inputs.emplace_back(Timestamp(INT64_MIN));
        inputs.emplace_back(TypedNull{DataType::Json});
        auto bindings = detail::bind_values(r, inputs);
        check(bool(bindings), "convert bindings");
        check(bindings.value().get_allocator().resource() == r, "binding vector belongs to user");
        check(std::get<String>(bindings.value()[0].data).get_allocator().resource() == r,
              "nested text belongs to user");
        check(bool(detail::bind_statement(r, *statement.handle, bindings.value())),
              "bind native values");
    }
    check(sqlite3_step(statement.handle) == SQLITE_ROW,
          "native bindings survive source reclamation");
    auto result =
        std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(statement.handle, 0)),
                         sqlite3_column_bytes(statement.handle, 0));
    check(result == text, "SQLite copied complete text including NUL and Unicode");
    check(sqlite3_column_int64(statement.handle, 1) == INT64_MIN,
          "raw timestamp is signed milliseconds");
    check(sqlite3_column_type(statement.handle, 2) == SQLITE_NULL, "typed null binds NULL");
    check(sqlite3_reset(statement.handle) == SQLITE_OK &&
              sqlite3_clear_bindings(statement.handle) == SQLITE_OK,
          "native reuse cleanup");
}

static void numeric_affinity() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& connection = user->make_resource<Connection>();
    connection.execute("CREATE TABLE amounts(v NUMERIC)");
    auto& statement = user->make_resource<Statement>(connection, "INSERT INTO amounts VALUES(?)");
    for (auto input : {"2.00", "10.50", "-3.25", "100.000"}) {
        Vector<Value> values(r);
        values.emplace_back(*Decimal::parse(input));
        auto bindings = detail::bind_values(r, values);
        check(bool(bindings) &&
                  bool(detail::bind_statement(r, *statement.handle, bindings.value())),
              "bind canonical decimal");
        check(sqlite3_step(statement.handle) == SQLITE_DONE, "insert decimal");
        sqlite3_reset(statement.handle);
        sqlite3_clear_bindings(statement.handle);
    }
    auto& query = user->make_resource<Statement>(
        connection, "SELECT sum(v), count(*) FROM amounts WHERE v > 3");
    check(sqlite3_step(query.handle) == SQLITE_ROW, "numeric predicate");
    check(sqlite3_column_double(query.handle, 0) == 110.5 &&
              sqlite3_column_int64(query.handle, 1) == 2,
          "decimal comparison and aggregate remain numeric");
}

static void errors_and_copies() {
    GlobalContext global;
    auto gr = global.memory_resource();
    MutationExecutorError* retained = nullptr;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto overflow = detail::bind_sqlite_value(r, Value(r, UINT64_MAX));
        check(!overflow && overflow.error().kind() == MutationExecutorError::Kind::Bind,
              "overflow remains a bind error");
        check(overflow.error().to_string(r) ==
                  "rusqlite bind error: u64 value 18446744073709551615 exceeds i64 range",
              "overflow display");
        retained = &global.make<MutationExecutorError>(overflow.error());
        Vector<Value> values(r);
        values.emplace_back(Value::List(r));
        values.emplace_back(UINT64_MAX);
        auto first = detail::bind_values(r, values);
        check(!first && first.error().kind() == MutationExecutorError::Kind::UnsupportedValue &&
                  first.error().detail() == "list",
              "first conversion error wins");
        auto& connection = user->make_resource<Connection>();
        auto& statement = user->make_resource<Statement>(connection, "SELECT ?");
        values.clear();
        values.emplace_back(1);
        values.emplace_back(2);
        values.emplace_back(3);
        auto bindings = detail::bind_values(r, values);
        auto extra = detail::bind_statement(r, *statement.handle, bindings.value());
        check(!extra && extra.error().kind() == MutationExecutorError::Kind::Sqlite,
              "parameter count remains driver error");
        check(extra.error().to_string(r) ==
                  "Wrong number of parameters passed to query. Got 2, needed 1",
              "iterator reports first excess parameter");
        auto missing = detail::bind_statement(r, *statement.handle, {});
        check(!missing && missing.error().to_string(r) ==
                              "Wrong number of parameters passed to query. Got 0, needed 1",
              "missing parameter rejected");
        sqlite3_reset(statement.handle);
        sqlite3_clear_bindings(statement.handle);
        // Native resource errors must survive as typed SQLite failures.
        const int old_limit = sqlite3_limit(connection.handle, SQLITE_LIMIT_LENGTH, 64);
        values.clear();
        values.emplace_back(String(100, 'x', r));
        bindings = detail::bind_values(r, values);
        auto too_big = detail::bind_statement(r, *statement.handle, bindings.value());
        check(!too_big && too_big.error().sqlite_extended_code() == SQLITE_TOOBIG,
              "preserve native bind error code");
        sqlite3_limit(connection.handle, SQLITE_LIMIT_LENGTH, old_limit);
        SqlCompileError compile(std::allocator_arg,
                                Allocator(r),
                                SqlCompileError::Code::UnknownEntity,
                                String(100, 'e', r));
        auto error = MutationExecutorError::sql_compile(r, std::move(compile));
        auto& copy = global.make<MutationExecutorError>(error);
        check(copy.compile_error().name.get_allocator().resource() == gr &&
                  copy.to_string(gr) == error.to_string(gr),
              "deep-copy compile payload");
    }
    check(retained->memory_resource() == gr &&
              retained->detail() == "u64 value 18446744073709551615 exceeds i64 range",
          "explicit global error copy survives user destruction");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        binding_lifetimes();
        numeric_affinity();
        errors_and_copies();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite binding contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
