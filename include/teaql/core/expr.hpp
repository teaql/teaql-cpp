#pragma once
#include <teaql/core/value.hpp>
#include <teaql/memory/scoped.hpp>
#include <span>

namespace teaql {
struct EntityDescriptor;
struct SelectQuery;
struct SubQueryExpr;
enum class BinaryOp { Eq, Ne, Gt, Gte, Lt, Lte, Like, NotLike, In, NotIn, InLarge, NotInLarge };
enum class ExprFunction {
    Soundex,
    Gbk,
    Count,
    Sum,
    Avg,
    Min,
    Max,
    Stddev,
    StddevPop,
    VarSamp,
    VarPop,
    BitAnd,
    BitOr,
    BitXor
};

struct Expr {
    using allocator_type = Allocator;
    enum class Kind {
        Column,
        Value,
        Function,
        Binary,
        SubQuery,
        Between,
        IsNull,
        IsNotNull,
        And,
        Or,
        Not
    };
    Kind kind;
    String name;
    teaql::Value literal;
    BinaryOp op = BinaryOp::Eq;
    ExprFunction function_kind = ExprFunction::Soundex;
    Vector<Expr> args;
    detail::ScopedBox<SubQueryExpr> subquery_data;

    Expr(Memory, Kind);
    Expr(std::allocator_arg_t, Allocator, Kind);
    Expr(std::allocator_arg_t, Allocator, const Expr&);
    Expr(std::allocator_arg_t, Allocator, Expr&&);
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) noexcept;
    Expr& operator=(Expr&&);
    ~Expr();
    Expr clone(Memory) const;

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    static constexpr std::size_t LARGE_IN_THRESHOLD = 20;
    static Expr column(Memory, std::string_view);

    template <class T>
    static Expr value(Memory r, T&& v) {
        Expr e(r, Kind::Value);
        e.literal = teaql::Value(r, std::forward<T>(v));
        return e;
    }

    static Expr function(Memory, ExprFunction, std::span<const Expr>);
    static Expr soundex(const Expr&);
    static Expr gbk(const Expr&);
    static Expr count_all(Memory);
    static Expr count_expr(const Expr&);
    static Expr sum_expr(const Expr&);
    static Expr avg_expr(const Expr&);
    static Expr min_expr(const Expr&);
    static Expr max_expr(const Expr&);
    static Expr stddev_expr(const Expr&);
    static Expr stddev_pop_expr(const Expr&);
    static Expr var_samp_expr(const Expr&);
    static Expr var_pop_expr(const Expr&);
    static Expr bit_and_expr(const Expr&);
    static Expr bit_or_expr(const Expr&);
    static Expr bit_xor_expr(const Expr&);

    template <class T>
    static Expr sound_like(Memory r, std::string_view field, T&& v) {
        return binary(
            soundex(column(r, field)), BinaryOp::Eq, soundex(value(r, std::forward<T>(v))));
    }

    static Expr binary(Expr left, BinaryOp op, Expr right);

    template <class T>
    static Expr eq(Memory r, std::string_view field, T&& v) {
        return binary(column(r, field), BinaryOp::Eq, value(r, std::forward<T>(v)));
    }

    template <class T>
    static Expr ne(Memory r, std::string_view field, T&& v) {
        return binary(column(r, field), BinaryOp::Ne, value(r, std::forward<T>(v)));
    }

    template <class T>
    static Expr gt(Memory r, std::string_view field, T&& v) {
        return binary(column(r, field), BinaryOp::Gt, value(r, std::forward<T>(v)));
    }

    template <class T>
    static Expr gte(Memory r, std::string_view field, T&& v) {
        return binary(column(r, field), BinaryOp::Gte, value(r, std::forward<T>(v)));
    }

    template <class T>
    static Expr lt(Memory r, std::string_view field, T&& v) {
        return binary(column(r, field), BinaryOp::Lt, value(r, std::forward<T>(v)));
    }

    template <class T>
    static Expr lte(Memory r, std::string_view field, T&& v) {
        return binary(column(r, field), BinaryOp::Lte, value(r, std::forward<T>(v)));
    }

    static Expr like(Memory, std::string_view, std::string_view);
    static Expr not_like(Memory, std::string_view, std::string_view);
    static Expr contain(Memory, std::string_view, std::string_view);
    static Expr not_contain(Memory, std::string_view, std::string_view);
    static Expr begin_with(Memory, std::string_view, std::string_view);
    static Expr not_begin_with(Memory, std::string_view, std::string_view);
    static Expr end_with(Memory, std::string_view, std::string_view);
    static Expr not_end_with(Memory, std::string_view, std::string_view);
    static Expr compare_columns(Memory, std::string_view, BinaryOp, std::string_view);
    static Expr
    in_list(Memory, std::string_view, std::span<const teaql::Value>, bool negate = false);

    static Expr
    not_in_list(Memory r, std::string_view field, std::span<const teaql::Value> values) {
        return in_list(r, field, values, true);
    }

    static Expr in_large(Memory, std::string_view, std::span<const teaql::Value>);
    static Expr not_in_large(Memory, std::string_view, std::span<const teaql::Value>);
    static Expr in_subquery(
        Memory, std::string_view, const EntityDescriptor&, const SelectQuery&, std::string_view);
    static Expr not_in_subquery(
        Memory, std::string_view, const EntityDescriptor&, const SelectQuery&, std::string_view);
    static Expr
    subquery(Expr, BinaryOp, const EntityDescriptor&, const SelectQuery&, std::string_view);
    static Expr between(Memory, std::string_view, const teaql::Value&, const teaql::Value&);
    static Expr is_null(Memory, std::string_view);
    static Expr is_not_null(Memory, std::string_view);
    // "and"/"or" are C++ alternative operator tokens.
    static Expr all_of(Memory, std::span<const Expr>);
    static Expr any_of(Memory, std::span<const Expr>);
    static Expr negate(const Expr&);
    Expr and_expr(const Expr&) const;
    Expr or_expr(const Expr&) const;
    bool operator==(const Expr&) const;
};
}
