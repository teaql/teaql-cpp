// Original SafeExpression, SmartList, trimmed serde and WebResponse fixtures.
#include <teaql/core/safe_expression.hpp>
#include <teaql/core/query.hpp>
#include <teaql/core/serde_utils.hpp>
#include <teaql/core/web.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <atomic>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void safe_expression_eval_with_uses_the_supplied_root(UserContext& user) {
    auto& expression = user.make<SafeExpression<std::int32_t, std::int32_t>>(
        std::int32_t(2), [](const std::int32_t& root) -> std::optional<std::int32_t> {
            return root * 3;
        });
    check(expression.eval() == std::optional<std::int32_t>(6), "original stored root");
    check(expression.eval_with(4) == std::optional<std::int32_t>(12), "original supplied root");
}

static void safe_expression_apply_optional_short_circuits_remaining_mappers(Memory owner) {
    std::atomic<std::size_t> optional_calls{0}, remaining_calls{0};
    // Counter borrows are used only during this synchronous invocation.
    auto expression = SafeExpression<std::int32_t, std::int32_t>::value(owner, 5)
                          .apply_optional([&](std::int32_t) -> std::optional<std::int32_t> {
                              optional_calls.fetch_add(1);
                              return {};
                          })
                          .apply([&](std::int32_t value) {
                              remaining_calls.fetch_add(1);
                              return value * 2;
                          });
    check(!expression.eval(), "original optional mapper returns None");
    check(optional_calls.load() == 1 && remaining_calls.load() == 0,
          "one optional call and zero remaining calls");
}

static void safe_expression_lazy_fallback_and_error_only_run_for_missing_values(Memory owner) {
    int present_fallback_calls = 0, present_error_calls = 0;
    auto present = SafeExpression<std::int32_t, std::int32_t>::value(owner, 7);
    check(present.or_else_with([&] {
        ++present_fallback_calls;
        return 9;
    }) == 7 && present_fallback_calls == 0,
          "original present value does not evaluate fallback");
    auto success = present.or_else_throw([&] {
        ++present_error_calls;
        return std::string_view("unused error");
    });
    check(success && success.value() == 7 && present_error_calls == 0,
          "original present result skips error factory");

    auto missing = SafeExpression<std::monostate, std::int32_t>(
        std::allocator_arg, Allocator(owner), std::monostate{}, [](const auto&) {
            return std::optional<std::int32_t>{};
        });
    int missing_fallback_calls = 0, missing_error_calls = 0;
    check(missing.or_else_with([&] {
        ++missing_fallback_calls;
        return 9;
    }) == 9 && missing_fallback_calls == 1,
          "original missing value evaluates fallback once");
    auto failure = missing.or_else_throw([&] {
        ++missing_error_calls;
        return std::string_view("missing value");
    });
    check(failure.is_err() && failure.error() == "missing value" && missing_error_calls == 1,
          "original missing result evaluates error factory once");
}

static void safe_expression_callbacks_only_run_for_their_matching_branch(Memory owner) {
    auto present = SafeExpression<String, String>::value(owner, String("teaql", owner));
    int present_null_calls = 0;
    std::optional<String> present_value;
    present.when_is_null([&] {
        ++present_null_calls;
    });
    present.when_is_not_null([&](String value) {
        present_value.emplace(std::move(value));
    });
    check(present_null_calls == 0 && present_value && *present_value == "teaql",
          "original present callback branch");

    auto missing = SafeExpression<std::monostate, String>(
        std::allocator_arg, Allocator(owner), std::monostate{}, [](const auto&) {
            return std::optional<String>{};
        });
    int missing_null_calls = 0, missing_value_calls = 0;
    missing.when_is_null([&] {
        ++missing_null_calls;
    });
    missing.when_is_not_null([&](String) {
        ++missing_value_calls;
    });
    check(missing_null_calls == 1 && missing_value_calls == 0, "original missing callback branch");
}

struct Item {
    std::uint64_t id;
    std::string_view value; // Original &'static str fixtures borrow source literals.
    bool operator==(const Item&) const = default;
};

static void smart_list_merge_by_replaces_in_place_and_appends_new_keys(Memory owner) {
    const std::array initial{Item{1, "one"}, Item{2, "old two"}, Item{4, "four"}};
    SmartList<Item> items(std::allocator_arg, Allocator(owner), std::span<const Item>(initial));
    const std::array incoming{Item{2, "new two"}, Item{3, "three"}};
    items.merge_by(incoming, [](const Item& item) {
        return item.id;
    });
    const std::array expected{
        Item{1, "one"}, Item{2, "new two"}, Item{4, "four"}, Item{3, "three"}};
    auto actual = std::move(items).into_vec();
    check(std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()),
          "original complete replacement/appended vector retains positions");
}

static void smart_list_merge_by_keeps_one_position_for_repeated_incoming_keys(Memory owner) {
    const std::array initial{Item{1, "one"}};
    SmartList<Item> items(std::allocator_arg, Allocator(owner), std::span<const Item>(initial));
    const std::array incoming{Item{2, "first two"}, Item{2, "final two"}};
    items.merge_by(incoming, [](const Item& item) {
        return item.id;
    });
    const std::array expected{Item{1, "one"}, Item{2, "final two"}};
    auto actual = std::move(items).into_vec();
    check(std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()),
          "original repeated key occupies one position with its final value");
}

static void compare_columns_builds_property_to_property_filter(Memory owner) {
    auto expected = Expr::binary(
        Expr::column(owner, "updated_at"), BinaryOp::Gte, Expr::column(owner, "created_at"));
    check(Expr::compare_columns(owner, "updated_at", BinaryOp::Gte, "created_at") == expected,
          "original property-to-property Binary AST");
}

static void sound_like_builds_soundex_equality(Memory owner) {
    auto expected = Expr::binary(Expr::soundex(Expr::column(owner, "name")),
                                 BinaryOp::Eq,
                                 Expr::soundex(Expr::value(owner, "Robert")));
    check(Expr::sound_like(owner, "name", "Robert") == expected,
          "original two soundex calls and equality operator");
}

static void java_style_string_match_builders_expand_like_patterns(Memory owner) {
    check(Expr::contain(owner, "name", "tea") == Expr::like(owner, "name", "%tea%") &&
              Expr::not_contain(owner, "name", "tea") == Expr::not_like(owner, "name", "%tea%") &&
              Expr::begin_with(owner, "name", "tea") == Expr::like(owner, "name", "tea%") &&
              Expr::not_begin_with(owner, "name", "tea") == Expr::not_like(owner, "name", "tea%") &&
              Expr::end_with(owner, "name", "tea") == Expr::like(owner, "name", "%tea") &&
              Expr::not_end_with(owner, "name", "tea") == Expr::not_like(owner, "name", "%tea"),
          "all six original string-match ASTs and patterns");
}

static void large_in_builders_use_large_binary_ops(Memory owner) {
    Vector<Value> values(owner);
    values.emplace_back(std::uint64_t(1));
    auto positive = Expr::binary(
        Expr::column(owner, "id"), BinaryOp::InLarge, Expr::value(owner, Value(owner, values)));
    auto negative = Expr::binary(
        Expr::column(owner, "id"), BinaryOp::NotInLarge, Expr::value(owner, Value(owner, values)));
    check(Expr::in_large(owner, "id", values) == positive &&
              Expr::not_in_large(owner, "id", values) == negative,
          "original explicit large-IN operators and typed U64 list literal");
}

static void ordinary_in_builders_promote_more_than_twenty_values(Memory owner) {
    Vector<Value> twenty(owner), twenty_one(owner);
    for (std::uint64_t index = 1; index <= 21; ++index) {
        twenty_one.emplace_back(index);
        if (index <= 20) {
            twenty.emplace_back(index);
        }
    }
    auto check_operator = [&](const Expr& expression, BinaryOp expected) {
        check(expression.kind == Expr::Kind::Binary && expression.op == expected,
              "original IN threshold and Binary variant");
    };
    check_operator(Expr::in_list(owner, "id", twenty), BinaryOp::In);
    check_operator(Expr::in_list(owner, "id", twenty_one), BinaryOp::InLarge);
    check_operator(Expr::not_in_list(owner, "id", twenty), BinaryOp::NotIn);
    check_operator(Expr::not_in_list(owner, "id", twenty_one), BinaryOp::NotInLarge);
}

static void subquery_builder_projects_requested_field(Memory owner) {
    // Handwritten original OrderLine metadata, without derive/generator changes.
    EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "OrderLine");
    descriptor.table_name = "orderline";
    descriptor.property("id", DataType::U64).id().not_null();
    descriptor.property("order_id", DataType::U64).not_null();
    descriptor.relation("product", "Product")
        .local("product_id")
        .foreign("id")
        .detached()
        .keep_missing();
    SelectQuery query(std::allocator_arg, Allocator(owner), "OrderLine");
    query.filter(Expr::eq(owner, "name", "line-1"));
    auto expression = Expr::in_subquery(owner, "id", descriptor, query, "order_id");
    check(expression.kind == Expr::Kind::SubQuery && expression.args.size() == 1 &&
              expression.args[0] == Expr::column(owner, "id") && expression.op == BinaryOp::In &&
              expression.subquery_data->entity.name == "OrderLine" &&
              expression.subquery_data->query.projection.size() == 1 &&
              expression.subquery_data->query.projection[0] == "order_id",
          "original SubQuery variant, left column, operator, entity and projected field");
}

static Json
trimmed_fields(Memory owner, std::string_view required, std::optional<std::string_view> optional) {
    auto result = detail::model_json(owner);
    detail::model_put(result, "required", serde_utils::trimmed_string::serialize(owner, required));
    detail::model_put(
        result, "optional", serde_utils::trimmed_opt_string::serialize(owner, optional));
    return result;
}

static void trimmed_string_helpers_trim_during_serialization(Memory owner) {
    auto actual = trimmed_fields(owner, "  required value\n", "\toptional value  ");
    auto expected =
        Json::parse(owner, R"({"required":"required value","optional":"optional value"})");
    check(expected && actual == *expected, "original complete trimmed serialization object");
}

static void trimmed_optional_string_preserves_none_during_serialization(Memory owner) {
    auto actual = trimmed_fields(owner, " value ", {});
    auto expected = Json::parse(owner, R"({"required":"value","optional":null})");
    check(expected && actual == *expected, "original None serializes as a present null field");
}

static void trimmed_string_helpers_trim_during_deserialization(Memory owner) {
    auto input =
        Json::parse(owner, R"({"required":"  required value\n","optional":"\toptional value  "})");
    check(input.has_value(), "original trimmed deserialization input");
    auto& fields = std::get<Json::Object>(input->data);
    auto required =
        serde_utils::trimmed_string::deserialize(owner, fields.at(String("required", owner)));
    auto optional =
        serde_utils::trimmed_opt_string::deserialize(owner, fields.at(String("optional", owner)));
    check(required && required.value() == "required value" && optional && optional.value() &&
              *optional.value() == "optional value",
          "original complete trimmed deserialization fields");
}

static void trimmed_optional_string_keeps_whitespace_only_input_as_some_empty(Memory owner) {
    auto input = Json::parse(owner, R"({"required":" value ","optional":" \t\n "})");
    check(input.has_value(), "original optional whitespace input");
    auto& fields = std::get<Json::Object>(input->data);
    auto required =
        serde_utils::trimmed_string::deserialize(owner, fields.at(String("required", owner)));
    auto optional =
        serde_utils::trimmed_opt_string::deserialize(owner, fields.at(String("optional", owner)));
    check(required && required.value() == "value" && optional && optional.value() &&
              optional.value()->empty(),
          "original whitespace is Some(empty), not None");
}

static void test_web_response_constructors_and_json_shape(Memory owner) {
    auto success = WebResponse::success(owner);
    check(success.result_code == 0 && success.status == "YES" && !success.message &&
              success.version == WEB_RESPONSE_VERSION && success.data.empty() &&
              success.record_count == 0,
          "original success constructor fields");
    auto failure = WebResponse::fail(owner, "Internal Error");
    check(failure.result_code == 1 && failure.status == "NO" &&
              failure.message == "Internal Error" && failure.data.empty() &&
              failure.record_count == 0,
          "original failure constructor fields");
    auto empty = WebResponse::empty_list(owner, "No items found");
    check(empty.result_code == 0 && !empty.status && empty.message == "No items found" &&
              empty.data.empty() && empty.record_count == 0,
          "original empty-list constructor fields");
    auto json = success.to_json_value(owner);
    const auto& object = std::get<Json::Object>(json.data);
    auto field = [&](std::string_view name) -> const Json& {
        return object.at(String(name, owner));
    };
    check(field("resultCode") == Json(owner, 0) && field("status") == Json(owner, "YES") &&
              !object.contains("message") &&
              field("version") == Json(owner, WEB_RESPONSE_VERSION) &&
              field("recordCount") == Json(owner, 0) &&
              std::get<Json::Array>(field("data").data).empty() &&
              std::get<Json::Object>(field("facets").data).empty(),
          "original success JSON object and skipped None message");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        {
            GlobalContext global;
            auto user = global.make_user_context();
            auto owner = user->memory_resource();
            safe_expression_eval_with_uses_the_supplied_root(*user);
            safe_expression_apply_optional_short_circuits_remaining_mappers(owner);
            safe_expression_lazy_fallback_and_error_only_run_for_missing_values(owner);
            safe_expression_callbacks_only_run_for_their_matching_branch(owner);
            smart_list_merge_by_replaces_in_place_and_appends_new_keys(owner);
            smart_list_merge_by_keeps_one_position_for_repeated_incoming_keys(owner);
            compare_columns_builds_property_to_property_filter(owner);
            sound_like_builds_soundex_equality(owner);
            java_style_string_match_builders_expand_like_patterns(owner);
            large_in_builders_use_large_binary_ops(owner);
            ordinary_in_builders_promote_more_than_twenty_values(owner);
            subquery_builder_projects_requested_field(owner);
            trimmed_string_helpers_trim_during_serialization(owner);
            trimmed_optional_string_preserves_none_during_serialization(owner);
            trimmed_string_helpers_trim_during_deserialization(owner);
            trimmed_optional_string_keeps_whitespace_only_input_as_some_empty(owner);
            test_web_response_constructors_and_json_shape(owner);
        }
        std::pmr::set_default_resource(previous);
        std::cout << "Original seventeen core functional contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
