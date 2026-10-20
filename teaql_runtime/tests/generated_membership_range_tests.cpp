#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "../../tests/generated_business_ids/include/cpp_business_ids.hpp"
#include <teaql/core/query_debug.hpp>
#include <ranges>
#include <sstream>
#include <iostream>
using namespace cpp_dynamic_json;

template <class Request, class Inputs>
concept NameMembership =
    requires(Request& request, Inputs& inputs) { request.whoseNameIn(inputs); };
template <class Request, class Inputs>
concept BusinessMembership =
    requires(Request& request, Inputs& inputs) { request.withReceiptNumberIn(inputs); };

struct Unsupported {};

static_assert(NameMembership<PersonRequest<false>, Vector<Value>>);
static_assert(!NameMembership<PersonRequest<false>, std::array<Unsupported, 1>>);
static_assert(!BusinessMembership<cpp_business_ids::ReceiptRequest<false>, Vector<Value>>);

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

static void lifetime() {
    GlobalContext global;
    auto destination = global.make_user_context();
    auto Q = destination->queries<query_factory>();
    auto query = Q.peopleMinimal();
    {
        auto source = global.make_user_context();
        {
            Vector<Value> values(source->memory_resource());
            values.emplace_back(
                "Long independent membership value allocated in the source Context");
            query.whoseNameIn(values);
            values.clear();
        }
        source.reset();
    }
    check(query.query().predicate.has_value(), "Membership captures source values before deletion");
    auto copy = query.copyToGlobal(*destination);
    std::istringstream integers("1 2 3 3");
    auto stream = std::ranges::istream_view<std::int64_t>(integers);
    auto streamed = Q.ordersMinimal().withIdNotIn(stream);
    check(integers.eof() && streamed.query().predicate.has_value(),
          "Single-pass input is consumed once");
    {
        const std::array<std::uint64_t, 2> ids{1, 2};
        auto legacy = Q.ordersMinimal().withIdIn(std::span<const std::uint64_t>(ids));
        check(legacy.query().predicate.has_value(), "Existing typed span remains callable");
    }
    destination.reset();
    auto next_user_context = global.make_user_context();
    auto rebound = copy.rebindGlobal(*next_user_context);
    auto debug = debug_string(next_user_context->memory_resource(), rebound.query());
    check(debug.find("Long independent membership value") != String::npos,
          "Explicit Global retention preserves copied membership after source deletion");
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
