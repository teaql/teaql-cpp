#pragma once
#include <teaql/core/query.hpp>
#include <teaql/core/detail/text.hpp>

namespace teaql {
template <class T>
struct PurposedQuery {
    using allocator_type = Allocator;
    T inner;
    String purpose;

    PurposedQuery(std::allocator_arg_t, Allocator a, const T& value, std::string_view purpose)
        : inner(detail::clone_in(a.resource(), value)), purpose(purpose, a.resource()) {}

    PurposedQuery(std::allocator_arg_t, Allocator a, T&& value, std::string_view purpose)
        : inner(detail::move_in(a.resource(), std::move(value))), purpose(purpose, a.resource()) {}

    PurposedQuery(std::allocator_arg_t, Allocator a, const PurposedQuery& other)
        : PurposedQuery(std::allocator_arg, a, other.inner, other.purpose) {}

    PurposedQuery(std::allocator_arg_t, Allocator a, PurposedQuery&& other)
        : inner(detail::move_in(a.resource(), std::move(other.inner))),
          purpose(std::move(other.purpose), a.resource()) {}

    PurposedQuery(const PurposedQuery&) = delete;
    PurposedQuery(PurposedQuery&&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

    Memory memory_resource() const {
        return purpose.get_allocator().resource();
    }
};

// The runtime's public select boundary, independent of the application generator.
class PurposedSelectQuery {
public:
    using allocator_type = Allocator;

    PurposedSelectQuery(std::allocator_arg_t,
                        Allocator a,
                        const SelectQuery& query,
                        std::string_view purpose)
        : query_(std::allocator_arg, a, query) {
        auto trimmed = purpose;
        detail::trim(trimmed);
        if (trimmed.empty()) {
            throw std::invalid_argument("query purpose must not be empty");
        }
        query_.trace_chain.emplace_back(TraceKind::Purpose, query_.entity, purpose);
    }

    PurposedSelectQuery(std::allocator_arg_t, Allocator a, const PurposedSelectQuery& other)
        : query_(std::allocator_arg, a, other.query_) {}

    PurposedSelectQuery(std::allocator_arg_t, Allocator a, PurposedSelectQuery&& other)
        : query_(std::allocator_arg, a, std::move(other.query_)) {}

    PurposedSelectQuery(const PurposedSelectQuery&) = delete;
    PurposedSelectQuery(PurposedSelectQuery&&) noexcept = default;

    const SelectQuery& as_query() const {
        return query_;
    }

    SelectQuery into_query() && {
        return std::move(query_);
    }

    Memory memory_resource() const {
        return query_.memory_resource();
    }

private:
    SelectQuery query_;
};
}
