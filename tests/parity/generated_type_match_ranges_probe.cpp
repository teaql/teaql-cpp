#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
#include <sstream>
#include <ranges>

using namespace cpp_dynamic_json;

template <class Request, class Apply>
static Json run(UserContext& user_context, Request query, Apply apply, const Json& input) {
    auto r = user_context.memory_resource();
    Vector<Value> values(r);
    for (const auto& entry : std::get<Json::Array>(field(input, "values").data)) {
        values.emplace_back(value(r, entry));
    }
    auto carrier = text(field(input, "carrier"));
    for (int repeat = 0; repeat < (std::get<bool>(field(input, "repeat").data) ? 2 : 1); ++repeat) {
        if (carrier == "i64") {
            Vector<std::int64_t> numbers(r);
            for (const auto& item : values) {
                numbers.push_back(*item.try_i64());
            }
            apply(query, numbers);
        } else if (carrier == "text") {
            Vector<std::string_view> strings(r);
            for (const auto& item : values) {
                strings.emplace_back(std::get<String>(item.data));
            }
            apply(query, strings);
        } else if (carrier == "stream") {
            std::ostringstream rendered;
            for (const auto& item : values) {
                rendered << *item.try_i64() << ' ';
            }
            std::istringstream source(rendered.str());
            auto stream = std::ranges::istream_view<std::int64_t>(source);
            apply(query, stream);
        } else if (carrier == "view") {
            auto view = values | std::views::transform([](const auto& item) -> const Value& {
                            return item;
                        });
            apply(query, view);
        } else {
            apply(query, values);
        }
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "query", debug_string(r, query.query()));
    if (std::get<bool>(field(input, "execute").data)) {
        teaql::test::initialize_dynamic_json_database(user_context);
        const auto& result = std::move(query)
                                 .comment("inspect type matching inputs")
                                 .purpose("qualify type matching inputs")
                                 .executeForRows(user_context)
                                 .wait();
        Json::Array rows(r);
        Json error(r);
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array columns(r);
                for (auto key : {"id", "version"}) {
                    auto* item = row.get(key);
                    columns.emplace_back(item ? Json(r, debug_string(r, *item)) : Json(r));
                }
                rows.emplace_back(columns);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
        detail::model_put(output, "rows", Json(r, rows));
        detail::model_put(output, "error", error);
    }
    return output;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    bool minimal = std::get<bool>(field(input, "minimal").data);
    auto apply = [](auto& request, auto&& values) {
        request.matchTypes(std::forward<decltype(values)>(values));
    };
    if (text(field(input, "field")) == "name") {
        return run(user_context, minimal ? Q.peopleMinimal() : Q.people(), apply, input);
    }
    return run(user_context, minimal ? Q.ordersMinimal() : Q.orders(), apply, input);
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
                throw std::logic_error("Membership retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
