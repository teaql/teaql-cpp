#pragma once
#include <teaql/core/value.hpp>
#include <teaql/core/trace.hpp>
#include <teaql/memory/clone.hpp>

namespace teaql {
// Distinct input, provider-output and persisted-snapshot types, as in Rust mutation.rs.
struct MutationValues {
    using allocator_type = Allocator;
    Record fields;
    explicit MutationValues(Memory r) : fields(r) {}
    MutationValues(Memory r, const MutationValues& other) : fields(clone_record(r, other.fields)) {}
    MutationValues(std::allocator_arg_t, Allocator a) : MutationValues(a.resource()) {}
    MutationValues(std::allocator_arg_t, Allocator a, const MutationValues& other) : MutationValues(a.resource(), other) {}
    MutationValues(std::allocator_arg_t, Allocator a, MutationValues&& other) : MutationValues(detail::move_to(a.resource(), std::move(other))) {}
    Memory memory_resource() const { return fields.get_allocator().resource(); }
    MutationValues& operator=(MutationValues&& other) { return detail::assign_scoped(*this, std::move(other)); }
    MutationValues(const MutationValues&) = delete;
    MutationValues(MutationValues&&) noexcept = default;
};
struct EntitySnapshot {
    using allocator_type = Allocator;
    Record fields;
    explicit EntitySnapshot(Memory r) : fields(r) {}
    EntitySnapshot(Memory r, const EntitySnapshot& other) : fields(clone_record(r, other.fields)) {}
    EntitySnapshot(std::allocator_arg_t, Allocator a) : EntitySnapshot(a.resource()) {}
    EntitySnapshot(std::allocator_arg_t, Allocator a, const EntitySnapshot& other) : EntitySnapshot(a.resource(), other) {}
    EntitySnapshot(std::allocator_arg_t, Allocator a, EntitySnapshot&& other) : EntitySnapshot(detail::move_to(a.resource(), std::move(other))) {}
    Memory memory_resource() const { return fields.get_allocator().resource(); }
    EntitySnapshot& operator=(EntitySnapshot&& other) { return detail::assign_scoped(*this, std::move(other)); }
    EntitySnapshot(const EntitySnapshot&) = delete;
    EntitySnapshot(EntitySnapshot&&) noexcept = default;
};
struct GeneratedValues {
    using allocator_type = Allocator;
    Record fields;
    explicit GeneratedValues(Memory r) : fields(r) {}
    GeneratedValues(Memory r, const GeneratedValues& other) : fields(clone_record(r, other.fields)) {}
    GeneratedValues(std::allocator_arg_t, Allocator a) : GeneratedValues(a.resource()) {}
    GeneratedValues(std::allocator_arg_t, Allocator a, const GeneratedValues& other) : GeneratedValues(a.resource(), other) {}
    GeneratedValues(std::allocator_arg_t, Allocator a, GeneratedValues&& other) : GeneratedValues(detail::move_to(a.resource(), std::move(other))) {}
    Memory memory_resource() const { return fields.get_allocator().resource(); }
    GeneratedValues& operator=(GeneratedValues&& other) { return detail::assign_scoped(*this, std::move(other)); }
    GeneratedValues(const GeneratedValues&) = delete;
    GeneratedValues(GeneratedValues&&) noexcept = default;
};
enum class MutationKind { Insert, Update, Delete, Recover };

struct InsertCommand {
    using allocator_type = Allocator;
    String entity;
    MutationValues values;
    TraceChain trace_chain;
    InsertCommand(std::allocator_arg_t, Allocator a, std::string_view name)
        : entity(name, a.resource()), values(a.resource()), trace_chain(a.resource()) {}
    InsertCommand(std::allocator_arg_t, Allocator a, const InsertCommand& other)
        : entity(other.entity, a.resource()), values(a.resource(), other.values), trace_chain(other.trace_chain, a.resource()) {}
    InsertCommand(std::allocator_arg_t, Allocator a, InsertCommand&& other) : InsertCommand(detail::move_to(a.resource(), std::move(other))) {}
    InsertCommand(const InsertCommand&) = delete;
    InsertCommand(InsertCommand&&) noexcept = default;
    InsertCommand& operator=(InsertCommand&& other) { return detail::assign_scoped(*this, std::move(other)); }
    Memory memory_resource() const { return entity.get_allocator().resource(); }
    template<class T> InsertCommand& value(std::string_view field, T&& v) {
        values.fields.insert_or_assign(String(field, memory_resource()), Value(memory_resource(), std::forward<T>(v))); return *this;
    }
    InsertCommand& audit_as(std::string_view reason) {
        trace_chain.emplace_back(TraceKind::AuditReason, entity, reason); return *this;
    }
};
struct UpdateCommand {
    using allocator_type = Allocator;
    String entity;
    Value id;
    std::optional<std::int64_t> expected_version_value;
    MutationValues values;
    TraceChain trace_chain;
    std::optional<EntitySnapshot> old_values;
    template<class T> UpdateCommand(std::allocator_arg_t, Allocator a, std::string_view name, T&& key)
        : entity(name, a.resource()), id(a.resource(), std::forward<T>(key)), values(a.resource()), trace_chain(a.resource()) {}
    UpdateCommand(std::allocator_arg_t, Allocator a, const UpdateCommand& other)
        : entity(other.entity, a.resource()), id(a.resource(), other.id), expected_version_value(other.expected_version_value), values(a.resource(), other.values), trace_chain(other.trace_chain, a.resource()), old_values(detail::clone_in(a.resource(), other.old_values)) {}
    UpdateCommand(std::allocator_arg_t, Allocator a, UpdateCommand&& other) : UpdateCommand(detail::move_to(a.resource(), std::move(other))) {}
    UpdateCommand(const UpdateCommand&) = delete;
    UpdateCommand(UpdateCommand&&) noexcept = default;
    UpdateCommand& operator=(UpdateCommand&& other) { return detail::assign_scoped(*this, std::move(other)); }
    Memory memory_resource() const { return entity.get_allocator().resource(); }
    template<class T> UpdateCommand& value(std::string_view field, T&& v) {
        values.fields.insert_or_assign(String(field, memory_resource()), Value(memory_resource(), std::forward<T>(v))); return *this;
    }
    UpdateCommand& expected_version(std::int64_t value) { expected_version_value = value; return *this; }
    UpdateCommand& audit_as(std::string_view reason) {
        trace_chain.emplace_back(TraceKind::AuditReason, entity, reason); return *this;
    }
};
struct DeleteCommand {
    using allocator_type = Allocator;
    String entity;
    Value id;
    std::optional<std::int64_t> expected_version_value;
    bool soft_delete = true;
    TraceChain trace_chain;
    template<class T> DeleteCommand(std::allocator_arg_t, Allocator a, std::string_view name, T&& key)
        : entity(name, a.resource()), id(a.resource(), std::forward<T>(key)), trace_chain(a.resource()) {}
    DeleteCommand(std::allocator_arg_t, Allocator a, const DeleteCommand& other)
        : entity(other.entity, a.resource()), id(a.resource(), other.id), expected_version_value(other.expected_version_value), soft_delete(other.soft_delete), trace_chain(other.trace_chain, a.resource()) {}
    DeleteCommand(std::allocator_arg_t, Allocator a, DeleteCommand&& other) : DeleteCommand(detail::move_to(a.resource(), std::move(other))) {}
    DeleteCommand(const DeleteCommand&) = delete;
    DeleteCommand(DeleteCommand&&) noexcept = default;
    DeleteCommand& operator=(DeleteCommand&& other) { return detail::assign_scoped(*this, std::move(other)); }
    Memory memory_resource() const { return entity.get_allocator().resource(); }
    DeleteCommand& expected_version(std::int64_t value) { expected_version_value = value; return *this; }
    DeleteCommand& hard_delete() { soft_delete = false; return *this; }
    DeleteCommand& audit_as(std::string_view reason) {
        trace_chain.emplace_back(TraceKind::AuditReason, entity, reason); return *this;
    }
};
struct RecoverCommand {
    using allocator_type = Allocator;
    String entity;
    Value id;
    std::int64_t expected_version;
    TraceChain trace_chain;
    template<class T> RecoverCommand(std::allocator_arg_t, Allocator a, std::string_view name, T&& key, std::int64_t version)
        : entity(name, a.resource()), id(a.resource(), std::forward<T>(key)), expected_version(version), trace_chain(a.resource()) {}
    RecoverCommand(std::allocator_arg_t, Allocator a, const RecoverCommand& other)
        : entity(other.entity, a.resource()), id(a.resource(), other.id), expected_version(other.expected_version), trace_chain(other.trace_chain, a.resource()) {}
    RecoverCommand(std::allocator_arg_t, Allocator a, RecoverCommand&& other) : RecoverCommand(detail::move_to(a.resource(), std::move(other))) {}
    RecoverCommand(const RecoverCommand&) = delete;
    RecoverCommand(RecoverCommand&&) noexcept = default;
    RecoverCommand& operator=(RecoverCommand&& other) { return detail::assign_scoped(*this, std::move(other)); }
    Memory memory_resource() const { return entity.get_allocator().resource(); }
    RecoverCommand& audit_as(std::string_view reason) {
        trace_chain.emplace_back(TraceKind::AuditReason, entity, reason); return *this;
    }
};
struct BatchInsertCommand {
    using allocator_type = Allocator;
    String entity;
    Vector<MutationValues> batch_values;
    Vector<TraceChain> trace_chains;
    BatchInsertCommand(std::allocator_arg_t, Allocator a, std::string_view name)
        : entity(name, a.resource()), batch_values(a.resource()), trace_chains(a.resource()) {}
};
struct BatchUpdateCommand {
    using allocator_type = Allocator;
    String entity;
    Vector<Value> batch_ids;
    Vector<std::optional<std::int64_t>> batch_expected_versions;
    Vector<MutationValues> batch_values;
    Vector<String> update_fields;
    Vector<TraceChain> trace_chains;
    Vector<std::optional<EntitySnapshot>> batch_old_values;
    BatchUpdateCommand(std::allocator_arg_t, Allocator a, std::string_view name,
                       std::initializer_list<std::string_view> fields = {})
        : entity(name, a.resource()), batch_ids(a.resource()), batch_expected_versions(a.resource()),
          batch_values(a.resource()), update_fields(a.resource()), trace_chains(a.resource()), batch_old_values(a.resource()) {
        for (auto field : fields) update_fields.emplace_back(field);
    }
};
}
