#include <teaql/sql.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

using Policy = SqlParameterLogPolicy;

static std::optional<Policy> policy(const Json& input) {
    if (auto* index = std::get_if<std::uint64_t>(&input.data)) {
        return static_cast<Policy>(*index);
    }
    return {};
}

static Json policy_result(Memory owner, std::optional<Policy> result) {
    return result ? Json(owner, debug_string(owner, *result)) : Json(owner);
}

static EntityDescriptor descriptor(Memory owner, std::uint64_t mode) {
    EntityDescriptor root(std::allocator_arg, Allocator(owner), "Device");
    root.property("plain", DataType::Text);
    root.property("masked", DataType::Text).column("db_masked");
    root.property("password", DataType::Text);
    root.property("token_alias", DataType::Text).column("access_token");
    if (mode == 1) {
        root.mask_fields({});
    } else if (mode == 2) {
        root.mask_fields({"masked"});
    } else if (mode == 3) {
        root.mask_fields({"plain", "password", "db_masked"});
    }
    return root;
}

static Expr expression(Memory owner, std::string_view field_name, std::uint64_t shape) {
    auto column = Expr::column(owner, field_name);
    if (shape == 0) {
        return column;
    }
    if (shape == 1) {
        return Expr::value(owner, "literal");
    }
    if (shape == 2) {
        return Expr::like_pattern(owner, "%original%", "original");
    }
    if (shape == 3) {
        return Expr::count_all(owner);
    }
    if (shape == 4) {
        Vector<Expr> args(owner);
        args.emplace_back(std::move(column));
        args.emplace_back(Expr::column(owner, "masked"));
        return Expr::function(owner, ExprFunction::Sum, args);
    }
    if (shape == 5) {
        return Expr::binary(std::move(column), BinaryOp::Eq, Expr::column(owner, "masked"));
    }
    if (shape == 9) {
        Expr result(owner, Expr::Kind::SubQuery);
        result.op = BinaryOp::In;
        result.args.emplace_back(std::move(column));
        auto child = descriptor(owner, 3);
        SelectQuery query(std::allocator_arg, Allocator(owner), "Child");
        query.filter(Expr::eq(owner, "password", "child-secret"));
        result.subquery_data = detail::make_box<SubQueryExpr>(owner, child, query);
        return result;
    }
    if (shape == 13) {
        return Expr::soundex(column);
    }
    auto kind = shape == 6    ? Expr::Kind::Between
                : shape == 7  ? Expr::Kind::IsNull
                : shape == 8  ? Expr::Kind::IsNotNull
                : shape == 10 ? Expr::Kind::And
                : shape == 11 ? Expr::Kind::Or
                              : Expr::Kind::Not;
    Expr result(owner, kind);
    result.args.emplace_back(std::move(column));
    if (shape == 6) {
        result.args.emplace_back(Expr::value(owner, 1));
        result.args.emplace_back(Expr::column(owner, "masked"));
    } else if (shape == 10 || shape == 11) {
        result.args.emplace_back(Expr::column(owner, "password"));
    }
    return result;
}

static Json context(Memory owner, const SqlLogContext& value) {
    Vector<String> safe(owner), debug(owner);
    value.intent_redactions.extend_secrets(false, safe);
    value.intent_redactions.extend_secrets(true, debug);
    Json::Array safe_json(owner), debug_json(owner);
    for (const auto& item : safe) {
        safe_json.emplace_back(item);
    }
    for (const auto& item : debug) {
        debug_json.emplace_back(item);
    }
    Json::Object result(owner);
    result.emplace("formatted", formatted_debug(owner, value));
    result.emplace("safe_secrets", Json(owner, std::move(safe_json)));
    result.emplace("debug_secrets", Json(owner, std::move(debug_json)));
    return Json(owner, std::move(result));
}

static Json observe(Memory owner, const Json& input) {
    auto kind = text(field(input, "kind"));
    if (kind == "combine") {
        return policy_result(owner,
                             detail::combine_sql_policy(policy(field(input, "left")),
                                                        policy(field(input, "right"))));
    }
    if (kind == "expression") {
        auto root = descriptor(owner, std::get<std::uint64_t>(field(input, "mode").data));
        auto name = text(field(input, "field"));
        auto expr = expression(owner, name, std::get<std::uint64_t>(field(input, "shape").data));
        Json::Object result(owner);
        result.emplace("field",
                       Json(owner, debug_string(owner, detail::sql_field_policy(root, name))));
        result.emplace("expression",
                       policy_result(owner, detail::sql_expression_policy(root, expr)));
        return Json(owner, std::move(result));
    }
    auto original = text(field(input, "text"));
    if (kind == "like") {
        auto variant = std::get<std::uint64_t>(field(input, "variant").data);
        String pattern(owner);
        if (variant == 0 || variant == 2) {
            pattern += '%';
        }
        if (variant <= 3) {
            pattern += original;
            if (variant <= 1) {
                pattern += '%';
            }
        } else {
            pattern = "%changed%";
        }
        auto right = variant == 4   ? Expr::value(owner, pattern)
                     : variant == 5 ? Expr::soundex(Expr::value(owner, pattern))
                                    : Expr::like_pattern(owner, pattern, original);
        auto op = static_cast<BinaryOp>(std::get<std::uint64_t>(field(input, "op").data));
        auto binary = Expr::binary(Expr::column(owner, "field"), op, std::move(right));
        auto result = binary.lowered_like_operand();
        if (!result) {
            return Json(owner);
        }
        Json::Array values(owner);
        values.emplace_back(result->first);
        values.emplace_back(result->second);
        return Json(owner, std::move(values));
    }
    if (kind != "bindings") {
        throw std::runtime_error("Unknown binding fixture");
    }
    auto root = descriptor(owner, std::get<std::uint64_t>(field(input, "mode").data));
    SqlBindings values(owner);
    detail::SqlBindingScope active(values, policy(field(input, "scope")), false);
    values.push(Value(owner, "generic"));
    String pattern("%", owner);
    pattern += original;
    pattern += '%';
    values.push_like_pattern(pattern, original);
    values.push_field(root, "plain", Value(owner, "PLAIN"));
    {
        detail::SqlBindingScope nested(values, Policy::Credential);
        String second(original, owner);
        second += "-private";
        String pattern_two(second, owner);
        pattern_two += '%';
        values.push_like_pattern(pattern_two, second);
        values.push_field(root, "masked", Value(owner, "MASKED"));
    }
    String suffix("%", owner);
    suffix += original;
    values.push_like_pattern(suffix, original);
    if (std::get<bool>(field(input, "untrusted").data)) {
        values.mark_untrusted_sql();
    }
    Json::Object output(owner);
    output.emplace("bindings", formatted_debug(owner, values));
    output.emplace("log", context(owner, values.log_context(owner, DatabaseKind::Sqlite)));
    SqlLogContext source(owner);
    source.database_kind.emplace(std::string_view("cached\0source", 13), owner);
    for (const auto& item : std::get<Json::Array>(field(input, "source_policies").data)) {
        source.parameter_policies.push_back(*policy(item));
    }
    source.generated_sql = std::get<bool>(field(input, "source_generated").data);
    source.masked_parameters.assign({true, false, true});
    source.execution_outcome = SqlExecutionOutcome::Cancelled;
    source.mode.emplace("source-mode", owner);
    source.omission_reason.emplace("source-omission", owner);
    SelectQuery old(std::allocator_arg, Allocator(owner), "Old");
    old.filter(Expr::eq(owner, "password", "STALE-PRIVATE"));
    source.intent_redactions = SqlIntentRedactions::from_unclassified_query(owner, old);
    output.emplace("source_before", context(owner, source));
    output.emplace("rebound", context(owner, values.rebind_log_context(owner, source)));
    output.emplace("source_after", context(owner, source));
    SqlBindings clone(std::allocator_arg, Allocator(owner), values);
    clone.push(Value(owner, "clone-only"));
    output.emplace("clone", formatted_debug(owner, clone));
    auto raw = std::move(values).into_values();
    Json::Array final_values(owner);
    for (const auto& item : raw) {
        final_values.emplace_back(debug_string(owner, item));
    }
    output.emplace("values", Json(owner, std::move(final_values)));
    return Json(owner, std::move(output));
}

struct DisableDefaultResource {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~DisableDefaultResource() {
        std::pmr::set_default_resource(previous);
    }
};

int main() {
    try {
        DisableDefaultResource guard;
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context_owner = global.make_user_context();
            {
                auto owner = user_context_owner->memory_resource();
                auto input = Json::parse(owner, line);
                if (!input) {
                    throw std::runtime_error("Invalid binding observation input");
                }
                std::cout << observe(owner, *input).dump(owner) << '\n';
            }
            user_context_owner.reset();
            if (global.live_user_bytes() != 0) {
                throw std::runtime_error("User binding bytes survived deletion");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
