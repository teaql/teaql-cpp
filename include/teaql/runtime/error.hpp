#pragma once
#include <teaql/runtime/check_result.hpp>
#include <teaql/runtime/context_error.hpp>
#include <teaql/sql/types.hpp>
#include <teaql/core/entity.hpp>

namespace teaql {
struct RuntimeMissingRelation {
    using allocator_type = Allocator;
    String entity, relation;

    RuntimeMissingRelation(std::allocator_arg_t,
                           Allocator a,
                           std::string_view entity,
                           std::string_view relation)
        : entity(entity, a.resource()), relation(relation, a.resource()) {}

    RuntimeMissingRelation(std::allocator_arg_t, Allocator a, const RuntimeMissingRelation& other)
        : RuntimeMissingRelation(std::allocator_arg, a, other.entity, other.relation) {}

    RuntimeMissingRelation(std::allocator_arg_t, Allocator a, RuntimeMissingRelation&& other)
        : RuntimeMissingRelation(detail::move_to(a.resource(), std::move(other))) {}

    RuntimeMissingRelation(const RuntimeMissingRelation&) = delete;
    RuntimeMissingRelation(RuntimeMissingRelation&&) noexcept = default;

    RuntimeMissingRelation& operator=(RuntimeMissingRelation&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entity.get_allocator().resource();
    }
};

struct RuntimeOptimisticLockConflict {
    using allocator_type = Allocator;
    String entity, id;

    RuntimeOptimisticLockConflict(std::allocator_arg_t,
                                  Allocator a,
                                  std::string_view entity,
                                  std::string_view id)
        : entity(entity, a.resource()), id(id, a.resource()) {}

    RuntimeOptimisticLockConflict(std::allocator_arg_t,
                                  Allocator a,
                                  const RuntimeOptimisticLockConflict& other)
        : RuntimeOptimisticLockConflict(std::allocator_arg, a, other.entity, other.id) {}

    RuntimeOptimisticLockConflict(std::allocator_arg_t,
                                  Allocator a,
                                  RuntimeOptimisticLockConflict&& other)
        : RuntimeOptimisticLockConflict(detail::move_to(a.resource(), std::move(other))) {}

    RuntimeOptimisticLockConflict(const RuntimeOptimisticLockConflict&) = delete;
    RuntimeOptimisticLockConflict(RuntimeOptimisticLockConflict&&) noexcept = default;

    RuntimeOptimisticLockConflict& operator=(RuntimeOptimisticLockConflict&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entity.get_allocator().resource();
    }
};

class RuntimeError {
public:
    using allocator_type = Allocator;
    enum class Kind {
        MissingEntity,
        SqlCompile,
        Behavior,
        Event,
        Policy,
        Check,
        Graph,
        IdGeneration,
        Language,
        UnsupportedLocale,
        Schema,
        Transaction,
        MissingRelation,
        OptimisticLockConflict
    };

    static RuntimeError missing_entity(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::MissingEntity, String(message, r));
    }

    static RuntimeError behavior(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::Behavior, String(message, r));
    }

    static RuntimeError event(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::Event, String(message, r));
    }

    static RuntimeError policy(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::Policy, String(message, r));
    }

    static RuntimeError graph(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::Graph, String(message, r));
    }

    static RuntimeError id_generation(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::IdGeneration, String(message, r));
    }

    static RuntimeError language(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::Language, String(message, r));
    }

    static RuntimeError unsupported_locale(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::UnsupportedLocale, String(message, r));
    }

    static RuntimeError schema(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::Schema, String(message, r));
    }

    static RuntimeError transaction(Memory r, std::string_view message) {
        return RuntimeError(r, Kind::Transaction, String(message, r));
    }

    static RuntimeError sql_compile(Memory r, SqlCompileError error) {
        return RuntimeError(r, Kind::SqlCompile, std::move(error));
    }

    static RuntimeError check(Memory r, CheckResults errors) {
        return RuntimeError(r, Kind::Check, std::move(errors));
    }

    static RuntimeError
    missing_relation(Memory r, std::string_view entity, std::string_view relation) {
        return RuntimeError(
            r,
            Kind::MissingRelation,
            RuntimeMissingRelation(std::allocator_arg, Allocator(r), entity, relation));
    }

    static RuntimeError
    optimistic_lock_conflict(Memory r, std::string_view entity, std::string_view id) {
        return RuntimeError(
            r,
            Kind::OptimisticLockConflict,
            RuntimeOptimisticLockConflict(std::allocator_arg, Allocator(r), entity, id));
    }

    RuntimeError(std::allocator_arg_t, Allocator a, const RuntimeError& other)
        : resource_(a.resource()), kind_(other.kind_),
          data_(std::visit(
              [&](const auto& value) -> Storage {
                  return detail::clone_in(a.resource(), value);
              },
              other.data_)) {}

    RuntimeError(std::allocator_arg_t, Allocator a, RuntimeError&& other)
        : RuntimeError(detail::move_to(a.resource(), std::move(other))) {}

    RuntimeError(const RuntimeError&) = delete;
    RuntimeError(RuntimeError&&) noexcept = default;

    RuntimeError& operator=(RuntimeError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Kind kind() const noexcept {
        return kind_;
    }

    std::string_view message() const {
        return std::get<String>(data_);
    }

    const SqlCompileError& sql_compile_error() const {
        return std::get<SqlCompileError>(data_);
    }

    const CheckResults& check_results() const {
        return std::get<CheckResults>(data_);
    }

    const RuntimeMissingRelation& missing_relation_error() const {
        return std::get<RuntimeMissingRelation>(data_);
    }

    const RuntimeOptimisticLockConflict& optimistic_lock_conflict_error() const {
        return std::get<RuntimeOptimisticLockConflict>(data_);
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

    String to_string(Memory) const;

    // The pinned Error implementation uses the default empty source chain.
    const std::exception* source() const noexcept {
        return nullptr;
    }

private:
    using Storage = std::variant<String,
                                 SqlCompileError,
                                 CheckResults,
                                 RuntimeMissingRelation,
                                 RuntimeOptimisticLockConflict>;

    template <class T>
    RuntimeError(Memory r, Kind kind, T&& value)
        : resource_(r), kind_(kind), data_(detail::move_in(r, std::forward<T>(value))) {}

    Memory resource_;
    Kind kind_;
    Storage data_;
};

// Preserve the executor's concrete error type and its own display contract.
template <class E>
class DataServiceError {
public:
    using allocator_type = Allocator;
    enum class Kind { Runtime, Entity, Executor };

    static DataServiceError runtime(Memory r, RuntimeError error) {
        return DataServiceError(r, std::in_place_index<0>, std::move(error));
    }

    static DataServiceError entity(Memory r, EntityError error) {
        return DataServiceError(r, std::in_place_index<1>, std::move(error));
    }

    static DataServiceError executor(Memory r, E error) {
        return DataServiceError(r, std::in_place_index<2>, std::move(error));
    }

    DataServiceError(std::allocator_arg_t, Allocator a, const DataServiceError& other)
        requires(detail::domain_copyable<E>)
        : resource_(a.resource()), data_(clone(a.resource(), other.data_)) {}

    DataServiceError(std::allocator_arg_t, Allocator a, DataServiceError&& other)
        : DataServiceError(detail::move_to(a.resource(), std::move(other))) {}

    DataServiceError(const DataServiceError&) = delete;
    DataServiceError(DataServiceError&&) noexcept(std::is_nothrow_move_constructible_v<E>) =
        default;

    DataServiceError& operator=(DataServiceError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Kind kind() const noexcept {
        return Kind(data_.index());
    }

    const RuntimeError& runtime_error() const {
        return std::get<0>(data_);
    }

    const EntityError& entity_error() const {
        return std::get<1>(data_);
    }

    const E& executor_error() const {
        return std::get<2>(data_);
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

    String to_string(Memory r) const {
        auto format = [&](const E& e) -> String {
            if constexpr (requires { e.to_string(r); }) {
                return String(e.to_string(r), r);
            } else if constexpr (requires { e.what(); }) {
                return String(e.what(), r);
            } else if constexpr (std::is_convertible_v<const E&, std::string_view>) {
                return String(std::string_view(e), r);
            } else {
                static_assert(sizeof(E) == 0, "Executor error needs a display formatter");
            }
        };
        return to_string(r, format);
    }

    const std::exception* source() const noexcept {
        return nullptr;
    }

    String to_string(Memory r, FunctionRef<String(const E&)> format) const {
        switch (kind()) {
        case Kind::Runtime:
            return runtime_error().to_string(r);
        case Kind::Entity:
            return entity_error().to_string(r);
        case Kind::Executor:
            return String(format(executor_error()), r);
        }
        throw std::logic_error("Invalid data service error kind");
    }

private:
    using Storage = std::variant<RuntimeError, EntityError, E>;

    static Storage clone(Memory r, const Storage& data) {
        switch (data.index()) {
        case 0:
            return Storage(std::in_place_index<0>, detail::clone_in(r, std::get<0>(data)));
        case 1:
            return Storage(std::in_place_index<1>, detail::clone_in(r, std::get<1>(data)));
        default:
            return Storage(std::in_place_index<2>, detail::clone_in(r, std::get<2>(data)));
        }
    }

    template <std::size_t I, class T>
    DataServiceError(Memory r, std::in_place_index_t<I> index, T&& value)
        : resource_(r), data_(index, detail::move_in(r, std::forward<T>(value))) {}

    Memory resource_;
    Storage data_;
};
}
