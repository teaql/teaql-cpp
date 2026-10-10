#include "fixture_value.hpp"
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/data_store.hpp>
#include <array>
#include <iostream>
#include <thread>

static Json evaluate(Memory output, const Json& input) {
    GlobalContext global;
    std::array<std::unique_ptr<UserContext>, 2> users{global.make_user_context(),
                                                      global.make_user_context()};
    auto& store = global.make_resource<InMemoryDataStore>();
    Json::Array results(output);
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        auto index = detail::model_integer<unsigned>(field(step, "user"), "user");
        auto& user = *users[index];
        auto r = user.memory_resource();
        auto op = text(field(step, "op"));
        auto key = text(field(step, "key"));
        auto direct = std::get<bool>(field(step, "direct").data);
        Json result(output);
        try {
            if (op == "install") {
                user.insert_resource(global, static_cast<DataStore&>(store));
            } else if (op == "new") {
                users[index] = global.make_user_context();
            } else if (op == "put") {
                std::optional<std::uint64_t> timeout;
                if (!std::holds_alternative<std::monostate>(field(step, "timeout").data)) {
                    timeout =
                        detail::model_integer<std::uint64_t>(field(step, "timeout"), "timeout");
                }
                auto v = value(r, field(step, "value"));
                if (direct) {
                    store.put(r, key, std::move(v), timeout).wait();
                } else {
                    user.put_in_store(key, std::move(v), timeout).wait();
                }
            } else if (op == "get") {
                auto& got = direct ? store.get(r, key).wait() : user.get_in_store(key).wait();
                auto object = detail::model_json(output);
                detail::model_put(object, "some", got.has_value());
                if (got) {
                    detail::model_put(object, "debug", detail::debug_value(output, *got));
                    detail::model_put(object, "json", got->to_json_value(output));
                }
                result = std::move(object);
            } else if (op == "remove") {
                if (direct) {
                    store.remove(r, key).wait();
                } else {
                    user.clear_in_store(key).wait();
                }
            } else if (op == "sleep") {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            } else {
                throw std::invalid_argument("unknown store action");
            }
        } catch (const std::out_of_range&) {
            result = detail::model_json(output);
            detail::model_put(result, "failure", true);
        } catch (const std::logic_error&) {
            result = detail::model_json(output);
            detail::model_put(result, "failure", true);
        }
        results.emplace_back(std::move(result));
    }
    return Json(output, std::move(results));
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena output(4096, std::pmr::new_delete_resource());
        auto input = Json::parse(&output, line);
        std::cout << evaluate(&output, *input).dump(&output) << '\n';
    }
}
