#include "../support/context_query_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;
using namespace context_query_fixture;

static bool flag(const Json& j, std::string_view k) {
    auto* v = optional(j, k);
    return v && std::get<bool>(v->data);
}

static std::uint64_t number(const Json& j, std::string_view k) {
    auto* v = optional(j, k);
    return v ? std::visit(
                   [](const auto& value) -> std::uint64_t {
                       using T = std::decay_t<decltype(value)>;
                       if constexpr (std::integral<T>) {
                           return value;
                       }
                       return 0;
                   },
                   v->data)
             : 0;
}

static Json evaluate(UserContext& c, const Json& j) {
    auto r = c.memory_resource();
    Config config{flag(j, "noop"),
                  flag(j, "start_fail"),
                  flag(j, "finish_fail"),
                  flag(j, "failure"),
                  flag(j, "bad_model"),
                  flag(j, "event"),
                  unsigned(number(j, "pending")),
                  text(field(j, "activation"))};
    auto& state = c.make<State>(config);
    auto& telemetry = c.make<Telemetry>(state);
    c.set_runtime_telemetry(c, telemetry);
    c.disable_sql_log();
    c.set_user_identifier_option({});
    if (flag(j, "capture")) {
        c.enable_select_sql_log();
    }
    if (auto* user = optional(j, "user");
        user && !std::holds_alternative<std::monostate>(user->data)) {
        c.set_user_identifier(text(*user));
    }
    auto& mock = c.make<Mock>(state);
    auto& repo = service(c, mock);
    auto& query = c.make<SelectQuery>("Item");
    if (auto* purpose = optional(j, "purpose");
        purpose && !std::holds_alternative<std::monostate>(purpose->data)) {
        query.purpose_text.emplace(text(*purpose), r);
    }
    if (auto* comment = optional(j, "comment");
        comment && !std::holds_alternative<std::monostate>(comment->data)) {
        query.comment_text.emplace(text(*comment), r);
    }
    if (flag(j, "trace")) {
        query.trace_chain.emplace_back(TraceKind::Request, "Item", "reason");
        query.trace_chain.emplace_back(TraceKind::Relation, "Child", "nested");
        query.trace_chain.back().entity_id = 7;
    }
    Json out = detail::model_json(r);
    auto drive = [&](auto& op, bool loaded) {
        bool ready = false;
        for (std::uint64_t i = 0; i < number(j, "polls"); ++i) {
            if (auto* result = op.poll()) {
                ready = true;
                if (!*result) {
                    detail::model_put(out, "error", result->error().to_string(r));
                } else {
                    detail::model_put(out,
                                      "size",
                                      std::uint64_t(std::distance(result->value().begin(),
                                                                  result->value().end())));
                    detail::model_put(out, "loaded", loaded);
                }
                break;
            }
        }
        if (!ready) {
            op.cancel();
            detail::model_put(out, "cancelled", true);
        }
    };
    auto mode = text(field(j, "mode"));
    if (mode == "all") {
        drive(repo.fetch_all(query), false);
    } else if (mode == "list") {
        drive(repo.fetch_smart_list(query), true);
    } else if (mode == "entities") {
        drive(repo.fetch_entities<decoder_fixture::Item<>>(query), true);
    } else {
        drive(repo.fetch_enhanced_entities<decoder_fixture::Item<>>(query), true);
    }
    Json::Array notes(r);
    for (const auto& note : state.notes) {
        notes.emplace_back(note);
    }
    detail::model_put(out, "notes", notes);
    Json::Array ops(r);
    for (const auto& operation : state.operations) {
        if (operation.family == "provider") {
            auto kind = detail::resource_type_name<Mock>();
            String expected(kind, r);
            expected += ".query";
            auto string_attribute = [&](std::string_view key) -> std::string_view {
                auto found = operation.attributes.find(key);
                if (found == operation.attributes.end()) {
                    throw std::logic_error("provider attribute missing");
                }
                return std::get<String>(found->second.data);
            };
            if (operation.name != expected || string_attribute("teaql.provider.kind") != kind ||
                string_attribute("teaql.provider.operation") != "query" ||
                string_attribute("teaql.operation.name") != expected) {
                throw std::logic_error("provider identity does not match the actual executor type");
            }
        }
        auto v = detail::model_json(r);
        detail::model_put(v, "family", operation.family);
        detail::model_put(v,
                          "name",
                          operation.family == "provider" ? std::string_view("Mock.query")
                                                         : std::string_view(operation.name));
        auto attrs = detail::model_json(r);
        for (const auto& [key, value] : operation.attributes) {
            if (key == "teaql.provider.kind") {
                detail::model_put(attrs, key, "Mock");
            } else if (key == "teaql.operation.name" && operation.family == "provider") {
                detail::model_put(attrs, key, "Mock.query");
            } else {
                std::visit(
                    [&](const auto& item) {
                        detail::model_put(attrs, key, item);
                    },
                    value.data);
            }
        }
        detail::model_put(v, "attributes", attrs);
        ops.emplace_back(std::move(v));
    }
    detail::model_put(out, "operations", ops);
    Json request(r);
    if (state.request) {
        request = detail::model_json(r);
        auto& q = *state.request;
        detail::model_put(request, "comment", q.comment());
        detail::model_put(request, "trace", q.trace_chain.size());
        detail::model_put(request, "capture", q.capture_execution_metadata);
        detail::model_put(request, "debug", q.capture_debug_query);
    }
    detail::model_put(out, "request", request);
    Json::Array logs(r);
    for (const auto& log : c.sql_logs()) {
        logs.emplace_back(log.result_count.value_or(0));
    }
    detail::model_put(out, "logs", logs);
    return out;
}

int main() {
    setenv("TEAQL_LOG_ENDPOINT", "off", 1);
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext g;
        auto c = g.make_user_context();
        auto j = Json::parse(c->memory_resource(), line);
        std::cout << evaluate(*c, *j).dump(c->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
