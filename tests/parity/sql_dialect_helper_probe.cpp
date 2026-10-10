#include "fixture_sql.hpp"
#include <iostream>

static Json evaluate(Memory owner, const Json& input) {
    auto output = detail::model_json(owner);
    auto mode = text(field(input, "mode"));
    if (mode == "in_direct") {
        FixtureDialect dialect;
        dialect.numbered = std::get<bool>(field(input, "numbered").data);
        auto descriptor = entity(owner, field(input, "entity"));
        auto left = expression(owner, field(input, "left"));
        auto right = expression(owner, field(input, "right"));
        auto op = static_cast<BinaryOp>(integer(field(input, "op")));
        SqlBindings params(owner);
        try {
            detail::model_put(
                output, "sql", dialect.compile_in(owner, descriptor, left, op, right, params));
        } catch (const SqlCompileError& error) {
            detail::model_put(output, "error", error.to_string(owner));
        } catch (const std::logic_error& error) {
            if (std::string_view(error.what()) != "Invalid direct IN operator") {
                throw;
            }
            detail::model_put(output, "panic", "operator");
        }
        Json::Array values(owner), policies(owner);
        for (const auto& item : params.values()) {
            values.emplace_back(item.to_json_value(owner));
        }
        auto log = params.log_context(owner, dialect.kind());
        for (auto policy : log.parameter_policies) {
            policies.emplace_back(static_cast<unsigned>(policy));
        }
        detail::model_put(output, "values", values);
        detail::model_put(output, "policies", policies);
    } else if (mode == "quote") {
        auto code = static_cast<char32_t>(integer(field(input, "quote_code")));
        detail::model_put(
            output,
            "quoted",
            quote_identifier_if_needed(owner, text(field(input, "identifier")), code));
    } else {
        detail::model_put(output,
                          "length",
                          storage_length_covers(integer(field(input, "expected_length")),
                                                uint_option(input, "actual_length")));
        detail::model_put(output,
                          "numeric",
                          storage_numeric_covers(integer(field(input, "expected_precision")),
                                                 integer(field(input, "expected_scale")),
                                                 uint_option(input, "actual_precision"),
                                                 uint_option(input, "actual_scale")));
    }
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::invalid_argument("Invalid dialect helper input");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
