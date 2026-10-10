#include "../../tests/support/graph_save_adapter_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <teaql/runtime/graph_debug.hpp>
#include <iostream>
using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static void require(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void install(UserContext& user) {
    auto r = user.memory_resource();
    auto& module = user.make<RuntimeModule>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Order");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text);
    module.descriptor(std::move(descriptor)).apply_to(user);
    user.disable_sql_log();
}

static adapter_fixture::Model& model(UserContext& user, mutation_fixture::State& state) {
    auto r = user.memory_resource();
    auto& entity = user.make<adapter_fixture::Model>(state);
    entity.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    entity.values.fields.emplace(String("version", r), Value(r, std::int64_t(1)));
    entity.values.fields.emplace(String("name", r), Value(r, "original allocated input payload"));
    return entity;
}

static void lazy_and_status_boundaries() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& state = user->make<mutation_fixture::State>();
    auto& executor = user->make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        *user, "Other", *user, executor);
    auto& no_op = Access::save_entity_internal(
        service, std::move(model(*user, state)), EntityStatus::Persisted);
    require(state.notes.empty(), "constructing a save future must not extract or query");
    const auto& result = no_op.wait();
    require(result && result.value().entity == "Other" && result.value().values.fields.empty() &&
                state.notes.empty(),
            "status shortcut precedes name/metadata/getter checks");
    auto& commented = Access::save_entity_with_comment_internal(
        service, std::move(model(*user, state)), EntityStatus::Persisted, "reason");
    const auto& rejected = commented.wait();
    require(!rejected && state.notes.empty(),
            "commented status route retains name rejection instead of skipping");
    auto& cancelled = Access::save_entity_graph_internal(service, std::move(model(*user, state)));
    cancelled.cancel();
    require(state.notes.empty(), "cancellation before first poll performs no model getters");
}

static void pending_close() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    unsigned cancellations = 0;
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.read_pending = true;
    state.external_cancel = &cancellations;
    auto& executor = user->make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        *user, "Order", *user, executor);
    auto& work = Access::save_entity_graph_internal(service, std::move(model(*user, state)));
    require(!work.poll() && state.child_wake, "adapter preserves pending provider execution");
    auto ticket = work.wake_signal().ticket();
    state.child_wake->notify();
    require(work.wake_signal().ticket() != ticket,
            "pending provider wake reaches the save adapter");
    user.reset();
    require(cancellations == 1 && global.live_user_bytes() == 0,
            "User destruction cancels provider once before reclaiming all source blocks");
}

static void result_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.rows.emplace_back();
    state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
    state.rows.back().emplace(String("version", r), Value(r, std::int64_t(4)));
    state.rows.back().emplace(String("name", r),
                              Value(r, "persisted allocated payload from provider"));
    auto& executor = user->make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        *user, "Order", *user, executor);
    auto& entity = model(*user, state);
    entity.dirty.emplace(r);
    entity.dirty->emplace("name");
    entity.original.emplace(r);
    entity.original->fields.emplace(String("name", r), Value(r, "original snapshot payload"));
    auto& result = Access::save_entity_graph_with_comment_internal(
                       service, std::move(entity), "retained audit reason")
                       .wait();
    require(bool(result), "save adapter returns authoritative provider graph");
    auto retained = user->make_global<GraphNode>(result.value());
    auto expected = user->make_global<String>(debug_string(r, result.value()));
    user.reset();
    require(global.live_user_bytes() == 0, "completed adapter source User is actually reclaimed");
    auto reader = global.make_user_context();
    auto& graph = reader->get(reader->rebind_global(retained));
    auto& before = reader->get(reader->rebind_global(expected));
    require(
        debug_string(reader->memory_resource(), graph) == before &&
            graph.memory_resource() == global.memory_resource() && graph.dirty_fields &&
            graph.original_values &&
            graph.values.fields.find("name")->second.try_text()->starts_with("persisted"),
        "explicit Global graph copy retains full snapshot, dirty set, annotation and authoritative values");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        lazy_and_status_boundaries();
        pending_close();
        result_retention();
        std::pmr::set_default_resource(previous);
        std::cout << "Graph save adapter lifecycle contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
