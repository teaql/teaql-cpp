#pragma once
#include <teaql/data_service/dynamic_fields.hpp>
#include <teaql/runtime/context.hpp>

namespace teaql {
struct DynamicFieldBatch {
    using allocator_type = Allocator;
    ArenaBorrow<DynamicFieldDefinitions> definitions;
    DynamicStorageRows rows;

    DynamicFieldBatch(std::allocator_arg_t,
                      Allocator,
                      const DynamicFieldDefinitions&,
                      const DynamicStorageRows&);
    DynamicFieldBatch(std::allocator_arg_t, Allocator, const DynamicFieldBatch&);

    DynamicFieldBatch(std::allocator_arg_t, Allocator a, DynamicFieldBatch&& other)
        : DynamicFieldBatch(detail::move_to(a.resource(), std::move(other))) {}

    DynamicFieldBatch(DynamicFieldBatch&&) noexcept = default;

    DynamicFieldBatch& operator=(DynamicFieldBatch&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return rows.get_allocator().resource();
    }

    Result<Vector<DynamicFieldValues>, DynamicFieldError>
    into_values(std::string_view owner_type,
                std::span<const std::uint64_t>,
                const DynamicFieldSelection&) &&;
};

class DynamicFieldsProvider {
public:
    virtual ~DynamicFieldsProvider() = default;
    virtual Result<DynamicFieldStreamPlan, DynamicFieldError>
    prepare_stream_read(const UserContext&,
                        const DynamicFieldStore&,
                        std::string_view owner_type,
                        const DynamicFieldSelection&,
                        const QueryIntent&) const;

    virtual bool supports_graph_save() const {
        return false;
    }

    virtual Result<DynamicFieldWrite, DynamicFieldError>
    prepare_graph_write(const UserContext&,
                        const DynamicFieldStore&,
                        const DynamicFieldDefinitions&,
                        std::uint64_t,
                        const DynamicFieldWrite::Changes&) const;
    virtual DynamicStorageFuture<std::monostate>& ensure_schema(const UserContext&) const;
    virtual DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext&,
                                                                 std::string_view owner_type,
                                                                 std::span<const std::uint64_t>,
                                                                 const DynamicFieldSelection&,
                                                                 const QueryIntent&) const = 0;
};

namespace detail {
// Type erasure keeps executor-specific registration separate from storage mechanics.
class DatabaseDynamicProvider : public DynamicFieldsProvider {
public:
    using allocator_type = Allocator;
    using StoreResult = Result<const DynamicFieldStore*, DynamicFieldError>;
    using Resolver = StoreResult (*)(const UserContext&, std::string_view);
    using Definitions = std::pmr::map<String, ArenaBorrow<DynamicFieldDefinitions>, std::less<>>;
    static Result<std::monostate, DynamicFieldError> validate_configuration(
        Memory, std::string_view, std::span<const DynamicFieldDefinitions* const>);
    DatabaseDynamicProvider(std::allocator_arg_t,
                            Allocator,
                            std::string_view,
                            std::span<const DynamicFieldDefinitions* const>,
                            Resolver);
    DatabaseDynamicProvider(std::allocator_arg_t, Allocator, const DatabaseDynamicProvider&);
    DatabaseDynamicProvider(DatabaseDynamicProvider&&) noexcept = default;

    Memory memory_resource() const {
        return namespace_.get_allocator().resource();
    }

    Result<ArenaBorrow<DynamicFieldDefinitions>, DynamicFieldError>
    definitions_for_context(const UserContext&, std::string_view owner_type) const;
    Result<DynamicFieldStreamPlan, DynamicFieldError>
    prepare_stream_read(const UserContext&,
                        const DynamicFieldStore&,
                        std::string_view,
                        const DynamicFieldSelection&,
                        const QueryIntent&) const override;

    bool supports_graph_save() const override {
        return true;
    }

    Result<DynamicFieldWrite, DynamicFieldError>
    prepare_graph_write(const UserContext&,
                        const DynamicFieldStore&,
                        const DynamicFieldDefinitions&,
                        std::uint64_t,
                        const DynamicFieldWrite::Changes&) const override;
    DynamicStorageFuture<std::monostate>& ensure_schema(const UserContext&) const override;
    DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext&,
                                                         std::string_view,
                                                         std::span<const std::uint64_t>,
                                                         const DynamicFieldSelection&,
                                                         const QueryIntent&) const override;

private:
    String namespace_;
    Definitions definitions_;
    Resolver resolve_;
};

Value mutation_audit_value(Memory, const DynamicFieldMutation&, const Value* actual);
}

template <class E>
class DatabaseDynamicFieldsProvider final : public detail::DatabaseDynamicProvider {
public:
    using allocator_type = Allocator;

    static Result<DatabaseDynamicFieldsProvider, DynamicFieldError>
    create(Memory r,
           std::string_view namespace_name,
           std::span<const DynamicFieldDefinitions* const> definitions) {
        auto checked = validate_configuration(r, namespace_name, definitions);
        if (!checked) {
            return Result<DatabaseDynamicFieldsProvider, DynamicFieldError>::err(
                r, std::move(checked).error());
        }
        return Result<DatabaseDynamicFieldsProvider, DynamicFieldError>::ok(
            r, DatabaseDynamicFieldsProvider(r, namespace_name, definitions));
    }

    DatabaseDynamicFieldsProvider(std::allocator_arg_t,
                                  Allocator a,
                                  const DatabaseDynamicFieldsProvider& other)
        : DatabaseDynamicProvider(std::allocator_arg, a, other) {}

    DatabaseDynamicFieldsProvider(std::allocator_arg_t,
                                  Allocator a,
                                  DatabaseDynamicFieldsProvider&& other)
        : DatabaseDynamicFieldsProvider(detail::move_to(a.resource(), std::move(other))) {}

    DatabaseDynamicFieldsProvider(DatabaseDynamicFieldsProvider&&) noexcept = default;

private:
    DatabaseDynamicFieldsProvider(Memory r,
                                  std::string_view namespace_name,
                                  std::span<const DynamicFieldDefinitions* const> definitions)
        : DatabaseDynamicProvider(
              std::allocator_arg,
              Allocator(r),
              namespace_name,
              definitions,
              [](const UserContext& user_context, std::string_view field) {
                  auto r = user_context.memory_resource();
                  auto executor = user_context.template require_resource<E>();
                  if (!executor) {
                      return StoreResult::err(
                          r, DynamicFieldError(r, "DYNAMIC_FIELD_EXECUTOR_MISSING", field));
                  }
                  auto* store = executor.value().get().dynamic_field_store();
                  if (!store) {
                      return StoreResult::err(
                          r, DynamicFieldError(r, "DYNAMIC_FIELD_STORAGE_UNSUPPORTED", field));
                  }
                  return StoreResult::ok(r, store);
              }) {}
};

class InMemoryDynamicFieldsProvider final : public DynamicFieldsProvider {
public:
    using allocator_type = Allocator;

    struct Owner {
        const DynamicFieldDefinitions* definitions;
        const DynamicStorageRows* rows;
    };

    static Result<InMemoryDynamicFieldsProvider, DynamicFieldError>
        from_owners(Memory, std::span<const Owner>);
    InMemoryDynamicFieldsProvider(std::allocator_arg_t,
                                  Allocator,
                                  const InMemoryDynamicFieldsProvider&);

    InMemoryDynamicFieldsProvider(std::allocator_arg_t,
                                  Allocator a,
                                  InMemoryDynamicFieldsProvider&& other)
        : InMemoryDynamicFieldsProvider(detail::move_to(a.resource(), std::move(other))) {}

    InMemoryDynamicFieldsProvider(InMemoryDynamicFieldsProvider&&) noexcept = default;

    Memory memory_resource() const {
        return definitions_.get_allocator().resource();
    }

    DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext&,
                                                         std::string_view,
                                                         std::span<const std::uint64_t>,
                                                         const DynamicFieldSelection&,
                                                         const QueryIntent&) const override;

private:
    explicit InMemoryDynamicFieldsProvider(Memory r) : definitions_(r), values_(r) {}

    detail::DatabaseDynamicProvider::Definitions definitions_;
    std::pmr::map<String, DynamicStorageRows, std::less<>> values_;
};
}
