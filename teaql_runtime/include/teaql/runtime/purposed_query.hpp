#pragma once
#include <teaql/core/query.hpp>
#include <teaql/core/request.hpp>
#include <teaql/core/detail/text.hpp>

namespace teaql::detail {
struct EntityDataServiceAccess;
class FacetExecution;

inline bool query_has_facet_diagnostics(const SelectQuery& query) {
    if (!query.facets.empty()) {
        return true;
    }
    for (const auto& relation : query.relations) {
        if (relation.query && query_has_facet_diagnostics(*relation.query)) {
            return true;
        }
    }
    for (const auto& child : query.child_enhancements) {
        if (query_has_facet_diagnostics(child)) {
            return true;
        }
    }
    for (const auto& group : query.object_group_bys) {
        if (query_has_facet_diagnostics(group.query)) {
            return true;
        }
    }
    return false;
}

inline void expand_query_facet_diagnostics(SelectQuery&);

inline SelectQuery facet_diagnostic_source(const request::QuerySelection& selection) {
    auto r = selection.memory_resource();
    auto copy = clone_in(r, selection);
    auto query = std::move(copy).into_query();
    for (const auto& aggregate : selection.query_options.relation_aggregates) {
        query.child_enhancements.emplace_back(facet_diagnostic_source(aggregate.query));
    }
    expand_query_facet_diagnostics(query);
    return query;
}

inline void expand_query_facet_diagnostics(SelectQuery& query) {
    for (const auto& facet : query.facets) {
        query.child_enhancements.emplace_back(facet_diagnostic_source(facet.query));
    }
    query.facets.clear();
    for (auto& relation : query.relations) {
        if (relation.query) {
            expand_query_facet_diagnostics(*relation.query);
        }
    }
    for (auto& child : query.child_enhancements) {
        expand_query_facet_diagnostics(child);
    }
    for (auto& group : query.object_group_bys) {
        expand_query_facet_diagnostics(group.query);
    }
}
}

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
        query_.purpose(purpose);
        // Execution lineage belongs to the runtime boundary, not the core setter.
        query_.trace_chain.emplace_back(TraceKind::Purpose, query_.entity, purpose);
    }

    PurposedSelectQuery(std::allocator_arg_t, Allocator a, const PurposedSelectQuery& other)
        : query_(std::allocator_arg, a, other.query_),
          diagnostic_source_(detail::clone_in(a.resource(), other.diagnostic_source_)) {}

    PurposedSelectQuery(std::allocator_arg_t, Allocator a, PurposedSelectQuery&& other)
        : query_(std::allocator_arg, a, std::move(other.query_)),
          diagnostic_source_(detail::move_in(a.resource(), std::move(other.diagnostic_source_))) {}

    PurposedSelectQuery(const PurposedSelectQuery&) = delete;
    PurposedSelectQuery(PurposedSelectQuery&&) noexcept = default;

    const SelectQuery& as_query() const {
        return query_;
    }

    PurposedSelectQuery& limit(std::uint64_t value) {
        query_.limit(value);
        return *this;
    }

    PurposedSelectQuery& for_exact_count(std::string_view alias) {
        if (!diagnostic_source_) {
            diagnostic_source_.emplace(std::allocator_arg, Allocator(memory_resource()), query_);
        }
        query_.projection.clear();
        query_.expr_projection.clear();
        query_.order_by.clear();
        query_.slice.reset();
        query_.relations.clear();
        query_.count(alias);
        return *this;
    }

    PurposedSelectQuery& with_facet_diagnostics(const request::QueryOptions& options) {
        if (options.facets.empty() && !detail::query_has_facet_diagnostics(query_)) {
            return *this;
        }
        if (!diagnostic_source_) {
            diagnostic_source_.emplace(std::allocator_arg, Allocator(memory_resource()), query_);
        }
        for (const auto& facet : options.facets) {
            diagnostic_source_->child_enhancements.emplace_back(
                detail::facet_diagnostic_source(facet.query));
        }
        detail::expand_query_facet_diagnostics(*diagnostic_source_);
        return *this;
    }

    SelectQuery into_query() && {
        return std::move(query_);
    }

    Memory memory_resource() const {
        return query_.memory_resource();
    }

private:
    friend struct detail::EntityDataServiceAccess;
    friend class detail::FacetExecution;
    friend void write_debug(DebugWriter&, const PurposedSelectQuery&);
    SelectQuery query_;
    std::optional<SelectQuery> diagnostic_source_;
};
}
