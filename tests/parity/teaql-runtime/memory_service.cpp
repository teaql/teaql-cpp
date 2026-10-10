// Direct roots that exercise the original cfg(test)-only memory service oracle.
#include "root_fixture.hpp"
#include "queue_fixture.hpp"
#include "../memory_data_service.hpp"

using namespace original_runtime_queue;
using original_memory::MemoryDataService;

struct MemoryFixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory owner = user->memory_resource();
    InMemoryMetadataStore& metadata = user->make<InMemoryMetadataStore>();
    MemoryDataService& service = user->make<MemoryDataService>(metadata);

    MemoryFixture() {
        metadata.with_entity(entity(owner));
    }

    void
    rows(std::initializer_list<std::tuple<std::uint64_t, std::int64_t, std::string_view>> input) {
        Vector<CompactRow> values(owner);
        for (const auto& [id, version, name] : input) {
            values.emplace_back(record(owner,
                                       {{"id", Value(owner, id)},
                                        {"version", Value(owner, version)},
                                        {"name", Value(owner, name)}}));
        }
        service.with_rows("Order", values);
    }

    SelectQuery query() const {
        return SelectQuery(std::allocator_arg, Allocator(owner), "Order");
    }
};

static void field_is(const CompactRow& row, std::string_view field, const Value& expected) {
    auto* actual = row.get(field);
    check(actual && *actual == expected,
          "original memory-oracle result preserves exact Value variant");
}

static void memory_data_service_fetches_smart_list_entities_with_query_features() {
    MemoryFixture f;
    f.rows({{1, 1, "alpha"}, {2, 1, "beta"}, {3, 1, "gamma"}});
    auto query = f.query();
    query
        .filter(Expr::binary(
            Expr::column(f.owner, "id"), BinaryOp::Gte, Expr::value(f.owner, std::uint64_t(2))))
        .order_desc("id")
        .limit(1);
    auto result = f.service.fetch_entities<Order>(query);
    check(result && result.value().data.size() == 1 && result.value().data[0].id == 3 &&
              result.value().data[0].revision == 1 && result.value().data[0].name == "gamma",
          "original complete memory-oracle typed entity, ID and version vectors");
}

static void memory_data_service_runs_relation_aggregates() {
    MemoryFixture f;
    f.metadata.with_entity(line_entity(f.owner));
    f.rows({{1, 1, "first"}, {2, 1, "second"}});
    Vector<CompactRow> lines(f.owner);
    for (const auto& [id, order, name] :
         {std::tuple{10u, 1u, "line1"}, {11u, 1u, "line2"}, {12u, 2u, "line3"}}) {
        lines.emplace_back(record(f.owner,
                                  {{"id", Value(f.owner, std::uint64_t(id))},
                                   {"version", Value(f.owner, std::int64_t(1))},
                                   {"order_id", Value(f.owner, std::uint64_t(order))},
                                   {"name", Value(f.owner, name)}}));
    }
    f.service.with_rows("OrderLine", lines);
    auto query = f.query();
    query.project("id").project("name");
    SelectQuery child(std::allocator_arg, Allocator(f.owner), "OrderLine");
    RelationAggregate aggregate(
        std::allocator_arg, Allocator(f.owner), "lines", "lineCount", child, true);
    auto rows = f.service.fetch_all_with_relation_aggregates(
        query, std::span<const RelationAggregate>(&aggregate, 1));
    check(rows && rows.value().size() == 2, "original memory relation-aggregate row count");
    for (std::uint64_t id : {1, 2}) {
        auto found = std::ranges::find_if(rows.value(), [&](const auto& row) {
            return row.get("id") && *row.get("id") == Value(f.owner, id);
        });
        check(found != rows.value().end(), "original parent ID exists");
        field_is(*found, "lineCount", Value(f.owner, std::uint64_t(id == 1 ? 2 : 1)));
    }
}

static void memory_data_service_runs_aggregates() {
    MemoryFixture f;
    f.rows({{1, 1, "alpha"}, {2, 2, "beta"}});
    auto query = f.query();
    query.count_field("id", "count").sum("version", "versionSum");
    auto rows = f.service.fetch_all(query);
    check(rows && rows.value().size() == 1, "original memory aggregate row count");
    field_is(rows.value()[0], "count", Value(f.owner, std::uint64_t(2)));
    field_is(rows.value()[0], "versionSum", Value(f.owner, std::uint64_t(3)));
}

static void memory_data_service_runs_grouped_aggregates_and_extended_filters() {
    MemoryFixture f;
    f.rows({{1, 1, "alpha"}, {2, 2, "alpha"}, {3, 3, "tmp-beta"}});
    Vector<Value> excluded(f.owner);
    excluded.emplace_back("deleted");
    auto predicate =
        Expr::between(
            f.owner, "version", Value(f.owner, std::int64_t(1)), Value(f.owner, std::int64_t(3)))
            .and_expr(Expr::not_like(f.owner, "name", "tmp%"))
            .and_expr(Expr::not_in_list(f.owner, "name", excluded));
    auto query = f.query();
    query.filter(predicate).group_by("name").count("total").sum("version", "versionSum");
    auto rows = f.service.fetch_all(query);
    check(rows && rows.value().size() == 1, "original filtered group count");
    field_is(rows.value()[0], "name", Value(f.owner, "alpha"));
    field_is(rows.value()[0], "total", Value(f.owner, std::uint64_t(2)));
    field_is(rows.value()[0], "versionSum", Value(f.owner, std::uint64_t(3)));
}

static void memory_data_service_runs_extended_aggregates_and_having() {
    MemoryFixture f;
    f.rows({{1, 1, "alpha"}, {2, 3, "alpha"}, {3, 7, "beta"}});
    auto query = f.query();
    query.group_by("name")
        .count("total")
        .stddev("version", "stddevVersion")
        .var_pop("version", "varPopVersion")
        .bit_or("version", "bitOrVersion")
        .having(Expr::gt(f.owner, "total", std::int64_t(1)));
    auto rows = f.service.fetch_all(query);
    check(rows && rows.value().size() == 1, "original HAVING selects only alpha");
    field_is(rows.value()[0], "name", Value(f.owner, "alpha"));
    field_is(rows.value()[0], "total", Value(f.owner, std::uint64_t(2)));
    auto* deviation = rows.value()[0].get("stddevVersion");
    check(deviation &&
              deviation->to_json_value(f.owner) == Json(f.owner, "1.4142135623730951454746218583"),
          "original exact retained-f64 Decimal standard-deviation JSON");
    field_is(rows.value()[0], "varPopVersion", Value(f.owner, Decimal::ONE));
    field_is(rows.value()[0], "bitOrVersion", Value(f.owner, std::int64_t(3)));
}

static void memory_data_service_runs_sound_like_filter() {
    MemoryFixture f;
    f.rows({{1, 1, "Robert"}, {2, 1, "Rupert"}, {3, 1, "Ashcraft"}});
    auto query = f.query();
    query.filter(Expr::sound_like(f.owner, "name", "Robert")).order_asc("id");
    auto rows = f.service.fetch_all(query);
    check(rows && rows.value().size() == 2, "original Soundex result count");
    field_is(rows.value()[0], "name", Value(f.owner, "Robert"));
    field_is(rows.value()[1], "name", Value(f.owner, "Rupert"));
}

static void memory_data_service_runs_java_style_string_match_filters() {
    MemoryFixture f;
    f.rows({{1, 1, "tea-order"}, {2, 1, "coffee-order"}, {3, 1, "tea-archived"}});
    auto predicate = Expr::contain(f.owner, "name", "tea")
                         .and_expr(Expr::begin_with(f.owner, "name", "tea"))
                         .and_expr(Expr::end_with(f.owner, "name", "order"))
                         .and_expr(Expr::not_contain(f.owner, "name", "coffee"))
                         .and_expr(Expr::not_begin_with(f.owner, "name", "archived"))
                         .and_expr(Expr::not_end_with(f.owner, "name", "draft"));
    auto query = f.query();
    query.filter(predicate).order_asc("id");
    auto rows = f.service.fetch_all(query);
    check(rows && rows.value().size() == 1, "original six string predicates retain one row");
    field_is(rows.value()[0], "name", Value(f.owner, "tea-order"));
}

static void memory_data_service_runs_property_to_property_filters() {
    MemoryFixture f;
    f.rows({{1, 2, "keep"}, {2, 1, "skip"}});
    auto query = f.query();
    query.filter(Expr::compare_columns(f.owner, "version", BinaryOp::Gte, "id")).order_asc("id");
    auto rows = f.service.fetch_all(query);
    check(rows && rows.value().size() == 1, "original property comparison retains one row");
    field_is(rows.value()[0], "name", Value(f.owner, "keep"));
}

static void memory_data_service_supports_mutations_and_optimistic_locking() {
    MemoryFixture f;
    InsertCommand insert(std::allocator_arg, Allocator(f.owner), "Order");
    insert.value("id", std::uint64_t(10)).value("version", std::int64_t(1)).value("name", "draft");
    check(bool(f.service.insert(insert)), "original oracle insert succeeds");
    UpdateCommand update(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(10));
    update.expected_version(1).value("name", "submitted");
    check(bool(f.service.update(update)), "original oracle version1 update succeeds");
    auto query = f.query();
    query.filter(Expr::eq(f.owner, "id", std::uint64_t(10)));
    auto rows = f.service.fetch_all(query);
    check(rows && !rows.value().empty(), "original oracle updated row exists");
    field_is(rows.value().back(), "name", Value(f.owner, "submitted"));
    field_is(rows.value().back(), "version", Value(f.owner, std::int64_t(2)));
    update.value("name", "stale");
    auto conflict = f.service.update(update);
    check(!conflict && conflict.error().kind() == MemoryDataService::Error::Kind::Runtime &&
              conflict.error().runtime_error().kind() == RuntimeError::Kind::OptimisticLockConflict,
          "original oracle stale update preserves typed optimistic-lock conflict");
    DeleteCommand deletion(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(10));
    deletion.expected_version(2);
    check(bool(f.service.delete_command(deletion)), "original oracle soft delete succeeds");
    rows = f.service.fetch_all(query);
    check(rows && !rows.value().empty(), "original soft-deleted row remains queryable");
    field_is(rows.value().back(), "version", Value(f.owner, std::int64_t(-3)));
    RecoverCommand recover(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(10), -3);
    check(bool(f.service.recover(recover)), "original oracle recovery succeeds");
    rows = f.service.fetch_all(query);
    check(rows && !rows.value().empty(), "original recovered row exists");
    field_is(rows.value().back(), "version", Value(f.owner, std::int64_t(4)));
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    struct Restore {
        Memory previous;

        ~Restore() {
            std::pmr::set_default_resource(previous);
        }
    } restore{previous};

    try {
        if (argc == 2 && std::string_view(argv[1]) == "mutation-only") {
            std::cout << "running 1 test\n";
            memory_data_service_supports_mutations_and_optimistic_locking();
            return 0;
        }
        memory_data_service_fetches_smart_list_entities_with_query_features();
        memory_data_service_runs_relation_aggregates();
        memory_data_service_runs_aggregates();
        memory_data_service_runs_grouped_aggregates_and_extended_filters();
        memory_data_service_runs_extended_aggregates_and_having();
        memory_data_service_runs_sound_like_filter();
        memory_data_service_runs_java_style_string_match_filters();
        memory_data_service_runs_property_to_property_filters();
        memory_data_service_supports_mutations_and_optimistic_locking();
        std::cout << "Nine original cfg(test) memory-service contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
