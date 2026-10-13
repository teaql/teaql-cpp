#include <teaql/runtime.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <bit>
#include <iostream>

static EntityKey key(Memory owner, const Json& input) {
    const auto& id_input = field(input, "id");
    auto identifier = [&]() -> Value {
        if (const auto* bits = optional(id_input, "bits")) {
            auto encoded = text(*bits);
            std::uint64_t raw{};
            auto parsed = std::from_chars(encoded.data(), encoded.data() + encoded.size(), raw, 16);
            if (parsed.ec != std::errc{} || parsed.ptr != encoded.data() + encoded.size()) {
                throw std::invalid_argument("Invalid explicit floating identity bits");
            }
            return Value(owner, std::bit_cast<double>(raw));
        }
        return value(owner, id_input);
    }();
    EntityKey result(
        std::allocator_arg, Allocator(owner), text(field(input, "entity")), identifier);
    if (const auto* changed = optional(input, "mutate_id")) {
        result.id = value(owner, *changed);
    }
    if (const auto* changed = optional(input, "mutate_entity")) {
        result.entity = text(*changed);
    }
    return result;
}

static ChangeSetStack changes(Memory owner, const Json& steps) {
    ChangeSetStack stack(std::allocator_arg, Allocator(owner));
    for (const auto& action : std::get<Json::Array>(steps.data)) {
        auto op = text(field(action, "op"));
        if (op == "push") {
            stack.push();
        } else if (op == "pop") {
            (void)stack.pop();
        } else if (op == "clear") {
            stack.clear_current();
        } else if (op == "clear_entity") {
            stack.clear_entity(key(owner, field(action, "key")));
        } else {
            stack.set(key(owner, field(action, "key")),
                      text(field(action, "field")),
                      value(owner, field(action, "value")));
        }
    }
    return stack;
}

static EntityRuntimeState state(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    EntityRuntimeState state(std::allocator_arg, Allocator(owner));
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto op = text(field(action, "op"));
        if (op == "set") {
            state.set(key(owner, field(action, "key")),
                      text(field(action, "field")),
                      value(owner, field(action, "value")));
        } else if (op == "push") {
            state.push_change_set();
        } else if (op == "pop") {
            (void)state.pop_change_set();
        } else if (op == "clear") {
            state.clear_current_change_set();
        } else if (op == "commit") {
            state.clear_committed();
        } else if (op == "comment") {
            state.set_comment(text(field(action, "value")));
        } else if (op == "new") {
            state.mark_as_new(key(owner, field(action, "key")));
        } else if (op == "delete") {
            state.mark_as_delete(key(owner, field(action, "key")));
        } else if (op == "version") {
            state.set_original_version(key(owner, field(action, "key")),
                                       number<std::int64_t>(field(action, "value")));
        } else {
            Record fields(owner);
            for (const auto& item : std::get<Json::Array>(field(action, "values").data)) {
                const auto& pair = std::get<Json::Array>(item.data);
                fields.emplace(String(text(pair[0]), owner), value(owner, pair[1]));
            }
            CompactRow row(std::allocator_arg, Allocator(owner), fields);
            state.set_original_compact_row(text(field(action, "entity")), row);
        }
    }
    return state;
}

static EntityGraphBuilder graph(Memory owner, const Json& input) {
    EntityGraphBuilder graph(std::allocator_arg, Allocator(owner));
    for (const auto& item : std::get<Json::Array>(field(input, "entities").data)) {
        auto id = number<std::uint64_t>(field(item, "id"));
        if (std::get<bool>(field(item, "integer").data)) {
            graph.install(id, detail::model_integer<std::int64_t>(field(item, "value"), "integer"));
        } else {
            graph.install(id, String(text(field(item, "value")), owner));
        }
    }
    for (const auto& item : std::get<Json::Array>(field(input, "relations").data)) {
        auto name = text(field(item, "name"));
        auto domain = text(field(item, "owner"));
        auto id = number<std::uint64_t>(field(item, "id"));
        if (std::get<bool>(field(item, "list").data)) {
            SmartList<String> list(std::allocator_arg, Allocator(owner));
            list.is_loaded = true;
            for (const auto& item : std::get<Json::Array>(field(item, "values").data)) {
                list.data.emplace_back(text(item));
            }
            graph.install_relation_list(domain, id, name, std::move(list));
        } else {
            std::optional<String> item_value;
            if (const auto* raw = std::get_if<String>(&field(item, "value").data)) {
                item_value.emplace(*raw, owner);
            }
            graph.install_relation_option(domain, id, name, std::move(item_value));
        }
    }
    return graph;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    Json::Object output(owner);
    auto put = [&](std::string_view name, auto&& item) {
        output.emplace(String(name, owner), Json(owner, std::forward<decltype(item)>(item)));
    };
    auto mode = text(field(input, "mode"));
    if (mode == "key") {
        auto original = key(owner, field(input, "key"));
        auto copy = detail::clone_in(owner, original);
        auto other = key(owner, field(input, "other"));
        put("debug", formatted_debug(owner, original));
        put("clone", formatted_debug(owner, copy));
        put("clone_equal", original == copy);
        put("other_equal", original == other);
        auto order = original <=> other;
        put("ordering", order < 0 ? "Less" : order > 0 ? "Greater" : "Equal");
    } else if (mode == "stack") {
        auto original = changes(owner, field(input, "steps"));
        auto copy = detail::clone_in(owner, original);
        auto current = original.current() ? detail::clone_in(owner, *original.current())
                                          : EntityChangeSet(std::allocator_arg, Allocator(owner));
        auto current_copy = detail::clone_in(owner, current);
        put("debug", formatted_debug(owner, original));
        put("clone", formatted_debug(owner, copy));
        put("clone_equal", original == copy);
        put("current", formatted_debug(owner, current));
        put("current_clone", formatted_debug(owner, current_copy));
        put("current_equal", current == current_copy);
    } else if (mode == "error") {
        auto variant = detail::model_integer<unsigned>(field(input, "variant"), "variant");
        LedgerCompositionError original(
            std::allocator_arg,
            Allocator(owner),
            static_cast<LedgerCompositionError::Kind>(variant),
            variant == 2 ? text(field(input, "entity")) : std::string_view{},
            variant == 2 ? text(field(input, "id")) : std::string_view{},
            variant == 2 ? number<std::int64_t>(field(input, "target")) : 0,
            variant == 2 ? number<std::int64_t>(field(input, "source")) : 0);
        auto copy = detail::clone_in(owner, original);
        put("debug", formatted_debug(owner, original));
        put("clone", formatted_debug(owner, copy));
        put("clone_equal", original == copy);
        put("display", original.to_string(owner));
        put("source_none", original.error_source() == nullptr);
    } else if (mode == "state") {
        auto original = state(user, input);
        put("debug", formatted_debug(owner, original));
        auto copy = detail::clone_in(owner, original);
        put("clone", formatted_debug(owner, copy));
        auto fresh = EntityRuntimeState::fresh_with_shared_graph(owner, original);
        put("fresh", formatted_debug(owner, fresh));
        put("equal_clone", original == copy);
        put("equal_fresh", original == fresh);
    } else {
        auto builder = graph(owner, input);
        put("builder", formatted_debug(owner, builder));
        auto original = state(user, input);
        auto frozen = original.freeze_graph(std::move(builder));
        if (!frozen) {
            throw std::logic_error("Original fixture graph must publish");
        }
        auto domain = text(field(input, "owner"));
        auto id = number<std::uint64_t>(field(input, "owner_id"));
        auto list = original.relation_list<String>(domain, id, "items");
        auto option = original.relation_option<String>(domain, id, "optional");
        put("state", formatted_debug(owner, original));
        auto copy = detail::clone_in(owner, original);
        put("clone_state", formatted_debug(owner, copy));
        put("state_equal", original == copy);
        put("list", formatted_debug(owner, list));
        put("list_clone", formatted_debug(owner, list));
        put("list_state", formatted_debug(owner, list.state()));
        put("list_loaded", list.is_loaded());
        put("list_empty", list.is_empty());
        put("option", formatted_debug(owner, option));
        put("option_clone", formatted_debug(owner, option));
        put("option_state", formatted_debug(owner, option.state()));
        put("option_loaded", option.is_loaded());
        put("option_empty", option.is_empty());
    }
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid entity-runtime fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
