#include "../support/runtime_mutation_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
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
    return out;
}

struct Id final : InternalIdGenerator {
    State& s;
    mutable std::uint64_t next = 100;

    explicit Id(State& s) : s(s) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        s.note("id");
        return Result<std::uint64_t, RuntimeError>::ok(r, next++);
    }
};

struct Hook final : EntityDataServiceBehavior {
    State& s;
    String mode;

    Hook(State& s, std::string_view mode) : s(s), mode(mode, s.resource) {}

    RuntimeHookResult call(Memory r, MutationValues* v) const {
        s.note("hook");
        if (mode == "error") {
            return RuntimeHookResult::err(r, RuntimeError::behavior(r, "hook failed"));
        }
        if (mode == "set" && v) {
            v->fields.insert_or_assign(String("name", r), Value(r, "hook"));
        }
        return RuntimeHookResult::ok(r, {});
    }

    RuntimeHookResult before_insert(Memory r, const UserContext&, InsertCommand& c) const override {
        return call(r, &c.values);
    }

    RuntimeHookResult before_update(Memory r, const UserContext&, UpdateCommand& c) const override {
        return call(r, &c.values);
    }

    RuntimeHookResult before_delete(Memory r, const UserContext&, DeleteCommand&) const override {
        return call(r, nullptr);
    }

    RuntimeHookResult before_recover(Memory r, const UserContext&, RecoverCommand&) const override {
        return call(r, nullptr);
    }
};

struct Policy final : RequestPolicy {
    State& s;
    String mode;

    Policy(State& s, std::string_view mode) : s(s), mode(mode, s.resource) {}

    RuntimeHookResult call(Memory r, MutationValues* v) const {
        s.note("policy");
        if (mode == "error") {
            return RuntimeHookResult::err(r, RuntimeError::policy(r, "policy failed"));
        }
        if (mode == "set" && v) {
            v->fields.insert_or_assign(String("name", r), Value(r, "policy"));
        }
        return RuntimeHookResult::ok(r, {});
    }

    RuntimeHookResult
    enforce_insert(Memory r, const UserContext&, InsertCommand& c) const override {
        return call(r, &c.values);
    }

    RuntimeHookResult
    enforce_update(Memory r, const UserContext&, UpdateCommand& c) const override {
        return call(r, &c.values);
    }

    RuntimeHookResult enforce_delete(Memory r, const UserContext&, DeleteCommand&) const override {
        return call(r, nullptr);
    }

    RuntimeHookResult
    enforce_recover(Memory r, const UserContext&, RecoverCommand&) const override {
        return call(r, nullptr);
    }
};

struct Fix final : Checker {
    State& s;

    explicit Fix(State& s) : s(s) {}

    std::string_view entity() const override {
        return "Order";
    }

    void check_and_fix(const UserContext&,
                       EntityValues& v,
                       const ObjectLocation&,
                       CheckResults&) const override {
        s.note("fix");
        v.fields.insert_or_assign(String("name", v.memory_resource()),
                                  Value(v.memory_resource(), "fixed"));
    }
};

static std::optional<std::int64_t> version(const Json& j) {
    return std::holds_alternative<std::monostate>(j.data)
               ? std::nullopt
               : std::optional<std::int64_t>(detail::model_integer<std::int64_t>(j, "version"));
}

template <class C>
static auto& dispatch(const EntityDataService<Executor>& service,
                      const C& command,
                      std::string_view layer,
                      std::span<const TraceNode> lineage) {
    const auto& context = Access::context_service(service);
    auto base = context.data_service();
    if (layer == "base") {
        if constexpr (std::same_as<C, InsertCommand>) {
            return base.insert(command);
        } else if constexpr (std::same_as<C, UpdateCommand>) {
            return base.update(command);
        } else if constexpr (std::same_as<C, DeleteCommand>) {
            return base.delete_command(command);
        } else if constexpr (std::same_as<C, RecoverCommand>) {
            return base.recover(command);
        } else if constexpr (std::same_as<C, BatchInsertCommand>) {
            return base.batch_insert(command);
        } else {
            return base.batch_update(command);
        }
    }
    if (layer == "context") {
        if constexpr (std::same_as<C, InsertCommand>) {
            return context.insert(command);
        } else if constexpr (std::same_as<C, UpdateCommand>) {
            return context.update(command);
        } else if constexpr (std::same_as<C, DeleteCommand>) {
            return context.delete_command(command);
        } else if constexpr (std::same_as<C, RecoverCommand>) {
            return context.recover(command);
        } else if constexpr (std::same_as<C, BatchInsertCommand>) {
            return context.batch_insert(command);
        } else {
            return context.batch_update(command);
        }
    }
    if constexpr (std::same_as<C, InsertCommand>) {
        if (layer == "prepared") {
            return Access::execute_prepared_insert(service, command, lineage);
        }
        return Access::insert_internal(service, command);
    } else if constexpr (std::same_as<C, UpdateCommand>) {
        if (layer == "prepared") {
            return Access::execute_prepared_update(service, command, lineage);
        }
        return Access::update_internal(service, command);
    } else if constexpr (std::same_as<C, DeleteCommand>) {
        return Access::delete_internal(service, command);
    } else if constexpr (std::same_as<C, RecoverCommand>) {
        return Access::recover_internal(service, command);
    } else if constexpr (std::same_as<C, BatchInsertCommand>) {
        return Access::execute_prepared_batch_insert(service, command);
    } else {
        return Access::execute_prepared_batch_update(service, command);
    }
}

static Json evaluate(Memory target, const Json& j) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->set_user_identifier("fixture");
    user->disable_sql_log();
    if (flag(j, "log")) {
        user->enable_mutation_sql_log();
    }
    auto& s = user->make<State>();
    s.failure = detail::model_integer<int>(field(j, "fail"), "fail");
    s.event_failure = detail::model_integer<int>(field(j, "event_fail"), "event_fail");
    for (const auto& n : std::get<Json::Array>(field(j, "counts").data)) {
        s.affected.push_back(detail::model_integer<std::uint64_t>(n, "count"));
    }
    auto& module = user->make<RuntimeModule>();
    if (flag(j, "metadata")) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), "Order");
        d.property("id", DataType::U64).id();
        d.property("version", DataType::I64).version();
        auto& p = d.property("name", DataType::Text);
        if (flag(j, "required")) {
            p.not_null();
        }
        module.descriptor(std::move(d));
    }
    auto& sink = user->make_resource<Sink>(s);
    module.event_sink(*user, sink);
    auto hook = text(field(j, "hook"));
    if (hook != "none") {
        auto& h = user->make_resource<Hook>(s, hook);
        module.behavior("Order", *user, h);
    }
    if (flag(j, "fix")) {
        auto& f = user->make_resource<Fix>(s);
        module.checker(*user, f);
    }
    module.apply_to(*user);
    auto p = text(field(j, "policy"));
    if (p != "none") {
        auto& policy = user->make_resource<Policy>(s, p);
        user->set_request_policy(*user, policy);
    }
    if (flag(j, "cache")) {
        auto& cache = user->make_resource<Cache>(s);
        user->insert_resource(*user, static_cast<AggregationCacheBackend&>(cache));
    }
    auto& id = user->make_resource<Id>(s);
    user->set_internal_id_generator(*user, id);
    auto& executor = user->make_resource<Executor>(s);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    if (const auto* bound = optional(j, "bound_intent"); bound && std::get<bool>(bound->data)) {
        auto result = Access::bind_mutation_intent(service, "verify event execution");
        if (!result) {
            throw std::logic_error("fixture intent binding failed");
        }
    }
    const auto* probe = optional(j, "privacy_probe");
    const bool privacy_probe = probe && std::get<bool>(probe->data);
    if (privacy_probe) {
        auto mode = text(field(j, "privacy"));
        if (mode != "none") {
            SqlIntentRedactions redactions(r);
            if (mode == "id") {
                redactions.capture_target_id(Value(r, std::uint64_t(7)));
            } else if (mode != "empty") {
                SqlLogContext context(std::allocator_arg, Allocator(r));
                context.parameter_policies = {mode == "masked" ? SqlParameterLogPolicy::Masked
                                              : mode == "credential"
                                                  ? SqlParameterLogPolicy::Credential
                                                  : SqlParameterLogPolicy::Unknown};
                Vector<Value> operands(r);
                operands.emplace_back("PRIVATE-OPERAND");
                redactions = SqlIntentRedactions::from_bindings(r, context, operands, "");
            }
            auto result = Access::bind_mutation_privacy(service, redactions);
            if (!result) {
                throw std::logic_error("fixture privacy binding failed");
            }
        }
        if (flag(j, "replace_intent")) {
            auto result = Access::bind_mutation_intent(service, "replacement event reason");
            if (!result) {
                throw std::logic_error("fixture replacement intent failed");
            }
        }
    }
    auto lineage = trace(r, field(j, "lineage"));
    service.with_trace_context(lineage);
    auto entity = text(field(j, "entity"));
    auto input = record(r, field(j, "values"));
    auto entry = text(field(j, "entry"));
    auto layer = text(field(j, "layer"));
    auto command_trace = trace(r, field(j, "trace"));
    auto key = value(r, field(j, "id"));
    auto v = version(field(j, "version"));
    auto outcome = detail::model_json(r);
    try {
        AsyncOperation<Result<std::uint64_t, DataServiceError<String>>>* work = nullptr;
        if (entry == "batch_insert") {
            auto& c = user->make<BatchInsertCommand>(entity);
            for (const auto& values : std::get<Json::Array>(field(j, "batch").data)) {
                c.batch_values.emplace_back();
                c.batch_values.back().fields = record(r, values);
            }
            for (const auto& t : std::get<Json::Array>(field(j, "traces").data)) {
                c.trace_chains.emplace_back(trace(r, t));
            }
            work = &dispatch(service, c, layer, lineage);
        } else if (entry == "batch_update") {
            auto& c = user->make<BatchUpdateCommand>(entity);
            for (const auto& values : std::get<Json::Array>(field(j, "batch").data)) {
                c.batch_values.emplace_back();
                c.batch_values.back().fields = record(r, values);
            }
            for (const auto& i : std::get<Json::Array>(field(j, "ids").data)) {
                c.batch_ids.emplace_back(value(r, i));
            }
            for (const auto& n : std::get<Json::Array>(field(j, "versions").data)) {
                c.batch_expected_versions.emplace_back(version(n));
            }
            for (const auto& name : std::get<Json::Array>(field(j, "fields").data)) {
                c.update_fields.emplace_back(text(name));
            }
            for (const auto& old : std::get<Json::Array>(field(j, "olds").data)) {
                std::optional<EntitySnapshot> snapshot;
                if (std::holds_alternative<Json::Object>(old.data)) {
                    snapshot.emplace(r);
                    snapshot->fields = record(r, old);
                }
                c.batch_old_values.emplace_back(std::move(snapshot));
            }
            for (const auto& t : std::get<Json::Array>(field(j, "traces").data)) {
                c.trace_chains.emplace_back(trace(r, t));
            }
            work = &dispatch(service, c, layer, lineage);
        } else {
            auto invoke = [&]<class C>(C& c) {
                TraceChain copy(command_trace, r);
                c.trace_chain.swap(copy);
                if constexpr (std::same_as<C, InsertCommand> || std::same_as<C, UpdateCommand>) {
                    c.values.fields = clone_record(r, input);
                }
                if constexpr (std::same_as<C, UpdateCommand>) {
                    if (std::holds_alternative<Json::Object>(field(j, "old").data)) {
                        c.old_values.emplace(r);
                        c.old_values->fields = record(r, field(j, "old"));
                    }
                }
                if (entry.ends_with("s")) {
                    Vector<C> list(r);
                    list.emplace_back(c);
                    list.emplace_back(c);
                    auto base = Access::context_service(service).data_service();
                    if constexpr (std::same_as<C, InsertCommand>) {
                        work = &base.insert_many(list);
                    } else if constexpr (std::same_as<C, UpdateCommand>) {
                        work = &base.update_many(list);
                    } else if constexpr (std::same_as<C, DeleteCommand>) {
                        work = &base.delete_many(list);
                    } else {
                        work = &base.recover_many(list);
                    }
                } else {
                    work = &dispatch(service, c, layer, lineage);
                }
            };
            if (entry == "insert" || entry == "inserts") {
                auto& c = user->make<InsertCommand>(entity);
                invoke(c);
            } else if (entry == "update" || entry == "updates") {
                auto& c = user->make<UpdateCommand>(entity, key);
                c.expected_version_value = v;
                invoke(c);
            } else if (entry == "delete" || entry == "deletes") {
                auto& c = user->make<DeleteCommand>(entity, key);
                c.expected_version_value = v;
                invoke(c);
            } else {
                auto& c = user->make<RecoverCommand>(entity, key, v.value_or(1));
                invoke(c);
            }
        }
        auto& result = work->wait();
        if (result) {
            detail::model_put(outcome, "rows", result.value());
        } else {
            detail::model_put(outcome, "error", result.error().to_string(r));
        }
    } catch (const std::exception&) {
        detail::model_put(outcome, "panic", true);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "outcome", outcome);
    Json::Array notes(r), requests(r), events(r);
    for (const auto& n : s.notes) {
        notes.emplace_back(n);
    }
    for (const auto& request : s.requests) {
        auto observed = request_out(r, request);
        if (privacy_probe) {
            detail::model_put(observed,
                              "privacy",
                              request.diagnostic_redactions()
                                  ? Json(r, request.diagnostic_redactions()->is_empty())
                                  : Json(r));
            detail::model_put(observed, "intent", request.comment());
        }
        requests.emplace_back(std::move(observed));
    }
    for (const auto& event : s.events) {
        events.emplace_back(event_out(r, event));
    }
    detail::model_put(out, "notes", notes);
    detail::model_put(out, "requests", requests);
    detail::model_put(out, "events", events);
    detail::model_put(out, "logs", user->sql_logs().size());
    detail::model_put(out, "next_id", id.next);
    if (privacy_probe) {
        Json::Array privacy_events(r);
        for (const auto& event : s.events) {
            auto observed = detail::model_json(r);
            detail::model_put(observed,
                              "privacy",
                              event.diagnostic_redactions()
                                  ? Json(r, event.diagnostic_redactions()->is_empty())
                                  : Json(r));
            auto safe = event.build_safe_event(r, {}, {});
            Json::Array fields(r);
            for (const auto& field : safe.fields) {
                auto item = detail::model_json(r);
                detail::model_put(item, "name", field.name);
                detail::model_put(item, "value", field.value ? Json(r, *field.value) : Json(r));
                detail::model_put(item, "masked", field.masked);
                detail::model_put(item, "truncated", field.truncated);
                detail::model_put(
                    item, "raw_length", field.raw_length ? Json(r, *field.raw_length) : Json(r));
                detail::model_put(item,
                                  "output_length",
                                  field.output_length ? Json(r, *field.output_length) : Json(r));
                detail::model_put(
                    item, "mask_reason", field.mask_reason ? Json(r, *field.mask_reason) : Json(r));
                detail::model_put(item,
                                  "truncate_reason",
                                  field.truncate_reason ? Json(r, *field.truncate_reason)
                                                        : Json(r));
                fields.emplace_back(std::move(item));
            }
            detail::model_put(observed, "fields", fields);
            detail::model_put(observed, "trace", trace_out(r, safe.trace_chain));
            privacy_events.emplace_back(std::move(observed));
        }
        detail::model_put(out, "event_privacy", privacy_events);
    }
    return Json(target, out);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena out(4096, std::pmr::new_delete_resource());
        auto j = Json::parse(&out, line);
        std::cout << evaluate(&out, *j).dump(&out) << '\n';
    }
}
