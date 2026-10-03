#pragma once
#include <teaql/core/value.hpp>

namespace teaql {
enum class BinaryOp { Eq, Ne, Gt, Gte, Lt, Lte, Like, NotLike, In, NotIn, InLarge, NotInLarge };
struct Expr {
    enum class Kind { Column, Value, Binary, Between, IsNull, IsNotNull, And, Or, Not };
    Kind kind;
    String name;
    teaql::Value literal;
    BinaryOp op = BinaryOp::Eq;
    Vector<Expr> args;
    Vector<teaql::Value> values; // IN values; not a scalar SQL binding.
    Expr(Memory r, Kind kind) : kind(kind), name(r), literal(r), args(r), values(r) {}
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) noexcept = default;
    Expr& operator=(Expr&&) = delete;
    Expr clone(Memory r) const;
    Memory memory_resource() const { return name.get_allocator().resource(); }
    static constexpr std::size_t LARGE_IN_THRESHOLD = 20;
    static Expr column(Memory r, std::string_view name);
    template<class T> static Expr value(Memory r, T&& value) { Expr e(r, Kind::Value); e.literal = teaql::Value(r, std::forward<T>(value)); return e; }
    static Expr binary(Expr left, BinaryOp op, Expr right);
    template<class T> static Expr eq(Memory r, std::string_view field, T&& v) { return binary(column(r, field), BinaryOp::Eq, value(r, std::forward<T>(v))); }
    template<class T> static Expr gt(Memory r, std::string_view field, T&& v) { return binary(column(r, field), BinaryOp::Gt, value(r, std::forward<T>(v))); }
    static Expr in_list(Memory r, std::string_view field, Vector<teaql::Value> values, bool negate = false);
    static Expr between(Memory r, std::string_view field, const teaql::Value& lower, const teaql::Value& upper);
    static Expr is_null(Memory r, std::string_view field);
    static Expr is_not_null(Memory r, std::string_view field);
    Expr and_expr(const Expr& other) const;
    bool operator==(const Expr& other) const;
};
}
