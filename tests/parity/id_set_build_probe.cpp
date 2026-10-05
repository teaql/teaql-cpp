#include "../id_set_build_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <charconv>
#include <iostream>
using namespace teaql;
using namespace teaql::detail;
using Work = AsyncOperation<ResourceBorrow<IdSetBuildGuard>>;

static std::uint64_t n(const Json& j, std::string_view key) {
    auto* value = optional(j, key);
    return value ? std::get<std::uint64_t>(value->data) : 0;
}

struct Task {
    Work* work = nullptr;
    IdSetBuildGuard* guard = nullptr;
    bool cancel_notification = false;
};

static Json evaluate(UserContext& c, const Json& j) {
    auto r = c.memory_resource();
    std::pmr::map<std::uint64_t, IdSetBuildLock*> handles(r);
    std::pmr::map<std::uint64_t, Task> tasks(r);
    Json::Array out(r);
    for (const auto& step : std::get<Json::Array>(field(j, "steps").data)) {
        auto row = model_json(r);
        auto op = text(field(step, "op"));
        auto h = n(step, "handle"), t = n(step, "task");
        if (op == "borrow") {
            handles.insert_or_assign(
                h, &ContextPaginationAccess::build_lock(c, text(field(step, "key"))));
        } else if (op == "same") {
            model_put(
                row, "same", IdSetBuildAccess::same(*handles.at(h), *handles.at(n(step, "other"))));
        } else if (op == "lock") {
            tasks.emplace(t, Task{&handles.at(h)->lock()});
        } else if (op == "poll") {
            auto& task = tasks.at(t);
            auto* value = task.work->poll();
            if (value) {
                task.guard = &value->get();
            }
            model_put(row, "ready", value != nullptr);
        } else if (op == "cancel") {
            auto& task = tasks.at(t);
            if (task.work->status() == AsyncStatus::Pending) {
                task.cancel_notification = true;
            }
            task.work->cancel();
        } else if (op == "release") {
            auto& task = tasks.at(t);
            if (task.guard) {
                task.guard->release();
            }
        } else if (op == "reset") {
            handles.at(h)->reset();
            handles.erase(h);
        } else {
            throw std::logic_error("unknown build lock operation");
        }
        auto states = model_json(r);
        for (const auto& [id, task] : tasks) {
            auto state = model_json(r);
            auto status = task.work->status();
            model_put(state,
                      "state",
                      status == AsyncStatus::Ready       ? "ready"
                      : status == AsyncStatus::Cancelled ? "cancelled"
                                                         : "pending");
            model_put(state, "held", task.guard && task.guard->owns_lock());
            model_put(state,
                      "wakes",
                      task.work->wake_signal().ticket() - std::uint64_t(task.cancel_notification));
            char key[32];
            auto end = std::to_chars(key, key + sizeof(key), id).ptr;
            model_put(states, std::string_view(key, end - key), state);
        }
        model_put(row, "tasks", states);
        out.emplace_back(std::move(row));
    }
    return Json(r, out);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto c = global.make_user_context();
        auto input = Json::parse(c->memory_resource(), line);
        std::cout << evaluate(*c, *input).dump(c->memory_resource()) << '\n';
    }
}
