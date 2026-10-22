#include "../../tests/support/runtime_mutation_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <teaql/runtime/graph_debug.hpp>
#include <iostream>
using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static void require(bool condition, const char* message) {
    if (!condition) {
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

static void reference_retention_and_cancel() {
    GlobalContext global;
    unsigned cancelled = 0;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.external_cancel = &cancelled;
    state.rows.emplace_back();
    state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
    state.rows.back().emplace(String("version", r), Value(r, std::int64_t(1)));
    state.rows.back().emplace(String("name", r),
                              Value(r, "authoritative allocated reference payload"));
    auto& executor = user->make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        *user, "Other", *user, executor);
    require(bool(Access::bind_mutation_intent(service, "Inspect native mutation lifecycle")),
            "private fixture binds explicit mutation and readback intent");
    auto& node = user->make<GraphNode>("Order");
    node.value("id", std::uint64_t(7))
        .value("version", std::int64_t(1))
        .with_comment("discarded caller comment");
    node.dirty_fields.emplace(r);
    node.dirty_fields->emplace("name");
    node.original_values.emplace(r);
    node.original_values->fields.emplace(String("name", r), Value(r, "discarded snapshot payload"));
    auto& result = Access::validate_reference_node(service, detail::clone_in(r, node), {}).wait();
    require(result && result.value().operation == GraphOperation::Reference &&
                !result.value().comment && !result.value().dirty_fields &&
                !result.value().original_values,
            "reference validation returns a fresh database graph without caller annotations");
    auto retained = user->make_global<GraphNode>(result.value());
    state.read_pending = true;
    auto& work = Access::validate_remove_node(service, node, {});
    require(!work.poll() && state.child_wake, "remove validation retains pending row query");
    auto ticket = work.wake_signal().ticket();
    state.child_wake->notify();
    require(work.wake_signal().ticket() != ticket, "row query wakes propagate through validation");
    user.reset();
    require(
        cancelled == 1 && global.live_user_bytes() == 0,
        "validation cancellation quiesces provider exactly once before User blocks are reclaimed");
    auto reader = global.make_user_context();
    auto& graph = reader->get(reader->rebind_global(retained));
    require(graph.memory_resource() == global.memory_resource() &&
                graph.values.fields.find("name")->second.try_text() ==
                    "authoritative allocated reference payload",
            "explicit Global copy retains authoritative reference data after actual User deletion");
}

static void reference_dynamic_retention() {
    GlobalContext global;
    for (int mode = 0; mode != 3; ++mode) {
        for (int payload = 0; payload != 4; ++payload) {
            auto user_context = global.make_user_context();
            install(*user_context);
            auto r = user_context->memory_resource();
            auto& state = user_context->make<mutation_fixture::State>();
            state.allow_reads = true;
            state.rows.emplace_back();
            state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
            state.rows.back().emplace(String("version", r), Value(r, std::int64_t(1)));
            state.rows.back().emplace(String("name", r), Value(r, "authoritative reference"));
            auto& executor = user_context->make_resource<mutation_fixture::Executor>(state);
            auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
                *user_context, "Order", *user_context, executor);
            require(
                bool(Access::bind_mutation_intent(service, "verify dynamic reference retention")),
                "reference fixture binds mutation intent");
            auto& node = user_context->make<GraphNode>("Order");
            node.operation = GraphOperation::Reference;
            node.value("id", std::uint64_t(7));
            if (payload != 0) {
                const DynamicFieldDefinitions::Field fields[] = {{"note", DataType::Text}};
                auto parsed = DynamicFieldDefinitions::from_fields(r, "Order", "1", fields);
                require(bool(parsed), "reference dynamic definitions are valid");
                auto& definitions =
                    user_context->make<DynamicFieldDefinitions>(std::move(parsed).value());
                DynamicFieldValues::Values values(r);
                if (payload == 2) {
                    values.emplace(String("note", r), Value(r));
                } else if (payload == 3) {
                    values.emplace(String("note", r), Value(r, "retained dynamic reference"));
                }
                auto dynamic = DynamicFieldValues::from_values(r, definitions, values);
                require(bool(dynamic), "reference dynamic values are valid");
                node.dynamic_fields.reset(
                    &user_context->make<DynamicFieldValues>(std::move(dynamic).value()));
            }
            auto retained = [&] {
                auto copy = detail::clone_in(r, node);
                auto& operation =
                    mode == 0 ? Access::validate_reference_node(service, std::move(copy), {})
                    : mode == 1
                        ? Access::insert_graph_node_scoped(service, std::move(copy), nullptr)
                        : Access::upsert_graph_node_scoped(service, std::move(copy), nullptr);
                auto& result = operation.wait();
                require(result && bool(result.value().dynamic_fields) == (payload != 0),
                        "all reference entries preserve absent versus present dynamic state");
                if (payload != 0) {
                    require(*result.value().dynamic_fields == *node.dynamic_fields,
                            "reference validation preserves definitions, selection and values");
                }
                return user_context->make_global<GraphNode>(result.value());
            }();
            user_context.reset();
            require(global.live_user_bytes() == 0, "reference Context blocks are reclaimed");
            auto reader = global.make_user_context();
            auto& graph = reader->get(reader->rebind_global(retained));
            require(graph.memory_resource() == global.memory_resource() &&
                        bool(graph.dynamic_fields) == (payload != 0),
                    "explicit Global graph retains dynamic presence after Context destruction");
            if (payload != 0) {
                const auto& dynamic = *graph.dynamic_fields;
                require(dynamic.memory_resource() == global.memory_resource() &&
                            dynamic.definitions().owner_type() == "Order" &&
                            dynamic.selected_codes().size() == (payload == 1 ? 0 : 1),
                        "Global copy retains dynamic definitions and loaded selection");
                if (payload == 2) {
                    require(dynamic.values().find("note")->second.is_null(),
                            "loaded NULL survives close");
                } else if (payload == 3) {
                    require(dynamic.values().find("note")->second.try_text() ==
                                "retained dynamic reference",
                            "loaded text survives close");
                }
            }
        }
    }
}

static void deletion_parent_scope_copy() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& state = user->make<mutation_fixture::State>();
    auto& executor = user->make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        *user, "Order", *user, executor);
    require(bool(Access::bind_mutation_intent(service, "Inspect native mutation lifecycle")),
            "private fixture binds explicit mutation and readback intent");
    auto& node = user->make<GraphNode>("Order");
    node.value("id", std::int64_t(-7))
        .value("version", std::int64_t(1))
        .with_comment("leaf allocation payload");
    auto foreign = global.make_user_context();
    auto& trace = foreign->make<TraceNode>(
        TraceKind::AuditReason, "Ancestor", "parent allocation payload", std::uint64_t(3));
    auto& scope = foreign->make<ScopedCommentNode>(nullptr, trace);
    auto& work = Access::delete_graph_node(service, node, &scope);
    foreign.reset();
    require(state.requests.empty(),
            "deletion remains lazy while its parent scope is explicitly copied");
    auto& result = work.wait();
    require(result && state.requests.size() == 1, "scoped deletion delegates one native mutation");
    const auto& command = std::get<DeleteCommand>(state.requests[0].data);
    require(
        command.trace_chain.size() == 3 &&
            command.trace_chain[0].comment == "parent allocation payload" &&
            command.trace_chain[1].comment == "leaf allocation payload" &&
            command.trace_chain[1].entity_id == std::uint64_t(-7) &&
            !command.trace_chain[2].entity_id,
        "legacy audit provenance casts negative identity while the operation frame remains unsigned-only");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        reference_retention_and_cancel();
        reference_dynamic_retention();
        deletion_parent_scope_copy();
        std::pmr::set_default_resource(previous);
        std::cout << "Graph node command lifecycle contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
