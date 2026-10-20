#include <teaql/runtime.hpp>
#include <teaql/runtime/detail/commit_audit.hpp>
#include <teaql/runtime/error_debug.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static RawAuditEvent
fact(Memory r, const Json& input, std::string_view entity, std::string_view marker) {
    Record values(r);
    values.emplace(String("name", r), Value(r, marker));
    auto id_kind = text(field(input, "id_kind"));
    if (id_kind == "u64") {
        values.emplace(String("id", r), Value(r, std::uint64_t(7)));
    } else if (id_kind == "negative") {
        values.emplace(String("id", r), Value(r, std::int64_t(-7)));
    }
    auto event = RawAuditEvent::created(r, entity, values);
    if (!std::get<bool>(field(input, "new_values").data)) {
        event.new_values.reset();
    }
    return event;
}

static Vector<EntityPropertyChange> changes(Memory r, const Json& input) {
    Vector<EntityPropertyChange> result(r);
    auto change = [&](std::optional<Value> value) {
        result.emplace_back("#title", std::optional<Value>(Value(r)), value);
    };
    auto kind = text(field(input, "change"));
    if (kind == "remove") {
        change({});
    } else if (kind == "null") {
        change(Value(r));
    } else if (kind == "text") {
        change(Value(r, std::string_view("雪\0retained", sizeof("雪\0retained") - 1)));
    } else if (kind == "repeated") {
        change(Value(r, "first"));
        change({});
        change(Value(r, "last"));
    }
    return result;
}

static Json outcome(Memory r, const Result<std::monostate, RuntimeError>& value) {
    auto result = detail::model_json(r);
    detail::model_put(result, "ok", bool(value));
    if (!value) {
        detail::model_put(result, "debug", debug_string(r, value.error()));
        detail::model_put(result, "display", value.error().to_string(r));
    }
    return result;
}

static Json observe_enrich(UserContext& owner, UserContext& other, const Json& input) {
    auto r = owner.memory_resource();
    auto result = detail::model_json(r);
    detail::PendingAuditBatch batch(std::allocator_arg, Allocator(r), owner);
    auto inspect = [&] {
        auto observation = detail::model_json(r);
        detail::model_put(observation, "scope_owner", detail::has_audit_scope(owner));
        detail::model_put(observation, "scope_other", detail::has_audit_scope(other));
        Json::Array returned(r);
        for (const auto& entry : {std::pair{"Item", "first"},
                                  std::pair{"Item", "latest"},
                                  std::pair{"Other", "other entity"}}) {
            returned.emplace_back(
                !detail::enqueue_audit(owner, fact(r, input, entry.first, entry.second)));
        }
        detail::model_put(observation, "returned", Json(r, returned));
        auto& target = text(field(input, "scope")) == "other" ? other : owner;
        auto values = changes(r, input);
        detail::model_put(observation,
                          "outcome",
                          outcome(r, detail::enrich_dynamic_audit(target, "Item", 7, values)));
        return observation;
    };
    if (text(field(input, "scope")) == "none") {
        detail::model_put(result, "observation", inspect());
    } else {
        detail::AuditActivation activation(owner, batch);
        detail::model_put(result, "observation", inspect());
    }
    Json::Array queued(r);
    for (const auto& event : batch.events) {
        auto entry = detail::model_json(r);
        detail::model_put(entry, "debug", debug_string(r, event));
        detail::model_put(entry, "pretty", debug_string(r, event, DebugOptions{.pretty = true}));
        queued.emplace_back(std::move(entry));
    }
    detail::model_put(result, "queued", Json(r, queued));
    detail::model_put(result, "scope_after", detail::has_audit_scope(owner));
    return result;
}

struct Sink final : RawAuditEventSink {
    using allocator_type = Allocator;
    std::uint64_t mask;
    mutable Vector<std::uint64_t> seen;

    Sink(std::allocator_arg_t, Allocator a, std::uint64_t mask) : mask(mask), seen(a.resource()) {}

    Memory memory_resource() const {
        return seen.get_allocator().resource();
    }

    Result<std::monostate, RuntimeError>
    on_event(Memory r, const UserContext&, const RawAuditEvent& event) const override {
        auto id = *event.values.at(String("id", r)).try_u64();
        seen.push_back(id);
        if (mask & (1ULL << (id - 1))) {
            String message("sink failure ", r);
            message += std::to_string(id);
            return Result<std::monostate, RuntimeError>::err(r, RuntimeError::event(r, message));
        }
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

static Json observe_deliver(UserContext& owner, const Json& input) {
    auto r = owner.memory_resource();
    auto& sink = owner.make<Sink>(std::get<std::uint64_t>(field(input, "mask").data));
    owner.set_event_sink(owner, sink);
    Vector<RawAuditEvent> events(r);
    auto count = std::get<std::uint64_t>(field(input, "count").data);
    for (std::uint64_t id = 1; id <= count; ++id) {
        Record values(r);
        values.emplace(String("id", r), Value(r, id));
        events.emplace_back(RawAuditEvent::created(r, "Item", values));
    }
    detail::PendingAuditBatch batch(std::allocator_arg, Allocator(r), owner);
    auto result = detail::model_json(r);
    {
        detail::AuditActivation active(owner, batch);
        detail::model_put(result,
                          "outcome",
                          outcome(r, detail::deliver_committed_audits(owner, std::move(events))));
    }
    Json::Array seen(r);
    for (auto id : sink.seen) {
        seen.emplace_back(id);
    }
    detail::model_put(result, "seen", Json(r, seen));
    detail::model_put(result, "queued", std::uint64_t(batch.events.size()));
    detail::model_put(result, "scope_after", detail::has_audit_scope(owner));
    return result;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto other_user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            return 1;
        }
        auto result = text(field(*input, "kind")) == "enrich"
                          ? observe_enrich(*user_context, *other_user_context, *input)
                          : observe_deliver(*user_context, *input);
        std::cout << result.dump(r) << '\n';
    }
}
