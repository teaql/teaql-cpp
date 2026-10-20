// Direct pinned SQLite row/query contracts using the actual provider and executor.
#include "fixture.hpp"
#include "feature_flag.hpp"
#include <teaql/provider_sqlite/id.hpp>
#include <sqlite3.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

using namespace original_provider;

static EntityDescriptor entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Order");
    result.table("orders");
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version().not_null();
    result.property("name", DataType::Text);
    return result;
}

static EntityDescriptor line_entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "OrderLine");
    result.table("order_line");
    result.property("id", DataType::U64).id().not_null();
    result.property("order_id", DataType::U64).not_null();
    result.property("name", DataType::Text);
    return result;
}

static Vector<Value> ids(Fixture& f, const EntityDescriptor& descriptor, const SelectQuery& query) {
    auto rows = f.transport.fetch_all_compact(
        f.resource, f.dialect.compile_select(f.resource, descriptor, query));
    check(bool(rows), "original compiled query executes against real SQLite");
    Vector<Value> result(f.resource);
    for (const auto& row : rows.value()) {
        check(row.get("id") != nullptr, "original id projection is present");
        result.emplace_back(*row.get("id"));
    }
    return result;
}

static void
ids_are(Fixture& f, const Vector<Value>& actual, std::initializer_list<std::int64_t> expected) {
    Vector<Value> values(f.resource);
    for (auto id : expected) {
        values.emplace_back(id);
    }
    check(actual == values, "complete original ordered I64 id vector");
}

static void complete_scalar_fixture_including_nullable_boolean_executes_on_sqlite() {
    Fixture f;
    f.exec(
        "CREATE TABLE query_record_scalar (id INTEGER PRIMARY KEY, required_text TEXT, optional_text TEXT, required_integer INTEGER, optional_long INTEGER, required_decimal NUMERIC, required_float REAL, required_double REAL, required_date DATE, required_time INTEGER, required_timestamp TIMESTAMP, active BOOLEAN, reviewed BOOLEAN, version INTEGER)");
    f.exec(
        "INSERT INTO query_record_scalar VALUES (1,'Alpha','optional',42,42000000000,42.125,42.5,42.75,'2026-08-29',34200000,1777632600000,1,0,1), (2,'Beta',NULL,7,NULL,7.500,7.5,7.75,'2026-08-30',36000000,1777720400000,0,NULL,1), (3,'Gamma','tail',99,99000000000,99.875,99.5,99.75,'2026-08-31',37800000,1777808200000,1,1,1)");
    EntityDescriptor descriptor(std::allocator_arg, Allocator(f.resource), "QueryRecord");
    descriptor.table("query_record_scalar");
    descriptor.property("id", DataType::U64).id().not_null();
    for (auto field : {"required_text", "optional_text"}) {
        descriptor.property(field, DataType::Text);
    }
    for (auto field : {"required_integer", "optional_long"}) {
        descriptor.property(field, DataType::I64);
    }
    descriptor.property("required_decimal", DataType::Decimal);
    descriptor.property("required_float", DataType::F64);
    descriptor.property("required_double", DataType::F64);
    descriptor.property("required_date", DataType::Date);
    descriptor.property("required_time", DataType::I64);
    descriptor.property("required_timestamp", DataType::Timestamp);
    descriptor.property("active", DataType::Bool);
    descriptor.property("reviewed", DataType::Bool);
    descriptor.property("version", DataType::I64).version().not_null();
    auto check_ids = [&](Expr predicate, std::initializer_list<std::int64_t> expected) {
        SelectQuery query(std::allocator_arg, Allocator(f.resource), "QueryRecord");
        query.project("id").filter(predicate).order_asc("id");
        ids_are(f, ids(f, descriptor, query), expected);
    };
    check_ids(Expr::eq(f.resource, "required_text", "Alpha"), {1});
    check_ids(Expr::ne(f.resource, "required_text", "Alpha"), {2, 3});
    Vector<Value> names(f.resource);
    names.emplace_back("Alpha");
    names.emplace_back("Gamma");
    check_ids(Expr::in_list(f.resource, "required_text", names), {1, 3});
    check_ids(Expr::contain(f.resource, "required_text", "et"), {2});
    check_ids(Expr::between(f.resource,
                            "required_integer",
                            Value(f.resource, std::int64_t(40)),
                            Value(f.resource, std::int64_t(100))),
              {1, 3});
    check_ids(Expr::gt(f.resource, "required_decimal", Decimal::from_i64(50)), {3});
    check_ids(Expr::lte(f.resource, "required_float", 7.5), {2});
    check_ids(Expr::gte(f.resource, "required_double", 99.75), {3});
    check_ids(Expr::between(f.resource,
                            "required_date",
                            Value(f.resource, *Date::from_ymd(2026, 8, 30)),
                            Value(f.resource, *Date::from_ymd(2026, 8, 31))),
              {2, 3});
    check_ids(Expr::gt(f.resource, "required_time", std::int64_t(36000000)), {3});
    check_ids(Expr::lt(f.resource, "required_timestamp", Timestamp(1777750000000LL)), {1, 2});
    check_ids(Expr::is_null(f.resource, "optional_text"), {2});
    check_ids(Expr::is_not_null(f.resource, "optional_long"), {1, 3});
    check_ids(Expr::eq(f.resource, "active", false), {2});
    check_ids(Expr::eq(f.resource, "reviewed", true), {3});
    check_ids(Expr::eq(f.resource, "reviewed", false), {1});
    check_ids(Expr::is_null(f.resource, "reviewed"), {2});
}

static void relation_subqueries_execute_positive_and_negative_predicates_on_sqlite() {
    Fixture f;
    f.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, version INTEGER, name TEXT)");
    f.exec("CREATE TABLE order_line (id INTEGER PRIMARY KEY, order_id INTEGER, name TEXT)");
    f.exec("INSERT INTO orders VALUES (1, 1, 'first'), (2, 1, 'second'), (3, 1, 'third')");
    f.exec(
        "INSERT INTO order_line VALUES (10, 1, 'priority'), (11, 1, 'ordinary'), (12, 2, 'ordinary'), (13, NULL, 'orphan')");
    auto order = entity(f.resource);
    auto line = line_entity(f.resource);
    auto check_order = [&](Expr predicate, std::initializer_list<std::int64_t> expected) {
        SelectQuery query(std::allocator_arg, Allocator(f.resource), "Order");
        query.project("id").filter(predicate).order_asc("id");
        ids_are(f, ids(f, order, query), expected);
    };
    auto check_line = [&](Expr predicate, std::initializer_list<std::int64_t> expected) {
        SelectQuery query(std::allocator_arg, Allocator(f.resource), "OrderLine");
        query.project("id").filter(predicate).order_asc("id");
        ids_are(f, ids(f, line, query), expected);
    };
    SelectQuery matching(std::allocator_arg, Allocator(f.resource), "OrderLine");
    matching.filter(Expr::eq(f.resource, "name", "priority"));
    check_order(Expr::in_subquery(f.resource, "id", line, matching, "order_id"), {1});
    check_order(Expr::not_in_subquery(f.resource, "id", line, matching, "order_id"), {2, 3});
    check_line(Expr::is_not_null(f.resource, "order_id"), {10, 11, 12});
    check_line(Expr::is_null(f.resource, "order_id"), {13});
    SelectQuery first(std::allocator_arg, Allocator(f.resource), "Order");
    first.filter(Expr::eq(f.resource, "name", "first"));
    check_line(Expr::in_subquery(f.resource, "order_id", order, first, "id"), {10, 11});
    check_line(Expr::not_in_subquery(f.resource, "order_id", order, first, "id"), {12});
    SelectQuery all(std::allocator_arg, Allocator(f.resource), "OrderLine");
    check_order(Expr::in_subquery(f.resource, "id", line, all, "order_id"), {1, 2});
    check_order(Expr::not_in_subquery(f.resource, "id", line, all, "order_id"), {3});
}

static void sqlite_executes_partitioned_relation_limit_per_parent() {
    Fixture f;
    auto descriptor = line_entity(f.resource);
    f.metadata.register_entity(detail::clone_in(f.resource, descriptor));
    use_sqlite_provider(*f.user, *f.user, f.transport);
    check(bool(f.user->ensure_schema().wait()), "original partition fixture schema");
    for (std::uint64_t parent : {11, 12}) {
        for (std::uint64_t index = 1; index <= 5; ++index) {
            auto id = parent * 100 + index;
            String name("line-", f.resource);
            name += std::to_string(id);
            InsertCommand insert(std::allocator_arg, Allocator(f.resource), "OrderLine");
            insert.value("id", id).value("order_id", parent).value("name", name);
            f.seed(descriptor, insert);
        }
    }
    SelectQuery query(std::allocator_arg, Allocator(f.resource), "OrderLine");
    query.project("id").project("order_id").order_desc("id").limit(3).partition_by("order_id");
    auto result = f.transport.fetch_all_compact(
        f.resource, f.dialect.compile_select(f.resource, descriptor, query));
    check(result && result.value().size() == 6, "original two-parent limit returns six rows");
    for (std::int64_t parent : {11, 12}) {
        Vector<Value> actual(f.resource);
        for (const auto& row : result.value()) {
            auto* owner = row.get("order_id");
            if (owner && *owner == Value(f.resource, parent)) {
                if (auto* id = row.get("id")) {
                    actual.emplace_back(*id);
                }
            }
        }
        ids_are(f, actual, {parent * 100 + 5, parent * 100 + 4, parent * 100 + 3});
    }
}

using Child = std::pair<std::optional<Value>, std::optional<Value>>;

static Vector<Child> children(Memory owner, std::span<const CompactRow> rows, std::int64_t parent) {
    Vector<Child> result(owner);
    for (const auto& row : rows) {
        auto* parent_id = row.get("order_id");
        if (!parent_id || *parent_id != Value(owner, parent)) {
            continue;
        }
        auto optional = [&](std::string_view field) -> std::optional<Value> {
            if (auto* value = row.get(field)) {
                return std::optional<Value>(std::in_place, owner, *value);
            }
            return {};
        };
        result.emplace_back(optional("id"), optional("name"));
    }
    return result;
}

static void topn_005_007_window_and_probes_preserve_results_and_predicates() {
    Fixture f;
    auto descriptor = line_entity(f.resource);
    f.metadata.register_entity(detail::clone_in(f.resource, descriptor));
    use_sqlite_provider(*f.user, *f.user, f.transport);
    check(bool(f.user->ensure_schema().wait()), "original TOPN fixture schema");
    for (std::uint64_t parent : {11, 12, 13}) {
        for (std::uint64_t index = 1; index <= 5; ++index) {
            InsertCommand insert(std::allocator_arg, Allocator(f.resource), "OrderLine");
            insert.value("id", parent * 100 + index)
                .value("order_id", parent)
                .value("name", index == 4 ? "excluded" : "visible");
            f.seed(descriptor, insert);
        }
    }
    Vector<Value> parents(f.resource);
    parents.emplace_back(std::uint64_t(11));
    parents.emplace_back(std::uint64_t(12));
    SelectQuery base(std::allocator_arg, Allocator(f.resource), "OrderLine");
    base.project("id")
        .project("order_id")
        .project("name")
        .filter(Expr::in_list(f.resource, "order_id", parents))
        .and_filter(Expr::eq(f.resource, "name", "visible"))
        .order_desc("id")
        .limit(3)
        .partition_by("order_id");
    auto execute = [&](SelectQuery query) -> const Vector<CompactRow>& {
        query.comment("TOPN plan equivalence").purpose("verify provider query behavior");
        QueryRequest request(
            std::allocator_arg, Allocator(f.resource), std::move(query), false, false);
        const auto& result = f.executor.query(f.resource, std::move(request)).wait();
        check(bool(result), "original actual query executor TOPN path succeeds");
        return result.value().rows;
    };
    const auto& probes = execute(base.clone(f.resource));
    base.top_n_probe_parent_threshold(0);
    const auto& window = execute(std::move(base));
    for (std::int64_t parent : {11, 12}) {
        check(children(f.resource, probes, parent) == children(f.resource, window, parent),
              "complete original optional ID/name tuple equivalence between both executor plans");
        Vector<Child> expected(f.resource);
        for (std::int64_t index : {5, 3, 2}) {
            expected.emplace_back(
                std::optional<Value>(std::in_place, f.resource, parent * 100 + index),
                std::optional<Value>(std::in_place, f.resource, "visible"));
        }
        check(children(f.resource, window, parent) == expected,
              "both original complete ordered parent tuple vectors");
    }
    check(children(f.resource, probes, 13).empty() && children(f.resource, window, 13).empty(),
          "original filtered third parent never leaks through probes or window");
}

static Record feature_flag_record(Memory owner, Value enabled, Value optional) {
    Record result(owner);
    result.emplace("id", Value(owner, std::uint64_t(1)));
    result.emplace("version", Value(owner, std::int64_t(1)));
    result.emplace("enabled", std::move(enabled));
    result.emplace("optional_enabled", std::move(optional));
    return result;
}

static Vector<CompactRow> flags(Fixture& f, const EntityDescriptor& descriptor) {
    SelectQuery query(std::allocator_arg, Allocator(f.resource), "FeatureFlag");
    query.order_asc("id");
    auto result = f.transport.fetch_all_compact(
        f.resource, f.dialect.compile_select(f.resource, descriptor, query));
    check(bool(result), "source flag query succeeds");
    return std::move(result).value();
}

static void sqlite_boolean_new_schema_roundtrips_as_bool() {
    Fixture f;
    auto descriptor = FeatureFlagRow::entity_descriptor(f.resource);
    auto ddl = f.dialect.compile_create_table(f.resource, descriptor);
    check(ddl.find("enabled BOOLEAN NOT NULL") != String::npos &&
              ddl.find("optional_enabled BOOLEAN") != String::npos &&
              ddl.find("enabled INTEGER") == String::npos,
          "all original new BOOLEAN declaration assertions");
    f.metadata.register_entity(detail::clone_in(f.resource, descriptor));
    use_sqlite_provider(*f.user, *f.user, f.transport);
    check(bool(f.user->ensure_schema().wait()), "new Boolean schema exists");
    for (std::uint64_t id : {1, 2}) {
        InsertCommand insert(std::allocator_arg, Allocator(f.resource), "FeatureFlag");
        insert.value("id", id)
            .value("version", std::int64_t(1))
            .value("enabled", id == 2)
            .value("optional_enabled", id == 1);
        auto result = f.transport.execute(f.resource,
                                          f.dialect.compile_insert(f.resource, descriptor, insert));
        check(result && result.value() == 1, "each source Boolean insert affects one row");
    }
    auto rows = flags(f, descriptor);
    check(rows.size() == 2 && *rows[0].get("enabled") == Value(f.resource, false) &&
              *rows[0].get("optional_enabled") == Value(f.resource, true) &&
              *rows[1].get("enabled") == Value(f.resource, true) &&
              *rows[1].get("optional_enabled") == Value(f.resource, false),
          "all four original provider Bool values");
    auto first =
        FeatureFlagRow::from_compact_row(f.resource, detail::clone_in(f.resource, rows[0]));
    auto second =
        FeatureFlagRow::from_compact_row(f.resource, detail::clone_in(f.resource, rows[1]));
    check(first && second && !first.value().enabled && first.value().optional_enabled == true &&
              second.value().enabled && second.value().optional_enabled == false,
          "all four original handwritten entity Boolean fields");
}

static void sqlite_boolean_legacy_integer_schema_maps_only_binary_values() {
    Fixture f;
    auto descriptor = FeatureFlagRow::entity_descriptor(f.resource);
    f.exec(
        "CREATE TABLE feature_flags (id INTEGER PRIMARY KEY, version INTEGER NOT NULL, enabled INTEGER NOT NULL, optional_enabled INTEGER)");
    InsertCommand insert(std::allocator_arg, Allocator(f.resource), "FeatureFlag");
    insert.value("id", std::uint64_t(1))
        .value("version", std::int64_t(1))
        .value("enabled", true)
        .value("optional_enabled", false);
    f.seed(descriptor, insert);
    CompiledQuery second(std::allocator_arg, Allocator(f.resource));
    second.sql =
        "INSERT INTO feature_flags (id, version, enabled, optional_enabled) VALUES (?, ?, ?, ?)";
    second.params.emplace_back(std::uint64_t(2));
    second.params.emplace_back(std::int64_t(1));
    second.params.emplace_back(std::int64_t(2));
    second.params.emplace_back();
    check(bool(f.transport.execute(f.resource, second)), "original nonbinary legacy row inserted");
    auto rows = flags(f, descriptor);
    check(rows.size() == 2 && *rows[0].get("version") == Value(f.resource, std::int64_t(1)) &&
              *rows[0].get("enabled") == Value(f.resource, std::int64_t(1)) &&
              *rows[0].get("optional_enabled") == Value(f.resource, std::int64_t(0)) &&
              *rows[1].get("enabled") == Value(f.resource, std::int64_t(2)),
          "all original legacy provider I64 variants remain integer values");
    auto decoded =
        FeatureFlagRow::from_compact_row(f.resource, detail::clone_in(f.resource, rows[0]));
    check(decoded && decoded.value().enabled && decoded.value().optional_enabled == false,
          "original binary legacy fields map to typed Boolean values");
    auto rejected =
        FeatureFlagRow::from_compact_row(f.resource, detail::clone_in(f.resource, rows[1]));
    check(!rejected && rejected.error().message.find("invalid field enabled") != String::npos,
          "original present integer2 is a field error");
    for (unsigned kind = 0; kind < 4; ++kind) {
        auto value = kind < 2 ? Value(f.resource, std::int64_t(kind))
                              : Value(f.resource, std::uint64_t(kind - 2));
        auto values = feature_flag_record(f.resource, std::move(value), Value(f.resource));
        auto result = FeatureFlagRow::from_compact_row(
            f.resource, CompactRow(std::allocator_arg, Allocator(f.resource), std::move(values)));
        check(result && result.value().enabled == (kind % 2 == 1) &&
                  !result.value().optional_enabled,
              "all four original signed/unsigned binary mappings preserve optional null");
    }
    for (auto value : {std::int64_t(-1), std::int64_t(2)}) {
        auto values = feature_flag_record(f.resource, Value(f.resource, value), Value(f.resource));
        auto result = FeatureFlagRow::from_compact_row(
            f.resource, CompactRow(std::allocator_arg, Allocator(f.resource), std::move(values)));
        check(!result && result.error().message.find("invalid field enabled") != String::npos,
              "both original invalid signed Boolean integers fail");
    }
    auto values =
        feature_flag_record(f.resource, Value(f.resource, std::uint64_t(2)), Value(f.resource));
    auto invalid = FeatureFlagRow::from_compact_row(
        f.resource, CompactRow(std::allocator_arg, Allocator(f.resource), std::move(values)));
    check(!invalid && invalid.error().message.find("invalid field enabled") != String::npos,
          "original unsigned integer2 Boolean field fails");
    auto optional_values = feature_flag_record(
        f.resource, Value(f.resource, true), Value(f.resource, std::uint64_t(2)));
    auto optional = FeatureFlagRow::from_compact_row(
        f.resource,
        CompactRow(std::allocator_arg, Allocator(f.resource), std::move(optional_values)));
    check(!optional &&
              optional.error().message.find("invalid field optional_enabled") != String::npos,
          "original optional nonbinary Boolean has its own field diagnostic");
}

static void sqlite_id_space_generator_is_safe_across_connections() {
    auto path =
        std::filesystem::temp_directory_path() /
        ("teaql-id-space-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");

    struct Remove {
        std::filesystem::path path;

        ~Remove() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } cleanup{path};

    std::array<std::array<std::uint64_t, 25>, 4> allocated{};
    std::array<std::exception_ptr, 4> failures{};
    std::array<std::thread, 4> threads;
    for (std::size_t index = 0; index < threads.size(); ++index) {
        threads[index] = std::thread([&, index] {
            try {
                GlobalContext global;
                auto user = global.make_user_context();
                auto owner = user->memory_resource();
                auto& connection = user->make_resource<SqliteConnection>(path.string());
                check(sqlite3_busy_timeout(connection.native_handle(), 5000) == SQLITE_OK,
                      "original five-second ID contention budget");
                auto& provider = user->make<SqliteMutationExecutor>(connection);
                auto& generator = user->make<SqliteIdSpaceGenerator>(provider);
                for (auto& id : allocated[index]) {
                    auto result = generator.next_id(owner, "Order");
                    if (!result) {
                        throw std::runtime_error(std::string(result.error().to_string(owner)));
                    }
                    id = result.value();
                }
            } catch (...) {
                failures[index] = std::current_exception();
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (const auto& failure : failures) {
        if (failure) {
            std::rethrow_exception(failure);
        }
    }
    std::array<std::uint64_t, 100> ids{};
    for (std::size_t index = 0; index < allocated.size(); ++index) {
        std::copy(allocated[index].begin(), allocated[index].end(), ids.begin() + index * 25);
    }
    std::sort(ids.begin(), ids.end());
    for (std::size_t index = 0; index < ids.size(); ++index) {
        check(ids[index] == index + 1, "complete original four-connection ID set is exactly1..100");
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        complete_scalar_fixture_including_nullable_boolean_executes_on_sqlite();
        relation_subqueries_execute_positive_and_negative_predicates_on_sqlite();
        sqlite_executes_partitioned_relation_limit_per_parent();
        topn_005_007_window_and_probes_preserve_results_and_predicates();
        sqlite_boolean_new_schema_roundtrips_as_bool();
        sqlite_boolean_legacy_integer_schema_maps_only_binary_values();
        sqlite_id_space_generator_is_safe_across_connections();
        std::cout << "Original seven SQLite row/query contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
