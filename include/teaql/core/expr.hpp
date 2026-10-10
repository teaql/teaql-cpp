#pragma once
#include <teaql/core/value.hpp>
#include <teaql/memory/scoped.hpp>
#include <span>
#include <ranges>

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

    template <std::ranges::input_range Parts>
        requires std::same_as<std::ranges::range_value_t<Parts>, Expr>
    static Expr function(Memory owner, ExprFunction function, Parts&& parts) {
        Expr result(owner, Kind::Function);
        result.function_kind = function;
        result.args = collect_range<Expr>(owner, std::forward<Parts>(parts));
        return result;
    }

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

    template <std::ranges::input_range Values>
        requires std::same_as<std::ranges::range_value_t<Values>, teaql::Value>
    static Expr
    in_list(Memory owner, std::string_view field, Values&& values, bool negative = false) {
        auto items = collect_range<teaql::Value>(owner, std::forward<Values>(values));
        bool large = items.size() > LARGE_IN_THRESHOLD;
        auto op = negative ? (large ? BinaryOp::NotInLarge : BinaryOp::NotIn)
                           : (large ? BinaryOp::InLarge : BinaryOp::In);
        return list_range(owner, field, op, std::move(items));
    }

    static Expr
    not_in_list(Memory r, std::string_view field, std::span<const teaql::Value> values) {
        return in_list(r, field, values, true);
    }

    static Expr in_large(Memory, std::string_view, std::span<const teaql::Value>);
    static Expr not_in_large(Memory, std::string_view, std::span<const teaql::Value>);

    template <std::ranges::input_range Values>
        requires std::same_as<std::ranges::range_value_t<Values>, teaql::Value>
    static Expr not_in_list(Memory owner, std::string_view field, Values&& values) {
        return in_list(owner, field, std::forward<Values>(values), true);
    }

    template <std::ranges::input_range Values>
        requires std::same_as<std::ranges::range_value_t<Values>, teaql::Value>
    static Expr in_large(Memory owner, std::string_view field, Values&& values) {
        return list_range(owner,
                          field,
                          BinaryOp::InLarge,
                          collect_range<teaql::Value>(owner, std::forward<Values>(values)));
    }

    template <std::ranges::input_range Values>
        requires std::same_as<std::ranges::range_value_t<Values>, teaql::Value>
    static Expr not_in_large(Memory owner, std::string_view field, Values&& values) {
        return list_range(owner,
                          field,
                          BinaryOp::NotInLarge,
                          collect_range<teaql::Value>(owner, std::forward<Values>(values)));
    }

    static Expr in_subquery(
        Memory, std::string_view, const EntityDescriptor&, const SelectQuery&, std::string_view);
    static Expr not_in_subquery(
        Memory, std::string_view, const EntityDescriptor&, const SelectQuery&, std::string_view);
    static Expr
    subquery(Expr, BinaryOp, const EntityDescriptor&, const SelectQuery&, std::string_view);
    static Expr between(Memory, std::string_view, const teaql::Value&, const teaql::Value&);

    template <class Lower, class Upper>
        requires(std::constructible_from<teaql::Value, Memory, Lower &&> &&
                 std::constructible_from<teaql::Value, Memory, Upper &&>)
    static Expr between(Memory owner, std::string_view field, Lower&& lower, Upper&& upper) {
        Expr result(owner, Kind::Between);
        result.args.emplace_back(column(owner, field));
        result.args.emplace_back(value(owner, std::forward<Lower>(lower)));
        result.args.emplace_back(value(owner, std::forward<Upper>(upper)));
        return result;
    }

    static Expr is_null(Memory, std::string_view);
    static Expr is_not_null(Memory, std::string_view);
    // "and"/"or" are C++ alternative operator tokens.
    static Expr all_of(Memory, std::span<const Expr>);
    static Expr any_of(Memory, std::span<const Expr>);

    template <std::ranges::input_range Parts>
        requires std::same_as<std::ranges::range_value_t<Parts>, Expr>
    static Expr all_of(Memory owner, Parts&& parts) {
        return logical_range(owner, Kind::And, std::forward<Parts>(parts));
    }

    template <std::ranges::input_range Parts>
        requires std::same_as<std::ranges::range_value_t<Parts>, Expr>
    static Expr any_of(Memory owner, Parts&& parts) {
        return logical_range(owner, Kind::Or, std::forward<Parts>(parts));
    }

    static Expr negate(const Expr&);
    Expr and_expr(const Expr&) const;
    Expr or_expr(const Expr&) const;
    bool operator==(const Expr&) const;

private:
    static Expr
    list_range(Memory owner, std::string_view field, BinaryOp op, Vector<teaql::Value>&& items) {
        teaql::Value literal(owner, std::move(items));
        return binary(column(owner, field), op, value(owner, std::move(literal)));
    }

    template <class T, std::ranges::input_range Items>
    static Vector<T> collect_range(Memory owner, Items&& items) {
        Vector<T> result(owner);
        if constexpr (std::ranges::sized_range<Items>) {
            result.reserve(std::ranges::size(items));
        }
        for (auto&& item : items) {
            if constexpr (!std::is_lvalue_reference_v<Items> &&
                          !std::ranges::borrowed_range<Items>) {
                result.emplace_back(std::move(item));
            } else {
                result.emplace_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }

    template <std::ranges::input_range Parts>
    static Expr logical_range(Memory owner, Kind kind, Parts&& parts) {
        Expr result(owner, kind);
        auto retain_unique = [&](auto&& item) {
            if (std::find(result.args.begin(), result.args.end(), item) == result.args.end()) {
                result.args.emplace_back(std::forward<decltype(item)>(item));
            }
        };
        for (auto&& item : parts) {
            if constexpr (!std::is_lvalue_reference_v<Parts> &&
                          !std::ranges::borrowed_range<Parts> &&
                          !std::is_const_v<std::remove_reference_t<decltype(item)>>) {
                // Consume each owned operand, including rejected duplicates,
                // before advancing the iterator, as Rust's IntoIterator does.
                Expr current(std::move(item));
                retain_unique(std::move(current));
            } else {
                retain_unique(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
};
}
