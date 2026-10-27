#include <cpp_privacy_contracts.hpp>
#include "fixture_value.hpp"
#include <teaql/core/entity_debug.hpp>
#include <array>
#include <iostream>
using namespace teaql;
namespace domain = cpp_privacy_contracts;

static Json edge(Memory r, const EntityRuntimeState& root, std::string_view owner) {
    auto result = detail::model_json(r);
    auto* found = root.resolve_relation_option<domain::Platform>(owner, 101, "platform");
    detail::model_put(result, "state", !found ? "absent" : !*found ? "empty" : "present");
    if (found && *found) {
        detail::model_put(result, "id", (**found).id());
    }
    return result;
}

static domain::Platform
platform(UserContext& user_context, std::uint64_t id, std::string_view name) {
    auto r = user_context.memory_resource();
    auto data = detail::model_json(r);
    detail::model_put(data, "id", id);
    detail::model_put(data, "version", 1);
    detail::model_put(data, "name", name);
    return std::move(user_context.decode_json_entity<domain::Platform>(data)).value();
}

template <class T, class Factory>
static Json
inspect(UserContext& user_context, Factory factory, std::string_view canonical, const Json& input) {
    auto r = user_context.memory_resource();
    bool selected = std::get<bool>(field(input, "selected").data);
    bool embedded = std::get<bool>(field(input, "embedded").data);
    bool cached = std::get<bool>(field(input, "cached").data);
    auto foreign = number<std::uint64_t>(field(input, "foreign_id"));
    auto mode = text(field(input, "owner"));
    auto view = text(field(input, "view"));
    auto data = detail::model_json(r);
    detail::model_put(data, "id", 101);
    detail::model_put(data, "version", 1);
    detail::model_put(data, "name", "OWNER");
    if (embedded) {
        auto nested = detail::model_json(r);
        detail::model_put(nested, "id", 77);
        detail::model_put(nested, "version", 1);
        detail::model_put(nested, "name", "EMBEDDED");
        detail::model_put(data, "platform", std::move(nested));
    } else {
        detail::model_put(data, "platform_id", foreign);
    }
    auto prepared = [&] {
        if (auto* construction = optional(input, "construction");
            construction && text(*construction) == "row") {
            Record fields(r);
            fields.emplace(String("id", r), Value(r, std::uint64_t{101}));
            fields.emplace(String("version", r), Value(r, std::int64_t{1}));
            fields.emplace(String("name", r), Value(r, "OWNER"));
            if (embedded) {
                Value::Object nested(r);
                nested.emplace(String("id", r), Value(r, std::uint64_t{77}));
                nested.emplace(String("version", r), Value(r, std::int64_t{1}));
                nested.emplace(String("name", r), Value(r, "EMBEDDED"));
                fields.emplace(String("platform", r), Value(r, std::move(nested)));
            } else {
                fields.emplace(String("platform_id", r), Value(r, foreign));
            }
            return T::from_compact_row(
                r, CompactRow(std::allocator_arg, Allocator(r), std::move(fields)));
        }
        return user_context.decode_json_entity<T>(data);
    }();
    auto& entity = user_context.make<T>(std::move(prepared).value());
    auto& root = user_context.make<EntityRuntimeState>();
    entity.attachRuntimeStateRecursive(root);
    const std::array<std::string_view, 5> fields{
        "id", "version", "name", "platform_id", "platform"};
    entity.setLoadState(eval::LoadState::partial(r, std::span(fields).first(selected ? 5 : 4)));
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    if (cached) {
        graph.install(100, platform(user_context, 100, "CACHE"));
    }
    auto install = [&](std::string_view owner, std::uint64_t id, std::string_view name) {
        if (view == "present") {
            graph.install_relation_option<domain::Platform>(
                owner,
                101,
                "platform",
                std::optional<domain::Platform>(std::in_place, platform(user_context, id, name)));
        } else if (view == "empty") {
            graph.install_relation_option<domain::Platform>(owner, 101, "platform", std::nullopt);
        } else if (view == "wrong_type") {
            graph.install_relation_option<std::uint64_t>(owner, 101, "platform", 900);
        } else {
            throw std::logic_error("Unknown view");
        }
    };
    auto raw = T::ID_SPACE_ENTITY_NAME;
    auto runtime_name = T::ENTITY_NAME;
    if (mode == "raw") {
        install(raw, 200, "MODEL");
    } else if (mode == "canonical") {
        install(canonical, 200, "MODEL");
    } else if (mode == "trait") {
        install(runtime_name, 300, "TRAIT");
    } else if (mode == "both") {
        install(canonical, 200, "MODEL");
        install(runtime_name, 300, "TRAIT");
    } else if (mode != "none") {
        throw std::logic_error("Unknown owner");
    }
    if (!root.freeze_graph(std::move(graph))) {
        throw std::logic_error("Fresh graph freeze failed");
    }
    auto result = detail::model_json(r);
    detail::model_put(result, "raw", raw);
    detail::model_put(result, "identity", runtime_name);
    auto* getter = entity.platform();
    detail::model_put(result, "getter", getter ? Json(r, getter->id()) : Json(r));
    auto evaluation = entity.evalPlatform();
    auto eval = detail::model_json(r);
    if (evaluation.is_value()) {
        detail::model_put(eval, "state", "Value");
        detail::model_put(eval, "id", evaluation.get().get().id());
    } else if (evaluation.is_null()) {
        detail::model_put(eval, "state", "Null");
    } else {
        detail::model_put(eval, "state", "NotLoaded");
        detail::model_put(eval, "node", evaluation.missing().failed_node);
        detail::model_put(eval, "path", evaluation.missing().attempted_path);
    }
    detail::model_put(result, "eval", std::move(eval));
    auto read = detail::model_json(r);
    try {
        auto value = factory(entity).getPlatform().getName().eval();
        String debug(r);
        DebugWriter writer(debug, {});
        if (value) {
            writer.tuple("Some", [&](auto& out) {
                write_debug(out, *value);
            });
        } else {
            writer.raw("None");
        }
        detail::model_put(read, "kind", "value");
        detail::model_put(read, "value", debug);
    } catch (const ExpressionNotLoadedError& error) {
        detail::model_put(read, "kind", "not_loaded");
        detail::model_put(read, "diagnostic", error.diagnostic().to_json(r));
    }
    detail::model_put(result, "expression", std::move(read));
    detail::model_put(result, "json", detail::clone_in(r, entity).into_json(r));
    auto kernel = detail::model_json(r);
    detail::model_put(kernel, "raw", edge(r, root, raw));
    detail::model_put(kernel, "canonical", edge(r, root, canonical));
    detail::model_put(kernel, "trait", edge(r, root, runtime_name));
    detail::model_put(result, "kernel", std::move(kernel));
    detail::model_put(result, "sql_count", user_context.sql_logs().size());
    return result;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        {
            domain::module(user_context).apply_to(user_context);
            auto r = user_context.memory_resource();
            auto input = Json::parse(r, line).value();
            auto output = text(field(input, "entity")) == "item"
                              ? inspect<domain::Item>(
                                    user_context,
                                    [](const auto& value) {
                                        return domain::E::item(value);
                                    },
                                    "Item",
                                    input)
                              : inspect<domain::Child>(
                                    user_context,
                                    [](const auto& value) {
                                        return domain::E::child(value);
                                    },
                                    "Child",
                                    input);
            std::cout << output.dump(r) << '\n';
        }
        owner.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Relation identity probe retains UserContext memory");
        }
    }
}
