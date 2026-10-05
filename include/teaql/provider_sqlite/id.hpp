#pragma once
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/id.hpp>

namespace teaql {
inline constexpr std::string_view DEFAULT_ID_SPACE_TABLE = "teaql_id_space";

// Explicitly scoped service. Uses the provider's connection and shares its
// transaction, just as the Rust ID-space generator does. No process counter.
class SqliteIdSpaceGenerator final : public InternalIdGenerator {
public:
    using allocator_type = Allocator;
    SqliteIdSpaceGenerator(std::allocator_arg_t, Allocator, SqliteMutationExecutor&);
    SqliteIdSpaceGenerator(std::allocator_arg_t, Allocator, const SqliteIdSpaceGenerator&);
    SqliteIdSpaceGenerator(const SqliteIdSpaceGenerator&) = delete;

    Memory memory_resource() const noexcept {
        return table_name_.get_allocator().resource();
    }

    SqliteIdSpaceGenerator& with_table_name(std::string_view name) {
        table_name_ = name;
        return *this;
    }

    Result<std::monostate, MutationExecutorError> ensure_table(Memory) const;
    Result<std::uint64_t, MutationExecutorError> next_id(Memory, std::string_view entity) const;
    Result<std::monostate, MutationExecutorError>
    ensure_floor(Memory, std::string_view entity, std::uint64_t floor) const;

    Result<std::uint64_t, RuntimeError> generate_id(Memory r,
                                                    std::string_view entity) const override {
        auto result = next_id(r, entity);
        using R = Result<std::uint64_t, RuntimeError>;
        if (!result)
            return R::err(r, RuntimeError::id_generation(r, result.error().to_string(r)));
        return R::ok(r, result.value());
    }

protected:
    Result<std::monostate, RuntimeError>
    do_ensure_floor(Memory r, std::string_view entity, std::uint64_t floor) const override {
        auto result = ensure_floor(r, entity, floor);
        using R = Result<std::monostate, RuntimeError>;
        if (!result)
            return R::err(r, RuntimeError::id_generation(r, result.error().to_string(r)));
        return R::ok(r, {});
    }

private:
    SqliteMutationExecutor& executor_;
    String table_name_;
};
}
