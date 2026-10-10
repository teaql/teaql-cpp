// Remaining original root-library composition and query hard-limit contracts.
#include "entity_mapping_fixture.hpp"
#include <teaql/core/query.hpp>
#include <teaql/core/safe_expression.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;
using teaql::test::source_mapping::OrderRow;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static OrderRow
order(Memory owner, std::uint64_t id, std::int64_t revision, std::string_view name) {
    OrderRow result(std::allocator_arg, Allocator(owner));
    result.id = id;
    result.revision = revision;
    result.name = name;
    return result;
}

static void query_builders_cover_filters_sort_aggregates_and_relations(Memory owner) {
    Vector<Value> excluded(owner), ids(owner);
    excluded.emplace_back("archived");
    excluded.emplace_back("deleted");
    ids.emplace_back(std::uint64_t(1));
    ids.emplace_back(std::uint64_t(2));
    SelectQuery customer(std::allocator_arg, Allocator(owner), "Customer");
    customer.project("name").filter(Expr::eq(owner, "status", "active"));
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    query.projects({"id", "name"})
        .filter(Expr::gte(owner, "version", std::int64_t(1)))
        .and_filter(Expr::not_in_list(owner, "name", excluded))
        .and_filter(Expr::in_large(owner, "id", ids))
        .and_filter(Expr::contain(owner, "name", "rob"))
        .and_filter(Expr::sound_like(owner, "name", "Robert"))
        .and_filter(Expr::compare_columns(owner, "updated_at", BinaryOp::Gte, "created_at"))
        .or_filter(Expr::is_null(owner, "name"))
        .project_expr("nameSound", Expr::soundex(Expr::column(owner, "name")))
        .order_desc("id")
        .order_gbk_asc("name")
        .group_by("name")
        .count("total")
        .sum("version", "versionSum")
        .stddev("version", "versionStddev")
        .enable_aggregation_cache_for(1000)
        .propagate_aggregation_cache(2000)
        .having(Expr::gt(owner, "total", std::int64_t(1)))
        .relation("lines")
        .relation_query("customer", customer)
        .page(20, 10);
    Vector<String> projection(owner), groups(owner);
    projection.emplace_back("id");
    projection.emplace_back("name");
    groups.emplace_back("name");
    Vector<NamedExpr> expressions(owner);
    expressions.emplace_back("nameSound", Expr::soundex(Expr::column(owner, "name")));
    Vector<OrderBy> orders(owner);
    orders.emplace_back(OrderBy::desc(owner, "id"));
    orders.emplace_back(OrderBy::asc_gbk(owner, "name"));
    Vector<Aggregate> aggregates(owner);
    aggregates.emplace_back(Aggregate::count(owner, "total"));
    aggregates.emplace_back(Aggregate::sum(owner, "version", "versionSum"));
    aggregates.emplace_back(Aggregate::stddev(owner, "version", "versionStddev"));
    Vector<RelationLoad> relations(owner);
    relations.emplace_back("lines");
    relations.emplace_back("customer", customer);
    check(query.projection == projection && query.expr_projection == expressions &&
              query.order_by == orders && query.group_by_fields == groups &&
              query.aggregates == aggregates,
          "original complete ordered query projections, sort, groups and aggregates");
    check(query.aggregation_cache ==
                  std::optional<AggregationCacheOptions>({true, 1000, true, 2000}) &&
              query.having_predicate &&
              *query.having_predicate == Expr::gt(owner, "total", std::int64_t(1)) &&
              query.relations == relations && query.slice == std::optional<Slice>({10, 20}) &&
              query.predicate && query.predicate->kind == Expr::Kind::Or,
          "original optional cache, having, relations, page and outer Or variant");
}

static void smart_list_supports_entity_ids_versions_and_records(Memory owner) {
    Vector<OrderRow> initial(owner);
    initial.emplace_back(order(owner, 1, 2, "a"));
    initial.emplace_back(order(owner, 3, 4, "b"));
    SmartList<OrderRow> rows(std::allocator_arg, Allocator(owner), std::move(initial));
    Vector<Value> ids(owner);
    ids.emplace_back(std::uint64_t(1));
    ids.emplace_back(std::uint64_t(3));
    check(rows.ids() == ids && rows.versions() == Vector<std::int64_t>({2, 4}, owner),
          "original complete typed U64 IDs and signed versions");
    auto records = std::move(rows).into_values();
    check(records.len() == 2 &&
              records.data[0].fields.at(String("id", owner)) == Value(owner, std::uint64_t(1)) &&
              records.data[1].fields.at(String("version", owner)) == Value(owner, std::int64_t(4)),
          "original consuming record conversion and exact Value variants");
}

static void smart_list_supports_java_style_collection_helpers(Memory owner) {
    SmartList<OrderRow> rows(std::allocator_arg, Allocator(owner));
    rows.with_total_count(10)
        .with_aggregation("count", std::uint64_t(2))
        .with_summary("label", "orders");
    rows.push(order(owner, 1, 2, "a"));
    Vector<OrderRow> incoming(owner);
    incoming.emplace_back(order(owner, 3, 4, "b"));
    rows.extend(std::move(incoming));
    auto* second = rows.get(1);
    auto* last = rows.last();
    check(rows.total_count_or_len() == 10 && second && second->name == "b" && last &&
              last->id == 3 && rows.aggregation("count") &&
              *rows.aggregation("count") == Value(owner, std::uint64_t(2)) &&
              rows.summary_value("label") && *rows.summary_value("label") == Value(owner, "orders"),
          "original optional lookups and list metadata");
    check(rows.aggregation_json(owner) == *Json::parse(owner, R"({"count":2})") &&
              rows.summary_json(owner) == *Json::parse(owner, R"({"label":"orders"})"),
          "original metadata JSON");
    auto names = rows.to_list([owner](const OrderRow& row) {
        return String(row.name, owner);
    });
    check(names.size() == 2 && names[0] == "a" && names[1] == "b", "original complete name list");
    auto ids = rows.to_set([](const OrderRow& row) {
        return row.id;
    });
    const std::array<std::uint64_t, 2> expected_ids{1, 3};
    check(std::equal(ids.begin(), ids.end(), expected_ids.begin(), expected_ids.end()),
          "original ordered unique IDs");
    auto by_id = rows.map_by_id();
    check(by_id.at(String("u:1", owner)).name == "a" && by_id.at(String("u:3", owner)).name == "b",
          "original typed ID keys");
    auto identity = rows.identity_map([owner](const OrderRow& row) {
        return String(row.name, owner);
    });
    auto groups = rows.group_by([](const OrderRow& row) {
        return row.revision % 2;
    });
    check(identity.at(String("a", owner)).id == 1 && groups.at(0).size() == 2,
          "original identity and grouping");
    Vector<OrderRow> updates(owner);
    updates.emplace_back(order(owner, 3, 5, "b2"));
    updates.emplace_back(order(owner, 4, 1, "c"));
    rows.merge_by(std::move(updates), [](const OrderRow& row) {
        return row.id;
    });
    check(rows.len() == 3 && rows.map_by_id().at(String("u:3", owner)).revision == 5,
          "original keyed merge replacement");
    rows.retain([](const OrderRow& row) {
        return row.id != 1;
    });
    Vector<Value> retained(owner);
    retained.emplace_back(std::uint64_t(3));
    retained.emplace_back(std::uint64_t(4));
    check(rows.ids() == retained && std::distance(rows.begin(), rows.end()) == 2 &&
              rows[0].name == "b2",
          "original retained IDs, borrowed iterator count and first-row name");
}

struct SafeExpressionEntity : BaseEntityDefaults<SafeExpressionEntity> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "SafeExpressionEntity";
    BaseEntityData data;
    String name;
    SmartList<OrderRow> lines;

    SafeExpressionEntity(std::allocator_arg_t, Allocator a)
        : data(std::allocator_arg, a), name(a.resource()), lines(std::allocator_arg, a) {}

    SafeExpressionEntity(std::allocator_arg_t, Allocator a, const SafeExpressionEntity& other)
        : data(std::allocator_arg, a, other.data), name(other.name, a.resource()),
          lines(std::allocator_arg, a, other.lines) {}

    SafeExpressionEntity(SafeExpressionEntity&&) noexcept = default;

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    const BaseEntityData& base() const {
        return data;
    }

    BaseEntityData& base_mut() {
        return data;
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        return EntityDescriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
    }

    static Result<SafeExpressionEntity, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("Original test helper does not need record mapping");
    }

    MutationValues into_values() && {
        return MutationValues(memory_resource());
    }
};

static void safe_expression_supports_null_safe_chaining_and_defaults(Memory owner) {
    SafeExpressionEntity entity(std::allocator_arg, Allocator(owner));
    entity.data.with_id(7).with_version(3);
    entity.name = "demo";
    entity.lines.push(order(owner, 11, 1, "line"));
    entity.lines.loaded();
    auto expression =
        SafeExpression<SafeExpressionEntity, SafeExpressionEntity>::value(owner, entity);
    check(expression.entity_id().eval() == std::optional<std::uint64_t>(7) &&
              expression.entity_version().eval() == std::optional<std::int64_t>(3),
          "original entity ID/version paths");
    check(expression
                  .apply([](SafeExpressionEntity entity) {
                      return std::move(entity.name);
                  })
                  .or_else(String("x", owner)) == "demo",
          "original name path and fallback");
    auto lines = expression.apply([](SafeExpressionEntity entity) {
        return std::move(entity.lines);
    });
    check(lines.first()
                      .apply([](OrderRow row) {
                          return row.id;
                      })
                      .eval() == std::optional<std::uint64_t>(11) &&
              lines.get(4)
                  .apply([](OrderRow row) {
                      return row.id;
                  })
                  .is_null() &&
              lines.size().or_else(0) == 1,
          "original first/missing/size list paths");
}

static void safe_expression_exposes_java_style_empty_and_callbacks(Memory owner) {
    auto empty = SafeExpression<String, String>::value(owner, String(owner));
    check(empty.is_empty() && empty.or_else(String("fallback", owner)).empty(),
          "original Some(empty) keeps its empty value");
    auto missing = SafeExpression<std::monostate, String>(
        std::allocator_arg, Allocator(owner), std::monostate{}, [](const auto&) {
            return std::optional<String>{};
        });
    check(missing.is_null() && missing.or_else(String("fallback", owner)) == "fallback",
          "original None uses fallback");
    bool saw_null = false;
    missing.when_is_null([&] {
        saw_null = true;
    });
    check(saw_null, "original missing callback");
    auto value = SafeExpression<String, String>::value(owner, String("teaql", owner));
    String captured(owner);
    value.when_not_empty([&](String text) {
        captured = std::move(text);
    });
    check(captured == "teaql", "original nonempty callback");
}

template <class Action>
static void rejects(Action&& action, std::string_view reason) {
    try {
        std::invoke(std::forward<Action>(action));
    } catch (const std::invalid_argument& error) {
        check(std::string_view(error.what()).find(reason) != std::string_view::npos,
              "original rejection reason");
        return;
    }
    throw std::runtime_error("Original rejected query was accepted");
}

static void list_limit_defaults_rejects_and_allows_explicit_override(Memory owner) {
    SelectQuery plain(std::allocator_arg, Allocator(owner), "Order");
    auto prepared = plain.prepare_for_list();
    check(prepared.slice && prepared.slice->limit == std::optional<std::uint64_t>(10000),
          "original default list limit");
    SelectQuery oversized(std::allocator_arg, Allocator(owner), "Order");
    oversized.limit(10001);
    rejects(
        [&] {
            (void)oversized.prepare_for_list();
        },
        "exceeds hard limit");
    SelectQuery overridden(std::allocator_arg, Allocator(owner), "Order");
    overridden.limit(10001).hard_limit(20000);
    (void)overridden.prepare_for_list();
}

static void continuous_page_fetch_is_explicit_and_validated(Memory owner) {
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    check(!query.continuous_page_fetch, "original default has no continuous-page options");
    query.optimize_for_continuous_page_fetch_with("recent-orders", 30);
    check(query.continuous_page_fetch &&
              query.continuous_page_fetch->namespace_name == "recent-orders" &&
              query.continuous_page_fetch->ttl_seconds == 30,
          "original explicit cursor options");
}

static void continuous_page_fetch_rejects_empty_namespace(Memory owner) {
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    rejects(
        [&] {
            query.optimize_for_continuous_page_fetch_with(" ", 30);
        },
        "continuous page namespace must not be empty");
}

static void id_set_pagination_is_explicit_and_validated(Memory owner) {
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    check(!query.id_set_pagination, "original default has no ID-set options");
    query.optimize_pagination_with_id_set_config("recent-orders", 30, 5000);
    check(query.id_set_pagination && query.id_set_pagination->namespace_name == "recent-orders" &&
              query.id_set_pagination->ttl_seconds == 30 &&
              query.id_set_pagination->max_ids == 5000,
          "original explicit ID-set options");
}

static void id_set_pagination_rejects_zero_limit(Memory owner) {
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    rejects(
        [&] {
            query.optimize_pagination_with_id_set_config("orders", 30, 0);
        },
        "ID set pagination max_ids must be positive");
}

static void topn_001_002_003_probe_threshold_is_explicit_and_bounded(Memory owner) {
    SelectQuery query(std::allocator_arg, Allocator(owner), "Trip");
    check(!query.top_n_probe_parent_threshold_value,
          "original default has no top-N probe threshold");
    query.top_n_probe_parent_threshold(32);
    check(query.top_n_probe_parent_threshold_value == std::optional<std::size_t>(32),
          "original explicit top-N threshold");
    SelectQuery zero(std::allocator_arg, Allocator(owner), "Trip");
    zero.top_n_probe_parent_threshold(0);
    check(zero.top_n_probe_parent_threshold_value == std::optional<std::size_t>(0),
          "original explicit zero threshold");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        {
            GlobalContext global;
            auto user = global.make_user_context();
            auto owner = user->memory_resource();
            query_builders_cover_filters_sort_aggregates_and_relations(owner);
            smart_list_supports_entity_ids_versions_and_records(owner);
            smart_list_supports_java_style_collection_helpers(owner);
            safe_expression_supports_null_safe_chaining_and_defaults(owner);
            safe_expression_exposes_java_style_empty_and_callbacks(owner);
            list_limit_defaults_rejects_and_allows_explicit_override(owner);
            continuous_page_fetch_is_explicit_and_validated(owner);
            continuous_page_fetch_rejects_empty_namespace(owner);
            id_set_pagination_is_explicit_and_validated(owner);
            id_set_pagination_rejects_zero_limit(owner);
            topn_001_002_003_probe_threshold_is_explicit_and_bounded(owner);
        }
        std::pmr::set_default_resource(previous);
        std::cout << "Original eleven composition/query contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
