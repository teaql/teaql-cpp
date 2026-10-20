#pragma once
#include <teaql/core/entity.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>

namespace teaql {
namespace detail {
template <class T>
struct GeneratedScalarOptional {
    static constexpr bool value = false;
};

template <class T>
struct GeneratedScalarOptional<std::optional<T>> {
    static constexpr bool value = true;
    using type = T;
};
}

// The entity derive has stricter conversions than Value's general try_* APIs.
// In particular, only an absent value or untyped Null supplies a field default.
class GeneratedScalarCodec {
public:
    template <class T>
    static T default_value(Memory r) {
        return std::make_obj_using_allocator<T>(Allocator(r));
    }

    template <class T>
    static Result<T, EntityError>
    decode(Memory r, const Value* input, std::string_view entity, std::string_view field) {
        using Outcome = Result<T, EntityError>;
        if (!input || std::holds_alternative<std::monostate>(input->data)) {
            return Outcome::ok(r, default_value<T>(r));
        }
        if constexpr (detail::GeneratedScalarOptional<T>::value) {
            auto result =
                decode<typename detail::GeneratedScalarOptional<T>::type>(r, input, entity, field);
            if (!result) {
                return Outcome::err(r, std::move(result).error());
            }
            return Outcome::ok(r, T(std::move(result).value()));
        } else if constexpr (std::same_as<T, String>) {
            if (auto text = input->try_text()) {
                return Outcome::ok(r, String(*text, r));
            }
        } else if constexpr (std::same_as<T, std::uint64_t>) {
            if (auto value = std::get_if<std::int64_t>(&input->data); value && *value < 0) {
                return error<T>(r, *input, entity, field, "negative i64 cannot map to u64");
            }
            if (std::holds_alternative<std::uint64_t>(input->data) ||
                std::holds_alternative<std::int64_t>(input->data) ||
                std::holds_alternative<Decimal>(input->data)) {
                if (auto value = input->try_u64()) {
                    return Outcome::ok(r, *value);
                }
                return error<T>(r, *input, entity, field, "decimal cannot map exactly to u64");
            }
        } else if constexpr (std::same_as<T, std::int64_t>) {
            if (auto value = std::get_if<std::int64_t>(&input->data)) {
                return Outcome::ok(r, *value);
            }
            if (auto value = std::get_if<std::uint64_t>(&input->data)) {
                if (*value > std::uint64_t(std::numeric_limits<std::int64_t>::max())) {
                    return error<T>(r, *input, entity, field, "u64 out of i64 range");
                }
                return Outcome::ok(r, static_cast<T>(*value));
            }
        } else if constexpr (std::same_as<T, bool>) {
            if (auto value = input->try_bool()) {
                return Outcome::ok(r, *value);
            }
            if (auto value = std::get_if<std::int64_t>(&input->data);
                value && (*value == 0 || *value == 1)) {
                return Outcome::ok(r, *value == 1);
            }
            if (auto value = std::get_if<std::uint64_t>(&input->data);
                value && (*value == 0 || *value == 1)) {
                return Outcome::ok(r, *value == 1);
            }
        } else if constexpr (std::same_as<T, Decimal>) {
            if (auto value = std::get_if<Decimal>(&input->data)) {
                return Outcome::ok(r, *value);
            }
            if (auto value = std::get_if<std::int64_t>(&input->data)) {
                return Outcome::ok(r, Decimal::from_i64(*value));
            }
            if (auto value = std::get_if<std::uint64_t>(&input->data)) {
                return Outcome::ok(r, Decimal::from_u64(*value));
            }
            if (auto value = std::get_if<double>(&input->data)) {
                if (!std::isfinite(*value)) {
                    return error<T>(r, *input, entity, field, "non-finite f64");
                }
                String text(r);
                detail::append_float(text, *value, false);
                if (auto converted = Decimal::parse(text)) {
                    return Outcome::ok(r, *converted);
                }
                return error<T>(r, *input, entity, field, "decimal conversion failed");
            }
        } else if constexpr (std::same_as<T, float>) {
            if (auto value = std::get_if<double>(&input->data)) {
                if (*value < -double(std::numeric_limits<float>::max()) ||
                    *value > double(std::numeric_limits<float>::max())) {
                    return error<T>(r, *input, entity, field, "f64 out of f32 range");
                }
                return Outcome::ok(r, static_cast<T>(*value));
            }
            if (auto value = std::get_if<std::int64_t>(&input->data)) {
                return Outcome::ok(r, static_cast<T>(*value));
            }
            if (auto value = std::get_if<std::uint64_t>(&input->data)) {
                return Outcome::ok(r, static_cast<T>(*value));
            }
        } else if constexpr (std::same_as<T, double>) {
            if (auto value = std::get_if<double>(&input->data)) {
                return Outcome::ok(r, *value);
            }
            if (auto value = std::get_if<std::int64_t>(&input->data)) {
                return Outcome::ok(r, static_cast<T>(*value));
            }
            if (auto value = std::get_if<Decimal>(&input->data)) {
                auto text = value->to_string(r);
                double converted;
                auto parsed = std::from_chars(text.data(), text.data() + text.size(), converted);
                if (parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size()) {
                    return Outcome::ok(r, converted);
                }
                return error<T>(r, *input, entity, field, "decimal out of f64 range");
            }
        } else if constexpr (std::same_as<T, Timestamp>) {
            if (auto value = std::get_if<Timestamp>(&input->data)) {
                return Outcome::ok(r, *value);
            }
            if (auto value = std::get_if<std::int64_t>(&input->data)) {
                return Outcome::ok(r, Timestamp(*value));
            }
            if (auto value = std::get_if<std::uint64_t>(&input->data)) {
                return Outcome::ok(r, Timestamp(std::bit_cast<std::int64_t>(*value)));
            }
        } else if constexpr (std::same_as<T, Date>) {
            if (auto value = std::get_if<Date>(&input->data)) {
                return Outcome::ok(r, *value);
            }
        } else if constexpr (std::same_as<T, Json>) {
            if (auto value = std::get_if<Json>(&input->data)) {
                return Outcome::ok(r, detail::clone_in(r, *value));
            }
        }
        return error<T>(r, *input, entity, field);
    }

private:
    template <class T>
    static Result<T, EntityError> error(Memory r,
                                        const Value& input,
                                        std::string_view entity,
                                        std::string_view field,
                                        std::string_view reason = {}) {
        String message("invalid field ", r);
        message += field;
        message += ": ";
        message += reason.empty() ? detail::debug_value(r, input) : String(reason, r);
        return Result<T, EntityError>::err(
            r, EntityError(std::allocator_arg, Allocator(r), entity, message));
    }
};
}
