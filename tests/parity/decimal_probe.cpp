#include <teaql/core/decimal.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
#include <bit>
using namespace teaql;

static const Json& field(const Json& object, std::string_view key) {
    return std::get<Json::Object>(object.data).find(key)->second;
}

static Json model(Memory r, const Decimal& value) {
    auto result = detail::model_json(r);
    try {
        detail::model_put(result, "text", value.to_string(r));
    } catch (const std::length_error&) {
        detail::model_put(result, "text", Json(r));
        detail::model_put(result, "text_panic", true);
    }
    detail::model_put(result, "scale", value.scale());
    detail::model_put(result, "negative", value.is_sign_negative());
    Json::Array limbs(r);
    for (auto n : value.coefficient())
        limbs.emplace_back(n);
    detail::model_put(result, "coefficient", limbs);
    return result;
}

static Json evaluate(Memory r, const Json& input) {
    if (std::get<String>(field(input, "kind").data) == "codec") {
        const auto& bytes = std::get<Json::Array>(field(input, "bytes").data);
        std::array<std::uint8_t, 16> raw{};
        for (unsigned i = 0; i < 16; ++i)
            raw[i] = std::uint8_t(std::get<std::uint64_t>(bytes[i].data));
        auto value = Decimal::deserialize(raw);
        auto result = detail::model_json(r);
        detail::model_put(result, "value", model(r, value));
        Json::Array output(r);
        for (auto byte : value.serialize())
            output.emplace_back(byte);
        detail::model_put(result, "bytes", output);
        return result;
    }
    auto a = *Decimal::parse(std::get<String>(field(input, "a").data));
    auto zero = std::get<Json::Object>(input.data).find("negative_zero");
    if (zero != std::get<Json::Object>(input.data).end() && std::get<bool>(zero->second.data))
        a.set_sign_negative(true);
    auto result = detail::model_json(r);
    auto put = [&](std::string_view name, const Decimal& value) {
        detail::model_put(result, name, model(r, value));
    };
    auto maybe = [&](std::string_view name, const std::optional<Decimal>& value) {
        detail::model_put(result, name, value ? model(r, *value) : Json(r));
    };
    if (std::get<String>(field(input, "kind").data) == "scale") {
        unsigned scale = unsigned(std::get<std::uint64_t>(field(input, "dp").data));
        auto changed = a, rescaled = a;
        auto valid = changed.set_scale(r, scale);
        rescaled.rescale(scale);
        put("set", changed);
        detail::model_put(result, "error", valid ? Json(r) : Json(r, valid.error().to_string(r)));
        put("rescale", rescaled);
        put("trunc_scale", a.trunc_with_scale(scale));
        try {
            detail::model_put(result, "f64_bits", std::bit_cast<std::uint64_t>(rescaled.to_f64()));
        } catch (const std::overflow_error&) {
            detail::model_put(result, "f64_panic", true);
        }
        Json::Array bytes(r);
        for (auto byte : a.serialize())
            bytes.emplace_back(byte);
        detail::model_put(result, "bytes", bytes);
        return result;
    }
    if (std::get<String>(field(input, "kind").data) == "binary") {
        auto b = *Decimal::parse(std::get<String>(field(input, "b").data));
        maybe("add", a.checked_add(b));
        maybe("sub", a.checked_sub(b));
        maybe("mul", a.checked_mul(b));
        maybe("div", a.checked_div(b));
        maybe("rem", a.checked_rem(b));
        put("saturating_add", a.saturating_add(b));
        put("saturating_sub", a.saturating_sub(b));
        put("saturating_mul", a.saturating_mul(b));
        detail::model_put(result, "compare", a < b ? -1 : a > b ? 1 : 0);
        return result;
    }
    unsigned dp = unsigned(std::get<std::uint64_t>(field(input, "dp").data));
    if (std::get<String>(field(input, "kind").data) == "significant") {
        Json::Array strategies(r);
        for (unsigned i = 0; i < 12; ++i) {
            auto item = detail::model_json(r);
            try {
                auto value = a.round_sf_with_strategy(dp, RoundingStrategy(i));
                detail::model_put(item, "value", value ? model(r, *value) : Json(r));
            } catch (const std::overflow_error&) {
                detail::model_put(item, "panic", true);
            }
            strategies.emplace_back(std::move(item));
        }
        detail::model_put(result, "strategies", strategies);
        return result;
    }
    put("abs", a.abs());
    put("neg", -a);
    put("trunc", a.trunc());
    put("fract", a.fract());
    put("floor", a.floor());
    put("ceil", a.ceil());
    put("normalize", a.normalize());
    put("round", a.round());
    put("round_dp", a.round_dp(dp));
    Json::Array strategies(r);
    for (unsigned i = 0; i < 12; ++i)
        strategies.emplace_back(model(r, a.round_dp_with_strategy(dp, RoundingStrategy(i))));
    detail::model_put(result, "strategies", strategies);
    return result;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input)
            throw std::runtime_error("Invalid decimal fixture JSON");
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
