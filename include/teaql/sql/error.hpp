#pragma once
#include <teaql/sql/types.hpp>

namespace teaql {
// Keep compile, typed transport, and persisted-record failures distinct.
// E follows the same allocator-aware error contract as the data-service SPI.
template <class E>
class SqlExecutorError {
public:
    using allocator_type = Allocator;
    enum class Kind { Compile, Transport, PersistedRecord };

    static SqlExecutorError compile(Memory r, SqlCompileError error) {
        return SqlExecutorError(r, std::in_place_index<0>, std::move(error));
    }

    static SqlExecutorError transport(Memory r, E error) {
        return SqlExecutorError(r, std::in_place_index<1>, std::move(error));
    }

    static SqlExecutorError persisted_record(Memory r, std::string_view message) {
        return SqlExecutorError(r, std::in_place_index<2>, String(message, r));
    }

    SqlExecutorError(std::allocator_arg_t, Allocator a, const SqlExecutorError& other)
        : resource_(a.resource()), data_(clone(a.resource(), other.data_)) {}

    SqlExecutorError(std::allocator_arg_t, Allocator a, SqlExecutorError&& other)
        : SqlExecutorError(detail::move_to(a.resource(), std::move(other))) {}

    SqlExecutorError(const SqlExecutorError&) = delete;
    SqlExecutorError(SqlExecutorError&&) noexcept(std::is_nothrow_move_constructible_v<E>) =
        default;

    SqlExecutorError& operator=(SqlExecutorError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Kind kind() const noexcept {
        return Kind(data_.index());
    }

    const SqlCompileError& compile_error() const {
        return std::get<0>(data_);
    }

    const E& transport_error() const {
        return std::get<1>(data_);
    }

    std::string_view persisted_record_error() const {
        return std::get<2>(data_);
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

    // C++ typed errors may supply to_string(Memory), what(), or be text values.
    // Custom error enums can use the explicit formatter overload.
    String to_string(Memory r) const {
        auto format = [&](const E& e) -> String {
            if constexpr (requires { e.to_string(r); })
                return String(e.to_string(r), r);
            else if constexpr (requires { e.what(); })
                return String(e.what(), r);
            else if constexpr (std::is_convertible_v<const E&, std::string_view>)
                return String(std::string_view(e), r);
            else {
                static_assert(sizeof(E) == 0, "Transport error needs a display formatter");
            }
        };
        return to_string(r, format);
    }

    String to_string(Memory r, FunctionRef<String(const E&)> format) const {
        switch (kind()) {
        case Kind::Compile: {
            String out("SQL compile error: ", r);
            out += compile_error().to_string(r);
            return out;
        }
        case Kind::Transport: {
            String out("Transport error: ", r);
            out += format(transport_error());
            return out;
        }
        case Kind::PersistedRecord: {
            String out("Persisted record error: ", r);
            out += persisted_record_error();
            return out;
        }
        }
        throw std::logic_error("Invalid SQL executor error kind");
    }

private:
    using Storage = std::variant<SqlCompileError, E, String>;

    static Storage clone(Memory r, const Storage& other) {
        switch (other.index()) {
        case 0:
            return Storage(std::in_place_index<0>, detail::clone_in(r, std::get<0>(other)));
        case 1:
            return Storage(std::in_place_index<1>, detail::clone_in(r, std::get<1>(other)));
        default:
            return Storage(std::in_place_index<2>, String(std::get<2>(other), r));
        }
    }

    template <std::size_t I, class T>
    SqlExecutorError(Memory r, std::in_place_index_t<I> index, T&& value)
        : resource_(r), data_(index, detail::move_in(r, std::forward<T>(value))) {}

    Memory resource_;
    Storage data_;
};
}
