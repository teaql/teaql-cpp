#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <iostream>
using namespace teaql;
using namespace cpp_dynamic_json;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void ownership() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto Q = user_context->queries<query_factory>();
    std::string alias = "relationship metric alias copied from a temporary caller string";
    auto child = Q.orders().comment("retained child comment").createPropertyAs("_custom", "42");
    auto parent = Q.peopleMinimal().countOrdersWith(alias, child);
    alias.assign("discarded");
    child.withDeletedRows().aggregateSum("amount", "sum");
    const auto& nested = parent.query().relation_aggregates.front();
    check(nested.alias == "relationship metric alias copied from a temporary caller string" &&
              nested.query.aggregates.empty() && nested.query.predicate &&
              nested.query.dynamic_properties.size() == 1,
          "relation requests take independent allocator-aware child snapshots");
    check(nested.query.memory_resource() == r && nested.alias.get_allocator().resource() == r,
          "relation descriptions and nested strings belong to the parent allocation scope");
    auto scalar = Q.people().sumAmountOfOrdersAs("sum", child);
    const auto& scalar_child = scalar.query().relation_aggregates.front().query;
    check(scalar_child.dynamic_properties.empty() && scalar_child.comment_text &&
              *scalar_child.comment_text == "retained child comment" &&
              scalar_child.aggregates.size() == 2,
          "field aggregation retains core/comment state and source option-conversion semantics");
    auto customer = Q.people().comment("object grouping child").createPropertyAs("_custom", "43");
    auto grouped = Q.orders().rollUpToCustomerWith(customer);
    check(grouped.query().object_group_bys.size() == 1 &&
              grouped.query().group_by_fields.size() == 1,
          "roll-up retains the independent object grouping protocol");
    const auto& predicate = grouped.query().predicate->args.back();
    check(predicate.subquery_data->query.dynamic_properties.empty() &&
              !predicate.subquery_data->query.comment_text &&
              grouped.query().object_group_bys.front().query.dynamic_properties.size() == 1,
          "membership uses bare child query while object grouping retains request options");
    auto retained = parent.copyToGlobal(*user_context);
    auto retained_group = grouped.copyToGlobal(*user_context);
    check(retained.query().relation_aggregates.front().query.memory_resource() ==
                  global.memory_resource() &&
              retained_group.query().object_group_bys.front().query.memory_resource() ==
                  global.memory_resource(),
          "explicit Global copies recursively move both relationship query protocols");
    auto other_user_context = global.make_user_context();
    auto other_Q = other_user_context->queries<query_factory>();
    bool rejected = false;
    try {
        parent.countOrdersWith("wrong Context", other_Q.orders());
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "relationship builders reject a foreign Context binding");
    user_context.reset();
    rejected = false;
    try {
        parent.countOrders();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "default child factories reject after the owning Context is deleted");
    auto rebound = retained.rebindGlobal(*other_user_context);
    rebound.countOrders();
    check(rebound.query().relation_aggregates.back().query.memory_resource() ==
              global.memory_resource(),
          "default related queries preserve a retained Global allocation domain");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ownership();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated relationship query Context contracts passed\n";
}
