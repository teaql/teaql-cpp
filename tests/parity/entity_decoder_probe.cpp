#include <teaql/runtime/module.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "../entity_decoder_fixture.hpp"
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;
using namespace decoder_fixture;

static std::span<const Json> items(const Json& j, std::string_view key) {
    auto* p = optional(j, key);
    return p ? std::span<const Json>(std::get<Json::Array>(p->data)) : std::span<const Json>{};
}

static unsigned kind(const Json& j) {
    auto* v = optional(j, "kind");
    return v ? unsigned(std::get<std::uint64_t>(v->data)) : 0;
}

static CompactRow row(Memory r, const Json& j) {
    Record data(r);
    for (const auto& [k, v] : std::get<Json::Object>(j.data))
        data.emplace(String(k, r), value(r, v));
    return CompactRow(std::allocator_arg, Allocator(r), data);
}

static Json decode(UserContext& c, const Json& input) {
    auto r = c.memory_resource();
    auto& root = c.make<EntityRuntimeState>();
    auto& registry = c.make<InMemoryEntityGraphDecoderRegistry>();
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    Json::Array steps(r);
    for (const auto& step : items(input, "steps")) {
        auto op = text(field(step, "op"));
        Json result(r);
        if (op == "register") {
            if (kind(step) == 1)
                registry.register_entity<Item<1>>();
            else
                registry.register_entity<Item<0>>();
        } else if (op == "copy") {
            auto& copy = c.make<InMemoryEntityGraphDecoderRegistry>(registry);
            registry.register_entity<Item<1>>();
            c.set_entity_graph_decoder_registry(
                copy); /* Copy remains independent of subsequent registration. */
            auto rows = Vector<CompactRow>(r);
            for (const auto& item : items(step, "rows"))
                rows.emplace_back(row(r, item));
            auto v = copy.decode_compact_batch("Item", std::move(rows), root, graph);
            result = v ? Json(r) : Json(r, v.error().to_string(r));
        } else {
            auto name =
                optional(step, "entity") ? text(field(step, "entity")) : std::string_view("Item");
            EntityGraphDecodeResult outcome = EntityGraphDecodeResult::ok(r, {});
            if (op == "compact")
                outcome = registry.decode_compact(name, row(r, field(step, "row")), root, graph);
            else {
                Vector<CompactRow> rows(r);
                for (const auto& item : items(step, "rows"))
                    rows.emplace_back(row(r, item));
                if (op == "batch")
                    outcome = registry.decode_compact_batch(name, std::move(rows), root, graph);
                else if (op == "list")
                    outcome = registry.decode_compact_list(name,
                                                           std::move(rows),
                                                           root,
                                                           graph,
                                                           "PurchaseOrder",
                                                           9,
                                                           text(field(step, "relation")));
                else if (op == "option")
                    outcome = registry.decode_compact_option(name,
                                                             std::move(rows),
                                                             root,
                                                             graph,
                                                             "PurchaseOrder",
                                                             9,
                                                             text(field(step, "relation")));
                else
                    throw std::runtime_error("decode fixture op");
            }
            result = outcome ? Json(r) : Json(r, outcome.error().to_string(r));
        }
        auto state = detail::model_json(r);
        detail::model_put(state, "error", result);
        detail::model_put(state, "entities", graph.entity_count());
        detail::model_put(state, "relations", graph.relation_list_count());
        detail::model_put(state, "contains", registry.contains("Item"));
        steps.emplace_back(std::move(state));
    }
    auto frozen = root.freeze_graph(std::move(graph));
    if (!frozen)
        throw std::runtime_error("first freeze");
    Vector<const EntityRuntimeState*> states(r);
    Json::Array entities(r), relations(r);
    auto describe = [&]<unsigned K>(const Item<K>* v) -> Json {
        if (!v)
            return Json(r);
        auto out = detail::model_json(r);
        detail::model_put(out, "id", detail::debug_value(r, v->id_value()));
        auto* label = v->row.get("label");
        detail::model_put(out, "label", label ? Json(r, detail::debug_value(r, *label)) : Json(r));
        detail::model_put(out, "kind", K);
        detail::model_put(out, "context", v->state.has_value());
        if (v->state) {
            states.push_back(&*v->state);
            detail::model_put(
                out, "sees_first", v->state->template resolve_entity<Item<0>>(1) != nullptr);
        }
        return out;
    };
    for (std::uint64_t id : {1, 2, 3}) {
        entities.emplace_back(describe(root.resolve_entity<Item<0>>(id)));
        entities.emplace_back(describe(root.resolve_entity<Item<1>>(id)));
    }
    for (auto name : {"rows", "optional", "empty"}) {
        auto out = detail::model_json(r);
        detail::model_put(out, "present", root.has_relation_view("purchase_order", 9, name));
        Json::Array types(r);
        auto inspect = [&]<unsigned K>() {
            auto out = detail::model_json(r);
            Json list(r), option(r);
            if (auto* l = root.resolve_relation_list<Item<K>>("purchase_order", 9, name)) {
                Json::Array values(r);
                for (const auto& v : l->data)
                    values.emplace_back(describe(&v));
                list = detail::model_json(r);
                detail::model_put(list, "loaded", l->is_loaded);
                detail::model_put(list, "data", values);
            }
            auto* v = root.resolve_relation_option<Item<K>>("purchase_order", 9, name);
            if (v && *v)
                option = describe(&**v);
            detail::model_put(out, "list", list);
            detail::model_put(out, "option_present", v != nullptr);
            detail::model_put(out, "option", option);
            types.emplace_back(std::move(out));
        };
        inspect.template operator()<0>();
        inspect.template operator()<1>();
        detail::model_put(out, "types", types);
        relations.emplace_back(std::move(out));
    }
    Json::Array equality(r), root_equal(r);
    for (auto* a : states) {
        root_equal.emplace_back(*a == root);
        for (auto* b : states)
            equality.emplace_back(*a == *b);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "steps", steps);
    detail::model_put(out, "entities", entities);
    detail::model_put(out, "relations", relations);
    detail::model_put(out, "shared_ledgers", equality);
    detail::model_put(out, "root_ledger", root_equal);
    return out;
}

static Json module(UserContext& c, const Json& input) {
    auto r = c.memory_resource();
    auto& m = c.make<RuntimeModule>();
    Json::Array out(r);
    for (const auto& step : items(input, "steps")) {
        auto op = text(field(step, "op"));
        auto k = kind(step);
        if (op == "entity") {
            if (k == 2)
                m.entity<Item<2>>();
            else if (k == 1)
                m.entity<Item<1>>();
            else
                m.entity<Item<0>>();
        } else if (op == "descriptor")
            m.descriptor(
                EntityDescriptor(std::allocator_arg, Allocator(r), text(field(step, "name"))));
        else if (op == "behavior") {
            auto& b = c.make<NamedBehavior>(text(field(step, "value")));
            m.behavior(text(field(step, "name")), c, b);
        } else if (op == "entity_behavior") {
            auto& b = c.make<NamedBehavior>(text(field(step, "value")));
            if (k == 2)
                m.entity_with_behavior<Item<2>>(c, b);
            else
                m.entity_with_behavior<Item<0>>(c, b);
        } else
            throw std::runtime_error("module fixture op");
        m.apply_to(c);
        Json::Array snapshot(r);
        for (auto name : {"Item", "Alias", "OnlyBehavior"}) {
            auto one = detail::model_json(r);
            auto* e = c.entity(name);
            detail::model_put(one, "entity", e ? Json(r, e->name) : Json(r));
            detail::model_put(one, "table", e ? Json(r, e->table_name) : Json(r));
            detail::model_put(one, "has", c.has_entity_data_service(name));
            auto b = c.entity_data_service_behavior(name);
            Json behavior(r);
            if (b) {
                Json::Array names(r);
                for (const auto& name : b->get().relation_loads(r, c))
                    names.emplace_back(name);
                behavior = Json(r, names);
            }
            detail::model_put(one, "behavior", behavior);
            snapshot.emplace_back(std::move(one));
        }
        out.emplace_back(snapshot);
    }
    return Json(r, out);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto c = global.make_user_context();
        auto r = c->memory_resource();
        auto j = Json::parse(r, line);
        std::cout << (text(field(*j, "mode")) == "module" ? module(*c, *j) : decode(*c, *j)).dump(r)
                  << '\n';
    }
}
