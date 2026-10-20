#include "../generated_relations/include/cpp_relations.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace cpp_relations;

static Json state(Memory r, const EntityRuntimeState& root, std::span<const EntityKey> keys) {
    Json::Array values(r);
    for (const auto& key : keys) {
        Json::Object item(r);
        item.emplace(String("entity", r), Json(r, key.entity));
        item.emplace(String("id", r), key.id.to_json_value(r));
        auto name = root.get(key, "name");
        item.emplace(String("name", r), name ? name->to_json_value(r) : Json(r));
        item.emplace(String("new", r), Json(r, root.is_new(key)));
        item.emplace(String("deleted", r), Json(r, root.is_marked_as_delete(key)));
        auto version = root.get_original_version(key);
        item.emplace(String("version", r), version ? Json(r, *version) : Json(r));
        auto comment = root.get_entity_comment(key);
        item.emplace(String("comment", r), comment ? Json(r, *comment) : Json(r));
        Json::Array fields(r);
        for (const auto& field : root.changed_field_names(key)) {
            fields.emplace_back(field);
        }
        item.emplace(String("fields", r), Json(r, fields));
        values.emplace_back(item);
    }
    return Json(r, values);
}

static Json eval_name(Memory r, const auto& result) {
    Json::Object out(r);
    if (result.is_value()) {
        out.emplace(String("state", r), Json(r, "Value"));
        out.emplace(String("value", r), Json(r, result.get()));
    } else if (result.is_null()) {
        out.emplace(String("state", r), Json(r, "Null"));
    } else {
        out.emplace(String("state", r), Json(r, "NotLoaded"));
        out.emplace(String("node", r), Json(r, result.missing().failed_node));
        out.emplace(String("path", r), Json(r, result.missing().attempted_path));
    }
    return Json(r, out);
}

static Json original_name(Memory r, const auto& entity) {
    auto original = entity.original_values();
    if (original) {
        auto found = original->fields.find("name");
        if (found != original->fields.end()) {
            return found->second.to_json_value(r);
        }
    }
    return Json(r);
}

template <class Model>
static Json run(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t(7)));
    row.emplace(String("name", r), Value(r, "parent"));
    row.emplace(String("version", r), Value(r, std::int64_t(3)));
    row.emplace(String("platform_id", r), Value(r, std::uint64_t(9)));
    if (std::get<bool>(field(input, "embedded").data)) {
        Record child(r);
        child.emplace(String("id", r), Value(r, std::uint64_t(9)));
        child.emplace(String("name", r), Value(r, "child"));
        child.emplace(String("version", r), Value(r, std::int64_t(5)));
        row.emplace(String("platform", r), Value(r, std::move(child)));
    }
    auto& entity = user_context.make<Model>(
        Model::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row)).value());
    if (std::get<bool>(field(input, "partial").data)) {
        const std::array<std::string_view, 2> identity{"id", "version"};
        entity.setLoadState(eval::LoadState::partial(r, identity));
    }
    auto original_root = detail::clone_in(r, entity.root);
    entity.updateName(text(field(input, "text")));
    original_root.set_entity_comment(entity.entity_key(), "source parent");
    auto parent_key = entity.entity_key();
    EntityKey child_key(std::allocator_arg, Allocator(r), "Platform", Value(r, std::uint64_t(9)));
    if (auto* child = entity.platform()) {
        child->root.set(child_key, "name", Value(r, "pending child"));
        child->root.set_entity_comment(child_key, "source child");
        if (text(field(input, "child_state")) == "new") {
            child->root.mark_as_new(child_key);
        }
        if (text(field(input, "child_state")) == "delete") {
            child->root.mark_as_delete(child_key);
        }
    }
    auto& target = user_context.make<EntityRuntimeState>();
    auto target_mode = text(field(input, "target"));
    if (target_mode == "name") {
        target.set(parent_key, "name", Value(r, "target name"));
    }
    if (target_mode == "version") {
        target.set_original_version(parent_key, 99);
    }
    if (target_mode == "child") {
        target.set(child_key, "name", Value(r, "target child"));
    }
    std::array<EntityKey, 2> keys{detail::clone_in(r, parent_key), detail::clone_in(r, child_key)};
    auto view = [&](const Model& model) {
        Json::Object out(r);
        out.emplace(String("name", r), Json(r, model.name()));
        out.emplace(String("name_eval", r), eval_name(r, model.evalName()));
        Json child(r);
        if (auto* value = model.platform()) {
            Json::Object fields(r);
            fields.emplace(String("name", r), Json(r, value->name()));
            fields.emplace(String("eval", r), eval_name(r, value->evalName()));
            fields.emplace(String("original", r), original_name(r, *value));
            child = Json(r, fields);
        }
        out.emplace(String("child", r), std::move(child));
        out.emplace(String("original", r), original_name(r, model));
        Json::Array loaded(r);
        for (auto name : {"name", "platform_id", "platform"}) {
            loaded.emplace_back(model.isLoaded(name));
        }
        out.emplace(String("loaded", r), Json(r, loaded));
        return Json(r, out);
    };
    auto before = state(r, target, keys), source = view(entity);
    entity.attachRuntimeStateRecursive(target);
    auto attached = view(entity), merged = state(r, target, keys);
    target.set(parent_key, "name", Value(r, "after parent"));
    target.set(child_key, "name", Value(r, "after child"));
    Json::Object result(r);
    result.emplace(String("before", r), before);
    result.emplace(String("source", r), source);
    result.emplace(String("attached", r), attached);
    result.emplace(String("merged", r), merged);
    result.emplace(String("after", r), view(entity));
    result.emplace(String("old", r), state(r, original_root, keys));
    result.emplace(String("target", r), state(r, target, keys));
    return Json(r, result);
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            Json retained(global.memory_resource());
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("invalid recursive-attachment fixture");
                }
                auto result = text(field(*input, "model")) == "school"
                                  ? run<School>(*user_context, *input)
                                  : run<Device>(*user_context, *input);
                retained = detail::clone_in(global.memory_resource(), result);
            }
            user_context.reset();
            std::cout << retained.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
