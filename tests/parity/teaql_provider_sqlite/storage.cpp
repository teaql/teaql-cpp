// Direct pinned SQLite provider storage, decoding and stream contracts.
#include "fixture.hpp"
#include <teaql/provider_sqlite/detail/bindings.hpp>
#include <teaql/provider_sqlite/detail/decoding.hpp>
#include <teaql/provider_sqlite/id.hpp>
#include <array>
#include <charconv>
#include <iostream>

using namespace original_provider;

static EntityDescriptor entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Order");
    result.table("orders");
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version().not_null();
    result.property("name", DataType::Text);
    return result;
}

static void ensure_schema_registers_soundex_idempotently() {
    Fixture f;
    use_sqlite_provider(*f.user, *f.user, f.transport);
    check(bool(f.user->ensure_schema().wait()), "first empty physical schema succeeds");
    check(bool(f.user->ensure_schema().wait()), "second empty physical schema succeeds");
    auto encoded = f.rows("SELECT soundex('Robert') AS encoded");
    auto matches = f.rows("SELECT soundex('Robert') = soundex('Rupert') AS matches");
    auto empty = f.rows("SELECT soundex(NULL) AS empty");
    check(*encoded[0].get("encoded") == Value(f.resource, "R163") &&
              *matches[0].get("matches") == Value(f.resource, std::int64_t(1)) &&
              *empty[0].get("empty") == Value(f.resource, "?000"),
          "all original registered SOUNDEX results");
}

static void streaming_sql_yields_bounded_chunks_and_releases_cursor_on_drop() {
    Fixture f;
    f.exec("CREATE TABLE stream_fixture(id INTEGER)");
    f.exec("INSERT INTO stream_fixture VALUES (1), (2), (3), (4), (5)");
    CompiledQuery query(std::allocator_arg, Allocator(f.resource));
    query.sql = "SELECT id FROM stream_fixture ORDER BY id";
    auto& stream = f.transport.stream_sql(f.resource, detail::clone_in(f.resource, query), 2);
    Vector<std::size_t> sizes(f.resource);
    while (auto* chunk = stream.wait_next()) {
        check(bool(*chunk), "source native stream chunk succeeds");
        sizes.push_back(chunk->value().rows.size());
    }
    check(sizes == Vector<std::size_t>({2, 2, 1}, f.resource),
          "original five-row stream retains all three bounded chunks");
    auto& early = f.transport.stream_sql(f.resource, std::move(query), 2);
    auto* first = early.wait_next();
    check(first && *first && first->value().rows.size() == 2, "original early chunk size");
    // Context owns the stream; explicit cancellation is the Rust drop boundary.
    early.cancel();
    auto count = f.rows("SELECT count(*) AS count FROM stream_fixture");
    check(*count[0].get("count") == Value(f.resource, std::int64_t(5)),
          "early cancellation releases the cursor before the next synchronous query");
}

static void decimal_bind_is_numeric_and_comparable() {
    Fixture f;
    Value value(f.resource, *Decimal::parse("123.450"));
    auto bound = detail::bind_sqlite_value(f.resource, value);
    check(bound && std::holds_alternative<String>(bound.value().data) &&
              std::get<String>(bound.value().data) == "123.450",
          "original decimal binding retains text storage and scale");
    CompiledQuery query(std::allocator_arg, Allocator(f.resource));
    query.sql = "SELECT 1 AS matches WHERE CAST(? AS NUMERIC) BETWEEN 120 AND 130";
    query.params.emplace_back(value);
    auto rows = f.transport.fetch_all_compact(f.resource, query);
    check(rows && rows.value().size() == 1 &&
              *rows.value()[0].get("matches") == Value(f.resource, std::int64_t(1)),
          "original bound decimal is numerically comparable in SQLite");
}

static void temporal_debug_sql_is_executable_and_matches_prepared_storage() {
    Fixture f;
    f.exec("CREATE TABLE temporal_fixture (id INTEGER PRIMARY KEY, d DATE, t TIMESTAMP)");
    CompiledQuery query(std::allocator_arg, Allocator(f.resource));
    query.sql = "INSERT INTO temporal_fixture VALUES (?, ?, ?)";
    query.params.emplace_back(std::int64_t(1));
    query.params.emplace_back(*Date::from_ymd(2024, 2, 29));
    query.params.emplace_back(Timestamp(1787110200123LL));
    check(bool(f.transport.execute(f.resource, query)), "source prepared temporal insert");
    auto debug = query.debug_sql(f.resource, DatabaseKind::Sqlite);
    auto position = debug.find("VALUES (1,");
    check(position != String::npos, "source debug insert contains its original id literal");
    debug.replace(position, std::string_view("VALUES (1,").size(), "VALUES (2,");
    f.exec(debug);
    auto equal = f.rows(
        "SELECT count(*) AS count FROM temporal_fixture a JOIN temporal_fixture b ON a.d=b.d AND a.t=b.t WHERE a.id=1 AND b.id=2");
    auto storage = f.rows("SELECT typeof(t) AS storage FROM temporal_fixture WHERE id=1");
    check(*equal[0].get("count") == Value(f.resource, std::int64_t(1)) &&
              *storage[0].get("storage") == Value(f.resource, "integer"),
          "original debug and prepared date/timestamp storage are identical");
}

static void sqlite_dialect_compiles_mutations_and_schema() {
    Fixture f;
    auto descriptor = entity(f.resource);
    check(f.dialect.prefers_small_parent_relation_probes(), "original SQLite probe preference");
    InsertCommand insert(std::allocator_arg, Allocator(f.resource), "Order");
    insert.value("id", std::uint64_t(1)).value("name", "A");
    check(f.dialect.compile_insert(f.resource, descriptor, insert).sql ==
              "INSERT INTO orders (id, name) VALUES (?, ?)",
          "original insert SQL");
    UpdateCommand update(std::allocator_arg, Allocator(f.resource), "Order", std::uint64_t(1));
    update.expected_version(3).value("name", "B");
    check(f.dialect.compile_update(f.resource, descriptor, update).sql ==
              "UPDATE orders SET name = ?, version = ? WHERE id = ? AND version = ?",
          "original update SQL");
    DeleteCommand deletion(std::allocator_arg, Allocator(f.resource), "Order", std::uint64_t(1));
    deletion.expected_version(3);
    RecoverCommand recovery(
        std::allocator_arg, Allocator(f.resource), "Order", std::uint64_t(1), -4);
    constexpr auto expected = "UPDATE orders SET version = ? WHERE id = ? AND version = ?";
    check(f.dialect.compile_delete(f.resource, descriptor, deletion).sql == expected &&
              f.dialect.compile_recover(f.resource, descriptor, recovery).sql == expected,
          "original soft-delete and recovery SQL");
    check(
        f.dialect.compile_create_table(f.resource, descriptor) ==
            "CREATE TABLE IF NOT EXISTS orders (id INTEGER PRIMARY KEY NOT NULL, version INTEGER NOT NULL, name VARCHAR(255))",
        "original complete table DDL");
}

static void column_layout_cache_uses_parameterized_sql_not_comments() {
    Fixture f;
    f.exec("CREATE TABLE sample (id INTEGER, enabled BOOLEAN)");
    f.exec("INSERT INTO sample (id, enabled) VALUES (1, 1)");
    CompiledQuery query(std::allocator_arg, Allocator(f.resource));
    query.sql = "SELECT id, enabled FROM sample WHERE id = ?";
    query.params.emplace_back(std::int64_t(1));
    query.comment.emplace("first purpose", f.resource);
    auto rows = f.transport.fetch_all_compact(f.resource, query);
    check(rows && *rows.value()[0].get("enabled") == Value(f.resource, true),
          "original declared boolean decode");
    query.comment.emplace("different purpose", f.resource);
    check(bool(f.transport.fetch_all_compact(f.resource, query)), "changed comment executes");
    check(f.transport.cached_column_layout_count() == 1,
          "original layout cache keys raw parameterized SQL independently of comments");
}

static void sqlite_executor_parses_json_only_for_json_columns() {
    Fixture f;
    f.exec("CREATE TABLE payloads (text_payload TEXT, json_payload JSON)");
    CompiledQuery query(std::allocator_arg, Allocator(f.resource));
    query.sql = "INSERT INTO payloads (text_payload, json_payload) VALUES (?, ?)";
    query.params.emplace_back(R"({"active":true})");
    query.params.emplace_back(*Json::parse(f.resource, R"({"active":true})"));
    check(bool(f.transport.execute(f.resource, query)), "source typed payload insert");
    auto rows = f.rows("SELECT text_payload, json_payload FROM payloads");
    check(*rows[0].get("text_payload") == Value(f.resource, R"({"active":true})") &&
              *rows[0].get("json_payload") ==
                  Value(f.resource, *Json::parse(f.resource, R"({"active":true})")),
          "original identical content preserves distinct Text and Json variants");
}

static void sqlite_id_space_generator_increments_ids() {
    Fixture f;
    auto& generator = f.user->make<SqliteIdSpaceGenerator>(f.transport);
    auto first = generator.next_id(f.resource, "Order");
    auto second = generator.next_id(f.resource, "Order");
    check(first && second && first.value() == 1 && second.value() == 2,
          "original Order ID sequence");
}

static void sqlite_id_space_overflow_reports_safe_actionable_context() {
    Fixture f;
    auto& generator = f.user->make<SqliteIdSpaceGenerator>(f.transport);
    generator.with_table_name("teaql_id_space_diagnostic");
    check(bool(generator.ensure_table(f.resource)), "source diagnostic table exists");
    CompiledQuery query(std::allocator_arg, Allocator(f.resource));
    query.sql = "INSERT INTO teaql_id_space_diagnostic(type_name, current_level) VALUES (?, ?)";
    query.params.emplace_back("order");
    query.params.emplace_back(INT64_MAX);
    check(bool(f.transport.execute(f.resource, query)), "source exhausted ID space stored");
    auto result = generator.next_id(f.resource, "Order");
    check(!result, "original exhausted ID allocation fails");
    auto message = result.error().to_string(f.resource);
    for (auto part :
         {"SQLite ID provider", "ID space order", "teaql_id_space_diagnostic", "attempt 1"}) {
        check(message.find(part) != String::npos, "original actionable ID overflow context");
    }
    check(message.find("sqlite:") == String::npos, "original ID diagnostic omits connection URL");
}

static void buffered_stream_contract(std::uint64_t count) {
    Fixture f;
    auto descriptor = entity(f.resource);
    f.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, version INTEGER, name VARCHAR(255))");
    for (std::uint64_t index = 1; index <= count; ++index) {
        char number[32];
        auto end = std::to_chars(number, number + sizeof(number), index).ptr;
        String name("order-", f.resource);
        name.append(number, end);
        InsertCommand insert(std::allocator_arg, Allocator(f.resource), "Order");
        insert.value("id", index).value("version", std::int64_t(1)).value("name", name);
        f.seed(descriptor, insert);
    }
    SelectQuery query(std::allocator_arg, Allocator(f.resource), "Order");
    query.filter(Expr::gt(f.resource, "version", std::int64_t(0))).stream(10);
    if (count != 0) {
        query.order_asc("id");
    }
    auto compiled = f.dialect.compile_select(f.resource, descriptor, query);
    auto result = f.transport.fetch_stream(f.resource, compiled, 10);
    check(bool(result), "original buffered stream succeeds");
    const auto& chunks = result.value();
    if (count == 0) {
        check(chunks.size() == 1 && chunks[0].rows.empty() && chunks[0].is_last,
              "original empty buffered result contains one final empty chunk");
        return;
    }
    check(chunks.size() == 3 && chunks[0].rows.size() == 10 && !chunks[0].is_last &&
              chunks[1].rows.size() == 10 && !chunks[1].is_last && chunks[2].is_last,
          "original full chunks and terminal chunk flags");
    if (count == 20) {
        check(chunks[2].rows.empty(), "original exact boundary adds a final empty buffered chunk");
    } else {
        check(chunks[0].chunk_index == 0 && chunks[1].chunk_index == 1 &&
                  chunks[2].chunk_index == 2 && chunks[2].rows.size() == 5 &&
                  *chunks[0].rows[0].get("name") == Value(f.resource, "order-1") &&
                  *chunks[2].rows[4].get("name") == Value(f.resource, "order-25"),
              "original 25-row chunk indices, sizes and endpoint rows");
    }
}

static void test_parse_sqlite_timestamp() {
    Fixture f;
    for (auto input : {"2023-01-01 12:30:45",
                       "2023-01-01",
                       "2023-01-01T12:30:45Z",
                       "2026-08-23 10:43:16.152546+00",
                       "2026-08-23 10:43:16.152546"}) {
        auto value =
            detail::decode_sqlite_text(f.resource, input, detail::SqliteDecodeKind::Timestamp);
        check(value && std::holds_alternative<Timestamp>(value.value().data),
              "all five original general timestamp inputs preserve their variant");
    }
    check(detail::parse_fixed_sqlite_timestamp("2024-01-01 00:00:00+00") ==
                  Timestamp(1704067200000LL) &&
              detail::parse_fixed_sqlite_timestamp("2024-01-01T08:00:00.123+08:00") ==
                  Timestamp(1704067200123LL) &&
              detail::parse_fixed_sqlite_timestamp("2023-12-31 19:00:00-0500") ==
                  Timestamp(1704067200000LL) &&
              !detail::parse_fixed_sqlite_timestamp("2024-13-01 00:00:00Z") &&
              !detail::parse_fixed_sqlite_timestamp("2024-01-01 00:00:00+24"),
          "all original private fast-path exact epoch and invalid offset/calendar assertions");
    check(!detail::decode_sqlite_text(f.resource, "invalid", detail::SqliteDecodeKind::Timestamp),
          "original general timestamp invalid input fails");
}

static void declared_text_does_not_infer_timestamp_from_content() {
    Fixture f;
    for (auto declaration : {"TEXT", "VARCHAR(255)", "CHAR(32)", "CLOB"}) {
        auto value = detail::decode_sqlite_text(
            f.resource, "2024-01-01 00:57:55", detail::sqlite_decode_kind(declaration));
        check(value && value.value() == Value(f.resource, "2024-01-01 00:57:55"),
              "all four source declarations preserve timestamp-looking Text exactly");
    }
}

static void declared_column_types_compile_to_decode_kinds() {
    using Kind = detail::SqliteDecodeKind;
    check(detail::sqlite_decode_kind("BOOLEAN") == Kind::Bool &&
              detail::sqlite_decode_kind("decimal(20, 4)") == Kind::Decimal &&
              detail::sqlite_decode_kind(" VARCHAR(255) ") == Kind::Text &&
              detail::sqlite_decode_kind("datetime") == Kind::Timestamp &&
              detail::sqlite_decode_kind("custom") == Kind::Infer &&
              detail::sqlite_decode_kind({}) == Kind::Infer,
          "all six original declared/absent column decode kinds");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ensure_schema_registers_soundex_idempotently();
        streaming_sql_yields_bounded_chunks_and_releases_cursor_on_drop();
        decimal_bind_is_numeric_and_comparable();
        temporal_debug_sql_is_executable_and_matches_prepared_storage();
        sqlite_dialect_compiles_mutations_and_schema();
        column_layout_cache_uses_parameterized_sql_not_comments();
        sqlite_executor_parses_json_only_for_json_columns();
        sqlite_id_space_generator_increments_ids();
        sqlite_id_space_overflow_reports_safe_actionable_context();
        buffered_stream_contract(25);
        buffered_stream_contract(0);
        buffered_stream_contract(20);
        test_parse_sqlite_timestamp();
        declared_text_does_not_infer_timestamp_from_content();
        declared_column_types_compile_to_decode_kinds();
        std::cout << "Original fifteen SQLite storage contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
