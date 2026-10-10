#include <teaql/core/query.hpp>
#include <teaql/runtime/context.hpp>
#include <sstream>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void dynamic_queries() {
    GlobalContext global;
    Ref<SelectQuery> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& query = user->make<SelectQuery>("RangeExample");
        auto& names = user->make<Vector<String>>();
        names.emplace_back(4096, 'a');
        names.emplace_back(4096, 'b');
        query.projects(names);
        auto reverse = query.projection | std::views::reverse;
        query.projects(reverse);
        check(query.projection.size() == 4 && query.projection[2].front() == 'b' &&
                  query.projection[3].front() == 'a',
              "aliased dynamic projection input is snapshotted before growth");
        std::istringstream input("dynamic single pass");
        query.projects(std::ranges::istream_view<std::string>(input));
        check(query.projection.size() == 7 && query.projection.back() == "pass",
              "projection accepts a single-pass input iterator");
        query.aggregation_cache = AggregationCacheOptions::for_duration(0).with_propagation(99);
        check(*query.aggregation_cache == AggregationCacheOptions{true, 0, true, 99},
              "standalone cache builders retain all independent fields");
        retained = user->make_global<SelectQuery>(query);
        check(query.projection[0].get_allocator().resource() == owner,
              "incoming foreign names belong to the query owner");
    }
    auto next = global.make_user_context();
    const auto& query = next->get(next->rebind_global(retained));
    check(query.projection[0].size() == 4096 && query.projection[2].front() == 'b' &&
              query.projection[0].get_allocator().resource() == global.memory_resource() &&
              query.aggregation_cache->propagate_cache_expired_millis == 99,
          "complete dynamic query survives actual source User deletion");
}

static void expression_ranges() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& parts = user->make<Vector<Expr>>();
    parts.emplace_back(Expr::eq(owner, "key", 7));
    parts.emplace_back(Expr::eq(owner, "key", 7));
    parts.emplace_back(Expr::eq(owner, "other", 9));
    auto joined = Expr::all_of(owner, parts | std::views::reverse);
    check(joined.args.size() == 2 && joined.args[0] == parts[2] && joined.args[1] == parts[0],
          "non-contiguous logical input retains source first-occurrence order and deduplication");
    auto function = Expr::function(owner, ExprFunction::Soundex, std::move(parts));
    check(function.args.size() == 3 && function.function_kind == ExprFunction::Soundex,
          "owned expression iterator preserves duplicates for ordinary functions");
    for (unsigned count : {0u, 20u, 21u}) {
        std::ostringstream text;
        for (unsigned index = 0; index < count; ++index) {
            text << index << ' ';
        }
        std::istringstream input(text.str());
        auto values =
            std::ranges::istream_view<int>(input) | std::views::transform([owner](int value) {
                return Value(owner, value);
            });
        auto expression = Expr::not_in_list(owner, "key", std::move(values));
        check(expression.op == (count > 20 ? BinaryOp::NotInLarge : BinaryOp::NotIn) &&
                  expression.args[1].literal.as_list()->size() == count,
              "single-pass list construction preserves empty and exact large-list threshold");
    }
    auto& foreign = global.make<Vector<Value>>();
    foreign.emplace_back(String(4096, 'f', global.memory_resource()));
    auto expression = Expr::in_large(owner, "key", foreign);
    check(expression.op == BinaryOp::InLarge &&
              expression.args[1].literal.as_list()->front().memory_resource() == owner &&
              foreign.front().try_text()->size() == 4096,
          "explicit large-list construction deep-copies borrowed foreign payloads");
}

static void bounded_logical_ranges() {
    GlobalContext global;
    auto& prototype = global.make<Expr>(
        Expr::eq(global.memory_resource(), "key", String(8192, 'p', global.memory_resource())));
    for (bool disjunction : {false, true}) {
        auto user = global.make_user_context(MemoryLimits{.max_bytes = 64 * 1024});
        auto owner = user->memory_resource();
        {
            unsigned yielded = 0;
            auto parts = std::views::iota(0, 1000) | std::views::transform([&](int) -> const Expr& {
                             ++yielded;
                             return prototype;
                         });
            auto joined = disjunction ? Expr::any_of(owner, parts) : Expr::all_of(owner, parts);
            check(yielded == 1000 && joined.args.size() == 1 && joined.args[0] == prototype &&
                      joined.args[0].memory_resource() == owner &&
                      joined.args[0].args[1].literal.try_text()->size() == 8192 &&
                      user->allocation_stats().peak_bytes <= user->memory_limits().max_bytes,
                  "logical streaming retains only unique operands within the User byte budget");
            Vector<Expr> consumed(owner);
            consumed.emplace_back(Expr::eq(owner, "owned", 7));
            consumed.emplace_back(Expr::eq(owner, "owned", 7));
            auto* first_operand = consumed[0].args.data();
            auto owned_join = disjunction ? Expr::any_of(owner, std::move(consumed))
                                          : Expr::all_of(owner, std::move(consumed));
            check(owned_join.args.size() == 1 && owned_join.args[0].args.data() == first_operand &&
                      consumed[0].args.empty() && consumed[1].args.empty(),
                  "owned logical input transfers retained nodes and consumes rejected duplicates");
        }
        user.reset();
        check(global.live_user_bytes() == 0 && prototype.args[1].literal.try_text()->size() == 8192,
              "logical User results are reclaimed without consuming borrowed Global input");
    }
}

static void between_value_inputs() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto integers = Expr::between(owner, "age", 18, 25);
    check(integers.kind == Expr::Kind::Between && integers.args[1].literal.try_i64() == 18 &&
              integers.args[2].literal.try_i64() == 25,
          "BETWEEN accepts scalar inputs through the same scoped Value conversion as eq");
    String lower(8192, 'a', owner), upper(8192, 'z', owner);
    auto* lower_pointer = lower.data();
    auto* upper_pointer = upper.data();
    auto strings = Expr::between(owner, "name", std::move(lower), std::move(upper));
    check(std::get<String>(strings.args[1].literal.data).data() == lower_pointer &&
              std::get<String>(strings.args[2].literal.data).data() == upper_pointer,
          "BETWEEN consumes same-owner string bounds without cloning their payloads");

    struct Bound {
        unsigned& calls;
        unsigned expected;
        bool fail = false;

        operator std::string_view() const {
            check(calls++ == expected, "BETWEEN converts its lower bound before its upper bound");
            if (fail) {
                throw std::invalid_argument("injected lower conversion failure");
            }
            return "bound";
        }
    };

    unsigned calls = 0;
    (void)Expr::between(owner, "name", Bound{calls, 0}, Bound{calls, 1});
    check(calls == 2, "BETWEEN invokes both conversions once");
    calls = 0;
    bool rejected = false;
    try {
        (void)Expr::between(owner, "name", Bound{calls, 0, true}, Bound{calls, 1});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && calls == 1, "a lower conversion failure never evaluates the upper bound");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        dynamic_queries();
        expression_ranges();
        bounded_logical_ranges();
        between_value_inputs();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
