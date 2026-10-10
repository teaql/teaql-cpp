// Original in-memory engine tests: exact fixtures, Value variants and complete outputs.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/inmemory_engine.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static CompactRow make_row(Memory owner,
                           std::initializer_list<std::pair<std::string_view, Value>> fields) {
    Record record(owner);
    for (const auto& [name, value] : fields) {
        record.emplace(String(name, owner), value);
    }
    return CompactRow(std::allocator_arg, Allocator(owner), record);
}

static Vector<CompactRow> sample_rows(Memory owner) {
    Vector<CompactRow> rows(owner);
    rows.emplace_back(make_row(owner,
                               {{"id", Value(owner, std::uint64_t(1))},
                                {"name", Value(owner, "Alice")},
                                {"age", Value(owner, std::int64_t(30))}}));
    rows.emplace_back(make_row(owner,
                               {{"id", Value(owner, std::uint64_t(2))},
                                {"name", Value(owner, "Bob")},
                                {"age", Value(owner, std::int64_t(25))}}));
    rows.emplace_back(make_row(owner,
                               {{"id", Value(owner, std::uint64_t(3))},
                                {"name", Value(owner, "Charlie")},
                                {"age", Value(owner, std::int64_t(35))}}));
    return rows;
}

static SelectQuery query(Memory owner) {
    return SelectQuery(std::allocator_arg, Allocator(owner), "User");
}

static QueryResult execute(Memory owner, const SelectQuery& query) {
    return InMemoryQueryEngine::execute(owner, query, sample_rows(owner));
}

static void field_is(const CompactRow& row, std::string_view field, const Value& expected) {
    const auto* actual = row.get(field);
    check(actual && *actual == expected,
          "original row field retains its exact Value variant and value");
}

static void test_execute_no_filter(Memory owner) {
    auto result = execute(owner, query(owner));
    check(result.rows.size() == 3 && result.metadata.backend == "memory",
          "original unfiltered result");
    check(result.rows == sample_rows(owner),
          "unfiltered output preserves every original row and field");
}

static void test_execute_with_eq_filter(Memory owner) {
    auto request = query(owner);
    request.filter(Expr::eq(owner, "name", "Bob"));
    auto result = execute(owner, request);
    check(result.rows.size() == 1, "original name equality result count");
    field_is(result.rows[0], "name", Value(owner, "Bob"));
}

static void test_execute_with_gt_filter(Memory owner) {
    auto request = query(owner);
    request.filter(Expr::gt(owner, "age", std::int64_t(28)));
    auto result = execute(owner, request);
    check(result.rows.size() == 2, "original age comparison result count");
    field_is(result.rows[0], "name", Value(owner, "Alice"));
    field_is(result.rows[1], "name", Value(owner, "Charlie"));
}

static void test_sort(Memory owner, bool descending) {
    auto request = query(owner);
    request.add_order(descending ? OrderBy::desc(owner, "age") : OrderBy::asc(owner, "age"));
    auto result = execute(owner, request);
    const std::array<std::int64_t, 3> ages = descending ? std::array<std::int64_t, 3>{35, 30, 25}
                                                        : std::array<std::int64_t, 3>{25, 30, 35};
    check(result.rows.size() == ages.size(), "original complete sorted row count");
    for (std::size_t index = 0; index < ages.size(); ++index) {
        field_is(result.rows[index], "age", Value(owner, ages[index]));
    }
}

static void test_paginate(Memory owner) {
    auto request = query(owner);
    request.page(1, 1);
    auto result = execute(owner, request);
    check(result.rows.size() == 1, "original page(1,1) count");
    field_is(result.rows[0], "name", Value(owner, "Bob"));
}

static void test_projection(Memory owner) {
    auto request = query(owner);
    request.project({"name"});
    auto result = execute(owner, request);
    check(result.rows.size() == 3, "projection does not vacuously discard every row");
    for (const auto& row : result.rows) {
        check(row.contains_key("name") && !row.contains_key("id") && !row.contains_key("age"),
              "original name-only projection removes id and age");
    }
}

static void
aggregate_is(Memory owner, Aggregate aggregate, std::string_view alias, Value expected) {
    auto request = query(owner);
    request.aggregate(std::move(aggregate));
    auto result = execute(owner, request);
    check(result.rows.size() == 1, "original scalar aggregate returns one row");
    field_is(result.rows[0], alias, expected);
}

static void test_aggregates(Memory owner) {
    aggregate_is(owner, Aggregate::count(owner, "total"), "total", Value(owner, std::int64_t(3)));
    aggregate_is(owner, Aggregate::sum(owner, "age", "age_sum"), "age_sum", Value(owner, 90.0));
    aggregate_is(owner, Aggregate::avg(owner, "age", "age_avg"), "age_avg", Value(owner, 30.0));
    aggregate_is(
        owner, Aggregate::max(owner, "age", "age_max"), "age_max", Value(owner, std::int64_t(35)));
    aggregate_is(
        owner, Aggregate::min(owner, "age", "age_min"), "age_min", Value(owner, std::int64_t(25)));
}

static void like_is(Memory owner, std::string_view text, std::string_view pattern, bool expected) {
    // Source like_match is private. Its production public evaluator reaches the same matcher.
    auto row = make_row(owner, {{"text", Value(owner, text)}});
    check(ExprEvaluator::eval(Expr::like(owner, "text", pattern), row) == expected,
          "original private LIKE input through the public expression evaluator");
}

static void test_like_match_percent(Memory owner) {
    like_is(owner, "hello world", "%world", true);
    like_is(owner, "hello world", "hello%", true);
    like_is(owner, "hello world", "%lo wo%", true);
    like_is(owner, "hello world", "%", true);
    like_is(owner, "hello world", "%xyz%", false);
}

static void test_like_match_underscore(Memory owner) {
    like_is(owner, "abc", "a_c", true);
    like_is(owner, "abbc", "a_c", false);
    like_is(owner, "abc", "___", true);
    like_is(owner, "ab", "___", false);
}

static void test_like_match_combined(Memory owner) {
    like_is(owner, "foobar", "f%r", true);
    like_is(owner, "foobar", "f__b%", true);
    like_is(owner, "foobar", "f__x%", false);
}

static void test_and_or_not(Memory owner) {
    auto row = make_row(
        owner, {{"a", Value(owner, std::int64_t(10))}, {"b", Value(owner, std::int64_t(20))}});
    Vector<Expr> both(owner);
    both.emplace_back(Expr::eq(owner, "a", std::int64_t(10)));
    both.emplace_back(Expr::eq(owner, "b", std::int64_t(20)));
    check(ExprEvaluator::eval(Expr::all_of(owner, both), row), "original AND expression");
    both[0] = Expr::eq(owner, "a", std::int64_t(99));
    check(ExprEvaluator::eval(Expr::any_of(owner, both), row), "original OR expression");
    check(ExprEvaluator::eval(Expr::negate(Expr::eq(owner, "a", std::int64_t(99))), row),
          "original NOT expression");
}

static void test_is_null_is_not_null(Memory owner) {
    auto row = make_row(owner, {{"x", Value(owner)}, {"y", Value(owner, std::int64_t(1))}});
    check(ExprEvaluator::eval(Expr::is_null(owner, "x"), row), "original Null IS NULL");
    check(!ExprEvaluator::eval(Expr::is_not_null(owner, "x"), row),
          "original Null IS NOT NULL rejects");
    check(ExprEvaluator::eval(Expr::is_not_null(owner, "y"), row), "original I64 IS NOT NULL");
}

static void test_between(Memory owner) {
    auto row = make_row(owner, {{"age", Value(owner, std::int64_t(30))}});
    check(ExprEvaluator::eval(
              Expr::between(
                  owner, "age", Value(owner, std::int64_t(25)), Value(owner, std::int64_t(35))),
              row),
          "original age inside BETWEEN");
    check(!ExprEvaluator::eval(
              Expr::between(
                  owner, "age", Value(owner, std::int64_t(31)), Value(owner, std::int64_t(35))),
              row),
          "original age outside BETWEEN");
}

static void test_in_list(Memory owner) {
    auto row = make_row(owner, {{"status", Value(owner, "active")}});
    Vector<Value> values(owner);
    values.emplace_back("active");
    values.emplace_back("pending");
    check(ExprEvaluator::eval(Expr::in_list(owner, "status", values), row), "original IN list hit");
    values.clear();
    values.emplace_back("closed");
    check(!ExprEvaluator::eval(Expr::in_list(owner, "status", values), row),
          "original IN list miss");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        test_execute_no_filter(owner);
        test_execute_with_eq_filter(owner);
        test_execute_with_gt_filter(owner);
        test_sort(owner, false);
        test_sort(owner, true);
        test_paginate(owner);
        test_projection(owner);
        test_aggregates(owner);
        test_like_match_percent(owner);
        test_like_match_underscore(owner);
        test_like_match_combined(owner);
        test_and_or_not(owner);
        test_is_null_is_not_null(owner);
        test_between(owner);
        test_in_list(owner);
        user.reset();
        check(global.live_user_bytes() == 0,
              "source User destruction returns every engine allocation");
        std::pmr::set_default_resource(previous);
        std::cout << "Original nineteen in-memory engine contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
