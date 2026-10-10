// Original aggregation-key and relation-plan contracts using production helpers.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/detail/query_keys.hpp>
#include <teaql/runtime/detail/data_service/relation_execution.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static String aggregation_key(Memory owner, const SelectQuery& query) {
    return detail::aggregation_cache_key(owner, "tenant", "entity:Trip", query);
}

static void key_canonicalizes_predicate_order_and_runtime_metadata(Memory owner) {
    SelectQuery first(std::allocator_arg, Allocator(owner), "Trip");
    first.and_filter(Expr::gt(owner, "version", std::int64_t(0)))
        .and_filter(Expr::eq(owner, "vendor_id", std::uint64_t(1)))
        .and_filter(Expr::eq(owner, "payment_type", std::int64_t(1)))
        .comment("first explanation")
        .enable_aggregation_cache_for(1000);
    SelectQuery second(std::allocator_arg, Allocator(owner), "Trip");
    second.and_filter(Expr::eq(owner, "payment_type", std::int64_t(1)))
        .and_filter(Expr::gt(owner, "version", std::int64_t(0)))
        .and_filter(Expr::eq(owner, "vendor_id", std::uint64_t(1)))
        .comment("different explanation")
        .enable_aggregation_cache_for(60000);
    check(aggregation_key(owner, first) == aggregation_key(owner, second),
          "original predicate order, comment and TTL do not split an aggregation key");
}

static void key_retains_result_affecting_values(Memory owner) {
    SelectQuery first(std::allocator_arg, Allocator(owner), "Trip");
    first.filter(Expr::eq(owner, "vendor_id", std::uint64_t(1)));
    SelectQuery second(std::allocator_arg, Allocator(owner), "Trip");
    second.filter(Expr::eq(owner, "vendor_id", std::uint64_t(2)));
    check(aggregation_key(owner, first) != aggregation_key(owner, second),
          "original U64 vendor IDs remain result-affecting inputs");
}

static RelationLoadPlan limited_many_plan(Memory owner) {
    RelationDescriptor relation(std::allocator_arg, Allocator(owner), "trips", "Trip");
    relation.local_key = "id";
    relation.foreign_key = "vendor_id";
    relation.many = true;
    RelationLoadPlan plan(std::allocator_arg, Allocator(owner), "Vendor", relation);
    plan.query.emplace(std::allocator_arg, Allocator(owner), "Trip");
    plan.query->order_desc("id").limit(10);
    return plan;
}

static void topn_001_002_003_004_plan_respects_default_threshold_and_sqlite_policy(Memory owner) {
    using Plan = detail::RelationTopNExecutionPlan;
    auto plan = limited_many_plan(owner);
    DataServiceCapabilities capabilities;
    check(detail::relation_top_n_execution_plan(capabilities, plan, 6) == Plan::Window,
          "original default uses the window plan");
    capabilities.small_parent_relation_probes = true;
    check(detail::relation_top_n_execution_plan(capabilities, plan, 10000) == Plan::BoundedProbes,
          "original provider default permits probes without an implicit parent-count cap");
    auto threshold_plan = detail::clone_in(owner, plan);
    threshold_plan.query->top_n_probe_parent_threshold(4);
    capabilities.small_parent_relation_probes = false;
    check(detail::relation_top_n_execution_plan(capabilities, threshold_plan, 4) ==
              Plan::BoundedProbes,
          "original explicit threshold permits its exact boundary");
    check(detail::relation_top_n_execution_plan(capabilities, threshold_plan, 5) == Plan::Window,
          "original explicit threshold rejects the next parent count");
    threshold_plan.query->top_n_probe_parent_threshold(0);
    capabilities.small_parent_relation_probes = true;
    check(detail::relation_top_n_execution_plan(capabilities, threshold_plan, 1) == Plan::Window,
          "original explicit zero disables the probe-capable provider default");

    // Additional branch checks keep the original private helper's early exits.
    plan.many = false;
    check(detail::relation_top_n_execution_plan(capabilities, plan, 1) == Plan::Window,
          "to-one relation cannot enter per-parent top-N probes");
    plan.many = true;
    plan.query->slice.reset();
    check(detail::relation_top_n_execution_plan(capabilities, plan, 1) == Plan::Window,
          "an unsliced relation uses the window plan");
    plan.query.reset();
    check(detail::relation_top_n_execution_plan(capabilities, plan, 1) == Plan::Window,
          "missing child query uses the window plan");
}

static RuntimeOperation topn_010_plan_telemetry_is_safe_and_complete(Memory owner) {
    auto plan = limited_many_plan(owner);
    plan.query->top_n_probe_parent_threshold(32);
    auto operation = detail::relation_top_n_operation(
        owner, plan, detail::RelationTopNExecutionPlan::BoundedProbes, 6);
    auto attribute = [&](std::string_view name) -> const RuntimeAttributeValue::Data& {
        auto found = operation.attributes.find(name);
        check(found != operation.attributes.end(), "original telemetry attribute exists");
        return found->second.data;
    };
    check(operation.family == "relation_load", "original relation operation family");
    check(std::get<String>(attribute("teaql.relation.selected_plan")) == "BOUNDED_PROBES",
          "original selected-plan String value");
    check(std::get<std::int64_t>(attribute("teaql.relation.parent_count")) == 6 &&
              std::get<std::int64_t>(attribute("teaql.relation.per_parent_limit")) == 10 &&
              std::get<std::int64_t>(attribute("teaql.relation.configured_probe_threshold")) ==
                  32 &&
              std::get<std::int64_t>(attribute("teaql.relation.probe_count")) == 6,
          "all original telemetry Integer values");
    check(!operation.attributes.contains("teaql.entity.id"),
          "original telemetry excludes entity IDs");
    check(operation.name == "Vendor.trips" && operation.attributes.size() == 9 &&
              std::get<String>(attribute("teaql.operation.family")) == "relation_load" &&
              std::get<String>(attribute("teaql.operation.name")) == "Vendor.trips" &&
              std::get<String>(attribute("teaql.entity.type")) == "Vendor" &&
              std::get<String>(attribute("teaql.relation.name")) == "trips",
          "complete original helper output keeps seven relation and two operation attributes");
    auto window =
        detail::relation_top_n_operation(owner, plan, detail::RelationTopNExecutionPlan::Window, 6);
    check(std::get<String>(window.attributes.find("teaql.relation.selected_plan")->second.data) ==
                  "WINDOW" &&
              std::get<std::int64_t>(
                  window.attributes.find("teaql.relation.probe_count")->second.data) == 0,
          "window telemetry does not report fictional probes");
    return operation;
}

static void original_contracts_and_retention() {
    GlobalContext global;
    Ref<RuntimeOperation> retained;
    Ref<String> retained_key;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        key_canonicalizes_predicate_order_and_runtime_metadata(owner);
        key_retains_result_affecting_values(owner);
        topn_001_002_003_004_plan_respects_default_threshold_and_sqlite_policy(owner);
        auto operation = topn_010_plan_telemetry_is_safe_and_complete(owner);
        retained = user->make_global<RuntimeOperation>(operation);
        SelectQuery query(std::allocator_arg, Allocator(owner), "Trip");
        query.filter(Expr::eq(owner, "vendor_id", std::uint64_t(1)));
        auto key = aggregation_key(owner, query);
        retained_key = user->make_global<String>(key);
    }
    check(global.live_user_bytes() == 0, "actual source User deletion returns all owned blocks");
    auto next = global.make_user_context();
    const auto& operation = next->get(next->rebind_global(retained));
    const auto& key = next->get(next->rebind_global(retained_key));
    auto* global_owner = operation.memory_resource();
    check(
        operation.name == "Vendor.trips" &&
            operation.attributes.get_allocator().resource() == global_owner &&
            std::get<String>(operation.attributes.find("teaql.relation.selected_plan")->second.data)
                    .get_allocator()
                    .resource() == global_owner,
        "retained telemetry copies propagate Global ownership into nested strings and map");
    SelectQuery query(std::allocator_arg, Allocator(next->memory_resource()), "Trip");
    query.filter(Expr::eq(next->memory_resource(), "vendor_id", std::uint64_t(1)));
    check(key == aggregation_key(next->memory_resource(), query),
          "explicit Global key copy retains its full value after source User destruction");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        original_contracts_and_retention();
        std::pmr::set_default_resource(previous);
        std::cout << "Original four cache-key/relation-plan contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
