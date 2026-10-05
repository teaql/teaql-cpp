#include <teaql/runtime/context.hpp>
#include <teaql/runtime/detail/query_keys.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
using namespace teaql;

static void check(bool b, const char* message) {
    if (!b) {
        throw std::runtime_error(message);
    }
}

static String aggregation(Memory r, const SelectQuery& q) {
    return detail::aggregation_cache_key(r, "tenant", "entity:Device", q);
}

static void normalization() {
    GlobalContext global;
    auto ctx = global.make_user_context();
    auto r = ctx->memory_resource();
    auto& first = ctx->make<SelectQuery>("Device");
    first.and_filter(Expr::gt(r, "version", 0))
        .and_filter(Expr::eq(r, "vendor", 1u))
        .and_filter(Expr::eq(r, "payment", 1))
        .comment("first")
        .enable_aggregation_cache_for(1000)
        .stream(99)
        .optimize_for_continuous_page_fetch_with("first", 31);
    auto& second = ctx->make<SelectQuery>("Device");
    second.and_filter(Expr::eq(r, "payment", 1))
        .and_filter(Expr::gt(r, "version", 0))
        .and_filter(Expr::eq(r, "vendor", 1u))
        .comment("second")
        .enable_aggregation_cache_for(60000)
        .stream(3)
        .optimize_for_continuous_page_fetch_with("second", 99);
    auto before = detail::debug_query(r, first);
    check(aggregation(r, first) == aggregation(r, second),
          "predicate order and runtime metadata canonicalize");
    check(detail::debug_query(r, first) == before, "key creation does not mutate its source");
    second.and_filter(Expr::eq(r, "vendor", 2u));
    check(aggregation(r, first) != aggregation(r, second), "result-affecting values remain in key");
    auto& list_query = ctx->make<SelectQuery>("Device");
    Value::List values(r);
    values.emplace_back(-0.0);
    values.emplace_back(0.0);
    values.emplace_back(std::numeric_limits<double>::quiet_NaN());
    values.emplace_back(std::numeric_limits<double>::quiet_NaN());
    list_query.filter(Expr::binary(Expr::column(r, "v"), BinaryOp::In, Expr::value(r, values)));
    auto canonical = detail::canonical_aggregation_query(r, list_query);
    auto* list = canonical.predicate->args[1].literal.as_list();
    check(list && list->size() == 3, "stable sorting deduplicates equal zero but retains NaN");
    check(std::signbit(std::get<double>((*list)[0].data)),
          "first Debug-sorted zero representation is retained");
    auto singleton = Expr::all_of(r, std::span<const Expr>(&first.predicate->args[0], 1));
    auto& one = ctx->make<SelectQuery>("Device");
    one.filter(singleton);
    auto normalized = detail::canonical_aggregation_query(r, one);
    check(normalized.predicate->kind == Expr::Kind::And && normalized.predicate->args.size() == 1,
          "canonicalization does not collapse a singleton And");
}

static void pagination_boundaries() {
    GlobalContext global;
    auto ctx = global.make_user_context();
    auto r = ctx->memory_resource();
    auto& q = ctx->make<SelectQuery>("Device");
    q.page(0, 10).order_asc("id").project("id").comment("first");
    auto continuous = detail::continuous_page_query_key(r, q, "ns", "user");
    auto ids = detail::id_set_query_key(r, q, "ns", "user");
    q.offset(40).comment("changed");
    q.trace_chain.emplace_back(TraceKind::Purpose, "Device", "trace");
    check(continuous == detail::continuous_page_query_key(r, q, "ns", "user"),
          "continuous key removes offset/comment/trace");
    check(ids == detail::id_set_query_key(r, q, "ns", "user"),
          "ID set removes slice/comment/trace");
    q.limit(20);
    check(continuous != detail::continuous_page_query_key(r, q, "ns", "user"),
          "continuous key retains page size");
    check(ids == detail::id_set_query_key(r, q, "ns", "user"), "ID set removes page size");
    q.project("name").relation("children");
    check(ids == detail::id_set_query_key(r, q, "ns", "user"),
          "ID set removes presentation and relation loading");
    check(ids != detail::id_set_query_key(r, q, "ns", {}), "identity absence is distinct");
    check(detail::id_set_query_key(r, q, "ns", {}) != detail::id_set_query_key(r, q, "ns", ""),
          "None and Some empty identity hash differently");
    auto root = detail::id_set_query_key(r, q, "ns", "user", detail::PaginationKeyRoot{"Root", 1});
    check(ids != root && root != detail::id_set_query_key(
                                     r, q, "ns", "user", detail::PaginationKeyRoot{"Root", 2}),
          "ID set includes active root identity");
    q.stream(5);
    check(ids != detail::id_set_query_key(r, q, "ns", "user"),
          "ID set retains source streaming configuration");
}

static void explicit_lifetime() {
    GlobalContext global;
    auto ctx = global.make_user_context();
    auto r = ctx->memory_resource();
    auto g = global.memory_resource();
    auto& q = ctx->make<SelectQuery>("Device with a long allocated name");
    q.filter(Expr::eq(r, "long allocated field", "nested allocation payload"));
    q.comment("private comment");
    auto& child = ctx->make<SelectQuery>("Child with a long allocated name");
    child.filter(Expr::eq(r, "child field", 123)).comment("child comment").stream(10);
    q.relation_query("children with a long name", child)
        .object_group_by("object", "object_id", child)
        .child_enhancement(child);
    auto& retained = global.make<SelectQuery>(detail::canonical_aggregation_query(g, q));
    auto& key = global.make<String>(aggregation(g, q));
    auto& debug = global.make<String>(detail::debug_query(g, q));
    auto& ids =
        global.make<String>(detail::id_set_query_key(g,
                                                     q,
                                                     "long namespace",
                                                     std::string_view("long user identity"),
                                                     detail::PaginationKeyRoot{"Root", 42}));
    auto expected = aggregation(g, retained);
    check(key == expected, "Global canonical copy and key agree");
    ctx->close();
    check(ctx->allocation_stats().live_blocks == 0, "all User blocks reclaimed");
    check(aggregation(g, retained) == key && debug.find("private comment") != String::npos &&
              ids.starts_with("teaql:id-set:v1:"),
          "Global keys and nested query survive source close");
    check(retained.memory_resource() == g && retained.relations[0].query->memory_resource() == g &&
              retained.object_group_bys[0].query.memory_resource() == g &&
              retained.child_enhancements[0].memory_resource() == g,
          "all canonical nested queries use destination");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        normalization();
        pagination_boundaries();
        explicit_lifetime();
        std::pmr::set_default_resource(previous);
        std::cout << "Query key contracts passed\n";
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
}
