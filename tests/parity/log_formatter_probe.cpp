#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_formatter.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <bit>
#include <cstdlib>
#include <iostream>
using namespace teaql;

static Json evaluate(Memory r, const Json& j) {
    auto op = text(field(j, "op"));
    if (op == "key") {
        const auto& input = field(j, "id");
        auto id = [&] {
            if (text(field(input, "kind")) == "f64_bits") {
                const auto encoded = text(field(input, "value"));
                std::uint64_t bits = 0;
                auto [end, error] =
                    std::from_chars(encoded.data(), encoded.data() + encoded.size(), bits, 16);
                if (error != std::errc{} || end != encoded.data() + encoded.size()) {
                    throw std::runtime_error("Invalid floating-point fixture bits");
                }
                return Value(r, std::bit_cast<double>(bits));
            }
            return value(r, input);
        }();
        EntityKey k(std::allocator_arg, Allocator(r), text(field(j, "entity")), id);
        if (auto* mutation = optional(j, "mutated")) {
            k.id = value(r, *mutation);
        }
        return Json(r, k.debug(r));
    }
    if (op == "bound") {
        auto out = detail::model_json(r);
        detail::model_put(out,
                          "sql",
                          detail::truncate_sensitive_sql_log(
                              r, text(field(j, "text")), number<std::size_t>(field(j, "max"))));
        detail::model_put(out,
                          "audit",
                          detail::truncate_sensitive_audit_log(
                              r, text(field(j, "text")), number<std::size_t>(field(j, "max"))));
        return out;
    }
    constexpr std::string_view names[]{"TEAQL_AUDIT_LOG",
                                       "TEAQL_SQL_LOG",
                                       "TEAQL_TOOL_LOG",
                                       "TEAQL_AUDIT_LOG_ENTITIES",
                                       "TEAQL_SQL_LOG_TABLES",
                                       "TEAQL_TOOL_LOG_FOCUS"};
    for (auto n : names) {
        auto* v = optional(field(j, "env"), n);
        if (v && std::holds_alternative<String>(v->data)) {
            setenv(n.data(), std::get<String>(v->data).c_str(), 1);
        } else {
            unsetenv(n.data());
        }
    }
    auto config = LogConfig::load(r);
    auto out = detail::model_json(r);
    detail::model_put(out,
                      "levels",
                      Json::Array({Json(r, unsigned(config.audit_level)),
                                   Json(r, unsigned(config.sql_level)),
                                   Json(r, unsigned(config.tool_level))},
                                  r));
    auto list = [&](const std::optional<Vector<String>>& v) {
        if (!v) {
            return Json(r);
        }
        Json::Array a(r);
        for (const auto& s : *v) {
            a.emplace_back(s);
        }
        return Json(r, a);
    };
    detail::model_put(out, "entities", list(config.audit_entities));
    detail::model_put(out, "tables", list(config.sql_tables));
    detail::model_put(out, "focus", list(config.tool_focus));
    detail::model_put(out, "audit", config.should_log_audit(text(field(j, "entity"))));
    detail::model_put(
        out, "sensitive", config.should_log_sensitive_audit(text(field(j, "entity"))));
    detail::model_put(out, "sql", config.should_log_sql(text(field(j, "sql"))));
    detail::model_put(out, "tool", config.should_log_tool(text(field(j, "module"))));
    return out;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto j = Json::parse(r, line);
        if (!j) {
            throw std::runtime_error("invalid JSON");
        }
        std::cout << evaluate(r, *j).dump(r) << '\n';
    }
}
