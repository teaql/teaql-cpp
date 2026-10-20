#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_dynamic_json;
template <class Request, class T>
concept SoundInput = requires(Request& query, const T& input) { query.whoseNameSoundsLike(input); };
static_assert(SoundInput<PersonRequest<false>, Value>);
static_assert(!SoundInput<PersonRequest<false>, std::optional<std::string_view>>);

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
            Value input(source->memory_resource(),
                        "Long independent sound input from a caller Context");
            query.whoseNameSoundsLike(input);
        }
        source.reset();
    }
    auto retained = query.copyToGlobal(*user_context);
    auto projections = Q.ordersMinimal()
                           .selectPriority()
                           .selectPriorityRaw("1")
                           .groupByPriorityAs("independent_alias")
                           .selectPriorityAsWithFunction("result", AggregateFunction::Sum)
                           .unselectPriority();
    check(projections.query().projection.size() == 2 && projections.query().raw_projections.empty(),
          "Unselect removes plain/raw field projections");
    check(projections.query().expr_projection.size() == 1 &&
              projections.query().aggregates.size() == 1,
          "Unselect preserves independent aliases/aggregates");
    user_context.reset();
    auto next_user_context = global.make_user_context();
    auto rebound = retained.rebindGlobal(*next_user_context);
    auto text = debug_string(next_user_context->memory_resource(), rebound.query());
    check(text.find("Long independent sound input") != String::npos,
          "Explicit Global copy retains sound argument after User deletion");
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
