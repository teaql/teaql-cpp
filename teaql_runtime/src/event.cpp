#include <teaql/runtime/event.hpp>
#include <teaql/runtime/context.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <limits>

namespace teaql {
namespace {
std::optional<Value> lookup(Memory r, const Record* values, std::string_view field) {
    if (!values) {
        return {};
    }
    auto it = values->find(field);
    if (it == values->end()) {
        return {};
    }
    return Value(r, it->second);
}

void changes_for_fields(RawAuditEvent& event,
                        const Record* old_values,
                        const Record* new_values,
                        std::span<const String> fields) {
    for (const auto& field : fields) {
        event.changes.emplace_back(field,
                                   lookup(event.memory_resource(), old_values, field),
                                   lookup(event.memory_resource(), new_values, field));
    }
}

void all_changes(RawAuditEvent& event, const Record& values, bool old) {
    for (const auto& [field, value] : values) {
        std::optional<Value> copy(Value(event.memory_resource(), value));
        std::optional<Value> absent;
        if (old) {
            event.changes.emplace_back(field, copy, absent);
        } else {
            event.changes.emplace_back(field, absent, copy);
        }
    }
}

RawAuditEvent
schema(Memory r, RawAuditEventKind kind, std::string_view entity, std::string_view table) {
    RawAuditEvent out(std::allocator_arg, Allocator(r), kind, entity);
    out.values.emplace("table_name", Value(r, table));
    return out;
}
}

RawAuditEvent RawAuditEvent::created(Memory r, std::string_view entity, const Record& values) {
    RawAuditEvent out(std::allocator_arg, Allocator(r), RawAuditEventKind::Created, entity);
    out.values = clone_record(r, values);
    out.new_values.emplace(clone_record(r, values));
    all_changes(out, values, false);
    return out;
}

RawAuditEvent RawAuditEvent::updated(Memory r, std::string_view entity, const Record& values) {
    RawAuditEvent out(std::allocator_arg, Allocator(r), RawAuditEventKind::Updated, entity);
    out.values = clone_record(r, values);
    out.new_values.emplace(clone_record(r, values));
    for (const auto& [field, _] : values) {
        out.updated_fields.emplace_back(field);
    }
    changes_for_fields(out, nullptr, &values, out.updated_fields);
    return out;
}

RawAuditEvent RawAuditEvent::updated_with_old_values(Memory r,
                                                     std::string_view entity,
                                                     const Record& values,
                                                     const std::optional<Record>& old_values,
                                                     const Record& new_values,
                                                     std::span<const String> fields) {
    RawAuditEvent out(std::allocator_arg, Allocator(r), RawAuditEventKind::Updated, entity);
    out.values = clone_record(r, values);
    out.old_values = detail::clone_in(r, old_values);
    out.new_values.emplace(clone_record(r, new_values));
    for (const auto& field : fields) {
        out.updated_fields.emplace_back(field);
    }
    changes_for_fields(out, old_values ? &*old_values : nullptr, &new_values, fields);
    return out;
}

RawAuditEvent RawAuditEvent::deleted(Memory r,
                                     std::string_view entity,
                                     const Value& id,
                                     std::optional<std::int64_t> version) {
    RawAuditEvent out(std::allocator_arg, Allocator(r), RawAuditEventKind::Deleted, entity);
    out.values.emplace("id", Value(r, id));
    if (version) {
        out.values.emplace("version", Value(r, *version));
    }
    return out;
}

RawAuditEvent RawAuditEvent::deleted_with_old_values(Memory r,
                                                     std::string_view entity,
                                                     const Value& id,
                                                     std::optional<std::int64_t> version,
                                                     const std::optional<Record>& old_values) {
    auto out = deleted(r, entity, id, version);
    out.old_values = detail::clone_in(r, old_values);
    if (old_values) {
        all_changes(out, *old_values, true);
    }
    return out;
}

RawAuditEvent
RawAuditEvent::recovered(Memory r, std::string_view entity, const Value& id, std::int64_t version) {
    auto out = deleted(r, entity, id, version);
    out.kind = RawAuditEventKind::Recovered;
    return out;
}

RawAuditEvent RawAuditEvent::recovered_with_old_values(Memory r,
                                                       std::string_view entity,
                                                       const Value& id,
                                                       std::int64_t version,
                                                       const std::optional<Record>& old_values) {
#ifndef NDEBUG
    if (version <= std::numeric_limits<std::int64_t>::min() + 1) {
        throw std::overflow_error("attempt to recover version with overflow");
    }
#endif
    auto recovered_version = std::bit_cast<std::int64_t>(std::uint64_t(1) - std::uint64_t(version));
    auto out = recovered(r, entity, id, version);
    out.old_values = detail::clone_in(r, old_values);
    out.new_values.emplace(old_values ? clone_record(r, *old_values) : Record(r));
    out.new_values->insert_or_assign(String("id", r), Value(r, id));
    out.new_values->insert_or_assign(String("version", r), Value(r, recovered_version));
    String field("version", r);
    changes_for_fields(
        out, old_values ? &*old_values : nullptr, &*out.new_values, std::span(&field, 1));
    return out;
}

RawAuditEvent RawAuditEvent::schema_created(Memory r,
                                            std::string_view entity,
                                            std::string_view table,
                                            std::size_t count) {
    auto out = schema(r, RawAuditEventKind::SchemaCreated, entity, table);
    out.values.emplace("field_count", Value(r, std::bit_cast<std::int64_t>(std::uint64_t(count))));
    all_changes(out, out.values, false);
    return out;
}

RawAuditEvent RawAuditEvent::schema_verified(Memory r,
                                             std::string_view entity,
                                             std::string_view table,
                                             std::size_t count) {
    auto out = schema_created(r, entity, table, count);
    out.kind = RawAuditEventKind::SchemaVerified;
    return out;
}

RawAuditEvent RawAuditEvent::field_added(Memory r,
                                         std::string_view entity,
                                         std::string_view table,
                                         std::string_view field) {
    auto out = schema(r, RawAuditEventKind::FieldAdded, entity, table);
    out.values.emplace("field_name", Value(r, field));
    all_changes(out, out.values, false);
    return out;
}

RawAuditEvent RawAuditEvent::data_seeded(Memory r,
                                         std::string_view entity,
                                         std::string_view table,
                                         std::size_t inserted,
                                         std::size_t updated) {
    auto out = schema(r, RawAuditEventKind::DataSeeded, entity, table);
    out.values.emplace("inserted", Value(r, std::bit_cast<std::int64_t>(std::uint64_t(inserted))));
    out.values.emplace("updated", Value(r, std::bit_cast<std::int64_t>(std::uint64_t(updated))));
    all_changes(out, out.values, false);
    return out;
}

Result<std::monostate, RuntimeError> InMemoryRawAuditEventSink::on_event(
    Memory r, const UserContext& context, const RawAuditEvent& event) const {
    // Each argument can borrow a different scope. Register each distinct arena
    // once; a synchronous callback cannot join its own stack during teardown.
    const std::array domains{
        context.memory_resource(), memory_resource(), event.memory_resource(), r};
    std::array<Arena::Quiescence, 4> active;
    for (std::size_t index = 0; index < domains.size(); ++index) {
        if (std::find(domains.begin(), domains.begin() + index, domains[index]) !=
            domains.begin() + index) {
            continue;
        }
        active[index].attach(detail::resource_arena(domains[index]), nullptr, [](void*) noexcept {
            std::terminate();
        });
    }
    for (const auto& sink : sinks_) {
        auto result = sink.get().on_event(r, context, event);
        if (!result) {
            return result;
        }
    }
    return Result<std::monostate, RuntimeError>::ok(r, {});
}
}
