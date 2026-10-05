#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
#include <cstdlib>
using namespace teaql;

static ExecutionMetadata metadata(Memory r, const Json& j) {
    ExecutionMetadata m(std::allocator_arg,
                        Allocator(r),
                        DataServiceOperation(number<unsigned>(field(j, "operation"))));
    m.backend = text(field(j, "backend"));
    m.started_at = ExecutionMetadata::SystemTime(
        std::chrono::nanoseconds(number<std::int64_t>(field(j, "start"))));
    m.ended_at = ExecutionMetadata::SystemTime(
        std::chrono::nanoseconds(number<std::int64_t>(field(j, "end"))));
    auto opt = [&](std::optional<String>& out, const char* key) {
        const auto& x = field(j, key);
        if (std::holds_alternative<String>(x.data)) {
            out.emplace(text(x), r);
        }
    };
    opt(m.parameterized_query, "sql");
    opt(m.debug_query, "debug");
    opt(m.comment, "comment");
    opt(m.sql_log.database_kind, "database");
    m.sql_log.generated_sql = std::get<bool>(field(j, "generated").data);
    for (const auto& p : std::get<Json::Array>(field(j, "policies").data)) {
        m.sql_log.parameter_policies.push_back(SqlParameterLogPolicy(number<unsigned>(p)));
    }
    for (const auto& v : std::get<Json::Array>(field(j, "params").data)) {
        m.params.emplace_back(value(r, v));
    }
    for (const auto& n : std::get<Json::Array>(field(j, "trace").data)) {
        m.trace_chain.emplace_back(TraceKind(number<unsigned>(field(n, "kind"))),
                                   text(field(n, "entity")),
                                   text(field(n, "comment")),
                                   std::uint64_t(1001));
    }
    if (std::holds_alternative<String>(field(j, "count").data)) {
        m.result_count = number<std::size_t>(field(j, "count"));
    }
    if (std::holds_alternative<String>(field(j, "affected").data)) {
        m.affected_rows = number<std::uint64_t>(field(j, "affected"));
    }
    for (const auto& child : std::get<Json::Array>(field(j, "children").data)) {
        m.statements.emplace_back(metadata(r, child));
    }
    return m;
}

static Json snapshot(UserContext& c, UnifiedLogBuffer& buffer) {
    auto r = c.memory_resource();
    auto out = detail::model_json(r);
    Json::Array entries(r), unified(r);
    for (const auto& e : c.sql_logs()) {
        entries.emplace_back(detail::debug_sql_log(r, e));
    }
    for (const auto& e : buffer.entries) {
        auto x = detail::model_json(r);
        detail::model_put(x, "entry", detail::debug_sql_log(r, std::get<SqlLogEntry>(e.payload)));
        detail::model_put(x, "user", e.user_identifier ? Json(r, *e.user_identifier) : Json(r));
        detail::model_put(x, "timestamp", std::to_string(e.timestamp.time_since_epoch().count()));
        detail::model_put(
            x, "trace_matches", e.trace_chain == std::get<SqlLogEntry>(e.payload).trace_path);
        unified.emplace_back(std::move(x));
    }
    detail::model_put(out, "entries", entries);
    detail::model_put(out, "unified", unified);
    detail::model_put(out, "select", c.sql_log_options().select);
    detail::model_put(out, "mutation", c.sql_log_options().mutation);
    return out;
}

int main() {
    setenv("TEAQL_LOG_ENDPOINT", "off", 1);
    unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        auto& buffer = user->make<UnifiedLogBuffer>();
        user->insert_resource(*user, buffer);
        user->set_user_identifier_option({});
        Json::Array states(r);
        for (const auto& step : std::get<Json::Array>(field(*input, "steps").data)) {
            auto op = text(field(step, "op"));
            if (op == "record") {
                auto m = metadata(r, field(step, "metadata"));
                user->record_metadata_log(m);
            } else if (op == "clear") {
                user->clear_sql_logs();
            } else if (op == "disable") {
                user->disable_sql_log();
            } else if (op == "all") {
                user->enable_all_sql_log();
            } else if (op == "select_on") {
                user->enable_select_sql_log();
            } else if (op == "select_off") {
                user->disable_select_sql_log();
            } else if (op == "mutation_on") {
                user->enable_mutation_sql_log();
            } else if (op == "mutation_off") {
                user->disable_mutation_sql_log();
            } else if (op == "options") {
                user->set_sql_log_options({std::get<bool>(field(step, "select").data),
                                           std::get<bool>(field(step, "mutation").data)});
            } else if (op == "user") {
                const auto& v = field(step, "value");
                user->set_user_identifier_option(std::holds_alternative<String>(v.data)
                                                     ? std::optional<std::string_view>(text(v))
                                                     : std::nullopt);
            }
            states.emplace_back(snapshot(*user, buffer));
        }
        std::cout << Json(r, states).dump(r) << '\n';
    }
}
