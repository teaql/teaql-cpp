#include <teaql/core/decimal.hpp>
#include <teaql/core/decimal_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
#include <bit>
#include <charconv>
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
    for (auto n : value.coefficient()) {
        limbs.emplace_back(n);
    }
    detail::model_put(result, "coefficient", limbs);
    return result;
}

static Json error_model(Memory r, const DecimalError& error) {
    auto result = detail::model_json(r);
    detail::model_put(result, "error", error.to_string(r));
    detail::model_put(result, "code", unsigned(error.code));
    return result;
}

static Json evaluate(Memory r, const Json& input) {
    const auto& kind = std::get<String>(field(input, "kind").data);
    if (kind == "ord_defaults") {
        auto value = [&](std::string_view name) {
            const auto& data = field(input, name);
            auto value = *Decimal::parse(std::get<String>(field(data, "text").data));
            value.set_sign_negative(std::get<bool>(field(data, "negative").data));
            value.rescale(unsigned(std::get<std::uint64_t>(field(data, "scale").data)));
            return value;
        };
        const auto left = value("left");
        const auto right = value("right");
        const auto minimum = value("minimum");
        const auto maximum = value("maximum");
        auto output = detail::model_json(r);
        detail::model_put(output, "left", model(r, left));
        detail::model_put(output, "right", model(r, right));
        detail::model_put(output, "minimum", model(r, minimum));
        detail::model_put(output, "maximum", model(r, maximum));
        detail::model_put(output, "inherent_min", model(r, left.min(right)));
        detail::model_put(output, "inherent_max", model(r, left.max(right)));
        detail::model_put(output, "ord_min", model(r, left.ord_min(right)));
        detail::model_put(output, "ord_max", model(r, left.ord_max(right)));
        auto clamped = detail::model_json(r);
        try {
            detail::model_put(clamped, "value", model(r, left.clamp(minimum, maximum)));
        } catch (const std::invalid_argument&) {
            detail::model_put(clamped, "panic", true);
        }
        detail::model_put(output, "clamp", clamped);
        Json::Array unchanged(r);
        for (const auto& value : {left, right, minimum, maximum}) {
            unchanged.emplace_back(model(r, value));
        }
        detail::model_put(output, "unchanged", unchanged);
        return output;
    }
    if (kind == "numeric_pointer") {
        auto value = *Decimal::parse(std::get<String>(field(input, "text").data));
        value.set_sign_negative(std::get<bool>(field(input, "negative").data));
        value.rescale(unsigned(std::get<std::uint64_t>(field(input, "scale").data)));
        auto output = detail::model_json(r);
        detail::model_put(output, "bits", sizeof(std::size_t) * 8);
        detail::model_put(output, "input", model(r, value));
        auto signed_value = value.to_integer<std::intptr_t>();
        auto unsigned_value = value.to_integer<std::uintptr_t>();
        detail::model_put(output, "to_isize", signed_value ? Json(r, *signed_value) : Json(r));
        detail::model_put(output, "to_usize", unsigned_value ? Json(r, *unsigned_value) : Json(r));
        auto checked = [&]<class T>(const Result<T, DecimalError>& result) {
            auto converted = detail::model_json(r);
            if (result) {
                detail::model_put(converted, "value", result.value());
            } else {
                detail::model_put(converted, "message", result.error().to_string(r));
                detail::model_put(converted, "debug", debug_string(r, result.error()));
            }
            return converted;
        };
        detail::model_put(output, "try_isize", checked(value.try_to_isize(r)));
        detail::model_put(output, "try_usize", checked(value.try_to_usize(r)));
        auto construct = [&]<class T>(std::string_view name) {
            const auto& text = std::get<String>(field(input, name).data);
            T primitive{};
            auto parsed = std::from_chars(text.data(), text.data() + text.size(), primitive);
            auto converted = detail::model_json(r);
            if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
                detail::model_put(converted, "input_rejected", true);
            } else {
                auto decimal = Decimal::from_integer(primitive);
                detail::model_put(converted, "optional", model(r, decimal));
                detail::model_put(converted, "infallible", model(r, decimal));
            }
            return converted;
        };
        detail::model_put(
            output, "from_isize", construct.template operator()<std::intptr_t>("signed"));
        detail::model_put(
            output, "from_usize", construct.template operator()<std::uintptr_t>("unsigned"));
        detail::model_put(output, "unchanged", model(r, value));
        return output;
    }
    if (kind == "identities") {
        auto value = *Decimal::parse(std::get<String>(field(input, "text").data));
        value.set_sign_negative(std::get<bool>(field(input, "negative").data));
        value.rescale(unsigned(std::get<std::uint64_t>(field(input, "scale").data)));
        auto result = detail::model_json(r);
        detail::model_put(result, "input", model(r, value));
        detail::model_put(result, "is_one", value.is_one());
        value.set_zero();
        detail::model_put(result, "zero", model(r, value));
        detail::model_put(result, "zero_is_one", value.is_one());
        value.set_one();
        detail::model_put(result, "one", model(r, value));
        detail::model_put(result, "one_is_one", value.is_one());
        return result;
    }
    if (kind == "reciprocal") {
        auto value = *Decimal::parse(std::get<String>(field(input, "text").data));
        value.set_sign_negative(std::get<bool>(field(input, "negative").data));
        value.rescale(unsigned(std::get<std::uint64_t>(field(input, "scale").data)));
        auto result = detail::model_json(r);
        detail::model_put(result, "input", model(r, value));
        try {
            detail::model_put(result, "inverse", model(r, value.inv()));
        } catch (const std::domain_error&) {
            detail::model_put(result, "panic", true);
        } catch (const std::overflow_error&) {
            detail::model_put(result, "panic", true);
        }
        detail::model_put(result, "after", model(r, value));
        return result;
    }
    if (kind == "decimal_json") {
        auto input_value = Json::parse(r, std::get<String>(field(input, "json").data));
        if (!input_value) {
            throw std::runtime_error("Invalid JSON fixture");
        }
        auto result = detail::model_json(r);
        auto decoded = Decimal::from_json_value_result(r, *input_value);
        if (decoded) {
            detail::model_put(result, "value", model(r, decoded.value()));
            detail::model_put(result, "serialized", decoded.value().to_json_value(r));
        } else {
            detail::model_put(result, "error", decoded.error().message);
        }
        return result;
    }
    if (kind == "decimal_format") {
        auto value = *Decimal::parse(std::get<String>(field(input, "text").data));
        value.set_sign_negative(std::get<bool>(field(input, "negative").data));
        value.rescale(unsigned(std::get<std::uint64_t>(field(input, "scale").data)));
        auto width = std::size_t(std::get<std::uint64_t>(field(input, "width").data));
        auto& p = field(input, "precision");
        std::optional<std::size_t> precision;
        if (!std::holds_alternative<std::monostate>(p.data)) {
            precision = std::size_t(std::get<std::uint64_t>(p.data));
        }
        auto result = detail::model_json(r);
        constexpr std::array<std::string_view, 4> modes{"display", "debug", "lower", "upper"};
        for (unsigned mode = 0; mode < modes.size(); ++mode) {
            auto formatted = detail::model_json(r);
            auto put = [&](std::string_view name, DecimalFormat options) {
                options.notation = DecimalNotation(mode);
                try {
                    detail::model_put(formatted, name, value.format(r, options));
                } catch (const std::length_error&) {
                    auto panic = detail::model_json(r);
                    detail::model_put(panic, "panic", true);
                    detail::model_put(formatted, name, panic);
                }
            };
            DecimalFormat options;
            put("plain", options);
            options.precision = precision;
            put("precision", options);
            options.precision.reset();
            options.width = width;
            put("width", options);
            options.precision = precision;
            put("padded", options);
            options.sign_plus = true;
            put("plus", options);
            options.zero_pad = true;
            put("zero", options);
            options.zero_pad = false;
            options.fill = U'_';
            options.alignment = DecimalAlignment::Left;
            put("left", options);
            options.alignment = DecimalAlignment::Right;
            put("right", options);
            options.alignment = DecimalAlignment::Center;
            put("center", options);
            options.fill = U'\U0001f980';
            put("unicode", options);
            detail::model_put(result, modes[mode], formatted);
        }
        try {
            detail::model_put(result, "serialized", value.to_json_value(r));
        } catch (const std::length_error&) {
            detail::model_put(result, "serialize_panic", true);
        }
        return result;
    }
    if (kind == "scientific") {
        auto result = detail::model_json(r);
        const auto& text = std::get<String>(field(input, "text").data);
        auto put = [&](std::string_view name, Result<Decimal, DecimalError> value) {
            detail::model_put(
                result, name, value ? model(r, value.value()) : error_model(r, value.error()));
        };
        put("strict", Decimal::from_scientific(r, text));
        put("lossy", Decimal::from_scientific_lossy(r, text));
        put("normal", Decimal::parse_result(r, text));
        return result;
    }
    if (kind == "constants") {
        auto result = detail::model_json(r);
        std::array values{Decimal::MIN,
                          Decimal::MAX,
                          Decimal::ZERO,
                          Decimal::ONE,
                          Decimal::NEGATIVE_ONE,
                          Decimal::TWO,
                          Decimal::TEN,
                          Decimal::ONE_HUNDRED,
                          Decimal::ONE_THOUSAND,
                          Decimal::zero(),
                          Decimal::one()};
        Json::Array output(r);
        for (auto value : values) {
            output.emplace_back(model(r, value));
        }
        detail::model_put(result, "values", output);
        detail::model_put(result, "max_scale", Decimal::MAX_SCALE);
        return result;
    }
    if (kind == "traits") {
        auto result = detail::model_json(r);
        auto value = *Decimal::parse(std::get<String>(field(input, "text").data));
        value.set_sign_negative(std::get<bool>(field(input, "negative").data));
        value.rescale(unsigned(std::get<std::uint64_t>(field(input, "scale").data)));
        auto other = *Decimal::parse(std::get<String>(field(input, "other").data));
        detail::model_put(result, "signum", model(r, value.signum()));
        detail::model_put(result, "positive", value.is_positive());
        detail::model_put(result, "negative", value.is_negative());
        auto changed = value;
        changed.set_sign(std::get<bool>(field(input, "positive").data));
        detail::model_put(result, "set_sign", model(r, changed));
        auto unpacked = value.unpack();
        auto unpacked_json = detail::model_json(r);
        detail::model_put(unpacked_json, "negative", unpacked.negative);
        detail::model_put(unpacked_json, "scale", unpacked.scale);
        detail::model_put(unpacked_json, "hi", unpacked.hi);
        detail::model_put(unpacked_json, "mid", unpacked.mid);
        detail::model_put(unpacked_json, "lo", unpacked.lo);
        detail::model_put(result, "unpacked", unpacked_json);
        try {
            detail::model_put(result, "repacked", model(r, Decimal::from_unpacked(unpacked)));
        } catch (const std::out_of_range&) {
            detail::model_put(result, "repack_panic", true);
        }
        Json::Array hashes(r);
        for (auto word : value.hash_words()) {
            hashes.emplace_back(word);
        }
        detail::model_put(result, "hash_words", hashes);
        try {
            detail::model_put(result, "abs_sub", model(r, value.abs_sub(other)));
        } catch (const std::overflow_error&) {
            detail::model_put(result, "abs_sub_panic", true);
        } catch (const std::domain_error&) {
            detail::model_put(result, "abs_sub_panic", true);
        }
        return result;
    }
    if (kind == "fold") {
        auto result = detail::model_json(r);
        Vector<Decimal> values(r);
        for (const auto& text : std::get<Json::Array>(field(input, "values").data)) {
            values.emplace_back(*Decimal::parse(std::get<String>(text.data)));
        }
        try {
            detail::model_put(result, "sum", model(r, Decimal::sum(values)));
        } catch (const std::overflow_error&) {
            detail::model_put(result, "sum_panic", true);
        }
        try {
            detail::model_put(result, "product", model(r, Decimal::product(values)));
        } catch (const std::overflow_error&) {
            detail::model_put(result, "product_panic", true);
        }
        return result;
    }
    if (kind == "numeric_float") {
        auto result = detail::model_json(r);
        auto bits = std::get<std::uint64_t>(field(input, "bits").data);
        std::optional<Decimal> value, retained;
        auto converted = detail::model_json(r);
        if (std::get<std::uint64_t>(field(input, "width").data) == 32) {
            auto floating = std::bit_cast<float>(std::uint32_t(bits));
            value = Decimal::from_f32(floating);
            retained = Decimal::from_f32_retain(floating);
            auto checked = Decimal::try_from_f32(r, floating);
            converted = checked ? model(r, checked.value()) : error_model(r, checked.error());
        } else {
            auto floating = std::bit_cast<double>(bits);
            value = Decimal::from_f64(floating);
            retained = Decimal::from_f64_retain(floating);
            auto checked = Decimal::try_from_f64(r, floating);
            converted = checked ? model(r, checked.value()) : error_model(r, checked.error());
        }
        detail::model_put(result, "value", value ? model(r, *value) : Json(r));
        detail::model_put(result, "retained", retained ? model(r, *retained) : Json(r));
        detail::model_put(result, "conversion", converted);
        return result;
    }
    auto words_json = [&](const std::array<std::uint32_t, 4>& words) {
        Json::Array result(r);
        for (auto word : words) {
            result.emplace_back(word);
        }
        return result;
    };
    if (kind == "numeric_wide") {
        auto result = detail::model_json(r);
        UInt128 unsigned_value;
        auto& words = std::get<Json::Array>(field(input, "words").data);
        for (unsigned i = 0; i < 4; ++i) {
            unsigned_value.words[i] = std::uint32_t(std::get<std::uint64_t>(words[i].data));
        }
        Int128 signed_value{unsigned_value.words};
        auto unsigned_decimal = Decimal::from_u128(unsigned_value);
        auto signed_decimal = Decimal::from_i128(signed_value);
        detail::model_put(
            result, "unsigned", unsigned_decimal ? model(r, *unsigned_decimal) : Json(r));
        detail::model_put(result, "signed", signed_decimal ? model(r, *signed_decimal) : Json(r));
        for (bool signed_input : {false, true}) {
            auto converted = detail::model_json(r);
            try {
                auto value = signed_input ? Decimal::from_integer(signed_value)
                                          : Decimal::from_integer(unsigned_value);
                converted = model(r, value);
            } catch (const std::overflow_error&) {
                detail::model_put(converted, "panic", true);
            }
            detail::model_put(result, signed_input ? "from_signed" : "from_unsigned", converted);
        }
        auto scale = unsigned(std::get<std::uint64_t>(field(input, "scale").data));
        auto checked = Decimal::try_from_i128_with_scale(r, signed_value, scale);
        if (checked) {
            detail::model_put(result, "scaled", model(r, checked.value()));
        } else {
            detail::model_put(result, "error", checked.error().to_string(r));
            detail::model_put(result, "code", unsigned(checked.error().code));
        }
        try {
            detail::model_put(
                result, "throwing", model(r, Decimal::from_i128_with_scale(signed_value, scale)));
        } catch (const std::out_of_range&) {
            detail::model_put(result, "panic", true);
        } catch (const std::overflow_error&) {
            detail::model_put(result, "panic", true);
        }
        return result;
    }
    if (kind == "numeric_decimal") {
        auto value = *Decimal::parse(std::get<String>(field(input, "text").data));
        if (std::get<bool>(field(input, "negative").data)) {
            value.set_sign_negative(true);
        }
        value.rescale(unsigned(std::get<std::uint64_t>(field(input, "scale").data)));
        auto result = detail::model_json(r);
        auto checked_values = detail::model_json(r);
        auto integer = [&]<class T>(std::string_view name) {
            auto converted = value.to_integer<T>();
            detail::model_put(result, name, converted ? Json(r, *converted) : Json(r));
            auto checked = value.try_to_integer<T>(r);
            detail::model_put(checked_values,
                              name,
                              checked ? Json(r, checked.value()) : error_model(r, checked.error()));
        };
        integer.template operator()<std::int8_t>("i8");
        integer.template operator()<std::int16_t>("i16");
        integer.template operator()<std::int32_t>("i32");
        integer.template operator()<std::int64_t>("i64");
        integer.template operator()<std::uint8_t>("u8");
        integer.template operator()<std::uint16_t>("u16");
        integer.template operator()<std::uint32_t>("u32");
        integer.template operator()<std::uint64_t>("u64");
        detail::model_put(result, "i128", words_json(value.to_i128().words));
        auto unsigned_value = value.to_u128();
        detail::model_put(
            result, "u128", unsigned_value ? Json(r, words_json(unsigned_value->words)) : Json(r));
        detail::model_put(result, "mantissa", words_json(value.mantissa().words));
        auto checked_unsigned = value.try_to_u128(r);
        detail::model_put(checked_values,
                          "u128",
                          checked_unsigned ? Json(r, words_json(checked_unsigned.value().words))
                                           : error_model(r, checked_unsigned.error()));
        detail::model_put(checked_values, "i128", words_json(value.try_to_i128(r).value().words));
        detail::model_put(result, "checked", checked_values);
        try {
            detail::model_put(result, "f32", std::bit_cast<std::uint32_t>(value.to_f32()));
            detail::model_put(result, "f64", std::bit_cast<std::uint64_t>(value.to_f64()));
            detail::model_put(
                result, "f32_result", std::bit_cast<std::uint32_t>(value.try_to_f32(r).value()));
            detail::model_put(
                result, "f64_result", std::bit_cast<std::uint64_t>(value.try_to_f64(r).value()));
        } catch (const std::overflow_error&) {
            detail::model_put(result, "float_panic", true);
        }
        return result;
    }
    if (kind == "numeric_i64") {
        auto value = std::stoll(std::string(std::get<String>(field(input, "text").data)));
        auto scale = unsigned(std::get<std::uint64_t>(field(input, "scale").data));
        auto result = detail::model_json(r);
        detail::model_put(result, "value", model(r, Decimal::from_integer(value)));
        auto checked = Decimal::try_new(r, value, scale);
        if (checked) {
            detail::model_put(result, "scaled", model(r, checked.value()));
        } else {
            detail::model_put(result, "error", checked.error().to_string(r));
            detail::model_put(result, "code", unsigned(checked.error().code));
        }
        try {
            detail::model_put(
                result, "throwing", model(r, Decimal::from_i64_with_scale(value, scale)));
        } catch (const std::out_of_range&) {
            detail::model_put(result, "panic", true);
        }
        return result;
    }
    if (std::get<String>(field(input, "kind").data) == "parse") {
        auto result = detail::model_json(r);
        const auto& method = std::get<String>(field(input, "method").data);
        const auto& text = std::get<String>(field(input, "text").data);
        try {
            auto parsed =
                method == "exact" ? Decimal::from_str_exact(r, text)
                : method == "radix"
                    ? Decimal::from_str_radix(
                          r, text, unsigned(std::get<std::uint64_t>(field(input, "radix").data)))
                    : Decimal::parse_result(r, text);
            if (parsed) {
                detail::model_put(result, "value", model(r, parsed.value()));
            } else {
                detail::model_put(result, "error", parsed.error().to_string(r));
                detail::model_put(result, "code", unsigned(parsed.error().code));
            }
        } catch (const std::length_error&) {
            detail::model_put(result, "panic", true);
        } catch (const std::out_of_range&) {
            detail::model_put(result, "panic", true);
        }
        return result;
    }
    if (std::get<String>(field(input, "kind").data) == "codec") {
        const auto& bytes = std::get<Json::Array>(field(input, "bytes").data);
        std::array<std::uint8_t, 16> raw{};
        for (unsigned i = 0; i < 16; ++i) {
            raw[i] = std::uint8_t(std::get<std::uint64_t>(bytes[i].data));
        }
        auto value = Decimal::deserialize(raw);
        auto result = detail::model_json(r);
        detail::model_put(result, "value", model(r, value));
        Json::Array output(r);
        for (auto byte : value.serialize()) {
            output.emplace_back(byte);
        }
        detail::model_put(result, "bytes", output);
        return result;
    }
    auto a = *Decimal::parse(std::get<String>(field(input, "a").data));
    auto zero = std::get<Json::Object>(input.data).find("negative_zero");
    if (zero != std::get<Json::Object>(input.data).end() && std::get<bool>(zero->second.data)) {
        a.set_sign_negative(true);
    }
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
        for (auto byte : a.serialize()) {
            bytes.emplace_back(byte);
        }
        detail::model_put(result, "bytes", bytes);
        return result;
    }
    if (kind == "extended_binary") {
        auto b = *Decimal::parse(std::get<String>(field(input, "b").data));
        a.set_sign_negative(std::get<bool>(field(input, "a_negative").data));
        b.set_sign_negative(std::get<bool>(field(input, "b_negative").data));
        a.rescale(unsigned(std::get<std::uint64_t>(field(input, "a_scale").data)));
        b.rescale(unsigned(std::get<std::uint64_t>(field(input, "b_scale").data)));
        put("a", a);
        put("b", b);
        auto capture = [&](std::string_view name, auto operation) {
            try {
                detail::model_put(result, name, operation());
            } catch (const std::overflow_error&) {
                auto panic = detail::model_json(r);
                detail::model_put(panic, "panic", true);
                detail::model_put(result, name, panic);
            } catch (const std::domain_error&) {
                auto panic = detail::model_json(r);
                detail::model_put(panic, "panic", true);
                detail::model_put(result, name, panic);
            }
        };
        capture("add", [&] {
            auto v = a.checked_add(b);
            return v ? model(r, *v) : Json(r);
        });
        capture("sub", [&] {
            auto v = a.checked_sub(b);
            return v ? model(r, *v) : Json(r);
        });
        capture("mul", [&] {
            auto v = a.checked_mul(b);
            return v ? model(r, *v) : Json(r);
        });
        capture("div", [&] {
            auto v = a.checked_div(b);
            return v ? model(r, *v) : Json(r);
        });
        capture("rem", [&] {
            auto v = a.checked_rem(b);
            return v ? model(r, *v) : Json(r);
        });
        capture("saturating_add", [&] {
            return model(r, a.saturating_add(b));
        });
        capture("saturating_sub", [&] {
            return model(r, a.saturating_sub(b));
        });
        capture("saturating_mul", [&] {
            return model(r, a.saturating_mul(b));
        });
        capture("compare", [&] {
            return Json(r, a < b ? -1 : a > b ? 1 : 0);
        });
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
    for (unsigned i = 0; i < 12; ++i) {
        strategies.emplace_back(model(r, a.round_dp_with_strategy(dp, RoundingStrategy(i))));
    }
    detail::model_put(result, "strategies", strategies);
    return result;
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--pointer-bits") {
        std::cout << sizeof(std::size_t) * 8 << '\n';
        return 0;
    }
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid decimal fixture JSON");
        }
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
