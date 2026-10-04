#include <teaql/provider_sqlite/detail/bindings.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "../provider_sqlite_fixture.hpp"
#include "fixture_value.hpp"
#include <charconv>
#include <cmath>
#include <iostream>
using namespace teaql;
static Json failure(Memory r, const MutationExecutorError& error) {
    constexpr std::string_view kinds[] = {"Sqlite", "SqlCompile", "UnsupportedValue", "UnsupportedColumnType", "Bind", "Lock"};
    auto out = detail::model_json(r); detail::model_put(out, "error", error.to_string(r));
    detail::model_put(out, "kind", kinds[unsigned(error.kind())]); return out;
}
static Json evaluate(Memory r, const Json& input) {
    sqlite_fixture::Connection connection;
    String create("CREATE TABLE data(v ", r); create += text(field(input, "affinity")); create += ')';
    connection.execute(create.c_str());
    Vector<Value> values(r);
    for (const auto& v : std::get<Json::Array>(field(input, "params").data)) values.emplace_back(value(r, v));
    auto bindings = detail::bind_values(r, values);
    if (!bindings) return failure(r, bindings.error());
    auto sql = optional(input, "sql") ? text(field(input, "sql")) : std::string_view("INSERT INTO data VALUES(?)");
    String sql_string(sql, r); sqlite_fixture::Statement statement(connection, sql_string.c_str());
    auto bound = detail::bind_statement(r, *statement.handle, bindings.value());
    if (!bound) return failure(r, bound.error());
    auto code = sqlite3_step(statement.handle);
    if (code != SQLITE_DONE) return failure(r, MutationExecutorError::sqlite(r, code, sqlite3_errmsg(connection.handle)));
    auto out = detail::model_json(r); detail::model_put(out, "affected", std::uint64_t(sqlite3_changes64(connection.handle)));
    sqlite_fixture::Statement observed(connection, "SELECT v FROM data");
    sqlite_fixture::check(sqlite3_step(observed.handle) == SQLITE_ROW, "missing inserted value");
    auto stored = detail::model_json(r); auto* stmt = observed.handle;
    switch (sqlite3_column_type(stmt, 0)) {
    case SQLITE_NULL: detail::model_put(stored, "type", "null"); detail::model_put(stored, "value", Json(r)); break;
    case SQLITE_INTEGER: {
        detail::model_put(stored, "type", "integer"); char buffer[32]; auto result = std::to_chars(buffer, buffer + sizeof(buffer), sqlite3_column_int64(stmt, 0));
        detail::model_put(stored, "value", std::string_view(buffer, result.ptr)); break;
    }
    case SQLITE_FLOAT: {
        detail::model_put(stored, "type", "real"); auto value = sqlite3_column_double(stmt, 0);
        if (std::isinf(value)) detail::model_put(stored, "value", value < 0 ? "-inf" : "inf");
        else detail::model_put(stored, "value", value); break;
    }
    case SQLITE_TEXT: detail::model_put(stored, "type", "text"); detail::model_put(stored, "value", std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0))); break;
    default: throw std::runtime_error("binding unexpectedly produced blob");
    }
    detail::model_put(out, "stored", stored); return out;
}
int main() {
    GlobalContext global; std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context(); auto r = user->memory_resource();
        auto input = Json::parse(r, line); if (!input) throw std::runtime_error("invalid fixture JSON");
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
