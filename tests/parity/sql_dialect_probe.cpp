#include "fixture_sql.hpp"
#include <iostream>

static Json evaluate(Memory r, const Json& input) {
    FixtureDialect d;
    if (auto p = optional(input, "numbered"))
        d.numbered = std::get<bool>(p->data);
    auto out = detail::model_json(r);
    auto mode = text(field(input, "mode"));
    if (mode == "identifier") {
        auto name = text(field(input, "name"));
        detail::model_put(
            out, "quoted", quote_identifier_if_needed(r, name, text(field(input, "quote"))[0]));
        detail::model_put(
            out, "bounded", bounded_sql_identifier(r, name, integer(field(input, "max"))));
        return out;
    }
    if (mode == "coverage") {
        detail::model_put(out,
                          "length",
                          storage_length_covers(integer(field(input, "length")),
                                                uint_option(input, "actual_length")));
        detail::model_put(out,
                          "numeric",
                          storage_numeric_covers(integer(field(input, "precision")),
                                                 integer(field(input, "scale")),
                                                 uint_option(input, "actual_precision"),
                                                 uint_option(input, "actual_scale")));
        return out;
    }
    auto e = entity(r, field(input, "entity"));
    SqlBindings bindings(r);
    try {
        String sql(r);
        if (mode == "expr")
            sql = d.compile_expr(r, e, expression(r, field(input, "expr")), bindings);
        else if (mode == "select") {
            auto q = query(r, field(input, "query"));
            sql = d.compile_select_sql(r, e, q, bindings);
            auto full = d.compile_select(r, e, q);
            detail::model_put(out, "with_comment", full.sql_with_comment(r));
            detail::model_put(out, "comment", full.comment ? Json(r, *full.comment) : Json(r));
            if (full.sql != sql || full.params.size() != bindings.size())
                throw std::runtime_error("inconsistent select entry points");
        } else if (mode == "create")
            sql = d.compile_create_table(r, e);
        else if (mode == "add")
            sql = d.compile_add_column(r, e, e.properties.at(0));
        else
            throw std::runtime_error("unknown mode");
        detail::model_put(out, "sql", sql);
    } catch (const SqlCompileError& err) {
        detail::model_put(out, "error", err.to_string(r));
    }
    if (auto p = optional(input, "after"))
        detail::model_put(out, "after", d.compile_expr(r, e, expression(r, *p), bindings));
    Json::Array policies(r), values(r);
    auto log = bindings.log_context(r, d.kind());
    for (auto p : log.parameter_policies)
        policies.emplace_back(unsigned(p));
    for (const auto& v : bindings)
        values.emplace_back(v.to_json_value(r));
    detail::model_put(out, "policies", policies);
    detail::model_put(out, "values", values);
    detail::model_put(out, "generated", log.generated_sql);
    return out;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input)
            throw std::runtime_error("invalid fixture JSON");
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
