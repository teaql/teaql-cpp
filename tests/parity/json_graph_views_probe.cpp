#include <teaql/runtime.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

struct ViewModel {
    using allocator_type = Allocator;
    Value id;
    String label;
    bool loaded;

    ViewModel(std::allocator_arg_t, Allocator a, const Json& input)
        : id(a.resource(),
             optional(input, "bad_id") && std::get<bool>(field(input, "bad_id").data)
                 ? Value(a.resource(), "invalid identity")
                 : Value(a.resource(), std::get<std::uint64_t>(field(input, "id").data))),
          label(text(field(input, "label")), a.resource()),
          loaded(std::get<bool>(field(input, "loaded").data)) {}

    ViewModel(std::allocator_arg_t, Allocator a, const ViewModel& other)
        : id(a.resource(), other.id), label(other.label, a.resource()), loaded(other.loaded) {}

    ViewModel(const ViewModel&) = delete;
    ViewModel(ViewModel&&) noexcept = default;

    Memory memory_resource() const {
        return label.get_allocator().resource();
    }

    Value id_value() const {
        return Value(memory_resource(), id);
    }

    bool is_field_loaded(std::string_view field) const {
        return field != "id" || loaded;
    }
};

static std::uint64_t integer(const Json& input) {
    return std::get<std::uint64_t>(input.data);
}

static Json fields(Memory r, const ViewModel& model) {
    auto out = detail::model_json(r);
    detail::model_put(out, "id", detail::debug_value(r, model.id));
    detail::model_put(out, "label", model.label);
    detail::model_put(out, "loaded", model.loaded);
    return out;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& graph = user_context.make<EntityGraphBuilder>(
        std::get<bool>(field(input, "json").data)
            ? EntityGraphBuilder::for_json_read(r)
            : EntityGraphBuilder(std::allocator_arg, Allocator(r)));
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto owner =
            optional(action, "owner") ? text(field(action, "owner")) : std::string_view("Owner");
        auto owner_id = optional(action, "owner_id") ? integer(field(action, "owner_id")) : 1;
        auto relation = optional(action, "relation") ? text(field(action, "relation"))
                                                     : std::string_view("items");
        auto view = optional(action, "view") ? integer(field(action, "view")) : 0;
        auto op = text(field(action, "op"));
        if (op == "option" || op == "plain_option") {
            std::optional<ViewModel> value;
            if (!std::holds_alternative<std::monostate>(field(action, "value").data)) {
                value.emplace(std::allocator_arg, Allocator(r), field(action, "value"));
            }
            if (op == "option") {
                graph.install_typed_relation_option(
                    view, owner, owner_id, relation, std::move(value));
            } else {
                graph.install_relation_option(owner, owner_id, relation, std::move(value));
            }
        } else if (op == "list" || op == "plain_list") {
            SmartList<ViewModel> list(std::allocator_arg, Allocator(r));
            for (const auto& value : std::get<Json::Array>(field(action, "values").data)) {
                list.data.emplace_back(value);
            }
            list.loaded();
            if (optional(action, "facets") && std::get<bool>(field(action, "facets").data)) {
                list.facets.emplace(String("choices", r),
                                    SmartList<CompactRow>(std::allocator_arg, Allocator(r)));
            }
            if (op == "list") {
                graph.install_json_relation_list(view, owner, owner_id, relation, std::move(list));
            } else {
                graph.install_relation_list(owner, owner_id, relation, std::move(list));
            }
        } else if (op == "table") {
            graph.install(integer(field(field(action, "value"), "id")),
                          ViewModel(std::allocator_arg, Allocator(r), field(action, "value")));
        } else {
            throw std::logic_error("Unknown graph action");
        }
    }
    auto counts = detail::model_json(r);
    detail::model_put(counts, "entities", std::uint64_t(graph.entity_count()));
    detail::model_put(counts, "relations", std::uint64_t(graph.relation_list_count()));
    auto& root = user_context.make<EntityRuntimeState>();
    if (!root.freeze_graph(std::move(graph))) {
        throw std::logic_error("Graph freeze failed");
    }
    Json::Array lookups(r);
    for (const auto& lookup : std::get<Json::Array>(field(input, "lookups").data)) {
        auto scoped = root.with_json_view(integer(field(lookup, "view")));
        auto owner = text(field(lookup, "owner"));
        auto owner_id = integer(field(lookup, "owner_id"));
        auto relation = text(field(lookup, "relation"));
        Json list(r);
        if (const auto* value =
                scoped.resolve_relation_list<ViewModel>(owner, owner_id, relation)) {
            list = detail::model_json(r);
            Json::Array data(r);
            for (const auto& model : value->data) {
                data.emplace_back(fields(r, model));
            }
            detail::model_put(list, "data", data);
            detail::model_put(list, "facet_count", std::uint64_t(value->facets.size()));
        }
        const auto* option = scoped.resolve_relation_option<ViewModel>(owner, owner_id, relation);
        const auto* entity = scoped.resolve_entity<ViewModel>(integer(field(lookup, "id")));
        auto out = detail::model_json(r);
        detail::model_put(out, "view", scoped.json_view_id());
        detail::model_put(out, "present", scoped.has_relation_view(owner, owner_id, relation));
        detail::model_put(out, "option_present", option != nullptr);
        detail::model_put(out, "option", option && *option ? fields(r, **option) : Json(r));
        detail::model_put(out, "list", list);
        detail::model_put(out, "entity", entity ? fields(r, *entity) : Json(r));
        detail::model_put(
            out, "facet", scoped.relation_facet(owner, owner_id, relation, "choices") != nullptr);
        detail::model_put(out, "debug", debug_string(r, scoped));
        lookups.emplace_back(std::move(out));
    }
    auto source = root.with_json_view(11);
    auto target = std::get<bool>(field(input, "shared_ledger").data)
                      ? root.with_json_view(22)
                      : EntityRuntimeState(std::allocator_arg, Allocator(r)).with_json_view(22);
    if (!std::get<bool>(field(input, "shared_ledger").data)) {
        target.set_comment("target ledger");
    }
    source.set_comment("source ledger");
    auto fresh = EntityRuntimeState::fresh_with_shared_graph(r, source);
    EntityRuntimeState copied(std::allocator_arg, Allocator(r), source);
    auto shared = target.with_shared_graph(source);
    auto loaded = target.with_loaded_view_from(source);
    Json::Array views(r);
    for (auto view : {fresh.json_view_id(),
                      copied.json_view_id(),
                      shared.json_view_id(),
                      loaded.json_view_id()}) {
        views.emplace_back(Json(r, view));
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "counts", counts);
    detail::model_put(out, "lookups", lookups);
    detail::model_put(out, "state_views", views);
    detail::model_put(
        out, "fresh_comment", fresh.get_comment() ? Json(r, *fresh.get_comment()) : Json(r));
    detail::model_put(
        out, "shared_comment", shared.get_comment() ? Json(r, *shared.get_comment()) : Json(r));
    detail::model_put(
        out, "loaded_comment", loaded.get_comment() ? Json(r, *loaded.get_comment()) : Json(r));
    detail::model_put(out, "same_ledger", source == target);
    return out;
}

static void lifetime_contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    EntityRuntimeState* retained = nullptr;
    std::optional<RelationHandle<ViewModel>> borrow;
    {
        auto first_input = Json::parse(
            r,
            R"({"id":10,"label":"long first projection that must remain isolated after Context destruction","loaded":true,"bad_id":false})");
        auto second_input = Json::parse(
            r,
            R"({"id":10,"label":"long second projection retained independently by its own JSON graph coordinate","loaded":true,"bad_id":false})");
        auto graph = EntityGraphBuilder::for_json_read(r);
        graph.install_typed_relation_option(
            1,
            "Owner",
            1,
            "items",
            std::optional<ViewModel>(
                std::in_place, std::allocator_arg, Allocator(r), *first_input));
        graph.install_typed_relation_option(
            1,
            "Owner",
            1,
            "other",
            std::optional<ViewModel>(
                std::in_place, std::allocator_arg, Allocator(r), *second_input));
        graph.install_typed_relation_option(
            2,
            "Owner",
            1,
            "items",
            std::optional<ViewModel>(
                std::in_place, std::allocator_arg, Allocator(r), *second_input));
        auto& root = user_context->make<EntityRuntimeState>();
        if (!root.freeze_graph(std::move(graph))) {
            throw std::logic_error("JSON graph freeze failed");
        }
        auto first = root.with_json_view(1);
        auto second = root.with_json_view(2);
        if (first.resolve_entity<ViewModel>(10) || !second.resolve_entity<ViewModel>(10)) {
            throw std::logic_error("Ambiguity must remain confined to its own view coordinate");
        }
        borrow.emplace(second.relation_option<ViewModel>("Owner", 1, "items"));
        retained = &global.make<EntityRuntimeState>(second);
        auto* copied = retained->resolve_entity<ViewModel>(10);
        auto* original = second.resolve_entity<ViewModel>(10);
        if (!copied || copied == original || copied->label.data() == original->label.data() ||
            copied->memory_resource() != global.memory_resource() ||
            retained->json_view_id() != 2) {
            throw std::logic_error(
                "Global graph copies must retain coordinates and independent indexed payloads");
        }
    }
    user_context->close();
    user_context.reset();
    bool rejected = false;
    try {
        (void)borrow->value();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Closed;
    }
    auto first = retained->with_json_view(1);
    auto* copied = retained->resolve_entity<ViewModel>(10);
    if (!rejected || first.resolve_entity<ViewModel>(10) || !copied || copied->label.size() < 40 ||
        first.resolve_relation_option<ViewModel>("Owner", 1, "items")->value().label ==
            copied->label) {
        throw std::logic_error(
            "Copied view/index/key/string state must survive actual User destruction");
    }
    std::cout << "JSON graph view lifetime contracts passed\n";
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        lifetime_contracts();
        std::pmr::set_default_resource(previous);
        return 0;
    }
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
