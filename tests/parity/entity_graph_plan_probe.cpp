#include "../support/entity_graph_plan_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/query_debug.hpp>
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

static Json record_out(Memory r, const Record& values, std::int64_t before, std::int64_t after) {
    auto out = detail::model_json(r);
    for (const auto& [k, v] : values) {
        auto* time = std::get_if<Timestamp>(&v.data);
        detail::model_put(out,
                          k,
                          time && time->as_millis() >= before && time->as_millis() <= after
                              ? String("Timestamp(<current>)", r)
                              : detail::debug_value(r, v));
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

static Json node_out(Memory r, const GraphNode& node, std::int64_t before, std::int64_t after) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", node.entity);
    detail::model_put(out, "values", record_out(r, node.values.fields, before, after));
    constexpr std::string_view names[]{"Upsert", "Create", "Reference", "Remove"};
    detail::model_put(out, "operation", names[std::size_t(node.operation)]);
    detail::model_put(out, "dirty", fields_out(r, node.dirty_fields));
    detail::model_put(out,
                      "old",
                      node.original_values
                          ? record_out(r, node.original_values->fields, before, after)
                          : Json(r));
    detail::model_put(out, "comment", node.comment ? Json(r, *node.comment) : Json(r));
    auto rels = detail::model_json(r);
    for (const auto& [name, children] : node.relations) {
        Json::Array list(r);
        for (const auto& child : children) {
            list.emplace_back(node_out(r, child, before, after));
        }
        detail::model_put(rels, name, list);
    }
    detail::model_put(out, "relations", rels);
    return out;
}

static Json trace_out(Memory r, std::span<const TraceNode> trace) {
    Json::Array out(r);
    for (const auto& node : trace) {
        auto t = detail::model_json(r);
        detail::model_put(t, "entity", node.entity_type);
        detail::model_put(t, "comment", node.comment);
        detail::model_put(t, "id", node.entity_id ? Json(r, *node.entity_id) : Json(r));
        detail::model_put(t, "kind", std::uint8_t(node.kind));
        out.emplace_back(std::move(t));
    }
    return Json(r, out);
}

static Json
plan_out(Memory r, const GraphMutationPlan& plan, std::int64_t before, std::int64_t after) {
    auto out = detail::model_json(r);
    detail::model_put(out, "root", node_out(r, *plan.planned_root, before, after));
    detail::model_put(out, "next", plan.next_item_index);
    Json::Array items(r), batches(r), visited(r);
    for (const auto& item : plan.items) {
        auto e = detail::model_json(r);
        detail::model_put(e, "entity", item.entity);
        detail::model_put(e, "kind", std::uint8_t(item.kind));
        detail::model_put(e, "index", item.item_index);
        detail::model_put(e, "values", record_out(r, item.values.fields, before, after));
        Json::Array fields(r);
        for (const auto& name : item.update_fields) {
            fields.emplace_back(name);
        }
        detail::model_put(e, "fields", fields);
        detail::model_put(e,
                          "old",
                          item.old_values ? record_out(r, item.old_values->fields, before, after)
                                          : Json(r));
        detail::model_put(e,
                          "trace",
                          item.scope_token ? trace_out(r, item.scope_token->recover_trace_chain(r))
                                           : Json(r, Json::Array(r)));
        Json::Array indices(r);
        for (auto* t = item.scope_token; t; t = t->parent()) {
            indices.emplace_back(t->node_index);
        }
        detail::model_put(e, "token_indices", indices);
        items.emplace_back(std::move(e));
    }
    for (const auto& batch : plan.batches) {
        auto b = detail::model_json(r);
        detail::model_put(b, "entity", batch.entity);
        detail::model_put(b, "kind", std::uint8_t(batch.kind));
        Json::Array fields(r), indices(r);
        for (const auto& name : batch.update_fields) {
            fields.emplace_back(name);
        }
        for (const auto& item : batch.items) {
            indices.emplace_back(item.item_index);
        }
        detail::model_put(b, "fields", fields);
        detail::model_put(b, "indices", indices);
        batches.emplace_back(std::move(b));
    }
    std::pmr::set<std::pair<String, String>> sorted(
        plan.visited_nodes.begin(), plan.visited_nodes.end(), r);
    for (const auto& [entity, id] : sorted) {
        Json::Array pair(r);
        pair.emplace_back(entity);
        pair.emplace_back(id);
        visited.emplace_back(std::move(pair));
    }
    detail::model_put(out, "items", items);
    detail::model_put(out, "batches", batches);
    detail::model_put(out, "visited", visited);
    return out;
}

struct Generator final : InternalIdGenerator {
    bool fail;
    mutable std::uint64_t next = 100;

    explicit Generator(bool fail) : fail(fail) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        if (fail) {
            return Result<std::uint64_t, RuntimeError>::err(
                r, RuntimeError::id_generation(r, "ID failed"));
        }
        return Result<std::uint64_t, RuntimeError>::ok(r, next++);
    }
};

static Json evaluate(Memory target, const Json& input) {
    GlobalContext global;
    auto user = global.make_user_context();
    user->disable_sql_log();
    user->set_user_identifier("fixture");
    auto r = user->memory_resource();
    auto& metadata = user->make<InMemoryMetadataStore>();
    for (const auto& info : std::get<Json::Array>(field(input, "descriptors").data)) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), text(field(info, "name")));
        auto id = text(field(info, "id"));
        if (!id.empty()) {
            descriptor.property(id, DataType::U64).id();
        }
        if (flag(info, "version")) {
            descriptor.property("version", DataType::I64).version();
        }
        descriptor.property("name", DataType::Text);
        if (flag(info, "timestamps")) {
            descriptor.property("create_time", DataType::Timestamp);
            descriptor.property("update_time", DataType::Timestamp);
        }
        for (const auto& relation : std::get<Json::Array>(field(info, "relations").data)) {
            descriptor.relation(text(field(relation, "name")), text(field(relation, "target")))
                .local("id")
                .foreign("parent_id")
                .to_many();
        }
        metadata.register_entity(std::move(descriptor));
    }
    user->set_metadata(*user, metadata);
    auto& generator = user->make_resource<Generator>(flag(input, "id_fail"));
    user->set_internal_id_generator(*user, generator);
    auto& executor = user->make<graph_plan_fixture::Executor>();
    executor.failure_entity = text(field(input, "fail_entity"));
    if (flag(input, "exists")) {
        executor.rows.emplace_back();
        executor.rows.back().emplace(String("id", r), Value(r, std::uint64_t(999)));
    }
    auto& service = EntityDataService<graph_plan_fixture::Executor>::for_executor(
        *user, text(field(input, "service")), *user, executor);
    auto before = Timestamp::now().as_millis();
    auto& operation = service.plan_graph(node(r, field(input, "node")));
    const auto& result = operation.wait();
    auto after = Timestamp::now().as_millis();
    auto out = detail::model_json(r);
    if (result) {
        detail::model_put(out, "plan", plan_out(r, result.value(), before, after));
    } else {
        detail::model_put(out, "error", result.error().to_string(r));
    }
    Json::Array requests(r);
    for (const auto& request : executor.requests) {
        auto e = detail::model_json(r);
        detail::model_put(e, "entity", request.query.entity);
        detail::model_put(e,
                          "filter",
                          request.query.predicate
                              ? Json(r, detail::debug_expr(r, *request.query.predicate))
                              : Json(r));
        detail::model_put(e, "trace", trace_out(r, request.query.trace_chain));
        detail::model_put(e, "request_trace", trace_out(r, request.trace_chain));
        detail::model_put(e,
                          "comment",
                          request.query.comment_text ? Json(r, *request.query.comment_text)
                                                     : Json(r));
        requests.emplace_back(std::move(e));
    }
    detail::model_put(out, "requests", requests);
    detail::model_put(out, "next_id", generator.next);
    return Json(target, out);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena out(4096, std::pmr::new_delete_resource());
        auto input = Json::parse(&out, line);
        std::cout << evaluate(&out, *input).dump(&out) << '\n';
    }
}
