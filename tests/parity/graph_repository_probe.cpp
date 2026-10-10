#include "../support/relation_hydration_fixture.hpp"
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/runtime/graph_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <iostream>

using namespace teaql;

struct ObservedModel : EntityDefaults<ObservedModel> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Parent";
    MutationValues values;
    std::optional<FieldSet> dirty;
    std::optional<EntitySnapshot> original;
    std::optional<String> comment;
    bool fresh = false, removed = false;
    Vector<String>* calls;

    ObservedModel(std::allocator_arg_t, Allocator a, Vector<String>& calls)
        : values(a.resource()), calls(&calls) {}

    ObservedModel(ObservedModel&&) noexcept = default;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<ObservedModel, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("extraction attempted model hydration");
    }

    std::optional<FieldSet> dirty_fields() const {
        calls->emplace_back("dirty");
        return detail::clone_in(memory_resource(), dirty);
    }

    std::optional<EntitySnapshot> original_values() const {
        calls->emplace_back("original");
        return detail::clone_in(memory_resource(), original);
    }

    bool is_marked_as_delete() const {
        calls->emplace_back("deleted");
        return removed;
    }

    std::optional<String> get_comment() const {
        calls->emplace_back("comment");
        return detail::clone_in(memory_resource(), comment);
    }

    bool is_new() const {
        throw std::logic_error("repository extraction called is_new");
    }

    bool is_field_loaded(std::string_view) const {
        throw std::logic_error("repository extraction called is_field_loaded");
    }

    MutationValues into_values() && {
        calls->emplace_back("values");
        return std::move(values);
    }
};

static Json repository_case(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    auto& module = user.make<RuntimeModule>();
    for (auto name : {"Parent", "Child"}) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), name);
        descriptor.property("id", DataType::U64).id();
        descriptor.relation("kids", "Child");
        module.descriptor(std::move(descriptor));
    }
    if (!std::get<bool>(field(input, "missing").data)) {
        module.apply_to(user);
    }
    auto& executor = user.make<hydration_fixture::UnusedExecutor>();
    auto& service = EntityDataService<hydration_fixture::UnusedExecutor>::for_executor(
        user, text(field(input, "repository")), user, executor);
    auto& calls = user.make<Vector<String>>();
    // The source model belongs to another Context. Extraction must deep-copy all
    // retained values into the repository Context before that model is reclaimed.
    GlobalContext source_global;
    auto source = source_global.make_user_context();
    auto sr = source->memory_resource();
    auto& model = source->make<ObservedModel>(calls);
    model.fresh = std::get<bool>(field(input, "fresh").data);
    model.removed = std::get<bool>(field(input, "deleted").data);
    for (const auto& [name, entry] : std::get<Json::Object>(field(input, "values").data)) {
        model.values.fields.emplace(String(name, sr), value(sr, entry));
    }
    if (auto* dirty = optional(input, "dirty");
        dirty && !std::holds_alternative<std::monostate>(dirty->data)) {
        model.dirty.emplace(sr);
        for (const auto& name : std::get<Json::Array>(dirty->data)) {
            model.dirty->emplace(text(name));
        }
    }
    if (auto* old = optional(input, "old");
        old && !std::holds_alternative<std::monostate>(old->data)) {
        model.original.emplace(sr);
        for (const auto& [name, entry] : std::get<Json::Object>(old->data)) {
            model.original->fields.emplace(String(name, sr), value(sr, entry));
        }
    }
    if (auto* comment = optional(input, "comment");
        comment && !std::holds_alternative<std::monostate>(comment->data)) {
        model.comment.emplace(text(*comment), sr);
    }
    auto& result = user.make<Result<GraphNode, RuntimeError>>(
        service.graph_node_from_entity(std::move(model)));
    source.reset();
    auto out = detail::model_json(r);
    Json::Array call_output(r);
    for (const auto& call : calls) {
        call_output.emplace_back(call);
    }
    detail::model_put(out, "calls", call_output);
    if (!result) {
        detail::model_put(out, "error", result.error().to_string(r));
    } else {
        auto retained = user.make_global<GraphNode>(result.value());
        // Observe the explicitly retained graph after deleting the repository's
        // source model; the Global copy must preserve its complete diagnostics.
        detail::model_put(out, "graph", formatted_debug(r, result.value()));
        detail::model_put(
            out, "retained", formatted_debug(r, user.get(user.rebind_global(retained))));
    }
    return out;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            return 2;
        }
        std::cout << repository_case(*user, *input).dump(user->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
