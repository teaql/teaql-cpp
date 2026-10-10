#include "../support/runtime_mutation_fixture.hpp"
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

static EntityKey key(Memory r, const Json& input) {
    return EntityKey(std::allocator_arg,
                     Allocator(r),
                     text(field(input, "entity")),
                     value(r, field(input, "id")));
}

struct Id final : InternalIdGenerator {
    State& state;
    int fail;
    mutable std::uint64_t next = 100;

    Id(State& state, int fail) : state(state), fail(fail) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r,
                                                    std::string_view entity) const override {
        String note("id:", r);
        note += entity;
        state.note(note);
        if (fail >= 0 && next == 100 + std::uint64_t(fail)) {
            return Result<std::uint64_t, RuntimeError>::err(
                r, RuntimeError::id_generation(r, "id failed"));
        }
        return Result<std::uint64_t, RuntimeError>::ok(r, next++);
    }
};

static Json evaluate(Memory target, const Json& input) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->set_user_identifier("fixture");
    user->set_trace_id("fixture");
    user->disable_sql_log();
    auto& module = user->make<RuntimeModule>();
    for (const auto& info : std::get<Json::Array>(field(input, "descriptors").data)) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), text(field(info, "entity")));
        auto id = text(field(info, "id"));
        if (!id.empty()) {
            descriptor.property(id, DataType::U64).id();
        }
        auto version = text(field(info, "version"));
        if (!version.empty()) {
            descriptor.property(version, DataType::I64).version();
        }
        descriptor.property("name", DataType::Text);
        if (flag(info, "required")) {
            descriptor.properties.back().not_null();
        }
        if (flag(info, "timestamps")) {
            descriptor.property("create_time", DataType::Timestamp);
            descriptor.property("update_time", DataType::Timestamp);
        }
        for (const auto& input : std::get<Json::Array>(field(info, "relations").data)) {
            RelationDescriptor relation(std::allocator_arg,
                                        Allocator(r),
                                        text(field(input, "name")),
                                        text(field(input, "target")));
            relation.local_key = text(field(input, "local"));
            relation.foreign_key = text(field(input, "foreign"));
            relation.many = flag(input, "many");
            descriptor.relations.emplace_back(std::move(relation));
        }
        module.descriptor(std::move(descriptor));
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
    auto& ids = user->make_resource<Id>(
        state, detail::model_integer<int>(field(input, "id_fail"), "id_fail"));
    user->set_internal_id_generator(*user, ids);
    auto& executor = user->make_resource<Executor>(state);
    auto& service = EntityDataService<Executor>::for_executor(
        *user, text(field(input, "service")), *user, executor);
    auto& root = user->make<EntityRuntimeState>();
    if (std::holds_alternative<Json::Object>(field(input, "loaded").data)) {
        auto row = record(r, field(input, "loaded"));
        root.set_original_compact_row(text(field(input, "service")),
                                      CompactRow(std::allocator_arg, Allocator(r), row));
    }
    if (std::holds_alternative<String>(field(input, "comment").data)) {
        root.set_comment(text(field(input, "comment")));
    }
    auto& locations = user->make<detail::LedgerObjectLocations>();
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
        auto location = ObjectLocation::root(r);
        for (const auto& part : std::get<Json::Array>(field(seed, "path").data)) {
            if (auto name = std::get_if<String>(&part.data)) {
                location.member(*name);
            } else {
                location.element(std::get<std::uint64_t>(part.data));
            }
        }
        locations.insert_or_assign(detail::clone_in(r, identity), std::move(location));
    }
    if (flag(input, "composition_conflict")) {
        EntityKey identity(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
        root.set_original_version(identity, 1);
        auto& other = user->make<EntityRuntimeState>();
        other.set_original_version(identity, 2);
        root.adopt_mutations_from(other);
    }
    auto& result = Access::execute_ledger_plan_internal(service, root, locations).wait();
    auto outcome = detail::model_json(r);
    if (result) {
        Json::Array generated(r);
        for (const auto& [key, id] : result.value()) {
            auto entry = detail::model_json(r);
            detail::model_put(entry, "key", key.debug(r));
            detail::model_put(entry, "value", detail::debug_value(r, id));
            generated.emplace_back(std::move(entry));
        }
        detail::model_put(outcome, "generated", generated);
    } else {
        detail::model_put(outcome, "error", result.error().to_string(r));
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "outcome", outcome);
    Json::Array notes(r), requests(r), queries(r), events(r);
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
        auto entry = detail::model_json(r);
        detail::model_put(entry, "entity", query.query.entity);
        detail::model_put(entry,
                          "filter",
                          query.query.predicate
                              ? Json(r, detail::debug_expr(r, *query.query.predicate))
                              : Json(r));
        detail::model_put(entry, "trace", trace_out(r, query.query.trace_chain));
        detail::model_put(entry,
                          "comment",
                          query.query.comment_text ? Json(r, *query.query.comment_text) : Json(r));
        queries.emplace_back(std::move(entry));
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
        Arena output(4096, std::pmr::new_delete_resource());
        auto input = Json::parse(&output, line);
        std::cout << evaluate(&output, *input).dump(&output) << '\n';
    }
}
