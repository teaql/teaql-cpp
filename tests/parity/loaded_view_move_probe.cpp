#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

using namespace teaql;

static void
load(UserContext& user_context, EntityRuntimeState& state, unsigned mode, std::int64_t marker) {
    auto r = user_context.memory_resource();
    if ((mode & 1) != 0) {
        Record fields(r);
        fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
        fields.emplace(String("version", r), Value(r, marker));
        state.set_original_compact_row("Item",
                                       CompactRow(std::allocator_arg, Allocator(r), fields));
    }
    if ((mode & 2) != 0) {
        const std::array fields{DynamicFieldDefinitions::Field{"count", DataType::I64}};
        auto definitions = DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields);
        auto& owned = user_context.make<DynamicFieldDefinitions>(std::move(definitions).value());
        Value::Object values(r);
        values.emplace(String("count", r), Value(r, marker));
        auto loaded = DynamicFieldValues::from_values(r, owned, values);
        state.install_loaded_dynamic_fields(loaded.value());
    }
}

static Json describe(Memory r, const EntityRuntimeState& state) {
    auto output = detail::model_json(r);
    auto debug = debug_string(r, state);
    detail::model_put(output, "binding", debug.find("shared: OnceLock(<uninit>)") == String::npos);
    detail::model_put(output, "ledger", detail::EntityRuntimeAccess::has_mutation_context(state));
    detail::model_put(output, "view", state.json_view_id());
    auto comment = state.get_comment();
    detail::model_put(output, "comment", comment ? Json(r, *comment) : Json(r));
    auto snapshot = state.original_snapshot();
    detail::model_put(
        output, "row", snapshot ? record_to_json_value(r, snapshot->fields) : Json(r));
    const auto* fields = state.loaded_dynamic_fields();
    detail::model_put(
        output, "dynamic", fields ? record_to_json_value(r, fields->values()) : Json(r));
    return output;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto flag = [&](std::string_view name) {
        return std::get<bool>(field(input, name).data);
    };
    EntityRuntimeState root(std::allocator_arg, Allocator(r));
    if (flag("frozen")) {
        EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
        graph.install(10, std::int64_t{100});
        if (!root.freeze_graph(std::move(graph))) {
            throw std::runtime_error("Initial graph publication failed");
        }
    }
    auto receiver = flag("weak") ? detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root)
                                 : EntityRuntimeState::fresh_with_shared_graph(r, root);
    receiver = std::move(receiver).with_json_view(3);
    EntityRuntimeState source(std::allocator_arg, Allocator(r));
    source = std::move(source).with_json_view(17);
    load(user_context,
         receiver,
         unsigned(std::get<std::uint64_t>(field(input, "receiver_load").data)),
         11);
    load(user_context,
         source,
         unsigned(std::get<std::uint64_t>(field(input, "source_load").data)),
         22);
    if (flag("receiver_active")) {
        receiver.set_comment("receiver");
    }
    if (flag("source_active")) {
        source.set_comment("source");
    }
    auto before = describe(r, receiver);
    std::optional<EntityRuntimeState> retained(std::move(receiver));
    auto result = [&]() {
        if (flag("copy")) {
            return retained->with_loaded_view_from(source);
        }
        auto moved = std::move(*retained).with_loaded_view_from(source);
        retained.reset();
        return moved;
    }();
    auto output = detail::model_json(r);
    detail::model_put(output, "before", before);
    detail::model_put(output, "after", describe(r, result));
    detail::model_put(output, "source_after", describe(r, source));
    load(user_context, source, 3, 99);
    detail::model_put(output, "detached", describe(r, result));
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    graph.install(29, std::int64_t{808});
    auto publication = result.freeze_graph(std::move(graph));
    detail::model_put(output, "published", bool(publication));
    Json rejected(r);
    if (!publication) {
        EntityRuntimeState destination(std::allocator_arg, Allocator(r));
        if (!destination.freeze_graph(std::move(publication).error())) {
            throw std::runtime_error("Rejected builder could not be published");
        }
        rejected = Json(r, *destination.resolve_entity<std::int64_t>(29));
    }
    detail::model_put(output, "rejected_value", rejected);
    EntityGraphBuilder root_builder(std::allocator_arg, Allocator(r));
    root_builder.install(31, std::int64_t{909});
    detail::model_put(output, "root_published", bool(root.freeze_graph(std::move(root_builder))));
    auto resolved = [&](std::uint64_t id) {
        const auto* value = root.resolve_entity<std::int64_t>(id);
        return value ? Json(r, *value) : Json(r);
    };
    detail::model_put(output, "root_29", resolved(29));
    detail::model_put(output, "root_31", resolved(31));
    result.set_comment("result");
    auto retained_comment = retained ? retained->get_comment() : std::nullopt;
    detail::model_put(
        output, "retained_comment", retained_comment ? Json(r, *retained_comment) : Json(r));
    auto source_comment = source.get_comment();
    detail::model_put(output,
                      "source_comment_after_result_write",
                      source_comment ? Json(r, *source_comment) : Json(r));
    return output;
}

static void contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    {
        auto r = user_context->memory_resource();
        auto& root = user_context->make<EntityRuntimeState>();
        auto& source = user_context->make<EntityRuntimeState>();
        auto receiver = detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root);
        const auto before = user_context->allocation_stats().issued_objects;
        auto result = std::move(receiver).with_loaded_view_from(source);
        if (user_context->allocation_stats().issued_objects != before ||
            detail::EntityRuntimeAccess::has_mutation_context(result)) {
            throw std::runtime_error("An empty consuming transfer allocated mutation storage");
        }
        load(*user_context, source, 3, 22);
        source.set_comment("self");
        auto self = std::move(source).with_loaded_view_from(source);
        if (!self.loaded_dynamic_fields() || !self.original_snapshot() ||
            self.get_comment() != std::optional<String>(String("self", r))) {
            throw std::runtime_error("A self-sourced transfer lost its loaded view or ledger");
        }
    }
    constexpr std::string_view long_text =
        "An explicitly retained loaded view owns this long description and graph string.";
    auto expired = [&]() {
        auto r = user_context->memory_resource();
        auto& root = user_context->make<EntityRuntimeState>();
        EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
        graph.install(5, String(long_text, r));
        if (!root.freeze_graph(std::move(graph))) {
            throw std::runtime_error("Lifetime fixture graph publication failed");
        }
        auto receiver = detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root);
        auto& source = user_context->make<EntityRuntimeState>();
        load(*user_context, source, 3, 22);
        Record row(r);
        row.emplace(String("id", r), Value(r, std::uint64_t{7}));
        row.emplace(String("description", r), Value(r, long_text));
        source.set_original_compact_row("Item", CompactRow(std::allocator_arg, Allocator(r), row));
        auto result = std::move(receiver).with_loaded_view_from(source);
        return user_context->make_ref<EntityRuntimeState>(std::move(result));
    }();
    auto retained = user_context->make_global<EntityRuntimeState>(user_context->get(expired));
    user_context.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Consumed loaded-view storage survived UserContext deletion");
    }
    auto next_user_context = global.make_user_context();
    bool rejected = false;
    try {
        next_user_context->get(expired);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    auto& saved = next_user_context->get(next_user_context->rebind_global(retained));
    auto snapshot = saved.original_snapshot();
    auto r = next_user_context->memory_resource();
    auto* graph_text = saved.resolve_entity<String>(5);
    const auto* dynamic = saved.loaded_dynamic_fields();
    if (!rejected || !snapshot ||
        snapshot->fields.at(String("description", r)).try_text() != long_text || !graph_text ||
        *graph_text != long_text || !dynamic ||
        dynamic->values().at(String("count", r)).try_i64() != 22 ||
        saved.memory_resource() != global.memory_resource()) {
        throw std::runtime_error("Explicit Global retention lost nested loaded-view data");
    }
    std::cout << "loaded-view: empty move, self transfer, explicit retention and User deletion\n";
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        contracts();
        return 0;
    }
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            return 2;
        }
        std::cout << evaluate(*user_context, input.value()).dump(user_context->memory_resource())
                  << '\n';
    }
}
