#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

namespace req = teaql::request;

template <class Operation>
static Json outcome(Memory owner, Operation&& operation) {
    Json::Object output(owner);
    try {
        auto result = operation();
        output.emplace("ok", Json(owner, debug_string(owner, result)));
    } catch (const std::exception& error) {
        output.emplace("error", Json(owner, error.what()));
    }
    return Json(owner, std::move(output));
}

static String optional_debug(Memory owner, const std::optional<Expr>& value) {
    String output(owner);
    DebugWriter writer(output, {});
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_debug(out, *value);
        });
    } else {
        writer.raw("None");
    }
    return output;
}

static Json evaluate(Memory owner, const Json& input) {
    Json::Object output(owner);
    auto kind = text(field(input, "kind"));
    if (kind == "operators") {
        auto op = req::FieldOperator(std::get<std::uint64_t>(field(input, "operator").data));
        Vector<Value> values(owner);
        for (const auto& item : std::get<Json::Array>(field(input, "values").data)) {
            values.emplace_back(value(owner, item));
        }
        auto index = std::get<std::uint64_t>(field(input, "index").data);
        output.emplace("expression", outcome(owner, [&] {
                           return req::field_operator_expr(owner, "field", op, values);
                       }));
        output.emplace("column", outcome(owner, [&] {
                           return req::field_operator_column_expr(owner, "field", op, "other");
                       }));
        output.emplace("value", outcome(owner, [&] {
                           return req::required_value(owner, op, values, index);
                       }));
        output.emplace("text", outcome(owner, [&] {
                           return req::required_text(owner, op, values, index);
                       }));
    } else if (kind == "dynamic") {
        const auto& data = field(input, "json");
        output.emplace(
            "value",
            Json(owner, debug_string(owner, req::dynamic_json_value_to_teaql_value(owner, data))));
        auto values = req::dynamic_json_values(owner, data);
        String list(owner);
        DebugWriter writer(list, {});
        writer.sequence(values, DefaultDebugElement{});
        output.emplace("values", Json(owner, list));
        output.emplace("operator",
                       Json(owner, debug_string(owner, req::dynamic_json_operator(data))));
        output.emplace("expression", outcome(owner, [&] {
                           return req::dynamic_json_filter_expr(owner, "field", data);
                       }));
        Json::Object object(owner);
        object.emplace("field", Json(owner, data));
        auto n = req::dynamic_json_u64_field(object, "field");
        output.emplace("u64", n ? Json(owner, std::to_string(*n)) : Json(owner));
        auto missing = req::dynamic_json_u64_field(object, "absent");
        output.emplace("missing_u64",
                       missing ? Json(owner, std::to_string(*missing)) : Json(owner));
    } else if (kind == "filters") {
        auto target = Expr::eq(owner, "target", value(owner, field(input, "value")));
        auto live = Expr::gt(owner, "version", std::int64_t{0});
        auto shape = std::get<std::uint64_t>(field(input, "shape").data);
        Expr expression(owner, Expr::Kind::And);
        if (shape == 0) {
            expression = target.clone(owner);
        } else if (shape == 1 || shape == 2) {
            expression.kind = shape == 1 ? Expr::Kind::And : Expr::Kind::Or;
            expression.args.emplace_back(live.clone(owner));
            expression.args.emplace_back(target.clone(owner));
        } else if (shape == 3) {
            expression.kind = Expr::Kind::Not;
            expression.args.emplace_back(live.clone(owner));
        } else if (shape == 5) {
            Expr nested(owner, Expr::Kind::And);
            nested.args.emplace_back(live.clone(owner));
            nested.args.emplace_back(live.clone(owner));
            expression.args.emplace_back(std::move(nested));
            expression.args.emplace_back(target.clone(owner));
        } else if (shape != 4) {
            expression = live.clone(owner);
        }
        output.emplace("original", Json(owner, debug_string(owner, expression)));
        auto without_default =
            req::remove_default_live_filter(owner, std::optional<Expr>(expression.clone(owner)));
        auto without_target = req::remove_filter_expr(owner, expression, target);
        output.emplace("without_default", Json(owner, optional_debug(owner, without_default)));
        output.emplace("without_target", Json(owner, optional_debug(owner, without_target)));
        output.emplace(
            "absent",
            Json(owner, optional_debug(owner, req::remove_default_live_filter(owner, {}))));
    } else if (kind == "facets") {
        Vector<Value> data(owner);
        data.emplace_back(value(owner, field(input, "value")));
        SmartList<Value> rows(std::allocator_arg, Allocator(owner), std::move(data));
        rows.with_total_count(17);
        SmartList<CompactRow> empty(std::allocator_arg, Allocator(owner));
        rows.with_facet("replace", empty);
        Record fields(owner);
        fields.emplace("count", Value(owner, std::int64_t{1}));
        Vector<CompactRow> facet_data(owner);
        facet_data.emplace_back(fields);
        SmartList<CompactRow> facet(std::allocator_arg, Allocator(owner), std::move(facet_data));
        facet.with_total_count(9);
        std::pmr::map<String, SmartList<CompactRow>, std::less<>> facets(owner);
        facets.emplace("replace", std::move(facet));
        facets.emplace("雪", std::move(empty));
        req::attach_facets(rows, facets);
        output.emplace("list", Json(owner, debug_string(owner, rows)));
    } else {
        throw std::runtime_error("Unknown request helper fixture");
    }
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid request helper fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
