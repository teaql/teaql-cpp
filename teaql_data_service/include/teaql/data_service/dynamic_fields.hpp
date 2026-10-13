#pragma once
#include <teaql/core/dynamic_fields.hpp>
#include <teaql/core/request_intent.hpp>
#include <teaql/core/row.hpp>
#include <teaql/async/operation.hpp>

namespace teaql {
using DynamicStorageRows = std::pmr::map<std::uint64_t, Value::Object>;
template <class T>
using DynamicStorageFuture = AsyncOperation<Result<T, DynamicFieldError>>;

// Framework storage intent, separate from native SQL columns. The namespace is
// trusted provider configuration; it is never a request-selected tenant or table.
class DynamicFieldWrite {
public:
    using allocator_type = Allocator;
    using Changes = std::pmr::map<String, DynamicFieldMutation, std::less<>>;
    DynamicFieldWrite(std::allocator_arg_t,
                      Allocator,
                      std::string_view namespace_name,
                      const DynamicFieldDefinitions&,
                      std::uint64_t owner_id,
                      const Changes&);
    DynamicFieldWrite(std::allocator_arg_t, Allocator, const DynamicFieldWrite&);

    DynamicFieldWrite(std::allocator_arg_t, Allocator a, DynamicFieldWrite&& other)
        : DynamicFieldWrite(detail::move_to(a.resource(), std::move(other))) {}

    DynamicFieldWrite(DynamicFieldWrite&&) noexcept = default;

    DynamicFieldWrite& operator=(DynamicFieldWrite&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    String namespace_name; // 'namespace' is a reserved C++ keyword.
    ArenaBorrow<DynamicFieldDefinitions> definitions;
    std::uint64_t owner_id;
    Changes changes;

    Memory memory_resource() const {
        return namespace_name.get_allocator().resource();
    }
};

// Executor-owned low-level SPI. Writes require the exclusive native transaction
// store; async operations must obey the normal Context quiescence contract.
class DynamicFieldStore {
public:
    virtual ~DynamicFieldStore() = default;
    virtual bool is_transaction_bound() const = 0;
    virtual std::uintptr_t binding_key() const = 0;

    virtual std::optional<std::uint64_t> source_identity() const {
        return {};
    }

    virtual DynamicStorageFuture<std::monostate>& ensure_schema(Memory) const = 0;
    virtual DynamicStorageFuture<DynamicStorageRows>&
    load_values(Memory,
                std::string_view namespace_name,
                const DynamicFieldDefinitions&,
                std::span<const std::uint64_t> ids,
                const DynamicFieldSelection&,
                const QueryIntent&) const = 0;
    virtual Result<std::monostate, DynamicFieldError>
        validate_writes(Memory, std::span<const DynamicFieldWrite>) const = 0;
    virtual DynamicStorageFuture<std::monostate>&
    apply_writes(Memory, std::span<const DynamicFieldWrite>, const MutationIntent&) const = 0;
};

// Trusted cursor-local enhancement plan. Comparison identities are opaque and
// deliberately omitted from public serialization and diagnostic interfaces.
class DynamicFieldStreamPlan {
public:
    using allocator_type = Allocator;
    static Result<DynamicFieldStreamPlan, DynamicFieldError> create(Memory,
                                                                    std::string_view namespace_name,
                                                                    const DynamicFieldDefinitions&,
                                                                    const DynamicFieldSelection&,
                                                                    const DynamicFieldStore&,
                                                                    const QueryIntent&);
    DynamicFieldStreamPlan(std::allocator_arg_t, Allocator, const DynamicFieldStreamPlan&);

    DynamicFieldStreamPlan(std::allocator_arg_t, Allocator a, DynamicFieldStreamPlan&& other)
        : DynamicFieldStreamPlan(detail::move_to(a.resource(), std::move(other))) {}

    DynamicFieldStreamPlan(DynamicFieldStreamPlan&&) noexcept = default;

    DynamicFieldStreamPlan& operator=(DynamicFieldStreamPlan&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    std::string_view storage_namespace() const {
        return namespace_;
    }

    const DynamicFieldDefinitions& definitions() const {
        return definitions_.get();
    }

    const DynamicFieldSelection& selection() const {
        return selection_;
    }

    Result<std::monostate, DynamicFieldError> validate_store(const DynamicFieldStore&) const;
    Result<Vector<std::uint64_t>, DynamicFieldError> owner_ids(std::span<const CompactRow>) const;
    Result<std::monostate, DynamicFieldError>
    merge(std::span<CompactRow>, std::span<const std::uint64_t>, const DynamicStorageRows&) const;

    Memory memory_resource() const {
        return namespace_.get_allocator().resource();
    }

private:
    DynamicFieldStreamPlan(Memory,
                           std::string_view,
                           ArenaBorrow<DynamicFieldDefinitions>,
                           const DynamicFieldSelection&,
                           std::uintptr_t,
                           std::uint64_t,
                           const QueryIntent&);
    String namespace_;
    ArenaBorrow<DynamicFieldDefinitions> definitions_;
    DynamicFieldSelection selection_;
    std::uintptr_t binding_key_;
    std::uint64_t source_identity_;
    QueryIntent intent_;
};
}
