#define main teaql_typed_aggregate_probe_main
#include "../../tests/parity/typed_aggregate_scope_probe.cpp"
#undef main

static void check_aggregate(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void retained_typed_aggregate() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto resource = user->memory_resource();
    auto& input = user->make<Json>(*Json::parse(
        resource, R"({"projection":["label"],"rows":2,"failure":false,"fail_model":null})"));
    auto& service = aggregate_service(*user, input);
    auto& query = user->make<SelectQuery>("Parent");
    query.comment("Inspect typed aggregate retention")
        .purpose("Verify aggregate ownership and decoding");
    query.project("label").page(0, 1).optimize_pagination_with_id_set_config("fixture", 600, 2);
    detail::ContextPaginationAccess::observe_id_set(*user, "RETAINED-OBSERVATION", 37);
    auto& work =
        detail::EntityDataServiceAccess::fetch_entities_with_relation_aggregates<AggregateExecutor,
                                                                                 Model>(
            service, query, {});
    const auto& result = work.wait();
    check_aggregate(bool(result) && result.value().data.size() == 2,
                    "typed aggregate decodes the actual row result without ID-page preparation");
    check_aggregate(!result.value().total_count && user->id_set_plan() == "RETAINED-OBSERVATION" &&
                        user->id_set_count() == 37,
                    "typed aggregate does not replace independent pagination observations");
    auto& executor = detail::EntityDataServiceAccess::context_service(service).executor.get();
    check_aggregate(executor.projection.size() == 3 && executor.projection[0] == "label" &&
                        executor.projection[1] == "id" && executor.projection[2] == "version",
                    "source reduced identity/version projection is restored before row execution");
    check_aggregate(result.value().data[0].state && result.value().data[1].state,
                    "every typed aggregate model receives its decoding root");
    auto stale = ResourceBorrow<const EntityRuntimeState>(std::allocator_arg,
                                                          Allocator(resource),
                                                          *result.value().data[0].state,
                                                          detail::resource_arena(resource));
    auto retained_handle = user->make_global<SmartList<Model>>(result.value());
    user.reset();
    check_aggregate(global.live_user_bytes() == 0,
                    "actual User destruction reclaims every typed-aggregate block");
    bool rejected = false;
    try {
        (void)stale.get();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Closed;
    }
    check_aggregate(rejected, "checked typed-aggregate state expires after actual User deletion");
    auto reader = global.make_user_context();
    auto& retained = reader->get(reader->rebind_global(retained_handle));
    auto target = global.memory_resource();
    check_aggregate(retained.data.size() == 2 && retained.memory_resource() == target &&
                        retained.data[0].row.memory_resource() == target &&
                        retained.data[0].state->memory_resource() == target &&
                        retained.data[0].row.get("id")->try_u64() == 1 &&
                        std::get<String>(retained.data[0].row.get("label")->data) ==
                            "retained-row-0",
                    "explicit Global retention owns both nested row storage and runtime state");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        retained_typed_aggregate();
        std::pmr::set_default_resource(previous);
        std::cout << "Typed aggregate lifecycle contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
