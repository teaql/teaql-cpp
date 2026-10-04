#pragma once
#include <teaql/memory/types.hpp>
#include <optional>
#include <type_traits>

namespace teaql::detail {
template<class T> struct DomainCopy {
    static T copy(Memory r, const T& value) {
        static_assert(std::uses_allocator_v<T, Allocator> || std::is_trivially_destructible_v<T>,
                      "Context values must propagate their allocator or have no owned allocations");
        return std::make_obj_using_allocator<T>(Allocator(r), value);
    }
    static T move(Memory r, T&& value) {
        static_assert(std::uses_allocator_v<T, Allocator> || std::is_trivially_destructible_v<T>,
                      "Context values must propagate their allocator or have no owned allocations");
        return std::make_obj_using_allocator<T>(Allocator(r), std::move(value));
    }
};
template<class T> struct DomainCopy<std::optional<T>> {
    static std::optional<T> copy(Memory r, const std::optional<T>& value) {
        if (!value) return {};
        return DomainCopy<T>::copy(r, *value);
    }
    static std::optional<T> move(Memory r, std::optional<T>&& value) {
        if (!value) return {};
        return DomainCopy<T>::move(r, std::move(*value));
    }
};
template<class T> T clone_in(Memory r, const T& value) { return DomainCopy<T>::copy(r, value); }
template<class T> T move_in(Memory r, T&& value) { return DomainCopy<T>::move(r, std::move(value)); }
}
