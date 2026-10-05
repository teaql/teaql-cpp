#include <teaql/runtime/business_id.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static Json error(Memory r, const BusinessIdError& e) {
    auto out = detail::model_json(r);
    detail::model_put(out, "code", business_id_error_code_name(e.code));
    detail::model_put(out, "message", e.message);
    detail::model_put(out, "display", e.to_string(r));
    return out;
}

static BusinessIdScope scope(Memory r, const Json& j) {
    const auto& a = std::get<Json::Array>(j.data);
    return BusinessIdScope(
        std::allocator_arg, Allocator(r), text(a[0]), text(a[1]), text(a[2]), text(a[3]));
}

static BusinessIdDefinition definition(Memory r, const Json& j) {
    BusinessIdDefinition d(std::allocator_arg,
                           Allocator(r),
                           "business_id",
                           text(field(j, "prefix")),
                           text(field(j, "namespace")));
    d.profile = text(field(j, "profile"));
    d.separator = text(field(j, "separator"));
    d.split_by_date = std::get<bool>(field(j, "split").data);
    d.preserve_digits = std::uint8_t(std::get<std::uint64_t>(field(j, "digits").data));
    return d;
}

static BusinessIdPlan plan(Memory r, const Json& j) {
    auto date = std::get_if<String>(&field(j, "date").data);
    return BusinessIdPlan(std::allocator_arg,
                          Allocator(r),
                          scope(r, field(j, "scope")),
                          text(field(j, "prefix")),
                          date ? std::optional<std::string_view>(*date) : std::nullopt,
                          std::uint8_t(std::get<std::uint64_t>(field(j, "digits").data)),
                          text(field(j, "separator")),
                          number<std::uint64_t>(field(j, "initial")),
                          number<std::uint64_t>(field(j, "maximum")));
}

static String numeric(Memory r, std::uint64_t n) {
    char buffer[32];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), n);
    return String(buffer, result.ptr, r);
}

static Json evaluate(UserContext& context, const Json& j) {
    auto r = context.memory_resource();
    std::array<std::uint8_t, 32> bytes{};
    const auto& input = std::get<Json::Array>(field(j, "key").data);
    for (std::size_t i = 0; i < 32; ++i)
        bytes[i] = std::uint8_t(std::get<std::uint64_t>(input[i].data));
    auto key = BusinessIdEncodingKey::create(
        r, std::uint32_t(std::get<std::uint64_t>(field(j, "version").data)), bytes);
    if (!key)
        return error(r, key.error());
    auto op = text(field(j, "op"));
    auto out = detail::model_json(r);
    if (op == "key") {
        auto d = BusinessIdDefinition::daily_sequence(r, "id", "P", "ns");
        StaticBusinessIdKeyProvider provider(key.value());
        auto supplied = provider.current_key(r, context, d);
        detail::model_put(out, "version", supplied.value().version());
        detail::model_put(out, "equal", supplied.value() == key.value());
        return out;
    }
    if (op == "encode") {
        auto result = BusinessIdPermutationV1::encode(r,
                                                      number<std::uint64_t>(field(j, "sequence")),
                                                      scope(r, field(j, "scope")),
                                                      key.value());
        if (!result)
            return error(r, result.error());
        detail::model_put(out, "ok", result.value());
        return out;
    }
    DailySequenceBusinessIdProfile legacy;
    PermutedDailyBusinessIdProfile v1(key.value());
    const BusinessIdProfile& profile = text(field(j, "implementation")) == "legacy"
                                           ? static_cast<const BusinessIdProfile&>(legacy)
                                           : static_cast<const BusinessIdProfile&>(v1);
    if (op == "plan") {
        auto d = definition(r, field(j, "definition"));
        BusinessIdGenerationRequest request(std::allocator_arg,
                                            Allocator(r),
                                            d,
                                            text(field(j, "root")),
                                            text(field(j, "aggregate")),
                                            *Date::parse(text(field(j, "date"))));
        auto result = profile.plan(r, request);
        if (!result)
            return error(r, result.error());
        auto& p = result.value();
        Json::Array fields(r);
        for (std::string_view f : {p.scope.domain_root_key,
                                   p.scope.aggregate_type,
                                   p.scope.namespace_name,
                                   p.scope.period_key})
            fields.emplace_back(f);
        detail::model_put(out, "scope", fields);
        detail::model_put(out, "prefix", p.prefix);
        detail::model_put(out, "date", p.date_text ? Json(r, *p.date_text) : Json(r));
        detail::model_put(out, "digits", p.preserve_digits);
        detail::model_put(out, "separator", p.separator);
        detail::model_put(out, "initial", numeric(r, p.initial_sequence));
        detail::model_put(out, "maximum", numeric(r, p.max_sequence));
        return out;
    }
    if (op == "format") {
        auto p = plan(r, field(j, "plan"));
        BusinessIdAllocation allocation(std::allocator_arg,
                                        Allocator(r),
                                        scope(r, field(j, "allocation_scope")),
                                        number<std::uint64_t>(field(j, "sequence")));
        auto result = profile.format(r, p, allocation);
        if (!result)
            return error(r, result.error());
        detail::model_put(out, "ok", result.value().value);
        return out;
    }
    if (op == "validate") {
        auto result =
            profile.validate(r, definition(r, field(j, "definition")), text(field(j, "value")));
        if (!result)
            return error(r, result.error());
        detail::model_put(out, "ok", result.value().value);
        return out;
    }
    throw std::runtime_error("unknown op");
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input)
            throw std::runtime_error("invalid input");
        std::cout << evaluate(*user, *input).dump(r) << '\n';
    }
}
