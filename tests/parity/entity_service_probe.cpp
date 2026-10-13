#include <teaql/runtime/data_service.hpp>
#include "fixture_sql.hpp"
#include <teaql/core/detail/debug.hpp>
#include <iostream>

struct UnusedExecutor : QueryExecutor<String>, MutationExecutor<String> {
    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("planning performed I/O");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("planning performed I/O");
    }
};

struct Loads : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    Vector<String> values;

    Loads(std::allocator_arg_t, Allocator a, const Json& j) : values(a.resource()) {
        for (const auto& v : items(j, "loads")) {
            values.emplace_back(text(v));
        }
    }

    Vector<String> relation_loads(Memory r, const UserContext&) const override {
        return Vector<String>(values, r);
    }
};

static Json strings(Memory r, std::span<const String> values) {
    Json::Array out(r);
    for (const auto& value : values) {
        out.emplace_back(value);
    }
    return Json(r, out);
}

static Json expr_out(Memory r, const Expr& e) {
    auto out = detail::model_json(r);
    unsigned kind;
    switch (e.kind) {
    case Expr::Kind::Column:
        kind = 0;
        break;
    case Expr::Kind::Value:
        kind = 1;
        break;
    case Expr::Kind::Binary:
        kind = 3;
        break;
    case Expr::Kind::And:
        kind = 8;
        break;
    case Expr::Kind::Or:
        kind = 9;
        break;
    default:
        throw std::logic_error("Unsupported planning fixture expression");
    }
    detail::model_put(out, "kind", kind);
    detail::model_put(out, "op", unsigned(e.op));
    detail::model_put(out, "name", e.name);
    detail::model_put(out, "value", detail::debug_value(r, e.literal));
    Json::Array args(r);
    for (const auto& a : e.args) {
        args.emplace_back(expr_out(r, a));
    }
    detail::model_put(out, "args", args);
    return out;
}

static SelectQuery query_in(Memory r, const Json& j) {
    auto q = query(r, j);
    {
        std::size_t i = 0;
        for (const auto& input : items(j, "order")) {
            if (auto* name = optional(input, "field")) {
                q.order_by[i].field = text(*name);
            }
            ++i;
        }
    }
    q.entity = optional(j, "entity") ? text(field(j, "entity")) : "Root";
    if (auto* t = optional(j, "threshold")) {
        q.top_n_probe_parent_threshold_value = integer(*t);
    }
    for (const auto& load : items(j, "relations")) {
        if (auto* nested = optional(load, "query")) {
            q.relations.emplace_back(text(field(load, "name")), query_in(r, *nested));
        } else {
            q.relations.emplace_back(text(field(load, "name")));
        }
    }
    return q;
}

static Json query_out(Memory r, const SelectQuery& q) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", q.entity);
    detail::model_put(out, "projection", strings(r, q.projection));
    detail::model_put(out, "filter", q.predicate ? expr_out(r, *q.predicate) : Json(r));
    Json::Array orders(r), relations(r);
    for (const auto& o : q.order_by) {
        auto v = detail::model_json(r);
        detail::model_put(v, "field", o.field);
        detail::model_put(v, "direction", unsigned(o.direction));
        detail::model_put(v, "expr", o.expr ? expr_out(r, *o.expr) : Json(r));
        orders.emplace_back(std::move(v));
    }
    for (const auto& l : q.relations) {
        auto v = detail::model_json(r);
        detail::model_put(v, "name", l.name);
        detail::model_put(v, "query", l.query ? query_out(r, *l.query) : Json(r));
        relations.emplace_back(std::move(v));
    }
    detail::model_put(out, "order", orders);
    detail::model_put(out, "relations", relations);
    detail::model_put(out, "partition", q.partition_field ? Json(r, *q.partition_field) : Json(r));
    Json slice(r);
    if (q.slice) {
        slice = detail::model_json(r);
        detail::model_put(slice, "offset", q.slice->offset);
        detail::model_put(slice, "limit", q.slice->limit ? Json(r, *q.slice->limit) : Json(r));
    }
    detail::model_put(out, "slice", slice);
    detail::model_put(out, "comment", q.comment_text ? Json(r, *q.comment_text) : Json(r));
    detail::model_put(out,
                      "threshold",
                      q.top_n_probe_parent_threshold_value
                          ? Json(r, *q.top_n_probe_parent_threshold_value)
                          : Json(r));
    return out;
}

static Json plan_out(Memory r, const RelationLoadPlan& p) {
    auto out = detail::model_json(r);
    detail::model_put(out, "parent", p.parent_entity);
    detail::model_put(out, "name", p.relation_name);
    detail::model_put(out, "path", p.path);
    detail::model_put(out, "target", p.target_entity);
    detail::model_put(out, "local", p.local_key);
    detail::model_put(out, "foreign", p.foreign_key);
    detail::model_put(out, "many", p.many);
    detail::model_put(out, "query", p.query ? query_out(r, *p.query) : Json(r));
    Json::Array children(r);
    for (const auto& child : p.children) {
        children.emplace_back(plan_out(r, child));
    }
    detail::model_put(out, "children", children);
    return out;
}

static Json evaluate(UserContext& c, const Json& j) {
    auto r = c.memory_resource();
    auto& store = c.make<InMemoryMetadataStore>();
    for (const auto& input : items(j, "entities")) {
        auto e = entity(r, input);
        for (const auto& rel : items(input, "relations")) {
            auto& p = e.relation(text(field(rel, "name")), text(field(rel, "target")));
            p.local_key = text(field(rel, "local"));
            p.foreign_key = text(field(rel, "foreign"));
            p.many = std::get<bool>(field(rel, "many").data);
        }
        store.register_entity(std::move(e));
    }
    c.set_metadata(c, store);
    auto& loads = c.make<Loads>(j);
    auto& behaviors = c.make<InMemoryEntityDataServiceBehaviorRegistry>();
    behaviors.register_behavior("Root", c, loads);
    c.set_entity_data_service_behavior_registry(c, behaviors);
    auto& executor = c.make_resource<UnusedExecutor>();
    auto& service = EntityDataService<UnusedExecutor>::for_executor(c, "Root", c, executor);
    Vector<CompactRow> rows(r);
    for (const auto& input : items(j, "rows")) {
        Record record(r);
        for (const auto& [k, v] : std::get<Json::Object>(input.data)) {
            record.emplace(String(k, r), value(r, v));
        }
        rows.emplace_back(record);
    }
    auto explicit_loads = optional(j, "query");
    auto q = explicit_loads ? query_in(r, *explicit_loads) : service.select();
    auto result = explicit_loads ? detail::build_relation_plans_from_loads(c, q.entity, q.relations)
                                 : service.relation_plans();
    auto out = detail::model_json(r);
    if (!result) {
        detail::model_put(out, "error", result.error().to_string(r));
        return out;
    }
    Json::Array plans(r), queries(r), compact(r);
    for (const auto& p : result.value()) {
        plans.emplace_back(plan_out(r, p));
        queries.emplace_back(query_out(r, detail::query_for_relation_plan(c, p, rows, false)));
        compact.emplace_back(query_out(r, detail::query_for_relation_plan(c, p, rows, true)));
    }
    detail::model_put(out, "plans", plans);
    detail::model_put(out, "queries", queries);
    detail::model_put(out, "compact", compact);
    if (!explicit_loads) {
        auto found =
            service.relation_query(optional(j, "find") ? text(field(j, "find")) : "children", rows);
        detail::model_put(out,
                          "lookup",
                          found ? query_out(r, found.value())
                                : Json(r, found.error().to_string(r)));
    }
    return out;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext g;
        auto c = g.make_user_context();
        auto r = c->memory_resource();
        auto j = Json::parse(r, line);
        std::cout << evaluate(*c, *j).dump(r) << '\n';
    }
}
