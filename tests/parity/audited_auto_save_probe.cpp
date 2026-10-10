#include "../support/audited_save_fixture.hpp"
#include "../support/graph_saver_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
#include <teaql/core/detail/query_debug.hpp>
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
        auto* timestamp = std::get_if<Timestamp>(&v.data);
        detail::model_put(out,
                          k,
                          timestamp && timestamp->as_millis() > 1'000'000
                              ? String("Timestamp(<now>)", r)
                              : detail::debug_value(r, v));
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

static Json trace_out(Memory r, std::span<const TraceNode> nodes) {
    Json::Array out(r);
    for (const auto& n : nodes) {
        auto e = detail::model_json(r);
        detail::model_put(e, "kind", std::uint8_t(n.kind));
        detail::model_put(e, "entity", n.entity_type);
        detail::model_put(e, "comment", n.comment);
        out.emplace_back(std::move(e));
    }
    return Json(r, out);
}

static Json request_out(Memory r, const MutationRequest& request) {
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
                detail::model_put(out, "trace", trace_out(r, c.trace_chain));
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

static Json event_out(Memory r, const RawAuditEvent& e) {
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
    detail::model_put(out, "trace", trace_out(r, e.trace_chain));
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

struct Id final : InternalIdGenerator {
    State& state;
    mutable std::uint64_t next = 100;

    explicit Id(State& state) : state(state) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r,
                                                    std::string_view entity) const override {
        String note("id:", r);
        note += entity;
        state.note(note);
        return Result<std::uint64_t, RuntimeError>::ok(r, next++);
    }
};

static EntityKey key(Memory r, const Json& input) {
    return EntityKey(std::allocator_arg,
                     Allocator(r),
                     text(field(input, "entity")),
                     value(r, field(input, "id")));
}

static std::optional<FieldSet> fields(Memory r, const Json& input) {
    if (!std::holds_alternative<Json::Array>(input.data)) {
        return {};
    }
    FieldSet result(r);
    for (const auto& name : std::get<Json::Array>(input.data)) {
        result.emplace(text(name));
    }
    return result;
}

static Json evaluate(Memory target, const Json& input) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->set_user_identifier("fixture");
    user->set_trace_id("fixture");
    user->disable_sql_log();
    auto& module = user->make<RuntimeModule>();
    for (const auto& info : std::get<Json::Array>(field(input, "descriptors").data)) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), text(field(info, "entity")));
        auto id = text(field(info, "id"));
        if (!id.empty()) {
            d.property(id, DataType::U64).id();
        }
        auto version = text(field(info, "version"));
        if (!version.empty()) {
            d.property(version, DataType::I64).version();
        }
        d.property("name", DataType::Text);
        if (flag(info, "required")) {
            d.properties.back().not_null();
        }
        for (const auto& rel : std::get<Json::Array>(field(info, "relations").data)) {
            RelationDescriptor relation(std::allocator_arg,
                                        Allocator(r),
                                        text(field(rel, "name")),
                                        text(field(rel, "target")));
            relation.local_key = text(field(rel, "local"));
            relation.foreign_key = text(field(rel, "foreign"));
            relation.many = flag(rel, "many");
            d.relations.emplace_back(std::move(relation));
        }
        module.descriptor(std::move(d));
    }
    auto& state = user->make<State>();
    state.allow_reads = true;
    state.read_failure_index =
        detail::model_integer<int>(field(input, "read_fail_at"), "read_fail_at");
    state.failure = detail::model_integer<int>(field(input, "fail"), "fail");
    state.event_failure = detail::model_integer<int>(field(input, "event_fail"), "event_fail");
    for (const auto& count : std::get<Json::Array>(field(input, "counts").data)) {
        state.affected.push_back(detail::model_integer<std::uint64_t>(count, "count"));
    }
    for (const auto& row : std::get<Json::Array>(field(input, "rows").data)) {
        state.rows.emplace_back(record(r, row));
    }
    for (const auto& rows : std::get<Json::Array>(field(input, "read_batches").data)) {
        state.read_batches.emplace_back();
        for (const auto& row : std::get<Json::Array>(rows.data)) {
            state.read_batches.back().emplace_back(record(r, row));
        }
    }
    auto& sink = user->make_resource<Sink>(state);
    module.event_sink(*user, sink);
    module.apply_to(*user);
    auto& ids = user->make_resource<Id>(state);
    user->set_internal_id_generator(*user, ids);
    if (text(field(input, "checker")) != "none") {
        auto& checker =
            user->make_resource<audited_fixture::Checker>(state, text(field(input, "checker")));
        auto& registry = user->make<InMemoryCheckerRegistry>();
        registry.register_checker(*user, checker);
        user->set_checker_registry(*user, registry);
    }
    auto& controls = user->make<saver_fixture::Controls>();
    controls.fail_begin = flag(input, "begin_fail");
    controls.fail_commit = flag(input, "commit_fail");
    controls.fail_rollback = flag(input, "rollback_fail");
    auto executor = user->make_resource_ref<saver_fixture::Ambient>(state, controls);
    for (const auto& row : std::get<Json::Array>(field(input, "ambient_rows").data)) {
        user->get(executor).ambient_rows.emplace_back(record(r, row));
    }
    if (flag(input, "registered")) {
        user->register_executor(executor);
    }
    auto& root = user->make<EntityRuntimeState>();
    for (const auto& seed : std::get<Json::Array>(field(input, "seeds").data)) {
        auto identity = key(r, seed);
        if (flag(seed, "new")) {
            root.mark_as_new(identity);
        }
        if (!std::holds_alternative<std::monostate>(field(seed, "version").data)) {
            root.set_original_version(
                identity, detail::model_integer<std::int64_t>(field(seed, "version"), "version"));
        }
        for (const auto& [name, v] : record(r, field(seed, "values"))) {
            root.set(identity, name, v);
        }
        if (flag(seed, "delete")) {
            root.mark_as_delete(identity);
        }
    }
    audited_fixture::Model model(std::allocator_arg, Allocator(r));
    model.values.fields = record(r, field(input, "values"));
    if (flag(input, "runtime")) {
        model.runtime.emplace(std::allocator_arg, Allocator(r), root);
    }
    model.fresh = flag(input, "fresh");
    model.removed = flag(input, "removed");
    model.loaded = fields(r, field(input, "loaded"));
    model.dirty = fields(r, field(input, "dirty"));
    if (std::holds_alternative<Json::Object>(field(input, "old").data)) {
        model.original.emplace(r);
        model.original->fields = record(r, field(input, "old"));
    }
    Audited<audited_fixture::Model> audited(
        std::allocator_arg, Allocator(r), std::move(model), text(field(input, "reason")));
    auto& future = text(field(input, "mode")) == "plain"
                       ? AuditedSaveExt<audited_fixture::Model>::save(std::move(audited), *user)
                       : save_audited_ledger_entity(std::move(audited), *user);
    auto& result = future.wait();
    auto outcome = detail::model_json(r);
    if (result) {
        detail::model_put(outcome, "values", record_out(r, result.value().values.fields));

        detail::model_put(outcome, "restored", result.value().restored);
    } else {
        auto message = result.error().to_string(r);
        if (!flag(input, "registered") &&
            message.starts_with("graph write error: no DynGraphSaver registered")) {
            message = "graph write error: no DynGraphSaver registered (<type>)";
        }
        detail::model_put(outcome, "error", message);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "outcome", outcome);
    detail::model_put(out, "abandoned", std::uint64_t(controls.abandoned));
    Json::Array notes(r), requests(r), queries(r), events(r), changes(r), fresh(r), deleted(r),
        evidence(r);
    for (const auto& note : state.notes) {
        notes.emplace_back(note);
    }
    for (const auto& request : state.requests) {
        requests.emplace_back(request_out(r, request));
    }
    for (const auto& event : state.events) {
        events.emplace_back(event_out(r, event));
    }
    for (const auto& query : state.queries) {
        auto q = detail::model_json(r);
        detail::model_put(q, "entity", query.query.entity);
        detail::model_put(q,
                          "filter",
                          query.query.predicate
                              ? Json(r, detail::debug_expr(r, *query.query.predicate))
                              : Json(r));
        detail::model_put(q, "trace", trace_out(r, query.query.trace_chain));
        detail::model_put(
            q, "comment", query.query.comment_text ? Json(r, *query.query.comment_text) : Json(r));
        queries.emplace_back(std::move(q));
    }
    auto current = root.current_change_set();
    for (const auto& [key, v] : current.changes()) {
        auto change = detail::model_json(r);
        detail::model_put(change, "key", key.debug(r));
        detail::model_put(change, "values", record_out(r, v.fields));
        changes.emplace_back(std::move(change));
    }
    for (const auto& key : root.new_keys()) {
        fresh.emplace_back(Json(r, key.debug(r)));
    }
    for (const auto& key : root.deleted_keys()) {
        deleted.emplace_back(Json(r, key.debug(r)));
    }
    for (const auto& entry : user->last_fix_evidence()) {
        auto e = detail::model_json(r);
        detail::model_put(e, "entity", entry.entity_type);
        detail::model_put(e, "path", entry.model_path);
        detail::model_put(e, "source", fix_evidence_source_name(entry.source));
        detail::model_put(e, "label", entry.source_label);
        evidence.emplace_back(std::move(e));
    }
    detail::model_put(out, "notes", notes);
    detail::model_put(out, "requests", requests);
    detail::model_put(out, "queries", queries);
    detail::model_put(out, "events", events);
    detail::model_put(out, "changes", changes);
    detail::model_put(out, "new", fresh);
    detail::model_put(out, "deleted", deleted);
    detail::model_put(out, "evidence", evidence);
    return Json(target, out);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena output(4096, std::pmr::new_delete_resource());
        auto input = Json::parse(&output, line);
        std::cout << evaluate(&output, *input).dump(&output) << '\n';
    }
}
