#include "../support/runtime_mutation_fixture.hpp"
#include "../support/graph_save_adapter_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
#include <teaql/core/detail/query_debug.hpp>
#include <teaql/runtime/graph_debug.hpp>
using namespace teaql;
using namespace mutation_fixture;
using Access = detail::EntityDataServiceAccess;

static bool flag(const Json& j, std::string_view name) {
    return std::get<bool>(field(j, name).data);
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

static TraceChain trace(Memory r, const Json& j) {
    TraceChain out(r);
    constexpr TraceKind kinds[]{TraceKind::AuditReason, TraceKind::Entity, TraceKind::Comment};
    for (const auto& n : std::get<Json::Array>(j.data)) {
        out.emplace_back(kinds[std::get<std::uint64_t>(field(n, "kind").data)],
                         text(field(n, "entity")),
                         text(field(n, "comment")));
    }
    return out;
}

static Json trace_out(Memory r, std::span<const TraceNode> nodes, bool include_ids = false) {
    Json::Array out(r);
    for (const auto& n : nodes) {
        auto e = detail::model_json(r);
        detail::model_put(e, "kind", std::uint8_t(n.kind));
        detail::model_put(e, "entity", n.entity_type);
        detail::model_put(e, "comment", n.comment);
        if (include_ids) {
            detail::model_put(e, "entity_id", n.entity_id ? Json(r, *n.entity_id) : Json(r));
        }
        out.emplace_back(std::move(e));
    }
    return Json(r, out);
}

static Json request_out(Memory r, const MutationRequest& request, bool include_ids = false) {
    return std::visit(
        [&](const auto& c) {
            auto out = detail::model_json(r);
            using C = std::decay_t<decltype(c)>;
            if constexpr (!std::same_as<C, MutationRequest::Batch>) {
                constexpr auto name = std::same_as<C, InsertCommand>   ? "insert"
                                      : std::same_as<C, UpdateCommand> ? "update"
                                      : std::same_as<C, DeleteCommand> ? "delete"
                                                                       : "recover";
                detail::model_put(out, "kind", name);
                detail::model_put(out, "entity", c.entity);
                detail::model_put(out, "trace", trace_out(r, c.trace_chain, include_ids));
                if constexpr (std::same_as<C, InsertCommand> || std::same_as<C, UpdateCommand>) {
                    detail::model_put(out, "values", record_out(r, c.values.fields));
                }
                if constexpr (!std::same_as<C, InsertCommand>) {
                    detail::model_put(out, "id", detail::debug_value(r, c.id));
                    if constexpr (std::same_as<C, RecoverCommand>) {
                        detail::model_put(out, "version", c.expected_version);
                    } else {
                        detail::model_put(out,
                                          "version",
                                          c.expected_version_value
                                              ? Json(r, *c.expected_version_value)
                                              : Json(r));
                    }
                }
                if constexpr (std::same_as<C, UpdateCommand>) {
                    detail::model_put(
                        out, "old", c.old_values ? record_out(r, c.old_values->fields) : Json(r));
                }
            }
            return out;
        },
        request.data);
}

static Json event_out(Memory r, const RawAuditEvent& e, bool include_ids = false) {
    auto out = detail::model_json(r);
    constexpr std::string_view names[]{"Created",
                                       "Updated",
                                       "Deleted",
                                       "Recovered",
                                       "SchemaCreated",
                                       "SchemaVerified",
                                       "FieldAdded",
                                       "DataSeeded"};
    detail::model_put(out, "kind", names[std::size_t(e.kind)]);
    detail::model_put(out, "entity", e.entity);
    detail::model_put(out, "values", record_out(r, e.values));
    detail::model_put(out, "old", e.old_values ? record_out(r, *e.old_values) : Json(r));
    detail::model_put(out, "new", e.new_values ? record_out(r, *e.new_values) : Json(r));
    Json::Array fields(r);
    for (const auto& name : e.updated_fields) {
        fields.emplace_back(name);
    }
    detail::model_put(out, "fields", fields);
    detail::model_put(out, "trace", trace_out(r, e.trace_chain, include_ids));
    if (e.mutation_governance) {
        auto g = detail::model_json(r);
        detail::model_put(g,
                          "source",
                          e.mutation_governance->source == MutationPolicySource::Customer
                              ? "Customer"
                              : "GeneratedDefault");
        detail::model_put(g, "request", e.mutation_governance->request_key);
        Json::Array warnings(r);
        for (const auto& w : e.mutation_governance->warning_codes) {
            warnings.emplace_back(w);
        }
        detail::model_put(g, "warnings", warnings);
        detail::model_put(out, "governance", g);
    } else {
        detail::model_put(out, "governance", Json(r));
    }
    return out;
}

static GraphNode node(Memory r, const Json& j) {
    GraphNode n(std::allocator_arg, Allocator(r), text(field(j, "entity")));
    n.operation = GraphOperation(std::get<std::uint64_t>(field(j, "operation").data));
    n.values.fields = record(r, field(j, "values"));
    if (std::holds_alternative<String>(field(j, "comment").data)) {
        n.set_comment(text(field(j, "comment")));
    }
    if (auto* dirty = optional(j, "dirty");
        dirty && std::holds_alternative<Json::Array>(dirty->data)) {
        n.dirty_fields.emplace(r);
        for (const auto& entry : std::get<Json::Array>(dirty->data)) {
            n.dirty_fields->emplace(text(entry));
        }
    }
    if (auto* old = optional(j, "old"); old && std::holds_alternative<Json::Object>(old->data)) {
        n.original_values.emplace(r);
        n.original_values->fields = record(r, *old);
    }
    for (const auto& [name, children] : std::get<Json::Object>(field(j, "relations").data)) {
        auto& list = n.relations.try_emplace(String(name, r)).first->second;
        for (const auto& child : std::get<Json::Array>(children.data)) {
            list.emplace_back(node(r, child));
        }
    }
    return n;
}

static Json node_out(Memory r, const GraphNode& n) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", n.entity);
    detail::model_put(out, "values", record_out(r, n.values.fields));
    detail::model_put(out, "operation", std::uint8_t(n.operation));
    detail::model_put(out, "comment", n.comment ? Json(r, *n.comment) : Json(r));
    auto children = detail::model_json(r);
    for (const auto& [name, list] : n.relations) {
        Json::Array values(r);
        for (const auto& child : list) {
            values.emplace_back(node_out(r, child));
        }
        detail::model_put(children, name, values);
    }
    detail::model_put(out, "relations", children);
    return out;
}

static GraphMutationPlan plan(Memory r, const Json& j) {
    GraphMutationPlan p(std::allocator_arg, Allocator(r));
    if (std::holds_alternative<Json::Object>(field(j, "root").data)) {
        p.planned_root.emplace(std::allocator_arg, Allocator(r), node(r, field(j, "root")));
    }
    for (const auto& item : std::get<Json::Array>(field(j, "items").data)) {
        const TraceScopeToken* token = nullptr;
        for (const auto& frame : std::get<Json::Array>(field(item, "token").data)) {
            TraceNode t(std::allocator_arg,
                        Allocator(r),
                        TraceKind::AuditReason,
                        text(field(frame, "entity")),
                        text(field(frame, "comment")));
            token = &detail::resource_arena(r).make_payload<TraceScopeToken>(
                token, t, detail::model_integer<std::uint64_t>(field(frame, "index"), "index"));
        }
        MutationValues values(r);
        values.fields = record(r, field(item, "values"));
        Vector<String> fields(r);
        for (const auto& name : std::get<Json::Array>(field(item, "fields").data)) {
            fields.emplace_back(text(name));
        }
        std::optional<EntitySnapshot> old;
        if (std::holds_alternative<Json::Object>(field(item, "old").data)) {
            old.emplace(r);
            old->fields = record(r, field(item, "old"));
        }
        p.push(text(field(item, "entity")),
               GraphMutationKind(std::get<std::uint64_t>(field(item, "kind").data)),
               values,
               fields,
               token,
               old);
    }
    p.rebuild_batches();
    return p;
}

struct Id final : InternalIdGenerator {
    mutable std::uint64_t next = 100;

    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        return Result<std::uint64_t, RuntimeError>::ok(r, next++);
    }
};

static core::EntityGraphNode core_node(Memory r, const Json& input) {
    MutationValues values(r);
    values.fields = record(r, field(input, "values"));
    core::EntityGraphNode out(
        std::allocator_arg, Allocator(r), text(field(input, "entity")), std::move(values));
    out.operation = core::EntityGraphOperation(
        detail::model_integer<unsigned>(field(input, "operation"), "core operation"));
    if (auto* comment = optional(input, "comment");
        comment && std::holds_alternative<String>(comment->data)) {
        out.comment.emplace(text(*comment), r);
    }
    for (const auto& child : std::get<Json::Array>(field(input, "children").data)) {
        out.children.emplace_back(String(text(field(child, "name")), r),
                                  core_node(r, field(child, "node")));
    }
    return out;
}

static auto& adapter_work(UserContext& user,
                          const EntityDataService<Executor>& service,
                          State& state,
                          const Json& input) {
    auto r = user.memory_resource();
    auto mode = text(field(input, "mode"));
    if (mode == "adapter_core") {
        return Access::save_entity_graph_from_internal(
            service,
            core::EntityGraph(
                std::allocator_arg, Allocator(r), core_node(r, field(input, "core"))));
    }
    auto& model = user.make<adapter_fixture::Model>(state);
    model.values.fields = record(r, field(field(input, "root"), "values"));
    model.removed = std::get<bool>(field(input, "deleted").data);
    if (auto* dirty = optional(input, "dirty");
        dirty && std::holds_alternative<Json::Array>(dirty->data)) {
        model.dirty.emplace(r);
        for (const auto& field : std::get<Json::Array>(dirty->data)) {
            model.dirty->emplace(text(field));
        }
    }
    if (auto* old = optional(input, "old");
        old && std::holds_alternative<Json::Object>(old->data)) {
        model.original.emplace(r);
        model.original->fields = record(r, *old);
    }
    if (auto* comment = optional(field(input, "root"), "comment");
        comment && std::holds_alternative<String>(comment->data)) {
        model.comment.emplace(text(*comment), r);
    }
    auto status = EntityStatus(detail::model_integer<unsigned>(field(input, "status"), "status"));
    auto comment = text(field(input, "annotation"));
    if (mode == "adapter_graph") {
        return Access::save_entity_graph_internal(service, std::move(model));
    }
    if (mode == "adapter_status") {
        return Access::save_entity_internal(service, std::move(model), status);
    }
    if (mode == "adapter_status_comment") {
        return Access::save_entity_with_comment_internal(
            service, std::move(model), status, comment);
    }
    if (mode == "adapter_comment") {
        return Access::save_entity_graph_with_comment_internal(service, std::move(model), comment);
    }
    if (mode == "adapter_create") {
        return Access::create_entity_graph_with_comment_internal(
            service, std::move(model), comment);
    }
    throw std::logic_error("unknown adapter mode");
}

static Json
node_command(UserContext& user, const EntityDataService<Executor>& service, const Json& input) {
    auto r = user.memory_resource();
    const bool include_ids = optional(input, "bound_intent") != nullptr;
    auto mode = text(field(input, "mode"));
    auto graph = node(r, field(input, "root"));
    auto out = detail::model_json(r);
    auto lineage = trace(r, field(input, "trace"));
    const ScopedCommentNode* parent = nullptr;
    for (const auto& frame : lineage) {
        parent = &user.make<ScopedCommentNode>(parent, frame);
    }
    if (mode.starts_with("recursive_")) {
        auto& work = mode == "recursive_insert"
                         ? Access::insert_graph_node_scoped(service, std::move(graph), parent)
                         : Access::upsert_graph_node_scoped(service, std::move(graph), parent);
        auto& result = work.wait();
        if (result) {
            detail::model_put(out, "raw", debug_string(r, result.value()));
        } else {
            detail::model_put(out, "error", result.error().to_string(r));
        }
    } else if (mode == "node_reference") {
        auto& result = Access::validate_reference_node(service, std::move(graph), lineage).wait();
        if (result) {
            detail::model_put(out, "raw", debug_string(r, result.value()));
        } else {
            detail::model_put(out, "error", result.error().to_string(r));
        }
    } else if (mode == "node_remove") {
        auto& result = Access::validate_remove_node(service, graph, lineage).wait();
        if (result) {
            detail::model_put(out, "ok", true);
        } else {
            detail::model_put(out, "error", result.error().to_string(r));
        }
    } else if (mode == "node_delete") {
        auto& result = Access::delete_graph_node(service, graph, parent).wait();
        if (result) {
            detail::model_put(out, "affected", result.value());
        } else {
            detail::model_put(out, "error", result.error().to_string(r));
        }
    } else if (mode == "node_update") {
        auto descriptor = user.require_entity(graph.entity);
        if (!descriptor) {
            detail::model_put(out, "error", descriptor.error().to_string(r));
        } else {
            auto& desc = descriptor.value().get();
            auto* property = desc.id_property();
            if (!property) {
                property = &desc.properties[0];
            }
            auto result = Access::graph_update_command(
                service, graph, desc, *property, value(r, field(input, "id")));
            if (result) {
                MutationRequest request(
                    std::allocator_arg, Allocator(r), result.value(), "graph execution command");
                detail::model_put(out, "command", request_out(r, request, include_ids));
            } else {
                detail::model_put(out, "error", result.error().to_string(r));
            }
        }
        detail::model_put(out, "node", debug_string(r, graph));
    } else {
        throw std::logic_error("unknown graph node command");
    }
    return out;
}

struct NodeChecker final : Checker {
    State& state;
    String action;

    NodeChecker(State& state, std::string_view action)
        : state(state), action(action, state.resource) {}

    std::string_view entity() const override {
        return "Order";
    }

    void check_and_fix(const UserContext&,
                       EntityValues& values,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        auto r = values.memory_resource();
        String note("checker.", r);
        note += check_object_status_name(check_object_status(values));
        state.note(note);
        if (action == "fill" || action == "error") {
            values.fields.insert_or_assign(String("name", r),
                                           Value(r, "fixed allocated checker payload"));
        }
        if (action == "erase") {
            values.fields.erase(String("name", r));
        }
        if (action == "error") {
            results.emplace_back(
                CheckResult::required(r, detail::clone_in(r, location).member("name")));
        }
    }
};

static Json evaluate(Memory target, const Json& j) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->set_user_identifier("fixture");
    user->set_trace_id("fixture");
    user->disable_sql_log();
    auto& module = user->make<RuntimeModule>();
    for (const auto& info : std::get<Json::Array>(field(j, "descriptors").data)) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), text(field(info, "entity")));
        auto id = text(field(info, "id"));
        if (!id.empty()) {
            d.property(id, DataType::U64).id();
        }
        d.property("version", DataType::I64).version();
        d.property("name", DataType::Text);
        for (const auto& relation : std::get<Json::Array>(field(info, "relations").data)) {
            auto& mapping =
                d.relation(text(field(relation, "name")), text(field(relation, "target")));
            auto* many = optional(relation, "many");
            mapping.many = !many || std::get<bool>(many->data);
            if (auto* attach = optional(relation, "attach")) {
                mapping.attach = std::get<bool>(attach->data);
            }
            if (auto* local = optional(relation, "local")) {
                mapping.local(text(*local));
            }
            if (auto* foreign = optional(relation, "foreign")) {
                mapping.foreign(text(*foreign));
            }
        }
        module.descriptor(std::move(d));
    }
    auto& s = user->make<State>();
    s.allow_reads = true;
    s.read_failure = flag(j, "read_fail");
    s.failure = detail::model_integer<int>(field(j, "fail"), "fail");
    s.event_failure = detail::model_integer<int>(field(j, "event_fail"), "event_fail");
    for (const auto& count : std::get<Json::Array>(field(j, "counts").data)) {
        s.affected.push_back(detail::model_integer<std::uint64_t>(count, "count"));
    }
    if (auto* batches = optional(j, "read_batches")) {
        for (const auto& batch : std::get<Json::Array>(batches->data)) {
            auto& records = s.read_batches.emplace_back();
            for (const auto& row : std::get<Json::Array>(batch.data)) {
                records.emplace_back(record(r, row));
            }
        }
    }
    for (const auto& row : std::get<Json::Array>(field(j, "rows").data)) {
        s.rows.emplace_back(record(r, row));
    }
    auto& sink = user->make_resource<Sink>(s);
    module.event_sink(*user, sink);
    module.apply_to(*user);
    if (auto* checker = optional(j, "checker"); checker && text(*checker) != "none") {
        auto& registry = user->make<InMemoryCheckerRegistry>();
        auto& callback = user->make_resource<NodeChecker>(s, text(*checker));
        registry.register_checker(*user, callback);
        user->set_checker_registry(*user, registry);
    }

    auto& ids = user->make_resource<Id>();
    user->set_internal_id_generator(*user, ids);
    auto& executor = user->make_resource<Executor>(s);
    auto& service = EntityDataService<Executor>::for_executor(
        *user, text(field(j, "service")), *user, executor);
    const bool include_ids = optional(j, "bound_intent") != nullptr;
    if (include_ids) {
        auto binding = Access::bind_mutation_intent(service, text(field(j, "bound_intent")));
        if (!binding) {
            throw std::runtime_error("Invalid observer mutation intent");
        }
    }
    auto outcome = detail::model_json(r);
    auto mode = text(field(j, "mode"));
    if (mode.starts_with("node_") || mode.starts_with("recursive_")) {
        outcome = node_command(*user, service, j);
    } else if (mode == "fetch") {
        auto lineage = trace(r, field(j, "trace"));
        auto& result = Access::fetch_graph_current_row_internal(service,
                                                                text(field(j, "entity")),
                                                                text(field(j, "property")),
                                                                value(r, field(j, "id")),
                                                                lineage)
                           .wait();
        if (result) {
            detail::model_put(outcome,
                              "row",
                              result.value() ? record_out(r, result.value()->into_map(r))
                                             : Json(r));
        } else {
            detail::model_put(outcome, "error", result.error().to_string(r));
        }
    } else {
        auto& work = mode.starts_with("adapter_") ? adapter_work(*user, service, s, j)
                     : mode == "save"
                         ? Access::save_graph_internal(service, node(r, field(j, "root")))
                         : Access::execute_graph_plan_internal(service, plan(r, j));
        auto& result = work.wait();
        if (result) {
            detail::model_put(outcome, "root", node_out(r, result.value()));
            if (mode.starts_with("adapter_")) {
                detail::model_put(outcome, "raw", debug_string(r, result.value()));
            }
        } else {
            detail::model_put(outcome, "error", result.error().to_string(r));
        }
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "outcome", outcome);
    Json::Array notes(r), requests(r), queries(r), events(r);
    for (const auto& n : s.notes) {
        notes.emplace_back(n);
    }
    for (const auto& request : s.requests) {
        requests.emplace_back(request_out(r, request, include_ids));
    }
    for (const auto& event : s.events) {
        events.emplace_back(event_out(r, event, include_ids));
    }
    for (const auto& q : s.queries) {
        auto value = detail::model_json(r);
        detail::model_put(value, "entity", q.query.entity);
        detail::model_put(value,
                          "filter",
                          q.query.predicate ? Json(r, detail::debug_expr(r, *q.query.predicate))
                                            : Json(r));
        detail::model_put(value, "trace", trace_out(r, q.query.trace_chain, include_ids));
        detail::model_put(
            value, "comment", q.query.comment_text ? Json(r, *q.query.comment_text) : Json(r));
        queries.emplace_back(std::move(value));
    }
    detail::model_put(out, "notes", notes);
    detail::model_put(out, "requests", requests);
    detail::model_put(out, "queries", queries);
    detail::model_put(out, "events", events);
    return Json(target, out);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena out(4096, std::pmr::new_delete_resource());
        auto j = Json::parse(&out, line);
        std::cout << evaluate(&out, *j).dump(&out) << '\n';
    }
    return 0;
}
