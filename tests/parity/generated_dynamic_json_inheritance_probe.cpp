#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_inheritance_graph;

template <class Request>
static Json apply(UserContext& user_context, Request query, const Json& input) {
    auto r = user_context.memory_resource();
    auto output = detail::model_json(r);
    try {
        if (std::get<bool>(field(input, "initial_slice").data)) {
            query.offset(3, 7);
        }
        for (const auto& value : std::get<Json::Array>(field(input, "filters").data)) {
            query.filterWithJson(text(value));
        }
        detail::model_put(output, "query", debug_string(r, query.query()));
    } catch (const GeneratedDynamicJsonFormatError& error) {
        detail::model_put(output, "panic", error.to_string(r));
    } catch (const std::exception& error) {
        detail::model_put(output, "panic", std::string_view(error.what()));
    }
    return output;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto entity = text(field(input, "entity"));
    bool minimal = std::get<bool>(field(input, "minimal").data);
    if (entity == "platform") {
        return apply(user_context, minimal ? Q.platformsMinimal() : Q.platforms(), input);
    }
    if (entity == "specialist") {
        return apply(user_context, minimal ? Q.specialistsMinimal() : Q.specialists(), input);
    }
    if (entity == "school") {
        return apply(user_context, minimal ? Q.schoolsMinimal() : Q.schools(), input);
    }
    if (entity == "academy") {
        return apply(user_context, minimal ? Q.academiesMinimal() : Q.academies(), input);
    }
    if (entity == "research_academy") {
        return apply(
            user_context, minimal ? Q.researchAcademiesMinimal() : Q.researchAcademies(), input);
    }
    return apply(user_context, minimal ? Q.enrollmentsMinimal() : Q.enrollments(), input);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("JSON query retained User bytes");
            }
        }
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
