#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>
using namespace teaql;

static Json rows(Memory r, const Vector<CompactRow>& input) {
    Json::Array out(r);
    for (const auto& row : input) {
        Json::Array values(r);
        for (const auto& value : row.values()) {
            values.emplace_back(value.to_json_value(r));
        }
        out.emplace_back(values);
    }
    return Json(r, out);
}

static Json observe(GlobalContext& global, const Json& input) {
    auto tx_user_context = global.make_user_context();
    auto waiting_user_context = global.make_user_context();
    auto r = waiting_user_context->memory_resource();
    auto t = tx_user_context->memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto& setup = waiting_user_context->make<CompiledQuery>();
    setup.sql = "CREATE TABLE records(id INTEGER PRIMARY KEY,name TEXT)";
    (void)provider.execute(r, setup).value();
    for (std::uint64_t id = 1; id <= std::get<std::uint64_t>(field(input, "count").data); ++id) {
        setup.sql = "INSERT INTO records VALUES(";
        setup.sql += std::to_string(id);
        setup.sql += ",'before')";
        (void)provider.execute(r, setup).value();
    }
    auto& transaction = provider.begin_sql(t).wait().value().get();
    auto& update = tx_user_context->make<CompiledQuery>();
    update.sql = "UPDATE records SET name='tx' WHERE id=1";
    auto changed = std::get<bool>(field(input, "write").data)
                       ? transaction.execute_sql(t, update).wait().value()
                       : 0;
    auto& inside_query = tx_user_context->make<CompiledQuery>();
    inside_query.sql = "SELECT id,name FROM records ORDER BY id";
    // Output values live in the waiting Context, independent of transaction close.
    auto inside = rows(r, transaction.fetch_all_compact_sql(t, inside_query).wait().value());
    auto& select = waiting_user_context->make<CompiledQuery>();
    select.sql = inside_query.sql;
    auto& outside = waiting_user_context->make<CompiledQuery>();
    outside.sql = "UPDATE records SET name='outside' WHERE id=1";
    auto parent = text(field(input, "parent"));
    auto finish = [&](auto& work, auto convert) {
        Json::Array pending(r);
        auto ticket = work.wake_signal().ticket();
        for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(input, "polls").data); ++i) {
            pending.emplace_back(!work.poll());
        }
        bool cancelled = parent == "cancel";
        if (cancelled) {
            work.cancel();
        }
        // Compare release notifications, independently of cancellation notification.
        ticket = work.wake_signal().ticket();
        auto end = text(field(input, "end"));
        if (end == "commit") {
            (void)transaction.commit_sql(t).wait().value();
        } else if (end == "rollback") {
            (void)transaction.rollback_sql(t).wait().value();
        }
        // Actual Context destruction is the C++ drop/abandon adaptation.
        tx_user_context.reset();
        bool woke = work.wake_signal().ticket() != ticket;
        Json result(r);
        if (!cancelled) {
            result = convert(work.wait().value());
        }
        auto after = rows(r, provider.fetch_all_compact_sql(r, select).wait().value());
        auto out = detail::model_json(r);
        detail::model_put(out, "changed", changed);
        detail::model_put(out, "inside", inside);
        detail::model_put(out, "pending", pending);
        detail::model_put(out, "cancelled", cancelled);
        detail::model_put(out, "woke", woke);
        detail::model_put(out, "result", result);
        detail::model_put(out, "after", after);
        detail::model_put(
            out, "autocommit", bool(sqlite3_get_autocommit(connection.native_handle())));
        return Json(r, out);
    };
    Json output(r);
    if (parent == "read") {
        output = finish(provider.fetch_all_compact_sql(r, select), [&](const auto& values) {
            auto out = detail::model_json(r);
            detail::model_put(out, "rows", rows(r, values));
            return Json(r, out);
        });
    } else if (parent == "begin") {
        output = finish(provider.begin_sql(r), [&](const auto& handle) {
            (void)handle.get().rollback_sql(r).wait().value();
            auto out = detail::model_json(r);
            detail::model_put(out, "began", true);
            return Json(r, out);
        });
    } else {
        output = finish(provider.execute_sql(r, outside), [&](std::uint64_t affected) {
            auto out = detail::model_json(r);
            detail::model_put(out, "affected", affected);
            return Json(r, out);
        });
    }
    return Json(global.memory_resource(), output);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto input = Json::parse(global.memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid lease input");
        }
        auto output = observe(global, *input);
        std::cout << output.dump(global.memory_resource()) << '\n';
    }
}
