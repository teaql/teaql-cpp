#include <teaql/core/business_id.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <charconv>
#include <iostream>
using namespace teaql;

static const Json& field(const Json& object, std::string_view name) {
    return std::get<Json::Object>(object.data).find(name)->second;
}

static std::string_view text(const Json& value) {
    return std::get<String>(value.data);
}

static Json error(Memory r, const BusinessIdError& e) {
    auto result = detail::model_json(r);
    detail::model_put(result, "code", business_id_error_code_name(e.code));
    detail::model_put(result, "message", e.message);
    detail::model_put(result, "display", e.to_string(r));
    detail::model_put(result, "source_none", !e.source());
    return result;
}

static Json equality(Memory r, const Json& input) {
    const auto model = text(field(input, "model"));
    const int index = detail::model_integer<int>(field(input, "field"), "field");
    auto definition = BusinessIdDefinition::daily_sequence(r, "field", "prefix", "namespace");
    auto scope = BusinessIdScope(
        std::allocator_arg, Allocator(r), "root", "Aggregate", "namespace", "period");
    auto change_definition = [&](BusinessIdDefinition& value, int field) {
        switch (field) {
        case 0:
            value.field_name += "!";
            break;
        case 1:
            value.profile += "!";
            break;
        case 2:
            value.prefix += "!";
            break;
        case 3:
            value.namespace_name += "!";
            break;
        case 4:
            value.split_by_date = !value.split_by_date;
            break;
        case 5:
            ++value.preserve_digits;
            break;
        case 6:
            value.separator += "!";
            break;
        }
    };
    auto change_scope = [&](BusinessIdScope& value, int field) {
        switch (field) {
        case 0:
            value.domain_root_key += "!";
            break;
        case 1:
            value.aggregate_type += "!";
            break;
        case 2:
            value.namespace_name += "!";
            break;
        case 3:
            value.period_key += "!";
            break;
        }
    };
    auto compare = [&](const auto& original, const auto& changed) {
        auto output = detail::model_json(r);
        detail::model_put(output, "clone_equal", original == detail::clone_in(r, original));
        detail::model_put(output, "changed_equal", original == changed);
        detail::model_put(output, "reverse_equal", changed == original);
        return output;
    };
    if (model == "definition") {
        auto changed = detail::clone_in(r, definition);
        change_definition(changed, index);
        return compare(definition, changed);
    }
    if (model == "scope") {
        auto changed = detail::clone_in(r, scope);
        change_scope(changed, index);
        return compare(scope, changed);
    }
    if (model == "request") {
        BusinessIdGenerationRequest original(std::allocator_arg,
                                             Allocator(r),
                                             definition,
                                             "root",
                                             "Aggregate",
                                             *Date::from_ymd(2024, 2, 29));
        auto changed = detail::clone_in(r, original);
        if (index < 7) {
            change_definition(changed.definition, index);
        } else if (index == 7) {
            changed.domain_root_key += "!";
        } else if (index == 8) {
            changed.aggregate_type += "!";
        } else if (index == 9) {
            changed.business_date = *Date::from_ymd(2024, 3, 1);
        }
        return compare(original, changed);
    }
    if (model == "plan") {
        BusinessIdPlan original(
            std::allocator_arg, Allocator(r), scope, "prefix", std::nullopt, 6, "-", 1, 99);
        auto changed = detail::clone_in(r, original);
        if (index < 4) {
            change_scope(changed.scope, index);
        } else {
            switch (index) {
            case 4:
                changed.prefix += "!";
                break;
            case 5:
                changed.date_text.emplace("", r);
                break;
            case 6:
                ++changed.preserve_digits;
                break;
            case 7:
                changed.separator += "!";
                break;
            case 8:
                ++changed.initial_sequence;
                break;
            case 9:
                ++changed.max_sequence;
                break;
            }
        }
        return compare(original, changed);
    }
    if (model == "allocation") {
        BusinessIdAllocation original(std::allocator_arg, Allocator(r), scope, 7);
        auto changed = detail::clone_in(r, original);
        if (index < 4) {
            change_scope(changed.scope, index);
        } else if (index == 4) {
            ++changed.sequence;
        }
        return compare(original, changed);
    }
    if (model == "value") {
        BusinessIdValue original(std::allocator_arg, Allocator(r), "value");
        auto changed = detail::clone_in(r, original);
        if (index == 0) {
            changed.value += "!";
        }
        return compare(original, changed);
    }
    if (model == "error") {
        BusinessIdError original(
            std::allocator_arg, Allocator(r), BusinessIdErrorCode::Encoding, "message");
        auto changed = detail::clone_in(r, original);
        if (index == 0) {
            changed.code = BusinessIdErrorCode::Allocation;
        } else if (index == 1) {
            changed.message += "!";
        }
        return compare(original, changed);
    }
    if (model == "code") {
        auto output = detail::model_json(r);
        const auto left =
            BusinessIdErrorCode(detail::model_integer<int>(field(input, "left"), "left"));
        const auto right =
            BusinessIdErrorCode(detail::model_integer<int>(field(input, "right"), "right"));
        detail::model_put(output, "equal", left == right);
        return output;
    }
    throw std::runtime_error("Unknown business ID equality fixture");
}

static Json evaluate(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    auto result = detail::model_json(r);
    if (kind == "equality") {
        return equality(r, input);
    }
    if (kind == "definition") {
        const auto& object = std::get<Json::Object>(input.data);
        auto legacy = object.find("legacy");
        auto definition =
            legacy != object.end() && std::get<bool>(legacy->second.data)
                ? BusinessIdDefinition::legacy_daily_sequence(r, "business_id", "订单", "orders")
                : BusinessIdDefinition::daily_sequence(r, "business_id", "订单", "orders");
        if (auto profile = object.find("profile"); profile != object.end()) {
            definition.profile = text(profile->second);
        }
        if (auto digits = object.find("digits"); digits != object.end()) {
            definition.preserve_digits = std::uint8_t(std::get<std::uint64_t>(digits->second.data));
        }
        detail::model_put(result, "field_name", definition.field_name);
        detail::model_put(result, "profile", definition.profile);
        detail::model_put(result, "prefix", definition.prefix);
        detail::model_put(result, "namespace", definition.namespace_name);
        detail::model_put(result, "split_by_date", definition.split_by_date);
        detail::model_put(result, "preserve_digits", definition.preserve_digits);
        detail::model_put(result, "separator", definition.separator);
        auto range = definition.max_sequence();
        auto value = detail::model_json(r);
        if (range) {
            char buffer[32];
            auto [end, ec] = std::to_chars(buffer, buffer + sizeof(buffer), range.value());
            (void)ec;
            detail::model_put(value, "ok", std::string_view(buffer, end));
        } else {
            value = error(r, range.error());
        }
        detail::model_put(result, "range", value);
        return result;
    }
    if (kind == "scope") {
        const auto& fields = std::get<Json::Array>(field(input, "fields").data);
        BusinessIdScope scope(std::allocator_arg,
                              Allocator(r),
                              text(fields[0]),
                              text(fields[1]),
                              text(fields[2]),
                              text(fields[3]));
        detail::model_put(result, "canonical", scope.canonical_key(r));
        return result;
    }
    if (kind == "range") {
        auto number = [&](std::string_view name) {
            auto s = text(field(input, name));
            std::uint64_t n = 0;
            auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), n);
            if (ec != std::errc{} || end != s.data() + s.size()) {
                throw std::runtime_error("Invalid fixture number");
            }
            return n;
        };
        BusinessIdScope scope(std::allocator_arg, Allocator(r), "", "", "", "");
        BusinessIdPlan plan(std::allocator_arg,
                            Allocator(r),
                            scope,
                            "",
                            std::nullopt,
                            0,
                            "",
                            number("initial"),
                            number("maximum"));
        auto valid = plan.validate_allocation_range();
        if (!valid) {
            return error(r, valid.error());
        }
        detail::model_put(result, "ok", true);
        return result;
    }
    if (kind == "error") {
        BusinessIdError e(std::allocator_arg,
                          Allocator(r),
                          BusinessIdErrorCode(std::get<std::uint64_t>(field(input, "code").data)),
                          text(field(input, "message")));
        return error(r, e);
    }
    throw std::runtime_error("Unknown business ID fixture");
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid fixture JSON");
        }
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
