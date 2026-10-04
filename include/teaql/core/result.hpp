#pragma once
#include <teaql/memory/clone.hpp>
#include <teaql/memory/scoped.hpp>
#include <variant>

namespace teaql {
// C++20 carrier for Rust's Result. Payloads remain in the explicit Context domain.
template<class T, class E> class Result {
public:
    using allocator_type = Allocator;
    static Result ok(Memory r, T value) { return Result(r, std::in_place_index<0>, std::move(value)); }
    static Result err(Memory r, E error) { return Result(r, std::in_place_index<1>, std::move(error)); }
    Result(std::allocator_arg_t, Allocator a, const Result& other)
        : Result(other.is_ok() ? ok(a.resource(), detail::clone_in(a.resource(), other.value()))
                              : err(a.resource(), detail::clone_in(a.resource(), other.error()))) {}
    Result(std::allocator_arg_t, Allocator a, Result&& other) : Result(detail::move_to(a.resource(), std::move(other))) {}
    Result(const Result&) = delete;
    Result(Result&&) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<E>) = default;
    Result& operator=(Result&& other) { return detail::assign_scoped(*this, std::move(other)); }
    bool is_ok() const { return data_.index() == 0; }
    bool is_err() const { return !is_ok(); }
    explicit operator bool() const { return is_ok(); }
    T& value() & { return std::get<0>(data_); }
    const T& value() const& { return std::get<0>(data_); }
    T&& value() && { return std::get<0>(std::move(data_)); }
    E& error() & { return std::get<1>(data_); }
    const E& error() const& { return std::get<1>(data_); }
    E&& error() && { return std::get<1>(std::move(data_)); }
    Memory memory_resource() const { return resource_; }
private:
    template<std::size_t I, class U> Result(Memory r, std::in_place_index_t<I> index, U&& value)
        : resource_(r), data_(index, detail::move_in(r, std::forward<U>(value))) {}
    Memory resource_;
    std::variant<T, E> data_;
};
}
