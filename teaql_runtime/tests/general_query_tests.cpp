#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "../../tests/generated_relations/include/cpp_relations.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>

using namespace teaql;
using Query = EntityQuery<cpp_dynamic_json::Order, cpp_dynamic_json::DataServiceExecutor>;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void general_descriptions() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto query = Query(*user_context);
    query.appendSearchCriteria(Expr::eq(r, "code", "A"));
    query.withDeletedRows();
    check(query.query().predicate == Expr::eq(r, "code", "A"),
          "remove only the default live conjunct and preserve ordinary criteria");
    query.deletedRowsOnly().deletedRowsOnly();
    check(query.query().predicate->args.size() == 2,
          "repeated deleted-only criteria follow source expression deduplication");
    query.filterProperty("priority", request::FieldOperator::GreaterThan, "id");
    check(query.query().predicate->args.back() ==
              Expr::compare_columns(r, "priority", BinaryOp::Gt, "id"),
          "property filters compare column expressions rather than literal property names");
    query.pageNumber(std::numeric_limits<std::uint64_t>::max(), 2);
    check(query.query().slice == Slice{2, std::numeric_limits<std::uint64_t>::max()},
          "page offsets saturate instead of wrapping");
    query.pageDefault(0);
    check(query.query().slice == Slice{10, 0},
          "zero/default page preserves source offset semantics");
    query.offset(3, 7).offsetOnly(9).top(4);
    check(query.query().slice == Slice{4, 9},
          "offset-only and top preserve independent slice fields");
    query.selectRaw("code", "'raw code'").selectAs("code", "aliased").unselect("code");
    check(std::ranges::find(query.query().projection, "code") == query.query().projection.end() &&
              query.query().raw_projections.empty() && query.query().expr_projection.size() == 1,
          "unselect removes raw/self fields while preserving independent expression aliases");
    query.selectRelation("customer")
        .selectRelation("customer")
        .unselectRelation("customer_id", "customer");
    check(query.query().relations.empty(), "relationship unselect removes all matching selections");
    query.groupBy("customer_id")
        .aggregateCount("total")
        .aggregateSum("amount", "sum_amount")
        .aggregateStddev("amount", "stdDev_amount")
        .aggregateVarPop("amount", "varPop_amount")
        .enableAggregationCacheFor(250)
        .propagateAggregationCache(500);
    check(query.query().aggregates.size() == 4 && query.query().group_by_fields.size() == 1 &&
              query.query().aggregation_cache == AggregationCacheOptions{true, 250, true, 500},
          "generic aggregation retains separate field/alias/function and cache protocols");
    auto retained = query.copyToGlobal(*user_context);
    auto next_user_context = global.make_user_context();
    user_context.reset();
    auto rebound = retained.rebindGlobal(*next_user_context);
    rebound.createPropertyAs("_value", "42");
    check(rebound.query().dynamic_properties[0].memory_resource() == global.memory_resource(),
          "new string-bearing metadata follows explicitly retained Global allocation");
}

static void aggregate_descriptions() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    std::string group_alias = "customer bucket with a long independently owned name";
    std::string total_alias = "customer count with a long independently owned name";
    auto query = Q.ordersMinimal().groupByCustomerAs(group_alias).countCustomerAs(total_alias);
    group_alias.assign("discarded");
    total_alias.assign("discarded");
    check(query.query().expr_projection[0].alias ==
                  "customer bucket with a long independently owned name" &&
              query.query().aggregates[0].alias ==
                  "customer count with a long independently owned name",
          "generated grouping and counting copy caller-owned aliases into their Context");
    check(query.query().expr_projection[0].alias.get_allocator().resource() == r &&
              query.query().aggregates[0].alias.get_allocator().resource() == r,
          "nested aggregation aliases use the bound Context resource");
    auto transform = [](auto value) {
        return std::move(value).sumPriority().avgPriority().countId().sumVersion();
    };
    auto processed = transform(query);
    check(query.query().aggregates.size() == 1 && processed.query().aggregates.size() == 5,
          "aggregate helper recomposition preserves independent value requests");
    auto executable = std::move(processed).purpose("qualify aggregate ownership");
    executable.comment("copy aliases and fields").aggregateMin("priority", "minimum");
    auto retained = executable.copyToGlobal(*user_context);
    check(retained.query().aggregates[0].alias.get_allocator().resource() ==
                  global.memory_resource() &&
              retained.query().expr_projection[0].alias.get_allocator().resource() ==
                  global.memory_resource(),
          "explicit Global copy recursively changes aggregate and grouping allocation domains");
    user_context.reset();
    bool rejected = false;
    try {
        query.count();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, "ordinary aggregate wrappers reject after the owning Context is destroyed");
    auto next_user_context = global.make_user_context();
    auto rebound = retained.rebindGlobal(*next_user_context);
    rebound.countAs("added after rebinding retained aggregate query");
    check(rebound.query().aggregates.back().alias.get_allocator().resource() ==
                  global.memory_resource() &&
              rebound.query().aggregates.size() == 7,
          "rebound Global aggregate descriptions keep explicit allocation ownership");
}

static const Expr& membership(const Expr& expression) {
    if (expression.kind == Expr::Kind::SubQuery) {
        return expression;
    }
    for (const auto& child : expression.args) {
        if (child.kind == Expr::Kind::SubQuery) {
            return child;
        }
    }
    throw std::runtime_error("Missing membership subquery");
}

static void matching_any_ownership() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    auto next_Q = next_user_context->queries<cpp_dynamic_json::query_factory>();
    auto child = Q.peopleMinimal()
                     .filter(Expr::gt(user_context->memory_resource(), "id", 10))
                     .selectAll()
                     .comment("child-only comment")
                     .rawSql("SELECT 999 AS id");
    auto query = Q.ordersMinimal().matchingAnyOf(child);
    auto& selection = user_context->make<request::QuerySelection>(child.selection());
    auto& core_query = user_context->make<SelectQuery>(std::move(child.selection()).into_query());
    check(debug_string(user_context->memory_resource(),
                       Q.ordersMinimal().matchAnyOf(selection).query()) ==
              debug_string(user_context->memory_resource(), query.query()),
          "direct QuerySelection conversion preserves request membership semantics");
    auto raw = Q.ordersMinimal().matchingAnyOf(core_query);
    check(membership(*raw.query().predicate).subquery_data->query.raw_sql_text ==
              core_query.raw_sql_text,
          "direct core query membership keeps explicitly applied options");
    auto other_Q = user_context->queries<cpp_relations::query_factory>();
    auto other_child = other_Q.schoolsMinimal().comment("independent executor domain");
    auto cross_domain = Q.ordersMinimal().matchingAnyOf(other_child);
    check(membership(*cross_domain.query().predicate).subquery_data->query.entity == "School",
          "membership accepts another generated domain without executing its executor");
    auto& foreign_selection =
        next_user_context->make<request::QuerySelection>(next_Q.peopleMinimal().selection());
    bool raw_foreign_rejected = false;
    try {
        query.matchAnyOf(foreign_selection);
    } catch (const std::invalid_argument&) {
        raw_foreign_rejected = true;
    }
    check(raw_foreign_rejected, "direct selections require an explicit destination copy");
    query.matchAnyOf(user_context->make<request::QuerySelection>(foreign_selection));
    {
        const auto& embedded = membership(*query.query().predicate);
        auto before = debug_string(user_context->memory_resource(), embedded.subquery_data->query);
        child.filter(Expr::gt(user_context->memory_resource(), "id", 20));
        check(debug_string(user_context->memory_resource(), embedded.subquery_data->query) ==
                  before,
              "membership owns an independent child query snapshot");
        check(
            !embedded.subquery_data->query.relations.empty() &&
                !embedded.subquery_data->query.raw_sql_text &&
                !embedded.subquery_data->query.comment_text &&
                embedded.subquery_data->entity.properties.empty(),
            "membership retains core relations, ignores selection options and uses a name-only descriptor");
    }
    bool foreign_rejected = false;
    try {
        query.matchAnyOf(next_Q.people());
    } catch (const std::exception&) {
        foreign_rejected = true;
    }
    check(foreign_rejected, "membership rejects a foreign Context child");
    query.matchAnyOf(next_Q.peopleMinimal()
                         .filter(Expr::gt(next_user_context->memory_resource(), "id", 30))
                         .copyTo(*user_context));
    auto retained = query.copyToGlobal(*user_context);
    const auto& retained_child = membership(*retained.query().predicate).subquery_data->query;
    check(retained_child.memory_resource() == global.memory_resource() &&
              retained_child.predicate->memory_resource() == global.memory_resource(),
          "explicit Global retention recursively copies the child query and expression");
    user_context.reset();
    bool closed_rejected = false;
    try {
        (void)query.query();
    } catch (const std::exception&) {
        closed_rejected = true;
    }
    check(closed_rejected, "ordinary membership requests reject actual Context destruction");
    auto rebound = retained.rebindGlobal(*next_user_context);
    auto global_child = next_Q.peopleMinimal()
                            .filter(Expr::gt(next_user_context->memory_resource(), "id", 40))
                            .copyToGlobal(*next_user_context);
    rebound.matchingAnyOf(global_child);
    check(membership(*rebound.query().predicate).subquery_data->query.memory_resource() ==
              global.memory_resource(),
          "rebound Global requests retain child allocation ownership");
}

static void field_criteria_ownership() {
    using O = request::FieldOperator;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    auto next_Q = next_user_context->queries<cpp_dynamic_json::query_factory>();
    std::array<std::string, 1> values{
        "a long transient string with a literal percent % and underscore _"};
    auto criterion = Q.peopleMinimal().createNameCriteria(O::Contain, values);
    auto query = Q.peopleMinimal().appendSearchCriteria(criterion);
    values[0].assign("discarded");
    check(user_context->get(criterion).memory_resource() == user_context->memory_resource() &&
              debug_string(user_context->memory_resource(), user_context->get(criterion))
                      .find("transient") != String::npos,
          "independent criteria own nested transient values in the creating Context");
    auto processed = query.appendSearchCriteria(criterion);
    check(processed.query().predicate == query.query().predicate,
          "repeated managed criteria preserve source expression deduplication");
    bool foreign_rejected = false;
    try {
        next_Q.peopleMinimal().appendSearchCriteria(criterion);
    } catch (const ReferenceError&) {
        foreign_rejected = true;
    }
    check(foreign_rejected, "managed criteria reject foreign UserContext references");
    auto copied = next_user_context->make_ref<Expr>(user_context->get(criterion));
    next_Q.peopleMinimal().appendSearchCriteria(copied);
    auto numbers = std::views::iota(std::int64_t(1), std::int64_t(4));
    auto numeric = Q.ordersMinimal().withPriority(O::In, numbers);
    check(std::get<Value::List>(numeric.query().predicate->args.back().args.back().literal.data)
                  .size() == 3,
          "generic field helpers accept primitive range views");
    auto global_query = query.copyToGlobal(*user_context);
    auto global_criterion =
        global_query.createNameCriteria(O::Equal, std::array{"retained global text"});
    global_query.appendSearchCriteria(global_criterion);
    check(user_context->get(global_criterion).memory_resource() == global.memory_resource(),
          "criteria factories on explicit Global requests retain Global allocation");
    user_context.reset();
    bool closed_rejected = false;
    try {
        next_Q.peopleMinimal().appendSearchCriteria(criterion);
    } catch (const ReferenceError&) {
        closed_rejected = true;
    }
    check(closed_rejected, "criterion IDs remain safely rejected after actual Context deletion");
    auto rebound = global_query.rebindGlobal(*next_user_context);
    auto rebound_criterion = next_user_context->rebind_global(global_criterion);
    rebound.appendSearchCriteria(rebound_criterion);
    check(next_user_context->get(rebound_criterion).memory_resource() == global.memory_resource(),
          "explicit Global criteria survive source Context deletion through checked references");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        general_descriptions();
        aggregate_descriptions();
        matching_any_ownership();
        field_criteria_ownership();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "General query Context contracts passed\n";
}
