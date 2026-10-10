#include <teaql/provider_sqlite/detail/decoding.hpp>
#include <teaql/runtime/context.hpp>
#include "provider_sqlite_fixture.hpp"
#include <iostream>
using namespace teaql;
using namespace sqlite_fixture;

static void row_lifetimes() {
    GlobalContext global;
    auto gr = global.memory_resource();
    CompactRow* retained = nullptr;
    CompactRow* transferred = nullptr;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        CompactRow* row = nullptr;
        {
            Connection connection;
            connection.execute(
                "CREATE TABLE records(flag BOOLEAN, amount NUMERIC, doc JSON, day DATE, instant TIMESTAMP, label TEXT); INSERT INTO records VALUES(-2, '1_23.40', '{\"long nested property name\":[\"long nested string value belongs to user\"]}', '2024-02-29', 123, '2024-02-29')");
            Statement statement(
                connection,
                "SELECT flag, amount, doc, day, instant, label, +label AS inferred FROM records");
            auto& layout = user->make<detail::SqliteRowLayout>(*statement.handle);
            check(sqlite3_step(statement.handle) == SQLITE_ROW, "SQLite row available");
            auto result = detail::decode_sqlite_row(r, *statement.handle, layout);
            check(bool(result), "decode typed row");
            row = &user->make<CompactRow>(std::move(result).value());
            check(row->shared_columns() == &layout.columns(),
                  "same-domain row borrows immutable layout");
            check(std::get<bool>(row->get("flag")->data), "BOOLEAN integer becomes Bool");
            check(std::get<Decimal>(row->get("amount")->data).to_string(r) == "123.40",
                  "NUMERIC text becomes Decimal");
            check(std::holds_alternative<std::int64_t>(row->get("instant")->data),
                  "TIMESTAMP integer stays I64");
            check(std::holds_alternative<String>(row->get("label")->data) &&
                      std::holds_alternative<Date>(row->get("inferred")->data),
                  "declared text differs from expression inference");
            auto& json = std::get<Json>(row->get("doc")->data);
            check(json.memory_resource() == r, "JSON belongs to result Context");
            auto& object = std::get<Json::Object>(json.data);
            auto& array = std::get<Json::Array>(object.begin()->second.data);
            check(object.get_allocator().resource() == r &&
                      object.begin()->first.get_allocator().resource() == r &&
                      std::get<String>(array[0].data).get_allocator().resource() == r,
                  "nested object, key, array and text propagate domain");
            retained = &global.make<CompactRow>(*row);
            check(retained->shared_columns() != &layout.columns() &&
                      retained->shared_columns()->memory_resource() == gr &&
                      retained->keys()[0].get_allocator().resource() == gr,
                  "cross-domain row copies layout");
            Vector<Value> owned(row->values().begin(), row->values().end(), r);
            transferred = &global.make<CompactRow>(layout.columns(), std::move(owned));
            check(transferred->shared_columns() != &layout.columns() &&
                      transferred->shared_columns()->memory_resource() == gr &&
                      std::get<Json>(transferred->get("doc")->data).memory_resource() == gr,
                  "moving a value vector across domains still deep-copies");
        }
        check(row->get("label")->try_text() == "2024-02-29",
              "decoded text survives native statement and connection cleanup");
        check(std::get<Json>(row->get("doc")->data).dump(r).find("long nested string") !=
                  String::npos,
              "nested JSON survives native cleanup");
    }
    check(retained->get("label")->try_text() == "2024-02-29",
          "explicit global copy survives UserContext reclamation");
    check(std::get<Json>(retained->get("doc")->data).memory_resource() == gr,
          "global nested value owns its domain");
    check(std::get<Json>(transferred->get("doc")->data).dump(gr).find("long nested string") !=
              String::npos,
          "cross-domain transferred values survive source arena reclamation");
}

static void physical_types_and_failures() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& connection = user->make_resource<Connection>();
    auto& statement = user->make_resource<Statement>(
        connection, "SELECT NULL AS same, 2.5 AS same, x'ff' AS payload");
    auto& layout = user->make<detail::SqliteRowLayout>(*statement.handle);
    check(layout.columns().names()[0] == layout.columns().names()[1],
          "duplicate column labels retained");
    check(sqlite3_step(statement.handle) == SQLITE_ROW, "blob row available");
    auto result = detail::decode_sqlite_row(r, *statement.handle, layout);
    check(!result && result.error().kind() == MutationExecutorError::Kind::UnsupportedColumnType &&
              result.error().detail() == "BLOB",
          "BLOB reports explicit unsupported column type");
    auto& shorter = user->make_resource<Statement>(connection, "SELECT NULL");
    check(sqlite3_step(shorter.handle) == SQLITE_ROW, "short row available");
    auto mismatch = detail::decode_sqlite_row(r, *shorter.handle, layout);
    check(!mismatch && mismatch.error().to_string(r) == "Invalid column index: 1",
          "stale wide layout reports native index error");
    auto bytes = std::string_view("a\xf0\x9f", 3);
    auto malformed = detail::decode_sqlite_text(r, bytes, detail::SqliteDecodeKind::Json);
    check(
        !malformed &&
            malformed.error().to_string(r) ==
                "rusqlite bind error: invalid sqlite text: incomplete utf-8 byte sequence from index 1",
        "UTF-8 validation precedes logical type decoding");
    auto decimal = Decimal::parse_result(r, "1e-29");
    check(!decimal && decimal.error().code == DecimalError::Code::ScaleExceedsMaximumPrecision &&
              decimal.error().scale == 29,
          "decimal parse preserves structured scale error");
    auto json = Json::parse_result(r, "{\n\"x\": [1,]\n}");
    check(!json && json.error().line == 2 && json.error().column == 9 &&
              json.error().message == "trailing comma",
          "JSON parse retains line/column and reason");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        row_lifetimes();
        physical_types_and_failures();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite decoding contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
