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
    return result;
}

static Json evaluate(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    auto result = detail::model_json(r);
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
