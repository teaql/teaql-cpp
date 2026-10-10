#include "../save_preparation_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static bool flag(const Json& j, std::string_view key) {
    auto* v = optional(j, key);
    return v && std::get<bool>(v->data);
}

static Record record(Memory r, const Json& j) {
    Record out(r);
    for (const auto& [k, v] : std::get<Json::Object>(j.data)) {
        out.emplace(String(k, r), value(r, v));
    }
    return out;
}

static Json record_out(Memory r, const Record& values) {
    auto out = detail::model_json(r);
    for (const auto& [k, v] : values) {
        detail::model_put(out, k, detail::debug_value(r, v));
    }
    return out;
}

static std::optional<FieldSet> fields(Memory r, const Json* input) {
    if (!input || std::holds_alternative<std::monostate>(input->data)) {
        return {};
    }
    FieldSet out(r);
    for (const auto& v : std::get<Json::Array>(input->data)) {
        out.emplace(text(v));
    }
    return out;
}

static Json fields_out(Memory r, const std::optional<FieldSet>& fields) {
    if (!fields) {
        return Json(r);
    }
    Json::Array out(r);
    for (const auto& f : *fields) {
        out.emplace_back(f);
    }
    return Json(r, out);
}

static GraphNode node(Memory r, const Json& input) {
    GraphNode out(std::allocator_arg, Allocator(r), text(field(input, "entity")));
    out.values.fields = record(r, field(input, "values"));
    out.operation = GraphOperation(std::get<std::uint64_t>(field(input, "operation").data));
    out.dirty_fields = fields(r, optional(input, "dirty"));
    if (auto* old = optional(input, "old");
        old && !std::holds_alternative<std::monostate>(old->data)) {
        out.original_values.emplace(r);
        out.original_values->fields = record(r, *old);
    }
    if (auto* comment = optional(input, "comment");
        comment && std::holds_alternative<String>(comment->data)) {
        out.set_comment(text(*comment));
    }
    for (const auto& [name, children] : std::get<Json::Object>(field(input, "relations").data)) {
        auto& list = out.relations.try_emplace(String(name, r)).first->second;
        for (const auto& child : std::get<Json::Array>(children.data)) {
            list.emplace_back(node(r, child));
        }
    }
    return out;
}

static Json node_out(Memory r, const GraphNode& node) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", node.entity);
    detail::model_put(out, "values", record_out(r, node.values.fields));
    constexpr std::string_view names[]{"Upsert", "Create", "Reference", "Remove"};
    detail::model_put(out, "operation", names[std::size_t(node.operation)]);
    detail::model_put(out, "dirty", fields_out(r, node.dirty_fields));
    detail::model_put(
        out, "old", node.original_values ? record_out(r, node.original_values->fields) : Json(r));
    detail::model_put(out, "comment", node.comment ? Json(r, *node.comment) : Json(r));
    auto rels = detail::model_json(r);
    for (const auto& [name, children] : node.relations) {
        Json::Array list(r);
        for (const auto& child : children) {
            list.emplace_back(node_out(r, child));
        }
        detail::model_put(rels, name, list);
    }
    detail::model_put(out, "relations", rels);
    return out;
}

static EntityKey key(Memory r, const Json& j) {
    return EntityKey(
        std::allocator_arg, Allocator(r), text(field(j, "entity")), value(r, field(j, "id")));
}

static Json state_out(Memory r, const EntityRuntimeState& root, const Json& input) {
    auto out = detail::model_json(r);
    Json::Array changes(r), fresh(r), deleted(r), versions(r);
    auto current = root.current_change_set();
    for (const auto& [k, v] : current.changes()) {
        auto entry = detail::model_json(r);
        detail::model_put(entry, "key", k.debug(r));
        detail::model_put(entry, "values", record_out(r, v.fields));
        changes.emplace_back(std::move(entry));
    }
    for (const auto& k : root.new_keys()) {
        fresh.emplace_back(k.debug(r));
    }
    for (const auto& k : root.deleted_keys()) {
        deleted.emplace_back(k.debug(r));
    }
    for (const auto& j : std::get<Json::Array>(field(input, "keys").data)) {
        auto k = key(r, j);
        auto item = detail::model_json(r);
        detail::model_put(item, "key", k.debug(r));
        auto v = root.get_original_version(k);
        detail::model_put(item, "version", v ? Json(r, *v) : Json(r));
        versions.emplace_back(std::move(item));
    }
    detail::model_put(out, "changes", changes);
    detail::model_put(out, "new", fresh);
    detail::model_put(out, "deleted", deleted);
    detail::model_put(out, "versions", versions);
    return out;
}

static void seed(Memory r, EntityRuntimeState& root, const Json& input) {
    for (const auto& entry : std::get<Json::Array>(field(input, "seed").data)) {
        auto k = key(r, entry);
        for (const auto& [f, v] : record(r, field(entry, "values"))) {
            root.set(k, f, v);
        }
        if (flag(entry, "new")) {
            root.mark_as_new(k);
        }
        if (flag(entry, "delete")) {
            root.mark_as_delete(k);
        }
        if (auto* v = optional(entry, "version")) {
            std::int64_t n;
            if (auto* i = std::get_if<std::int64_t>(&v->data)) {
                n = *i;
            } else {
                n = std::get<std::uint64_t>(v->data);
            }
            root.set_original_version(k, n);
        }
    }
}

static Json evaluate(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    auto out = detail::model_json(r);
    auto& module = user.make<RuntimeModule>();
    auto& checkers = user.make<InMemoryCheckerRegistry>();
    Vector<save_fixture::Checker*> callbacks(r);
    for (const auto& info : std::get<Json::Array>(field(input, "descriptors").data)) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), text(field(info, "name")));
        for (auto name : {"id", "version", "name", "parent_id"}) {
            auto& p = descriptor.property(name, DataType::Text);
            if (name == std::string_view("id")) {
                p.id();
            }
            if (name == std::string_view("version")) {
                p.version();
            }
            if (flag(info, "required") && name != std::string_view("parent_id")) {
                p.not_null();
            }
        }
        for (const auto& relation : std::get<Json::Array>(field(info, "relations").data)) {
            descriptor.relation(text(field(relation, "name")), text(field(relation, "target")))
                .local(text(field(relation, "local")))
                .foreign(text(field(relation, "foreign")));
        }
        module.descriptor(std::move(descriptor));
        if (auto* action = optional(info, "checker")) {
            auto& callback =
                user.make<save_fixture::Checker>(text(field(info, "name")), text(*action));
            callbacks.push_back(&callback);
            checkers.register_checker(user, callback);
        }
    }
    module.apply_to(user);
    user.set_checker_registry(user, checkers);
    auto& root = user.make<EntityRuntimeState>();
    seed(r, root, input);
    auto op = text(field(input, "op"));
    RuntimeHookResult status = RuntimeHookResult::ok(r, {});
    std::optional<GraphNode> graph;
    if (op == "extract" || op == "typed") {
        MutationValues values(r);
        values.fields = record(r, field(input, "values"));
        Result<GraphNode, RuntimeError> result = Result<GraphNode, RuntimeError>::err(
            r, RuntimeError::graph(r, "uninitialized fixture"));
        if (op == "extract") {
            result = detail::graph_node_from_values(
                user, text(field(input, "entity")), std::move(values));
        } else {
            save_fixture::Model model(std::allocator_arg, Allocator(r));
            model.values = std::move(values);
            model.loaded = fields(r, optional(input, "loaded"));
            model.dirty = fields(r, optional(input, "dirty"));
            model.fresh = flag(input, "fresh");
            model.removed = flag(input, "removed");
            if (auto* old = optional(input, "old");
                old && !std::holds_alternative<std::monostate>(old->data)) {
                model.original.emplace(r);
                model.original->fields = record(r, *old);
            }
            if (auto* comment = optional(input, "comment");
                comment && std::holds_alternative<String>(comment->data)) {
                model.comment.emplace(text(*comment), r);
            }
            result = graph_node_from_entity(user, std::move(model));
        }
        if (result) {
            graph.emplace(std::move(result).value());
        } else {
            status = RuntimeHookResult::err(r, std::move(result).error());
        }
    } else {
        graph.emplace(node(r, field(input, "node")));
        if (op == "preflight") {
            status = detail::preflight_graph(
                user, *graph, ObjectLocation::root(r), flag(input, "root") ? &root : nullptr);
        } else if (op == "merge") {
            status = detail::merge_relation_mutations_into_root(root, *graph);
        } else if (op == "hydrate") {
            EntityRuntimeState::Keys visited(r);
            if (const auto* id = graph->id()) {
                visited.emplace(graph->entity, *id);
            }
            status = detail::hydrate_ledger_relations(user, root, *graph, visited);
            Json::Array keys(r);
            for (const auto& k : visited) {
                keys.emplace_back(k.debug(r));
            }
            detail::model_put(out, "visited", keys);
        } else if (op == "flags") {
            auto k = key(r, field(input, "key"));
            status = detail::reject_cancelled_new_root(root, k);
            auto descriptor = user.require_entity(graph->entity);
            if (descriptor) {
                detail::model_put(
                    out,
                    "preserve",
                    detail::can_preserve_loaded_relations(root, k, descriptor.value().get()));
            }
        }
    }
    detail::model_put(out, "error", status ? Json(r) : Json(r, status.error().to_string(r)));
    detail::model_put(out, "graph", graph ? node_out(r, *graph) : Json(r));
    detail::model_put(out, "state", state_out(r, root, input));
    Json::Array calls(r);
    for (const auto* c : callbacks) {
        auto item = detail::model_json(r);
        detail::model_put(item, "entity", c->entity());
        Json::Array entries(r);
        for (const auto& path : c->calls) {
            entries.emplace_back(path);
        }
        detail::model_put(item, "paths", entries);
        calls.emplace_back(std::move(item));
    }
    detail::model_put(out, "calls", calls);
    Json::Array locations(r);
    if (graph) {
        for (const auto& [k, path] : detail::ledger_object_locations(r, *graph)) {
            auto item = detail::model_json(r);
            detail::model_put(item, "key", k.debug(r));
            detail::model_put(item, "model", path.model_path(r));
            detail::model_put(item, "instance", path.instance_path(r));
            locations.emplace_back(std::move(item));
        }
    }
    detail::model_put(out, "locations", locations);
    return out;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            return 2;
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
