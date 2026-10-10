#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static FixEvidence evidence(Memory r, std::string_view label) {
    return FixEvidence(
        std::allocator_arg, Allocator(r), "Item", "userName", FixEvidenceSource::Clock, label);
}

static Json describe(Memory r, std::span<const FixEvidence> evidence) {
    Json::Array out(r);
    for (const auto& value : evidence) {
        auto item = detail::model_json(r);
        detail::model_put(item, "entity", value.entity_type);
        detail::model_put(item, "path", value.model_path);
        detail::model_put(item, "source", fix_evidence_source_name(value.source));
        detail::model_put(item, "label", value.source_label);
        out.emplace_back(std::move(item));
    }
    return Json(r, out);
}

static std::int64_t integer(const Json& value) {
    if (auto* n = std::get_if<std::int64_t>(&value.data)) {
        return *n;
    }
    return static_cast<std::int64_t>(std::get<std::uint64_t>(value.data));
}

static void
nested(UserContext& user, const Json::Array& times, std::size_t index, Json::Array& trace) {
    auto r = user.memory_resource();
    detail::GraphFixState state(std::allocator_arg, Allocator(r), Timestamp(integer(times[index])));
    auto note = [&](std::string_view phase) {
        auto out = detail::model_json(r);
        detail::model_put(out, "time", user.fix_time().as_millis());
        detail::model_put(out, "level", index);
        detail::model_put(out, "phase", phase);
        trace.emplace_back(std::move(out));
    };
    {
        detail::GraphFixActivation active(user, state);
        note("before");
        String label("level:", r);
        label += std::to_string(index);
        user.record_fix_evidence(FixEvidence(std::allocator_arg,
                                             Allocator(r),
                                             "Item",
                                             "userName",
                                             FixEvidenceSource::Context,
                                             label));
        if (index + 1 < times.size()) {
            nested(user, times, index + 1, trace);
        }
        note("after");
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "evidence", describe(r, state.evidence));
    detail::model_put(out, "level", index);
    detail::model_put(out, "phase", "evidence");
    trace.emplace_back(std::move(out));
}

struct Body {
    using allocator_type = Allocator;
    const UserContext& context;
    std::size_t pending, polls = 0;
    String outcome;
    Json::Array& notes;
    std::optional<Timestamp> first;

    Body(std::allocator_arg_t,
         Allocator a,
         const UserContext& context,
         std::size_t pending,
         std::string_view outcome,
         Json::Array& notes)
        : context(context), pending(pending), outcome(outcome, a.resource()), notes(notes) {}

    void note(std::string_view label) {
        auto time = context.fix_time();
        if (!first) {
            first = time;
        }
        auto r = context.memory_resource();
        auto out = detail::model_json(r);
        detail::model_put(out, "label", label);
        detail::model_put(out, "stable", time == *first);
        notes.emplace_back(std::move(out));
        context.record_fix_evidence(evidence(r, label));
    }

    std::optional<RuntimeHookResult> poll(WakeSignal& wake) {
        String label("poll:", context.memory_resource());
        label += std::to_string(polls++);
        note(label);
        if (polls <= pending) {
            wake.notify();
            return {};
        }
        if (outcome == "panic") {
            throw std::runtime_error("fixture panic");
        }
        if (outcome == "error") {
            return RuntimeHookResult::err(
                context.memory_resource(),
                RuntimeError::graph(context.memory_resource(), "fixture error"));
        }
        return RuntimeHookResult::ok(context.memory_resource(), {});
    }

    void finish_and_wait() noexcept {
        if (polls) {
            note("drop");
            polls = 0;
        }
    }

    void cancel_and_wait() noexcept {
        finish_and_wait();
    }
};

static ObjectLocation location(Memory r, const Json& input) {
    auto out = ObjectLocation::root(r);
    for (const auto& part : std::get<Json::Array>(field(input, "path").data)) {
        if (auto* name = std::get_if<String>(&part.data)) {
            out.member(*name);
        } else {
            out.element(std::get<std::uint64_t>(part.data));
        }
    }
    return out;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    auto out = detail::model_json(r);
    auto op = text(field(input, "op"));
    if (op == "construct") {
        try {
            FixEvidence value(
                std::allocator_arg,
                Allocator(r),
                text(field(input, "entity")),
                text(field(input, "path")),
                FixEvidenceSource(std::get<std::uint64_t>(field(input, "source").data)),
                text(field(input, "label")));
            detail::model_put(out, "value", describe(r, std::span(&value, 1)));
        } catch (const std::invalid_argument& error) {
            detail::model_put(out, "error", error.what());
        }
        return out;
    }
    auto seed = evidence(r, "seed");
    detail::ContextFixAccess::replace_last(user, std::span(&seed, 1));
    user.record_fix_evidence(evidence(r, "outside"));
    if (op == "scope") {
        Json::Array trace(r);
        nested(user, std::get<Json::Array>(field(input, "times").data), 0, trace);
        detail::model_put(out, "trace", trace);
        detail::model_put(out, "last", describe(r, user.last_fix_evidence()));
        return out;
    }
    if (op == "payload") {
        auto& module = user.make<RuntimeModule>();
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
        for (const auto& property : std::get<Json::Array>(field(input, "properties").data)) {
            descriptor.properties.emplace_back(text(field(property, "name")), DataType::Text);
            auto& p = descriptor.properties.back();
            if (std::get<bool>(field(property, "required").data)) {
                p.not_null();
            }
            if (std::get<bool>(field(property, "id").data)) {
                p.id();
            }
            if (std::get<bool>(field(property, "version").data)) {
                p.version();
            }
        }
        module.descriptor(std::move(descriptor)).apply_to(user);
        (void)user.set_language_code(text(field(input, "language")));
        EntityValues values(std::allocator_arg, Allocator(r));
        for (const auto& [k, v] : std::get<Json::Object>(field(input, "values").data)) {
            values.fields.emplace(String(k, r), value(r, v));
        }
        auto result = detail::ContextFixAccess::validate_required_create_payload(
            user, text(field(input, "entity")), values, location(r, input));
        Json::Array checks(r);
        if (!result) {
            for (const auto& check : result.error().check_results()) {
                checks.emplace_back(check.to_wire(r, JsonFieldNamingProfile::CamelCase).to_json(r));
            }
        }
        detail::model_put(out, "error", result ? Json(r) : Json(r, result.error().to_string(r)));
        detail::model_put(out, "checks", checks);
        return out;
    }
    auto& notes = user.make<Json::Array>();
    auto& child = make_operation<RuntimeHookResult, Body>(
        r,
        user,
        std::get<std::uint64_t>(field(input, "pending").data),
        text(field(input, "outcome")),
        notes);
    auto& operation = detail::with_graph_fix_scope(user, child);
    Json::Array steps(r);
    for (std::size_t i = 0; i < std::get<std::uint64_t>(field(input, "polls").data); ++i) {
        std::string_view status = "pending";
        try {
            if (auto* result = operation.poll()) {
                status = result->is_ok() ? "ok" : "error";
            }
        } catch (const std::runtime_error&) {
            status = "panic";
        }
        auto step = detail::model_json(r);
        detail::model_put(step, "status", status);
        detail::model_put(step, "last", describe(r, user.last_fix_evidence()));
        steps.emplace_back(std::move(step));
        if (status != "pending") {
            break;
        }
    }
    operation.cancel();
    detail::model_put(out, "steps", steps);
    detail::model_put(out, "notes", notes);
    detail::model_put(out, "last", describe(r, user.last_fix_evidence()));
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
