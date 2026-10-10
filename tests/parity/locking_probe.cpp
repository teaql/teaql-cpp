#include "../locking_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
#include <thread>
using namespace teaql;

static const Json& field(const Json& j, std::string_view key) {
    return std::get<Json::Object>(j.data).find(key)->second;
}

static std::uint64_t number(const Json& j, std::string_view key) {
    return detail::model_integer<std::uint64_t>(field(j, key), key);
}

static std::string_view text(const Json& j) {
    return std::get<String>(j.data);
}

static Json evaluate(Memory out, const Json& input) {
    GlobalContext global;
    std::array<std::unique_ptr<UserContext>, 3> users;
    for (auto& u : users) {
        u = global.make_user_context();
    }
    std::array<Ref<lock_fixture::Provider>, 2> providers;
    for (auto& p : providers) {
        p = users[0]->make_global_resource<lock_fixture::Provider>();
    }
    auto r = global.memory_resource();
    Json::Array results(r);
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        auto index = number(step, "user");
        auto op = text(field(step, "op"));
        auto key = text(field(step, "key"));
        auto timeout = number(step, "timeout"), expiry = number(step, "expiry");
        Json result(r);
        if (op == "local") {
            result = Json(r, users[index]->try_local_lock(key, timeout, expiry));
        } else if (op == "unlock_local") {
            users[index]->unlock_local(key);
        } else if (op == "remote") {
            result = Json(r, users[index]->try_remote_lock(key, timeout, expiry).wait());
        } else if (op == "unlock_remote") {
            result = Json(r, users[index]->unlock_remote(key).wait());
        } else if (op == "provider") {
            auto ref = users[index]->rebind_global(providers[number(step, "provider")]);
            users[index]->set_remote_lock_provider(ref);
        } else if (op == "close") {
            users[index].reset();
        } else if (op == "new") {
            users[index] = global.make_user_context();
        } else if (op == "identifiers") {
            users[index]->set_user_identifier("changed");
            users[index]->set_trace_id("changed");
        } else if (op == "sleep") {
            std::this_thread::sleep_for(std::chrono::milliseconds(number(step, "millis")));
        } else {
            throw std::invalid_argument("unknown lock action");
        }
        results.emplace_back(std::move(result));
    }
    std::array<unsigned, 3> installed{2, 2, 2};
    std::array<std::size_t, 2> cursor{};
    Vector<String> owners(r);
    Json::Array calls(r);
    auto access = global.make_user_context();
    std::array<lock_fixture::Provider*, 2> actual{};
    for (unsigned i = 0; i < 2; ++i) {
        actual[i] = &access->get(access->rebind_global(providers[i]));
    }
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        auto u = number(step, "user");
        auto op = text(field(step, "op"));
        if (op == "provider") {
            installed[u] = number(step, "provider");
        } else if (op == "new") {
            installed[u] = 2;
        } else if ((op == "remote" || op == "unlock_remote") && installed[u] != 2) {
            auto p = installed[u];
            const auto& call = actual[p]->calls[cursor[p]++];
            auto found = std::find(owners.begin(), owners.end(), call.owner);
            auto owner = std::uint64_t(found - owners.begin());
            if (found == owners.end()) {
                owners.emplace_back(call.owner);
            }
            auto object = detail::model_json(r);
            detail::model_put(object, "provider", std::uint64_t(p));
            detail::model_put(object, "key", call.key);
            detail::model_put(object, "owner", owner);
            detail::model_put(object, "timeout", call.timeout);
            detail::model_put(object, "expiry", call.expiry);
            detail::model_put(object, "unlock", call.unlock);
            calls.emplace_back(std::move(object));
        }
    }
    auto result = detail::model_json(r);
    detail::model_put(result, "results", results);
    detail::model_put(result, "calls", calls);
    return Json(out, result);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena output(4096, std::pmr::new_delete_resource());
        auto input = Json::parse(&output, line);
        std::cout << evaluate(&output, *input).dump(&output) << '\n';
    }
}
