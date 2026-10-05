#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/id.hpp>
#include <teaql/provider_sqlite/business_id.hpp>
#include <teaql/sql/dialect.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>
using namespace teaql;

static std::uint64_t integer(const Json& j, std::string_view key) {
    return std::get<std::uint64_t>(field(j, key).data);
}

static Json error(Memory r, const MutationExecutorError& e) {
    auto out = detail::model_json(r);
    constexpr std::string_view kinds[] = {
        "Sqlite", "SqlCompile", "UnsupportedValue", "UnsupportedColumnType", "Bind", "Lock"};
    detail::model_put(out, "error", e.to_string(r));
    detail::model_put(out, "kind", kinds[unsigned(e.kind())]);
    detail::model_put(out, "code", e.sqlite_extended_code());
    return out;
}

static Json rows(Memory r, std::span<const CompactRow> rows) {
    Json::Array array(r);
    for (const auto& row : rows) {
        auto out = detail::model_json(r);
        Json::Array keys(r), values(r);
        for (const auto& key : row.keys()) {
            keys.emplace_back(key);
        }
        for (const auto& v : row.values()) {
            values.emplace_back(v.to_json_value(r));
        }
        detail::model_put(out, "keys", keys);
        detail::model_put(out, "values", values);
        array.emplace_back(std::move(out));
    }
    return Json(r, array);
}

static Json chunk(Memory r, const StreamChunk& c) {
    auto out = detail::model_json(r);
    detail::model_put(out, "rows", rows(r, c.rows));
    detail::model_put(out, "index", c.chunk_index);
    detail::model_put(out, "last", c.is_last);
    return out;
}

static CompiledQuery query(Memory r, std::string_view sql) {
    CompiledQuery q(std::allocator_arg, Allocator(r));
    q.sql = sql;
    return q;
}

static Json transaction_steps(Memory r, SqliteMutationExecutor& provider, const Json& j) {
    Json::Array results(r);
    SqlTransportTransaction<MutationExecutorError>* tx = nullptr;
    const bool asynchronous = std::get<bool>(field(j, "async").data);
    for (const auto& action : std::get<Json::Array>(field(j, "actions").data)) {
        auto result = detail::model_json(r);
        auto op = text(field(action, "op"));
        auto unit = [&](const auto& value) {
            if (value) {
                detail::model_put(result, "ok", true);
            } else {
                result = error(r, value.error());
            }
        };
        if (op == "begin") {
            if (asynchronous) {
                const auto& value = provider.begin_sql(r).wait();
                unit(value);
                if (value) {
                    tx = &value.value().get();
                }
            } else {
                unit(provider.begin_transaction(r));
            }
        } else if (op == "commit" || op == "rollback") {
            if (asynchronous && tx) {
                auto* previous = tx;
                tx = nullptr;
                unit((op == "commit" ? previous->commit_sql(r) : previous->rollback_sql(r)).wait());
            } else {
                unit(op == "commit" ? provider.commit_transaction(r)
                                    : provider.rollback_transaction(r));
            }
        } else {
            auto q = query(r, text(field(action, "sql")));
            for (const auto& p : std::get<Json::Array>(field(action, "params").data)) {
                q.params.emplace_back(value(r, p));
            }
            auto& transport = (asynchronous && tx && op != "outside_execute")
                                  ? static_cast<SqlTransport<MutationExecutorError>&>(*tx)
                                  : static_cast<SqlTransport<MutationExecutorError>&>(provider);
            if (op == "fetch") {
                const auto& value = transport.fetch_all_compact_sql(r, q).wait();
                if (value) {
                    detail::model_put(result, "rows", rows(r, value.value()));
                } else {
                    result = error(r, value.error());
                }
            } else {
                const auto& value = transport.execute_sql(r, q).wait();
                if (value) {
                    detail::model_put(result, "affected", value.value());
                } else {
                    result = error(r, value.error());
                }
            }
        }
        detail::model_put(result,
                          "autocommit",
                          bool(sqlite3_get_autocommit(provider.connection().native_handle())));
        results.emplace_back(std::move(result));
    }
    auto result = detail::model_json(r);
    detail::model_put(result, "steps", results);
    return result;
}

static Json id_steps(UserContext& context, SqliteMutationExecutor& provider, const Json& j) {
    auto r = context.memory_resource();
    auto& generator = context.make<SqliteIdSpaceGenerator>(provider);
    auto table = text(field(j, "table"));
    generator.with_table_name(table);
    Json::Array results(r);
    for (const auto& action : std::get<Json::Array>(field(j, "actions").data)) {
        auto result = detail::model_json(r);
        auto op = text(field(action, "op"));
        if (op == "sql") {
            auto value = provider.execute(r, query(r, text(field(action, "sql"))));
            if (value) {
                detail::model_put(result, "affected", value.value());
            } else {
                result = error(r, value.error());
            }
        } else if (op == "next") {
            auto value = generator.next_id(r, text(field(action, "entity")));
            if (value) {
                detail::model_put(result, "id", value.value());
            } else {
                result = error(r, value.error());
            }
        } else {
            auto value = op == "ensure" ? generator.ensure_table(r)
                                        : generator.ensure_floor(r,
                                                                 text(field(action, "entity")),
                                                                 integer(action, "floor"));
            if (value) {
                detail::model_put(result, "ok", true);
            } else {
                result = error(r, value.error());
            }
        }
        results.emplace_back(std::move(result));
    }
    String sql("SELECT type_name,current_level,typeof(current_level) AS storage_type FROM ", r);
    sql += quote_identifier_if_needed(r, table, '"');
    sql += " ORDER BY type_name";
    auto stored = provider.fetch_all_compact(r, query(r, sql));
    auto result = detail::model_json(r);
    detail::model_put(result, "steps", results);
    detail::model_put(result, "space", stored ? rows(r, stored.value()) : error(r, stored.error()));
    return result;
}

static Json business_error_json(Memory r, const BusinessIdError& error) {
    auto result = detail::model_json(r);
    detail::model_put(result, "code", business_id_error_code_name(error.code));
    detail::model_put(result, "message", error.message);
    detail::model_put(result, "display", error.to_string(r));
    return result;
}

static Json business_steps(UserContext& context, SqliteMutationExecutor& provider, const Json& j) {
    auto r = context.memory_resource();
    auto& allocator = context.make<SqliteBusinessIdAllocator>(provider);
    auto table = text(field(j, "table"));
    allocator.with_table_name(table);
    Json::Array results(r);
    for (const auto& action : std::get<Json::Array>(field(j, "actions").data)) {
        auto result = detail::model_json(r);
        auto op = text(field(action, "op"));
        if (op == "sql") {
            auto value = provider.execute(r, query(r, text(field(action, "sql"))));
            if (value) {
                detail::model_put(result, "affected", value.value());
            } else {
                result = error(r, value.error());
            }
        } else if (op == "ensure") {
            auto value = allocator.ensure_schema(r);
            if (value) {
                detail::model_put(result, "ok", true);
            } else {
                result = business_error_json(r, value.error());
            }
        } else {
            const auto& parts = std::get<Json::Array>(field(action, "scope").data);
            BusinessIdScope scope(std::allocator_arg,
                                  Allocator(r),
                                  text(parts[0]),
                                  text(parts[1]),
                                  text(parts[2]),
                                  text(parts[3]));
            BusinessIdPlan plan(std::allocator_arg,
                                Allocator(r),
                                scope,
                                "ignored",
                                std::nullopt,
                                0,
                                "",
                                integer(action, "initial"),
                                integer(action, "maximum"));
            auto value = allocator.allocate(r, plan);
            if (value) {
                detail::model_put(result, "sequence", value.value().sequence);
                detail::model_put(result, "scope", value.value().scope.canonical_key(r));
            } else {
                result = business_error_json(r, value.error());
            }
        }
        detail::model_put(result,
                          "autocommit",
                          bool(sqlite3_get_autocommit(provider.connection().native_handle())));
        results.emplace_back(std::move(result));
    }
    String sql(
        "SELECT scope_key,current_level,version,typeof(current_level) AS level_type,typeof(version) AS version_type FROM ",
        r);
    sql += quote_identifier_if_needed(r, table, '"');
    sql += " ORDER BY scope_key";
    auto stored = provider.fetch_all_compact(r, query(r, sql));
    auto result = detail::model_json(r);
    detail::model_put(result, "steps", results);
    detail::model_put(result, "space", stored ? rows(r, stored.value()) : error(r, stored.error()));
    return result;
}

static Json evaluate(UserContext& context, const Json& j) {
    auto r = context.memory_resource();
    auto& connection = context.make_resource<SqliteConnection>();
    auto& provider = context.make<SqliteMutationExecutor>(connection);
    sqlite3_exec(connection.native_handle(),
                 "CREATE TABLE records(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
                 nullptr,
                 nullptr,
                 nullptr);
    for (std::uint64_t i = 1; i <= integer(j, "count"); ++i) {
        auto sql = "INSERT INTO records VALUES(" + std::to_string(i) + ",1,'name-" +
                   std::to_string(i) + "')";
        if (sqlite3_exec(connection.native_handle(), sql.c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
            throw std::runtime_error("fixture insert");
        }
    }
    auto q = query(r, text(field(j, "sql")));
    for (const auto& p : std::get<Json::Array>(field(j, "params").data)) {
        q.params.emplace_back(value(r, p));
    }
    if (auto* comment = optional(j, "comment")) {
        if (std::holds_alternative<String>(comment->data)) {
            q.comment.emplace(text(*comment), r);
        }
    }
    auto out = detail::model_json(r), outcome = detail::model_json(r);
    auto mode = text(field(j, "mode"));
    try {
        if (mode == "business_id") {
            outcome = business_steps(context, provider, j);
        } else if (mode == "id_space") {
            outcome = id_steps(context, provider, j);
        } else if (mode == "transaction") {
            outcome = transaction_steps(r, provider, j);
        } else if (mode == "execute") {
            const auto& result = provider.execute_sql(r, q).wait();
            if (!result) {
                outcome = error(r, result.error());
            } else {
                detail::model_put(outcome, "affected", result.value());
            }
        } else if (mode == "fetch" || mode == "repeated") {
            Vector<Value> values(r);
            if (mode == "repeated") {
                for (const auto& v : std::get<Json::Array>(field(j, "values").data)) {
                    values.emplace_back(value(r, v));
                }
            }
            const auto& result =
                (mode == "fetch"
                     ? provider.fetch_all_compact_sql(r, q)
                     : provider.fetch_repeated_compact_sql(r, q, integer(j, "index"), values))
                    .wait();
            if (!result) {
                outcome = error(r, result.error());
            } else {
                detail::model_put(outcome, "rows", rows(r, result.value()));
            }
        } else if (mode == "buffered") {
            auto result = provider.fetch_stream(r, q, integer(j, "size"));
            if (!result) {
                outcome = error(r, result.error());
            } else {
                Json::Array chunks(r);
                for (const auto& c : result.value()) {
                    chunks.emplace_back(chunk(r, c));
                }
                detail::model_put(outcome, "chunks", chunks);
            }
        } else if (mode == "stream") {
            auto& stream = provider.stream_sql(r, std::move(q), integer(j, "size"));
            Json::Array polls(r);
            try {
                for (std::uint64_t i = 0; i < integer(j, "polls"); ++i) {
                    auto next = stream.poll_next();
                    if (next.kind ==
                        StreamPoll<Result<StreamChunk, MutationExecutorError>>::Kind::End) {
                        auto end = detail::model_json(r);
                        detail::model_put(end, "end", true);
                        polls.emplace_back(std::move(end));
                        break;
                    }
                    if (!next.item) {
                        throw std::runtime_error("unexpected pending fixture");
                    }
                    polls.emplace_back(*next.item ? chunk(r, next.item->value())
                                                  : error(r, next.item->error()));
                }
            } catch (...) {
                stream.cancel();
                throw;
            }
            stream.cancel();
            detail::model_put(outcome, "polls", polls);
        } else {
            throw std::runtime_error("unknown fixture mode");
        }
    } catch (const std::out_of_range&) {
        detail::model_put(outcome, "panic", true);
    }
    auto saved =
        provider.fetch_all_compact(r, query(r, "SELECT id,version,name FROM records ORDER BY id"));
    detail::model_put(out, "outcome", outcome);
    detail::model_put(out, "saved", saved ? rows(r, saved.value()) : error(r, saved.error()));
    return out;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("invalid fixture JSON");
        }
        std::cout << evaluate(*user, *input).dump(r) << '\n';
    }
}
