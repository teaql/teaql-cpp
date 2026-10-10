#include <teaql/runtime/graph.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static unsigned integer(const Json& j) {
    return unsigned(std::get<std::uint64_t>(j.data));
}

static Vector<String> strings(Memory r, const Json& j) {
    Vector<String> out(r);
    for (const auto& s : std::get<Json::Array>(j.data)) {
        out.emplace_back(text(s));
    }
    return out;
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

static Json fields_out(Memory r, std::span<const String> fields) {
    Json::Array result(r);
    for (const auto& f : fields) {
        result.emplace_back(f);
    }
    return Json(r, result);
}

static std::optional<EntitySnapshot> old_values(Memory r, const Json& j) {
    if (std::holds_alternative<std::monostate>(j.data)) {
        return {};
    }
    EntitySnapshot snapshot(r);
    snapshot.fields = record(r, j);
    return snapshot;
}

static GraphNode node(Memory r, const Json& j) {
    GraphNode n(std::allocator_arg, Allocator(r), text(field(j, "entity")));
    n.operation = static_cast<GraphOperation>(integer(field(j, "operation")));
    n.values.fields = record(r, field(j, "values"));
    if (!std::holds_alternative<std::monostate>(field(j, "comment").data)) {
        n.set_comment(text(field(j, "comment")));
    }
    if (!std::holds_alternative<std::monostate>(field(j, "dirty").data)) {
        n.dirty_fields.emplace(r);
        for (const auto& v : std::get<Json::Array>(field(j, "dirty").data)) {
            n.dirty_fields->emplace(text(v));
        }
    }
    n.original_values = old_values(r, field(j, "old"));
    for (const auto& [name, children] : std::get<Json::Object>(field(j, "relations").data)) {
        n.with_relations(name, {});
        for (const auto& child : std::get<Json::Array>(children.data)) {
            n.relation(name, node(r, child));
        }
    }
    return n;
}

static Json node_out(Memory r, const GraphNode& n) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", n.entity);
    detail::model_put(out, "values", record_out(r, n.values.fields));
    constexpr std::string_view names[]{"Upsert", "Create", "Reference", "Remove"};
    detail::model_put(out, "operation", names[unsigned(n.operation)]);
    detail::model_put(out, "comment", n.comment ? Json(r, *n.comment) : Json(r));
    Json dirty(r);
    if (n.dirty_fields) {
        Json::Array entries(r);
        for (const auto& f : *n.dirty_fields) {
            entries.emplace_back(f);
        }
        dirty = Json(r, entries);
    }
    detail::model_put(out, "dirty", dirty);
    detail::model_put(
        out, "old", n.original_values ? record_out(r, n.original_values->fields) : Json(r));
    detail::model_put(out, "id", n.id() ? Json(r, detail::debug_value(r, *n.id())) : Json(r));
    auto relations = detail::model_json(r);
    for (const auto& [name, children] : n.relations) {
        Json::Array list(r);
        for (const auto& child : children) {
            list.emplace_back(node_out(r, child));
        }
        detail::model_put(relations, name, list);
    }
    detail::model_put(out, "relations", relations);
    return out;
}

static std::string_view kind(GraphMutationKind k) {
    constexpr std::string_view names[]{"Create", "Update", "Delete", "Reference"};
    return names[unsigned(k)];
}

static Json trace_out(Memory r, const TraceScopeToken* t) {
    if (!t) {
        return Json(r);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "index", std::to_string(t->node_index));
    Json::Array chain(r);
    for (const auto& n : t->recover_trace_chain(r)) {
        auto j = detail::model_json(r);
        detail::model_put(j, "entity", n.entity_type);
        detail::model_put(j, "comment", n.comment);
        detail::model_put(j, "id", n.entity_id ? Json(r, std::to_string(*n.entity_id)) : Json(r));
        chain.emplace_back(std::move(j));
    }
    detail::model_put(out, "chain", chain);
    return out;
}

static Json item_out(Memory r, const GraphMutationPlanItem& i) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", i.entity);
    detail::model_put(out, "kind", kind(i.kind));
    detail::model_put(out, "fields", fields_out(r, i.update_fields));
    detail::model_put(out, "index", std::to_string(i.item_index));
    detail::model_put(out, "values", record_out(r, i.values.fields));
    detail::model_put(out, "old", i.old_values ? record_out(r, i.old_values->fields) : Json(r));
    detail::model_put(out, "scope", trace_out(r, i.scope_token));
    return out;
}

static Json plan_out(Memory r, const GraphMutationPlan& p) {
    auto out = detail::model_json(r);
    Json::Array items(r), batches(r), counts(r);
    for (const auto& i : p.items) {
        items.emplace_back(item_out(r, i));
    }
    for (const auto& b : p.batches) {
        auto j = detail::model_json(r);
        detail::model_put(j, "entity", b.entity);
        detail::model_put(j, "kind", kind(b.kind));
        detail::model_put(j, "fields", fields_out(r, b.update_fields));
        Json::Array entries(r);
        for (const auto& i : b.items) {
            entries.emplace_back(item_out(r, i));
        }
        detail::model_put(j, "items", entries);
        batches.emplace_back(std::move(j));
    }
    for (const auto& [key, count] : p.grouped_counts(r)) {
        Json::Array row(r);
        row.emplace_back(key.first);
        row.emplace_back(kind(key.second));
        row.emplace_back(std::to_string(count));
        counts.emplace_back(row);
    }
    detail::model_put(out, "items", items);
    detail::model_put(out, "batches", batches);
    detail::model_put(out, "counts", counts);
    detail::model_put(out, "next", std::to_string(p.next_item_index));
    detail::model_put(out, "len", std::to_string(p.len()));
    detail::model_put(out, "batch_count", std::to_string(p.batch_count()));
    detail::model_put(out, "empty", p.is_empty());
    detail::model_put(out, "root", p.planned_root ? node_out(r, *p.planned_root) : Json(r));
    return out;
}

static Json evaluate(UserContext& c, const Json& j) {
    auto r = c.memory_resource();
    if (text(field(j, "mode")) == "node") {
        auto n = node(r, field(j, "node"));
        auto copy = detail::clone_in(r, n);
        auto fields = sorted_update_fields(r, copy.values, strings(r, field(j, "excluded")));
        auto command = copy.values.into_mutation_values(r);
        EntityValues roundtrip(std::allocator_arg, Allocator(r), command);
        auto out = detail::model_json(r);
        detail::model_put(out, "node", node_out(r, copy));
        detail::model_put(out, "fields", fields_out(r, fields));
        detail::model_put(out, "roundtrip", roundtrip == copy.values);
        return out;
    }
    GraphMutationPlan p(std::allocator_arg, Allocator(r));
    p.next_item_index = number<std::uint64_t>(field(j, "start"));
    if (!std::holds_alternative<std::monostate>(field(j, "root").data)) {
        p.planned_root = node(r, field(j, "root"));
    }
    const TraceScopeToken* trace = nullptr;
    std::uint64_t index = 0;
    for (const auto& t : std::get<Json::Array>(field(j, "trace").data)) {
        TraceNode track(std::allocator_arg,
                        Allocator(r),
                        TraceKind::Entity,
                        text(field(t, "entity")),
                        text(field(t, "comment")),
                        index);
        trace = &c.make<TraceScopeToken>(trace, track, index++);
    }
    Json::Array out(r);
    for (const auto& step : std::get<Json::Array>(field(j, "steps").data)) {
        bool overflow = false;
        auto op = text(field(step, "op"));
        if (op == "push") {
            MutationValues values(r);
            values.fields = record(r, field(step, "values"));
            auto old = old_values(r, field(step, "old"));
            auto k = static_cast<GraphMutationKind>(integer(field(step, "kind")));
            try {
                p.push(text(field(step, "entity")),
                       k,
                       values,
                       strings(r, field(step, "fields")),
                       std::get<bool>(field(step, "scope").data) ? trace : nullptr,
                       old);
            } catch (const std::overflow_error&) {
                overflow = true;
            }
        } else if (op == "rebuild") {
            p.rebuild_batches();
        } else if (op == "clear_items") {
            p.items.clear();
        } else if (op == "clear_batches") {
            p.batches.clear();
        }
        auto snapshot = detail::clone_in(r, p);
        auto state = detail::model_json(r);
        detail::model_put(state, "plan", plan_out(r, snapshot));
        detail::model_put(state, "overflow", overflow);
        out.emplace_back(std::move(state));
    }
    return Json(r, out);
}

static Json graph_snapshot(Memory owner, const UserContext& context) {
    auto output = detail::model_json(owner);
    Json::Array initial(owner), roots(owner);
    for (const auto& graph : context.initial_graphs()) {
        initial.emplace_back(node_out(owner, graph));
    }
    for (const auto& graph : context.root_graphs()) {
        roots.emplace_back(node_out(owner, graph));
    }
    detail::model_put(output, "initial", initial);
    detail::model_put(output, "roots", roots);
    return output;
}

static Json module_graphs(GlobalContext& global, UserContext& installed, const Json& input) {
    auto owner = installed.memory_resource();
    auto source = global.make_user_context();
    auto& nodes = source->make<Vector<GraphNode>>();
    for (const auto& value : std::get<Json::Array>(field(input, "nodes").data)) {
        nodes.emplace_back(node(source->memory_resource(), value));
    }
    auto& module = source->make<RuntimeModule>();
    module.apply_to(installed);
    Json::Array steps(owner);
    steps.emplace_back(graph_snapshot(owner, installed));
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        Vector<GraphNode> selected(source->memory_resource());
        for (const auto& index : std::get<Json::Array>(field(step, "indices").data)) {
            selected.emplace_back(nodes.at(integer(index)));
        }
        auto operation = text(field(step, "op"));
        if (operation == "initial") {
            module.initial_graph(selected.at(0));
        } else if (operation == "initials") {
            module.initial_graphs(selected);
        } else if (operation == "root") {
            module.root_graph(selected.at(0));
        } else if (operation == "roots") {
            module.root_graphs(selected);
        } else {
            throw std::invalid_argument("module graph fixture operation");
        }
        module.apply_to(installed);
        steps.emplace_back(graph_snapshot(owner, installed));
    }
    auto& retained = global.make<RuntimeModule>(module);
    module.initial_graph(nodes.at(0));
    auto updated = module.into_context(global);
    source.reset();
    auto cloned = retained.into_context(global);
    auto& empty_module = global.make<RuntimeModule>();
    auto empty = empty_module.into_context(global);
    auto output = detail::model_json(owner);
    detail::model_put(output, "steps", steps);
    detail::model_put(output, "cloned", graph_snapshot(owner, *cloned));
    detail::model_put(output, "updated", graph_snapshot(owner, *updated));
    detail::model_put(output, "installed_after", graph_snapshot(owner, installed));
    detail::model_put(output, "empty", graph_snapshot(owner, *empty));
    return output;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto c = global.make_user_context();
        auto r = c->memory_resource();
        auto j = Json::parse(r, line);
        auto output = text(field(*j, "mode")) == "module_graphs" ? module_graphs(global, *c, *j)
                                                                 : evaluate(*c, *j);
        std::cout << output.dump(r) << '\n';
    }
    return 0;
}
