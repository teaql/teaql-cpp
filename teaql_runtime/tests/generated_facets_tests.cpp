#include "../../tests/support/dynamic_json_runtime_fixture.hpp"
#include <iostream>
using namespace cpp_dynamic_json;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void ownership_and_execution() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    teaql::test::initialize_dynamic_json_database(*user_context);
    auto Q = user_context->queries<query_factory>();
    auto child =
        Q.people().countOrdersWith("_metric", Q.orders().unlimited()).comment("facet comment");
    std::string name = "facet name copied from independent caller-owned storage";
    auto parent = Q.orders().facetByCustomerAsWithOptions(name, child, false);
    name.assign("discarded");
    child.withDeletedRows();
    const auto& facet = parent.query().facets.front();
    check(facet.facet_name == "facet name copied from independent caller-owned storage" &&
              facet.query.query_options.relation_aggregates.size() == 1 &&
              facet.query.memory_resource() == r && facet.query.query_options.comment &&
              !facet.query.query.comment_text,
          "facet snapshots preserve distinct raw query/options in the parent Context");
    const auto& result =
        parent.comment("outer comment").purpose("test facets").executeForList(*user_context).wait();
    check(bool(result) && result.value().facets.size() == 1 && result.value().data.size() == 3,
          "actual facet rows attach to typed list results");
    const auto& saved = result.value().facets.begin()->second;
    check(saved.data.size() == 2 &&
              saved.data.front().get("_metric")->try_u64() == std::optional<std::uint64_t>(2),
          "facet relation counts are actual scoped database values");
    auto& retained_rows = global.make<SmartList<Order>>(result.value());
    auto retained_query = parent.copyToGlobal(*user_context);
    check(retained_query.query()
                  .facets.front()
                  .query.query_options.relation_aggregates.front()
                  .query.memory_resource() == global.memory_resource(),
          "Global facet copies recursively change nested query/options domains");
    auto next_user_context = global.make_user_context();
    auto next_Q = next_user_context->queries<query_factory>();
    bool rejected = false;
    try {
        parent.facetByCustomerAs("foreign", next_Q.people());
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "facet builders reject another Context binding");
    user_context.reset();
    rejected = false;
    try {
        parent.facetByCustomerAs("expired", child);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "ordinary facet wrappers expire with the owning Context");
    check(retained_rows.facets.begin()->second.data.front().get("_metric")->try_u64() ==
              std::optional<std::uint64_t>(2),
          "explicit Global result copies survive actual UserContext destruction");
    auto rebound = retained_query.rebindGlobal(*next_user_context);
    rebound.facetByCustomerAs("new global facet", next_Q.people().comment("next facet"));
    check(rebound.query().facets.back().query.memory_resource() == global.memory_resource(),
          "rebound facet snapshots retain their explicit Global domain");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ownership_and_execution();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated facet Context contracts passed\n";
}
