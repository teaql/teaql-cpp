#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/runtime/module.hpp>
#include "fixture_sql.hpp"
#include <sqlite3.h>
#include <iostream>
using namespace teaql;

static bool boolean(const Json& j, std::string_view key) {
    auto* p = optional(j, key);
    return p && std::get<bool>(p->data);
}

static EntityDescriptor schema_entity(Memory r, const Json& j) {
    auto e = entity(r, j);
    if (auto* p = optional(j, "service")) {
        e.service(text(*p));
    }
    for (const auto& v : items(j, "relations")) {
        auto& rel = e.relation(text(field(v, "name")), text(field(v, "target")));
        if (auto* p = optional(v, "local")) {
            rel.local(text(*p));
        }
        if (auto* p = optional(v, "foreign")) {
            rel.foreign(text(*p));
        }
        rel.many = boolean(v, "many");
    }
    return e;
}

struct Sink final : RawAuditEventSink {
    using allocator_type = Allocator;
    sqlite3* db;
    mutable Json::Array events;
    bool fail;

    Sink(std::allocator_arg_t, Allocator a, sqlite3* db, bool fail)
        : db(db), events(a.resource()), fail(fail) {}

    Result<std::monostate, RuntimeError>
    on_event(Memory r, const UserContext&, const RawAuditEvent& e) const override {
        auto out = detail::model_json(events.get_allocator().resource());
        constexpr std::string_view names[]{"Created",
                                           "Updated",
                                           "Deleted",
                                           "Recovered",
                                           "SchemaCreated",
                                           "SchemaVerified",
                                           "FieldAdded",
                                           "DataSeeded"};
        detail::model_put(out, "kind", names[unsigned(e.kind)]);
        detail::model_put(out, "entity", e.entity);
        detail::model_put(out, "values", record_to_json_value(out.memory_resource(), e.values));
        detail::model_put(out, "autocommit", bool(sqlite3_get_autocommit(db)));
        detail::model_put(out, "bootstrap", e.bootstrap_audit.has_value());
        events.emplace_back(std::move(out));
        return fail ? Result<std::monostate, RuntimeError>::err(
                          r, RuntimeError::event(r, "fixture sink error"))
                    : Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

static void execute(sqlite3* db, std::string_view sql) {
    std::string copy(sql);
    if (sqlite3_exec(db, copy.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

static Json sql_rows(Memory r, sqlite3* db, std::string_view sql) {
    struct Statement {
        sqlite3_stmt* p = nullptr;

        ~Statement() {
            sqlite3_finalize(p);
        }
    } s;

    if (sqlite3_prepare_v2(db, sql.data(), int(sql.size()), &s.p, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    Json::Array all(r);
    int code;
    while ((code = sqlite3_step(s.p)) == SQLITE_ROW) {
        Json::Array row(r);
        for (int i = 0; i < sqlite3_column_count(s.p); ++i) {
            switch (sqlite3_column_type(s.p, i)) {
            case SQLITE_NULL:
                row.emplace_back();
                break;
            case SQLITE_INTEGER:
                row.emplace_back(std::int64_t(sqlite3_column_int64(s.p, i)));
                break;
            case SQLITE_FLOAT:
                row.emplace_back(sqlite3_column_double(s.p, i));
                break;
            case SQLITE_TEXT:
                row.emplace_back(
                    std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(s.p, i)),
                                     sqlite3_column_bytes(s.p, i)));
                break;
            case SQLITE_BLOB: {
                Json::Array bytes(r);
                auto* b = static_cast<const unsigned char*>(sqlite3_column_blob(s.p, i));
                for (int n = 0; n < sqlite3_column_bytes(s.p, i); ++n) {
                    bytes.emplace_back(std::uint64_t(b[n]));
                }
                row.emplace_back(bytes);
                break;
            }
            }
        }
        all.emplace_back(row);
    }
    if (code != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    return Json(r, all);
}

static Json evaluate(GlobalContext& global, UserContext& owner, const Json& j) {
    auto r = owner.memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make_resource<SqliteMutationExecutor>(connection);
    auto* db = connection.native_handle();
    for (const auto& sql : items(j, "setup")) {
        execute(db, text(sql));
    }
    Json::Array rounds(r);
    for (const auto& round : items(j, "rounds")) {
        for (const auto& sql : items(round, "sql")) {
            execute(db, text(sql));
        }
        auto context = global.make_user_context();
        auto cr = context->memory_resource();
        auto& sink = global.make<Sink>(db, boolean(round, "sink_error"));
        auto& store = context->make<InMemoryMetadataStore>();
        for (const auto& v : items(round, "entities")) {
            store.register_entity(schema_entity(cr, v));
        }
        context->set_metadata(*context, store);
        context->set_event_sink(global, sink);
        use_sqlite_provider(*context, global, provider);
        if (auto* legacy = optional(round, "legacy")) {
            GraphNode node(std::allocator_arg, Allocator(cr), "Legacy");
            if (text(*legacy) == "initial") {
                context->set_initial_graphs({&node, 1});
            } else if (text(*legacy) == "root") {
                context->set_root_graphs({&node, 1});
            }
        }
        const auto& result = context->ensure_schema().wait();
        auto outcome = detail::model_json(r);
        if (result) {
            detail::model_put(outcome, "ok", true);
        } else {
            detail::model_put(outcome, "message", result.error().message());
            detail::model_put(outcome, "display", result.error().to_string(r));
        }
        Json::Array queries(r);
        for (const auto& q : items(round, "queries")) {
            queries.emplace_back(sql_rows(r, db, text(q)));
        }
        auto out = detail::model_json(r);
        detail::model_put(out, "result", outcome);
        detail::model_put(out, "events", sink.events);
        detail::model_put(out, "autocommit", bool(sqlite3_get_autocommit(db)));
        detail::model_put(
            out,
            "master",
            sql_rows(r, db, "SELECT type,name,tbl_name,sql FROM sqlite_master ORDER BY type,name"));
        detail::model_put(out, "queries", queries);
        rounds.emplace_back(std::move(out));
    }
    auto tables = detail::model_json(r);
    for (const auto& name : items(j, "inspect")) {
        auto table = text(name);
        auto out = detail::model_json(r);
        auto exists = provider.table_exists(r, table);
        if (!exists) {
            throw std::runtime_error("table exists fixture");
        }
        detail::model_put(out, "exists", exists.value());
        auto columns = provider.table_columns(r, table);
        auto values = detail::model_json(r);
        if (columns) {
            for (const auto& [key, v] : columns.value()) {
                Json::Array pair(r);
                pair.emplace_back(v.first);
                pair.emplace_back(v.second);
                detail::model_put(values, key, pair);
            }
        } else {
            detail::model_put(values, "error", columns.error().to_string(r));
        }
        detail::model_put(out, "columns", values);
        detail::model_put(tables, table, out);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "rounds", rounds);
    detail::model_put(out, "tables", tables);
    return out;
}

int main() {
    setenv("TEAQL_LOG_ENDPOINT", "off", 1);
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto owner = global.make_user_context();
        auto r = owner->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("invalid JSON fixture");
        }
        std::cout << evaluate(global, *owner, *input).dump(r) << '\n';
    }
}
