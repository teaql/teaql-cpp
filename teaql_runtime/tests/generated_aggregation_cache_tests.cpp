#include "../../tests/support/dynamic_json_runtime_fixture.hpp"
#include <teaql/runtime/cache.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void global_cache_copies() {
    GlobalContext global;
    auto& cache = global.make<InMemoryAggregationCache>("lifetime");
    auto first_user_context = global.make_user_context();
    auto r = first_user_context->memory_resource();
    {
        Record nested(r);
        nested.emplace(String("text", r),
                       Value(r, "long caller text retained in the global cache"));
        Vector<CompactRow> rows(r);
        rows.emplace_back();
        rows.back().insert("_nested", Value(r, nested));
        cache.put("lifetime::entity:Order::query", rows);
        nested.clear();
        rows.clear();
    }
    first_user_context.reset();
    auto next_user_context = global.make_user_context();
    auto nr = next_user_context->memory_resource();
    {
        auto rows = cache.get(nr, "lifetime::entity:Order::query", 0);
        check(rows && rows->front().memory_resource() == nr &&
                  rows->front().get("_nested")->as_object()->at("text").try_text() ==
                      std::optional<std::string_view>(
                          "long caller text retained in the global cache"),
              "Global cache owns deep copies after the source UserContext is actually deleted");
        rows->front().insert("_changed", Value(nr, true));
        auto again = cache.get(nr, "lifetime::entity:Order::query", 0);
        check(!again->front().get("_changed"),
              "cache reads produce independent destination-scope values");
        cache.invalidate_namespace("lifetime::entity:Order");
        check(!cache.get(nr, "lifetime::entity:Order::query", 0) && rows->front().get("_nested"),
              "namespace invalidation cannot invalidate already copied Context rows");
    }
}

static void cached_declarations_execute_once() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    teaql::test::initialize_dynamic_json_database(*user_context);
    auto& cache = user_context->make<InMemoryAggregationCache>("dedup");
    user_context->insert_resource(*user_context, static_cast<AggregationCacheBackend&>(cache));
    auto Q = user_context->queries<query_factory>();
    auto query = Q.peopleMinimal()
                     .countOrdersWith("_metric", Q.orders().unlimited().enableAggregationCache())
                     .enableAggregationCache()
                     .comment("inspect declarations")
                     .purpose("deduplicate cache transport");
    auto& first = query.executeForList(*user_context);
    check(first.wait() && first.wait().value().data.front().dynamic.at("_metric").try_u64() ==
                              std::optional<std::uint64_t>(2),
          "core and explicit aggregate transports execute each identical declaration once");
    auto logs = user_context->sql_logs().size();
    const auto& second = query.executeForList(*user_context).wait();
    check(second &&
              second.value().data.front().dynamic.at("_metric").try_u64() ==
                  std::optional<std::uint64_t>(2) &&
              user_context->sql_logs().size() == logs,
          "cached base rows and child aggregates hit without synthetic SQL-success logs");
    check(query.query().relation_aggregates.size() == 1,
          "execution normalization preserves the caller's reusable query");
    (void)r;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        global_cache_copies();
        cached_declarations_execute_once();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated aggregation cache Context contracts passed\n";
}
