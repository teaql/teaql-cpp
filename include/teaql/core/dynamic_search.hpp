#pragma once
#include <teaql/core/query.hpp>
#include <teaql/core/result.hpp>
#include <teaql/memory/function_ref.hpp>
#include <exception>

namespace teaql::dynamic_search {
struct SearchModel {
    using allocator_type = Allocator;
    std::pmr::map<String, String, std::less<>> fields, relations;

    SearchModel(std::allocator_arg_t, Allocator a)
        : fields(a.resource()), relations(a.resource()) {}

    SearchModel(std::allocator_arg_t, Allocator a, const SearchModel& other)
        : fields(other.fields, a.resource()), relations(other.relations, a.resource()) {}

    SearchModel(const SearchModel&) = delete;
    SearchModel(SearchModel&&) noexcept = default;
};

using SearchModels = std::pmr::map<String, SearchModel, std::less<>>;

struct DynamicSearchWarning {
    using allocator_type = Allocator;
    String code, entity, clause, field_path;
    DynamicSearchWarning(std::allocator_arg_t,
                         Allocator,
                         std::string_view entity,
                         std::string_view clause,
                         std::string_view path);
    DynamicSearchWarning(std::allocator_arg_t, Allocator, const DynamicSearchWarning&);
    DynamicSearchWarning(const DynamicSearchWarning&) = delete;
    DynamicSearchWarning(DynamicSearchWarning&&) noexcept = default;
    bool operator==(const DynamicSearchWarning&) const = default;
    Json to_json(Memory) const;
};

struct DynamicSearchFilter {
    using allocator_type = Allocator;
    String field_path, operator_name; // Rust field `operator` is a C++ keyword.
    Json value;
    DynamicSearchFilter(
        std::allocator_arg_t, Allocator, std::string_view, std::string_view, const Json&);
    DynamicSearchFilter(std::allocator_arg_t, Allocator, const DynamicSearchFilter&);
    DynamicSearchFilter(const DynamicSearchFilter&) = delete;
    DynamicSearchFilter(DynamicSearchFilter&&) noexcept = default;
    bool operator==(const DynamicSearchFilter&) const = default;
};

struct DynamicSearchOrder {
    using allocator_type = Allocator;
    String field_path;
    bool descending;
    DynamicSearchOrder(std::allocator_arg_t, Allocator, std::string_view, bool);
    DynamicSearchOrder(std::allocator_arg_t, Allocator, const DynamicSearchOrder&);
    DynamicSearchOrder(const DynamicSearchOrder&) = delete;
    DynamicSearchOrder(DynamicSearchOrder&&) noexcept = default;
    bool operator==(const DynamicSearchOrder&) const = default;
};

struct NormalizedDynamicSearch {
    using allocator_type = Allocator;
    Vector<DynamicSearchFilter> filters;
    Vector<DynamicSearchOrder> orders;
    Vector<DynamicSearchWarning> warnings;
    NormalizedDynamicSearch(std::allocator_arg_t, Allocator);
    NormalizedDynamicSearch(std::allocator_arg_t, Allocator, const NormalizedDynamicSearch&);

    NormalizedDynamicSearch(std::allocator_arg_t, Allocator a, NormalizedDynamicSearch&& other)
        : NormalizedDynamicSearch(detail::move_to(a.resource(), std::move(other))) {}

    NormalizedDynamicSearch(const NormalizedDynamicSearch&) = delete;
    NormalizedDynamicSearch(NormalizedDynamicSearch&&) noexcept = default;

    Memory memory_resource() const {
        return filters.get_allocator().resource();
    }
};

struct DynamicSearchResult {
    using allocator_type = Allocator;
    SelectQuery query;
    Vector<DynamicSearchWarning> warnings;
    DynamicSearchResult(std::allocator_arg_t,
                        Allocator,
                        const SelectQuery&,
                        std::span<const DynamicSearchWarning>);
    DynamicSearchResult(std::allocator_arg_t, Allocator, const DynamicSearchResult&);

    DynamicSearchResult(std::allocator_arg_t, Allocator a, DynamicSearchResult&& other)
        : DynamicSearchResult(detail::move_to(a.resource(), std::move(other))) {}

    DynamicSearchResult(const DynamicSearchResult&) = delete;
    DynamicSearchResult(DynamicSearchResult&&) noexcept = default;

    Memory memory_resource() const {
        return query.memory_resource();
    }
};

struct DynamicSearchError {
    std::string_view message; // Static diagnostic text only, never submitted values.

    std::string_view to_string() const {
        return message;
    }

    const std::exception* source() const noexcept {
        return nullptr;
    }

    bool operator==(const DynamicSearchError&) const = default;
};
template <class T>
using SearchResult = teaql::Result<T, DynamicSearchError>;
using WarningSink = FunctionRef<void(const DynamicSearchWarning&)>;
using FilterBinding = FunctionRef<SearchResult<Expr>(const DynamicSearchFilter&)>;
using OrderBinding = FunctionRef<SearchResult<OrderBy>(const DynamicSearchOrder&)>;
SearchResult<NormalizedDynamicSearch> normalize_dynamic_search(Memory,
                                                               std::string_view source,
                                                               std::string_view entity,
                                                               const SearchModels&,
                                                               std::size_t max_clauses,
                                                               WarningSink* warn = nullptr);
SearchResult<DynamicSearchResult> merge_dynamic_search(Memory,
                                                       const SelectQuery&,
                                                       std::string_view source,
                                                       const SearchModels&,
                                                       FilterBinding,
                                                       OrderBinding,
                                                       WarningSink* warn = nullptr);
}
