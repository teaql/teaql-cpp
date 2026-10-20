#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_inheritance_graph;

template <class Request>
static Json observe_options(UserContext& user_context, const Json& input, Request query) {
    auto r = user_context.memory_resource();
    Json::Array steps(r);
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        Json::Object step(r);
        step.emplace("before", Json(r, debug_string(r, query.query())));
        Json error(r);
        try {
            auto op = text(field(action, "op"));
            if (op == "continuous_default") {
                query.optimizeForContinuousPageFetch();
            } else if (op == "continuous_config") {
                query.optimizeForContinuousPageFetchWith(
                    text(field(action, "namespace")), number<std::uint64_t>(field(action, "ttl")));
            } else if (op == "id_set_default") {
                query.optimizePaginationWithIdSet();
            } else if (op == "id_set_config") {
                query.optimizePaginationWithIdSetConfig(
                    text(field(action, "namespace")),
                    number<std::uint64_t>(field(action, "ttl")),
                    number<std::uint64_t>(field(action, "max")));
            } else if (op == "top_n") {
                query.topNProbeParentThreshold(number<std::size_t>(field(action, "threshold")));
            } else {
                throw std::invalid_argument("Unknown option action");
            }
        } catch (const std::exception& failure) {
            error = Json(r, failure.what());
        }
        step.emplace("after", Json(r, debug_string(r, query.query())));
        step.emplace("error", std::move(error));
        steps.emplace_back(step);
    }
    return Json(r, steps);
}

int main() {
    static_assert(sizeof(std::size_t) == 8, "This original comparison is native64");
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("Invalid option input");
                }
                auto Q = user_context->queries<query_factory>();
                auto entity = text(field(*input, "entity"));
                auto result = [&]() {
                    if (entity == "school") {
                        return observe_options(*user_context, *input, Q.schools());
                    }
                    if (entity == "academy") {
                        return observe_options(*user_context, *input, Q.academies());
                    }
                    if (entity == "research_academy") {
                        return observe_options(*user_context, *input, Q.researchAcademies());
                    }
                    return observe_options(*user_context, *input, Q.enrollments());
                }();
                std::cout << result.dump(user_context->memory_resource()) << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Options retained User memory");
            }
        }
    } catch (const std::exception& failure) {
        std::pmr::set_default_resource(previous);
        std::cerr << failure.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
