#pragma once
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/business_id.hpp>

namespace teaql {
inline constexpr std::string_view DEFAULT_BUSINESS_ID_SPACE_TABLE = "teaql_business_id_space";

// Use a separately scoped connection: allocation starts its own short native
// transaction and must not participate in an aggregate save transaction.
class SqliteBusinessIdAllocator final : public BusinessIdAllocator,
                                        public BusinessIdSchemaContributor {
public:
    using allocator_type = Allocator;
    SqliteBusinessIdAllocator(std::allocator_arg_t, Allocator, SqliteMutationExecutor&);
    SqliteBusinessIdAllocator(std::allocator_arg_t, Allocator, const SqliteBusinessIdAllocator&);
    SqliteBusinessIdAllocator(const SqliteBusinessIdAllocator&) = delete;

    Memory memory_resource() const noexcept {
        return table_name_.get_allocator().resource();
    }

    SqliteBusinessIdAllocator& with_table_name(std::string_view name) {
        table_name_ = name;
        return *this;
    }

    Result<std::monostate, BusinessIdError> ensure_schema(Memory) const;

    Result<std::monostate, BusinessIdError> ensure_schema(Memory r,
                                                          const UserContext&) const override {
        return ensure_schema(r);
    }

    Result<BusinessIdAllocation, BusinessIdError> allocate(Memory,
                                                           const BusinessIdPlan&) const override;

private:
    SqliteMutationExecutor& executor_;
    String table_name_;
};
}
