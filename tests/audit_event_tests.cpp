#include <teaql/runtime/context.hpp>
#include <teaql/runtime/event.hpp>
#include <teaql/runtime/detail/audit_privacy.hpp>
#include <teaql/core/detail/debug.hpp>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejected(F&& f, const char* message) {
    bool failed = false;
    try {
        f();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

static void value_domain(const Value& v, Memory r) {
    check(v.memory_resource() == r, "Value declares destination arena");
    if (auto* s = std::get_if<String>(&v.data)) {
        check(s->get_allocator().resource() == r, "text belongs to destination");
    }
    if (auto* list = v.as_list()) {
        for (const auto& item : *list) {
            value_domain(item, r);
        }
    }
    if (auto* map = v.as_object()) {
        for (const auto& [key, item] : *map) {
            check(key.get_allocator().resource() == r, "record key belongs to destination");
            value_domain(item, r);
        }
    }
}

static void event_domain(const RawAuditEvent& e, Memory r) {
    check(e.memory_resource() == r && e.changes.get_allocator().resource() == r,
          "event containers belong to destination");
    for (const auto& [_, v] : e.values) {
        value_domain(v, r);
    }
    for (const auto* snapshot : {&e.old_values, &e.new_values}) {
        if (*snapshot) {
            for (const auto& [_, v] : **snapshot) {
                value_domain(v, r);
            }
        }
    }
    for (const auto& c : e.changes) {
        check(c.memory_resource() == r, "change belongs to destination");
        if (c.old_value) {
            value_domain(*c.old_value, r);
        }
        if (c.new_value) {
            value_domain(*c.new_value, r);
        }
    }
    check(e.bootstrap_audit->reason.get_allocator().resource() == r &&
              e.trace_chain[0].comment.get_allocator().resource() == r,
          "bootstrap and trace belong to destination");
    const auto& g = *e.mutation_governance;
    check(g.policy->fingerprint.get_allocator().resource() == r &&
              g.warning_codes[0].get_allocator().resource() == r,
          "policy and warnings belong to destination");
    check(g.operations[0].entity.memory_resource() == r &&
              g.operations[0].changed_fields[0].get_allocator().resource() == r,
          "operation identity and changed fields belong to destination");
    value_domain(g.operations[0].entity.id, r);
}

static void copies() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource(), g = global.memory_resource();
    // Values live in the source arena; all escaping objects must explicitly clone.
    auto& e = [&]() -> RawAuditEvent& {
        Record nested(r);
        nested.emplace("long_field_name_to_require_allocation",
                       Value(r, "a long string that must be allocated in the selected arena"));
        Value::List list(r);
        list.emplace_back(Value(r, nested));
        Record record(r);
        record.emplace("id", Value(r, std::uint64_t(1001)));
        record.emplace("long_payload_field", Value(r, list));
        auto& out = user->make<RawAuditEvent>(
            RawAuditEvent::updated(r, "LongEntityNameForAllocation", record));
        out.old_values.emplace(clone_record(r, record));
        out.trace_chain.emplace_back(TraceKind::AuditReason,
                                     "LongEntityNameForAllocation",
                                     "a long trace comment containing 1001",
                                     1001);
        out.bootstrap_audit.emplace(std::allocator_arg,
                                    Allocator(r),
                                    "long bootstrap actor identity",
                                    "long bootstrap category identity",
                                    "a long bootstrap reason identity",
                                    3,
                                    1234);
        out.mutation_governance.emplace(std::allocator_arg,
                                        Allocator(r),
                                        "long execution identifier value",
                                        "long request identifier value",
                                        MutationPolicySource::Customer,
                                        MutationPolicyApprovalStatus::Approved);
        auto& governance = *out.mutation_governance;
        governance.policy.emplace(std::allocator_arg,
                                  Allocator(r),
                                  "long policy identifier value",
                                  "long policy version identifier",
                                  "long policy fingerprint value");
        governance.warning_codes.emplace_back("long warning code identifier");
        EntityKey key(
            std::allocator_arg, Allocator(r), "LongEntityNameForAllocation", Value(r, nested));
        governance.operations.emplace_back(MutationOperationKind::Update, key);
        governance.operations.back().changed_fields.emplace_back("long_payload_field");
        return out;
    }();
    auto& retained = global.make<RawAuditEvent>(e);
    auto& assigned = global.make<RawAuditEvent>(RawAuditEventKind::Created, "empty");
    assigned = detail::clone_in(r, e);
    auto& safe = global.make<SafeAuditEvent>(e.build_safe_event(r, {}, 17));
    auto& projection = global.make<RawAuditEvent>(detail::audit_event_projection(r, e, false));
    e.mutation_governance->policy->id = "changed";
    e.changes.clear();
    user.reset();
    event_domain(retained, g);
    event_domain(assigned, g);
    event_domain(projection, g);
    check(retained == assigned && retained.changes.size() == 2,
          "cross-domain move assignment deep copies and survives source destruction");
    check(retained.mutation_governance->policy->id == "long policy identifier value",
          "destination policy is independent");
    check(
        safe.fields[1].value->get_allocator().resource() == g &&
            safe.fields[1].truncate_reason->get_allocator().resource() == g &&
            safe.mutation_governance->operations[0].changed_fields[0].get_allocator().resource() ==
                g,
        "safe fields and governance survive source close");
    check(safe.mutation_governance->source == MutationPolicySource::Customer &&
              safe.mutation_governance->approval_status == MutationPolicyApprovalStatus::Approved &&
              safe.mutation_governance->operations[0].kind == MutationOperationKind::Update,
          "governance distinctions remain intact");
    check(std::get<String>(projection.values.at(String("long_payload_field", g)).data) ==
              "[REDACTED]",
          "raw projection redacts ordinary business values");
}

struct Sink final : RawAuditEventSink {
    Memory r;
    Vector<int>& calls;
    int id;
    bool fail;

    Sink(Memory r, Vector<int>& calls, int id, bool fail = false)
        : r(r), calls(calls), id(id), fail(fail) {}

    Memory memory_resource() const {
        return r;
    }

    Result<std::monostate, RuntimeError>
    on_event(Memory result, const UserContext&, const RawAuditEvent&) const override {
        calls.push_back(id);
        if (fail) {
            return Result<std::monostate, RuntimeError>::err(
                result, RuntimeError::event(result, "sink rejected event"));
        }
        return Result<std::monostate, RuntimeError>::ok(result, {});
    }
};

static void sinks() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    auto g = global.memory_resource();
    auto& calls = global.make<Vector<int>>();
    auto& one = global.make_resource<Sink>(g, calls, 1);
    auto& fail = global.make_resource<Sink>(g, calls, 2, true);
    auto& three = global.make_resource<Sink>(g, calls, 3);
    auto& local = user->make<InMemoryRawAuditEventSink>();
    local.with_sink(global, one).with_sink(global, fail).with_sink(global, three);
    auto event = RawAuditEvent::schema_verified(g, "Order", "orders", 4);
    auto result = local.on_event(g, *user, event);
    check(!result && result.error().kind() == RuntimeError::Kind::Event && calls.size() == 2 &&
              calls[0] == 1 && calls[1] == 2,
          "sinks run in order and stop on first error");
    auto& copied = global.make<InMemoryRawAuditEventSink>(local);
    auto& local_sink = user->make_resource<Sink>(user->memory_resource(), calls, 4);
    local.register_sink(*user, local_sink);
    rejected(
        [&] {
            global.make<InMemoryRawAuditEventSink>(local);
        },
        "Global clone cannot extend a User sink borrow");
    rejected(
        [&] {
            sibling->make<InMemoryRawAuditEventSink>(local);
        },
        "sibling cannot retain another User's sink");
    rejected(
        [&] {
            copied.register_sink(global, local_sink);
        },
        "sink registration checks the actual allocation domain");
    user.reset();
    calls.clear();
    check(!copied.on_event(g, *sibling, event) && calls.size() == 2,
          "copy of Global sink borrows survives User close");
    auto& success = global.make<InMemoryRawAuditEventSink>();
    success.with_sink(global, one).with_sink(global, three).with_sink(global, one);
    calls.clear();
    check(bool(success.on_event(g, *sibling, event)) && calls.size() == 3 && calls[2] == 1,
          "duplicate registrations are dispatched independently");
}

static void identities_and_boundaries() {
    GlobalContext global;
    auto r = global.memory_resource();
    EntityKey a(std::allocator_arg, Allocator(r), "Order", Value(r, std::int64_t(1))),
        b(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(1)));
    a.id = Value(r, "changed public ID");
    auto& copy = global.make<EntityKey>(a);
    check(copy == b && std::get<String>(copy.id.data) == "changed public ID",
          "EntityKey clone preserves cached identity independently of public ID");
    MutationPolicyIdentity policy(
        std::allocator_arg, Allocator(r), " id ", " version ", " fingerprint ");
    check(policy.id == " id ", "policy constructor validates trimmed text but retains original");
    policy.id.clear();
    check(global.make<MutationPolicyIdentity>(policy).id.empty(),
          "policy clone does not revalidate mutable source fields");
    check(detail::debug_value(r, Value(r, 1.23456789e100)) == "F64(1.23456789e100)",
          "float debug works without default PMR allocations");
    for (std::string_view bad :
         {"\x80", "\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82"}) {
        rejected(
            [&] {
                limit_audit_value(r, bad, 3);
            },
            "audit rejects invalid UTF-8");
        rejected(
            [&] {
                detail::debug_text(r, bad);
            },
            "debug rejects invalid UTF-8");
    }
    for (auto version :
         {std::numeric_limits<std::int64_t>::min(), std::numeric_limits<std::int64_t>::min() + 1}) {
#ifndef NDEBUG
        bool overflow = false;
        try {
            RawAuditEvent::recovered_with_old_values(
                r, "Order", Value(r, std::uint64_t(1)), version, {});
        } catch (const std::overflow_error&) {
            overflow = true;
        }
        check(overflow, "debug recovery detects source arithmetic overflow");
#else
        auto e = RawAuditEvent::recovered_with_old_values(
            r, "Order", Value(r, std::uint64_t(1)), version, {});
        check(std::get<std::int64_t>(e.new_values->at(String("version", r)).data) ==
                  std::bit_cast<std::int64_t>(std::uint64_t(1) - std::uint64_t(version)),
              "release recovery preserves source wrapping arithmetic");
#endif
    }
}

static void privacy() {
    static_assert(!std::same_as<RawAuditEvent, SafeAuditEvent>);
    GlobalContext global;
    auto r = global.memory_resource();
    Record values(r);
    values.emplace("null", Value(r));
    auto e = RawAuditEvent::created(r, "Order", values);
    check(!e.changes[0].old_value && e.changes[0].new_value && e.changes[0].new_value->is_null(),
          "missing old value is distinct from explicit Null");
    Record nested(r);
    nested.emplace("password", Value(r, "credential secret"));
    std::optional<Record> old;
    old.emplace(r);
    old->emplace("nested", Value(r, nested));
    auto deleted =
        RawAuditEvent::deleted_with_old_values(r, "Order", Value(r, std::uint64_t(1)), {}, old);
    auto safe = deleted.build_safe_event(r, {}, 0);
    check(safe.fields[0].value == "[REDACTED]" && safe.fields[0].masked &&
              !safe.fields[0].raw_length && !safe.fields[0].truncated,
          "nested old credential produces a redaction even with absent new value");
    Record fresh(r);
    fresh.emplace("nested", Value(r, "ordinary business text"));
    Vector<String> fields(r);
    fields.emplace_back("nested");
    auto update = RawAuditEvent::updated_with_old_values(r, "Order", values, old, fresh, fields);
    auto projected = update.build_safe_event(r, fields, 4);
    check(projected.fields[0].value == "[REDACTED]" && !projected.fields[0].truncated &&
              projected.fields[0].truncate_reason == "_audit_value_max_len",
          "nested redaction retains source truncation reason even after flag reset");
    setenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS",
           "I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK",
           1);
    auto credential = build_safe_audit_field(r, "password", "credential secret", {}, {});
    check(credential.masked && credential.value == "[REDACTED]",
          "plaintext acknowledgement never exposes credentials");
    unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        copies();
        sinks();
        identities_and_boundaries();
        privacy();
        std::pmr::set_default_resource(previous);
        std::cout << "Audit event contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
