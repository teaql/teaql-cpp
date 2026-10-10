#include <teaql/runtime.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <cstdlib>
#include <iostream>

static Record record(Memory owner, const Json& input) {
    Record result(owner);
    for (const auto& [name, item] : detail::model_object(input, "record")) {
        result.emplace(String(name, owner), value(owner, item));
    }
    return result;
}

static Vector<String> strings(Memory owner, const Json& input) {
    Vector<String> result(owner);
    for (const auto& item : detail::model_array(input, "strings")) {
        result.emplace_back(text(item));
    }
    return result;
}

static RawAuditEvent raw(Memory owner, const Json& input) {
    auto entity = text(field(input, "entity"));
    auto values = record(owner, field(input, "values"));
    std::optional<Record> old;
    if (std::holds_alternative<Json::Object>(field(input, "old").data)) {
        old.emplace(record(owner, field(input, "old")));
    }
    auto identifier = value(owner, field(input, "id"));
    std::optional<std::int64_t> version;
    if (std::holds_alternative<String>(field(input, "version").data)) {
        version = number<std::int64_t>(field(input, "version"));
    }
    auto kind = text(field(input, "kind"));
    auto count = detail::model_integer<std::size_t>(field(input, "count"), "count");
    auto event = [&] {
        if (kind == "created") {
            return RawAuditEvent::created(owner, entity, values);
        }
        if (kind == "updated") {
            return RawAuditEvent::updated(owner, entity, values);
        }
        if (kind == "updated_old") {
            return RawAuditEvent::updated_with_old_values(owner,
                                                          entity,
                                                          values,
                                                          old,
                                                          record(owner, field(input, "new")),
                                                          strings(owner, field(input, "fields")));
        }
        if (kind == "deleted") {
            return RawAuditEvent::deleted(owner, entity, identifier, version);
        }
        if (kind == "deleted_old") {
            return RawAuditEvent::deleted_with_old_values(owner, entity, identifier, version, old);
        }
        if (kind == "recovered") {
            return RawAuditEvent::recovered(owner, entity, identifier, *version);
        }
        if (kind == "recovered_old") {
            return RawAuditEvent::recovered_with_old_values(
                owner, entity, identifier, *version, old);
        }
        if (kind == "schema_created") {
            return RawAuditEvent::schema_created(owner, entity, "orders", count);
        }
        if (kind == "schema_verified") {
            return RawAuditEvent::schema_verified(owner, entity, "orders", count);
        }
        if (kind == "field_added") {
            return RawAuditEvent::field_added(owner, entity, "orders", "name");
        }
        return RawAuditEvent::data_seeded(owner, entity, "orders", count, 2);
    }();
    if (std::get<bool>(field(input, "trace").data)) {
        event.trace_chain.emplace_back(TraceKind::AuditReason, "Order", "reason secret 1001", 1001);
        event.trace_chain.emplace_back(TraceKind::Entity, "Order", "trace🧊");
    }
    if (std::get<bool>(field(input, "bootstrap").data)) {
        event.bootstrap_audit.emplace(std::allocator_arg,
                                      Allocator(owner),
                                      "actor🧊",
                                      "category",
                                      "bootstrap reason",
                                      -3,
                                      UINT64_MAX);
    }
    if (std::get<bool>(field(input, "governance").data)) {
        event.mutation_governance.emplace(std::allocator_arg,
                                          Allocator(owner),
                                          "execution",
                                          "Order.saveGraph",
                                          MutationPolicySource::Customer,
                                          MutationPolicyApprovalStatus::Approved);
        auto& snapshot = *event.mutation_governance;
        snapshot.policy.emplace(
            std::allocator_arg, Allocator(owner), "policy", "v1", "fingerprint");
        snapshot.warning_codes.emplace_back("warning");
        EntityKey key(
            std::allocator_arg, Allocator(owner), "Order", Value(owner, std::uint64_t(1001)));
        auto& operation = snapshot.operations.emplace_back(MutationOperationKind::Update, key);
        operation.changed_fields.emplace_back("secret");
        operation.changed_fields.emplace_back("name");
    }
    return event;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    if (std::get<bool>(field(input, "plaintext").data)) {
        setenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS",
               "I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK",
               1);
    } else {
        unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    }
    auto event = raw(owner, input);
    auto clone = detail::clone_in(owner, event);
    auto other = detail::clone_in(owner, event);
    other.entity += '!';
    std::optional<std::size_t> maximum;
    if (!std::holds_alternative<std::monostate>(field(input, "maximum").data)) {
        maximum = detail::model_integer<std::size_t>(field(input, "maximum"), "maximum");
    }
    auto safe = event.build_safe_event(owner, strings(owner, field(input, "mask")), maximum);
    auto safe_clone = detail::clone_in(owner, safe);
    auto safe_other = detail::clone_in(owner, safe);
    safe_other.entity += '!';
    auto output = detail::model_json(owner);
    auto put = [&](std::string_view name, auto&& value) {
        detail::model_put(output, name, std::forward<decltype(value)>(value));
    };
    put("raw", formatted_debug(owner, event));
    put("raw_clone", formatted_debug(owner, clone));
    put("raw_clone_equal", event == clone);
    put("raw_changed_equal", event == other);
    put("safe", formatted_debug(owner, safe));
    put("safe_clone", formatted_debug(owner, safe_clone));
    put("safe_clone_equal", safe == safe_clone);
    put("safe_changed_equal", safe == safe_other);
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid event model fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
