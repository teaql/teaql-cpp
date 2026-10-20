#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "../../tests/generated_business_ids/include/cpp_business_ids.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

struct Unsupported {};
template <class Request, class Input>
concept NameComparison = requires(Request& query, const Input& value) { query.whoseNameIs(value); };
template <class Request, class Input>
concept BusinessComparison =
    requires(Request& query, const Input& value) { query.withReceiptNumberIs(value); };
static_assert(NameComparison<PersonRequest<false>, Value>);
static_assert(!NameComparison<PersonRequest<false>, Unsupported>);
static_assert(!BusinessComparison<cpp_business_ids::ReceiptRequest<false>, Value>);

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

static void lifetime() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto Q = user_context->queries<query_factory>();
    auto query = Q.peopleMinimal();
    {
        auto source = global.make_user_context();
        {
            Value operand(source->memory_resource(),
                          "Long independently copied comparison operand from another UserContext");
            query.whoseNameIs(operand);
        }
        source.reset();
    }
    auto copy = query.copyToGlobal(*user_context);
    {
        auto mixed = Q.ordersMinimal().withPriorityBetween(std::string_view("1"), std::int64_t(3));
        check(mixed.query().predicate.has_value(), "Between accepts independent operand types");
        Value null(user_context->memory_resource());
        auto nullable = Q.peopleMinimal().whoseNameIs(null);
        check(nullable.query().predicate.has_value(), "Explicit Value NULL remains valid input");
        auto ordinary = Q.peopleMinimal().whoseNameIs(std::string_view("Alpha"));
        check(ordinary.query().predicate.has_value(),
              "Existing field-typed overload remains callable");
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    auto rebound = copy.rebindGlobal(*next_user_context);
    auto diagnostic = debug_string(next_user_context->memory_resource(), rebound.query());
    check(diagnostic.find("Long independently copied comparison operand") != String::npos,
          "Explicit Global copy survives actual User deletion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        lifetime();
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
