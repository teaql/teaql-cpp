#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>
using namespace teaql;
using E = MutationExecutorError;
using Begin = AsyncOperation<Result<SqlTransactionHandle<E>, E>>;
using Write = AsyncOperation<Result<std::uint64_t, E>>;

static Json failure(Memory r, const E& error) {
    auto out = detail::model_json(r);
    constexpr std::string_view kinds[]{
        "Sqlite", "SqlCompile", "UnsupportedValue", "UnsupportedColumnType", "Bind", "Lock"};
    detail::model_put(out, "error", error.to_string(r));
    detail::model_put(out, "kind", kinds[static_cast<unsigned>(error.kind())]);
    detail::model_put(out, "code", std::int64_t(error.sqlite_extended_code()));
    return Json(r, out);
}

static Json rows(Memory r, const Result<Vector<CompactRow>, E>& value) {
    if (!value) {
        return failure(r, value.error());
    }
    Json::Array result(r);
    for (const auto& row : value.value()) {
        auto entry = detail::model_json(r);
        Json::Array keys(r), values(r);
        for (auto key : row.keys()) {
            keys.emplace_back(key);
        }
        for (const auto& v : row.values()) {
            values.emplace_back(v.to_json_value(r));
        }
        detail::model_put(entry, "keys", keys);
        detail::model_put(entry, "values", values);
        result.emplace_back(entry);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "rows", result);
    return Json(r, out);
}

template <class T>
static Json result(Memory r, const Result<T, E>& value, std::string_view key) {
    if (!value) {
        return failure(r, value.error());
    }
    auto out = detail::model_json(r);
    if constexpr (std::same_as<T, std::uint64_t>) {
        detail::model_put(out, key, value.value());
    } else {
        detail::model_put(out, key, true);
    }
    return Json(r, out);
}

struct Work {
    std::size_t index;
    std::unique_ptr<UserContext> owner;
    Begin* begin = nullptr;
    Write* write = nullptr;
};

static Json observe(GlobalContext& global, const Json& input) {
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto& setup = user_context->make<CompiledQuery>();
    setup.sql = "CREATE TABLE records(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)";
    (void)provider.execute(r, setup).value();
    for (std::uint64_t i = 1; i <= std::get<std::uint64_t>(field(input, "count").data); ++i) {
        setup.sql = "INSERT INTO records VALUES(";
        setup.sql += std::to_string(i);
        setup.sql += ",1,'name-";
        setup.sql += std::to_string(i);
        setup.sql += "')";
        (void)provider.execute(r, setup).value();
    }
    auto autocommit = [&] {
        return bool(sqlite3_get_autocommit(connection.native_handle()));
    };
    SqlTransportTransaction<E>* active = nullptr;
    std::unique_ptr<UserContext> active_owner;
    Vector<Work> pending(r);
    auto poll = [&](Work& work) -> std::optional<Json> {
        if (work.begin) {
            if (auto* value = work.begin->poll()) {
                if (*value) {
                    if (active) {
                        throw std::logic_error("two active transaction owners");
                    }
                    active = &value->value().get();
                    active_owner = std::move(work.owner);
                }
                return result(r, *value, "ok");
            }
        } else if (auto* value = work.write->poll()) {
            return result(r, *value, "affected");
        }
        return {};
    };
    Json::Array steps(r), resumed(r);
    std::size_t index = 0;
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto op = text(field(action, "op"));
        Json value(r);
        if (op == "begin") {
            Work work{index, global.make_user_context()};
            work.begin = &provider.begin_sql(work.owner->memory_resource());
            if (auto finished = poll(work)) {
                value = std::move(*finished);
            } else {
                pending.push_back(std::move(work));
                auto out = detail::model_json(r);
                detail::model_put(out, "pending", true);
                value = Json(r, out);
            }
        } else if (op == "commit" || op == "rollback") {
            if (active) {
                auto t = active_owner->memory_resource();
                value = result(
                    r,
                    (op == "commit" ? active->commit_sql(t) : active->rollback_sql(t)).wait(),
                    "ok");
                active = nullptr;
                active_owner.reset();
            } else {
                value = result(r,
                               op == "commit" ? provider.commit_transaction(r)
                                              : provider.rollback_transaction(r),
                               "ok");
            }
        } else {
            auto t = active && op != "outside_execute" ? active_owner->memory_resource() : r;
            auto& query = detail::resource_arena(t).make<CompiledQuery>();
            query.sql = text(field(action, "sql"));
            for (const auto& entry : std::get<Json::Array>(field(action, "params").data)) {
                query.params.emplace_back(::value(t, entry));
            }
            auto& transport = active && op != "outside_execute"
                                  ? static_cast<SqlTransport<E>&>(*active)
                                  : static_cast<SqlTransport<E>&>(provider);
            if (op == "fetch") {
                value = rows(r, transport.fetch_all_compact_sql(t, query).wait());
            } else if (active && op != "outside_execute") {
                value = result(r, transport.execute_sql(t, query).wait(), "affected");
            } else {
                Work work{index, nullptr, nullptr, &provider.execute_sql(r, query)};
                if (auto finished = poll(work)) {
                    value = std::move(*finished);
                } else {
                    pending.push_back(std::move(work));
                    auto out = detail::model_json(r);
                    detail::model_put(out, "pending", true);
                    value = Json(r, out);
                }
            }
        }
        detail::model_put(value, "autocommit", autocommit());
        steps.emplace_back(std::move(value));
        for (std::size_t i = 0; i < pending.size();) {
            if (auto finished = poll(pending[i])) {
                detail::model_put(*finished, "autocommit", autocommit());
                auto event = detail::model_json(r);
                detail::model_put(event, "action", std::uint64_t(pending[i].index));
                detail::model_put(event, "result", *finished);
                resumed.emplace_back(event);
                pending.erase(pending.begin() + std::ptrdiff_t(i));
            } else {
                ++i;
            }
        }
        ++index;
    }
    Json::Array cancelled(r);
    for (auto& work : pending) {
        cancelled.emplace_back(std::uint64_t(work.index));
        if (work.begin) {
            work.begin->cancel();
        } else {
            work.write->cancel();
        }
    }
    pending.clear();
    bool dropped = active != nullptr;
    active = nullptr;
    active_owner.reset();
    auto& final_query = user_context->make<CompiledQuery>();
    final_query.sql = "SELECT id,version,name FROM records ORDER BY id";
    auto out = detail::model_json(r);
    detail::model_put(out, "steps", steps);
    detail::model_put(out, "resumed", resumed);
    detail::model_put(out, "cancelled", cancelled);
    detail::model_put(out, "dropped_active", dropped);
    detail::model_put(out, "final", rows(r, provider.fetch_all_compact_sql(r, final_query).wait()));
    detail::model_put(out, "autocommit", autocommit());
    return Json(global.memory_resource(), out);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto input = Json::parse(global.memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid schedule input");
        }
        auto out = observe(global, *input);
        std::cout << out.dump(global.memory_resource()) << '\n';
    }
}
