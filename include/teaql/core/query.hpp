#pragma once
#include <teaql/core/expr.hpp>
#include <teaql/core/trace.hpp>

namespace teaql {
enum class SortDirection { Asc, Desc };
struct OrderBy { String field; SortDirection direction; };
struct Slice { std::optional<std::uint64_t> limit; std::uint64_t offset = 0; };
struct SelectQuery {
    using allocator_type = Allocator;
    String entity;
    Vector<String> projection;
    std::optional<Expr> predicate;
    Vector<OrderBy> order_by;
    std::optional<Slice> slice;
    std::uint64_t hard_limit_value = 10'000;
    String comment_text;
    TraceChain trace_chain;
    SelectQuery(std::allocator_arg_t, Allocator a, std::string_view entity)
        : entity(entity, a.resource()), projection(a.resource()), order_by(a.resource()),
          comment_text(a.resource()), trace_chain(a.resource()) {}
    SelectQuery(const SelectQuery&) = delete;
    SelectQuery& operator=(const SelectQuery&) = delete;
    Memory memory_resource() const { return entity.get_allocator().resource(); }
    SelectQuery& project(std::string_view field) { projection.emplace_back(field); return *this; }
    SelectQuery& filter(const Expr& expr) { predicate.emplace(expr.clone(memory_resource())); return *this; }
    SelectQuery& and_filter(const Expr& expr) { auto combined = predicate ? predicate->and_expr(expr) : expr.clone(memory_resource()); predicate.emplace(std::move(combined)); return *this; }
    SelectQuery& order(std::string_view field, SortDirection dir = SortDirection::Asc) { order_by.push_back({String(field, memory_resource()), dir}); return *this; }
    SelectQuery& limit(std::uint64_t v) { if (!slice) slice = Slice{}; slice->limit = v; return *this; }
    SelectQuery& offset(std::uint64_t v) { if (!slice) slice = Slice{}; slice->offset = v; return *this; }
    SelectQuery& hard_limit(std::uint64_t v) { hard_limit_value = v; return *this; }
    SelectQuery& comment(std::string_view text) { comment_text = text; return *this; }
    SelectQuery& purpose(std::string_view text) { trace_chain.emplace_back(memory_resource(), TraceKind::Purpose, entity, text); return *this; }
    Slice prepare_for_list() const;
};
}
