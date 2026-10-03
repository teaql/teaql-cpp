#pragma once
#include <teaql/core/value.hpp>
#include <teaql/core/trace.hpp>

namespace teaql {
enum class MutationKind { Insert, Update, Delete, Recover };
// Shared implementation, distinct public command types as in Rust.
class MutationCommand {
public:
    using allocator_type = Allocator;
    String entity;
    Value id;
    MutationValues values;
    TraceChain trace_chain;
    std::optional<std::int64_t> expected_version_value;
    bool soft_delete = true;
    Memory memory_resource() const { return entity.get_allocator().resource(); }
    MutationCommand(const MutationCommand&) = delete;
    MutationCommand& operator=(const MutationCommand&) = delete;
protected:
    MutationCommand(Memory r, std::string_view entity)
        : entity(entity, r), id(r), values(r), trace_chain(r) {}
};
template<class Derived> class CommandBuilder : public MutationCommand {
protected:
    using MutationCommand::MutationCommand;
public:
    template<class T> Derived& value(std::string_view field, T&& value) {
        values.fields.insert_or_assign(String(field, memory_resource()), Value(memory_resource(), std::forward<T>(value)));
        return static_cast<Derived&>(*this);
    }
    Derived& audit_as(std::string_view reason) {
        trace_chain.emplace_back(memory_resource(), TraceKind::AuditReason, entity, reason); return static_cast<Derived&>(*this);
    }
    Derived& expected_version(std::int64_t version) { expected_version_value = version; return static_cast<Derived&>(*this); }
};
struct InsertCommand : CommandBuilder<InsertCommand> {
    InsertCommand(std::allocator_arg_t, Allocator a, std::string_view entity) : CommandBuilder(a.resource(), entity) {}
};
struct UpdateCommand : CommandBuilder<UpdateCommand> {
    template<class T> UpdateCommand(std::allocator_arg_t, Allocator a, std::string_view entity, T&& id)
        : CommandBuilder(a.resource(), entity) { this->id = Value(a.resource(), std::forward<T>(id)); }
};
struct DeleteCommand : CommandBuilder<DeleteCommand> {
    template<class T> DeleteCommand(std::allocator_arg_t, Allocator a, std::string_view entity, T&& id)
        : CommandBuilder(a.resource(), entity) { this->id = Value(a.resource(), std::forward<T>(id)); }
    DeleteCommand& hard_delete() { soft_delete = false; return *this; }
};
struct RecoverCommand : CommandBuilder<RecoverCommand> {
    template<class T> RecoverCommand(std::allocator_arg_t, Allocator a, std::string_view entity, T&& id, std::int64_t version)
        : CommandBuilder(a.resource(), entity) { this->id = Value(a.resource(), std::forward<T>(id)); expected_version_value = version; }
};
}
