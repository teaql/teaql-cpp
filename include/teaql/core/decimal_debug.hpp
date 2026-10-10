#pragma once
#include <teaql/core/debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const UnpackedDecimal& value) {
    writer.structure("UnpackedDecimal", [&](auto& fields) {
        fields.field("negative", [&](auto& out) {
            out.boolean(value.negative);
        });
        fields.field("scale", [&](auto& out) {
            out.integer(value.scale);
        });
        fields.field("hi", [&](auto& out) {
            out.integer(value.hi);
        });
        fields.field("mid", [&](auto& out) {
            out.integer(value.mid);
        });
        fields.field("lo", [&](auto& out) {
            out.integer(value.lo);
        });
    });
}

inline void write_debug(DebugWriter& writer, const DecimalError& value) {
    switch (value.code) {
    case DecimalError::Code::ErrorString:
        writer.tuple("ErrorString", [&](auto& out) {
            out.text(value.text);
        });
        return;
    case DecimalError::Code::ExceedsMaximumPossibleValue:
        writer.raw("ExceedsMaximumPossibleValue");
        return;
    case DecimalError::Code::LessThanMinimumPossibleValue:
        writer.raw("LessThanMinimumPossibleValue");
        return;
    case DecimalError::Code::Underflow:
        writer.raw("Underflow");
        return;
    case DecimalError::Code::ScaleExceedsMaximumPrecision:
        writer.tuple("ScaleExceedsMaximumPrecision", [&](auto& out) {
            out.integer(value.scale);
        });
        return;
    case DecimalError::Code::ConversionTo:
        writer.tuple("ConversionTo", [&](auto& out) {
            out.text(value.text);
        });
        return;
    }
    throw std::invalid_argument("Invalid Decimal diagnostic error");
}

inline void write_debug(DebugWriter& writer, RoundingStrategy value) {
    constexpr std::string_view names[]{"MidpointNearestEven",
                                       "MidpointAwayFromZero",
                                       "MidpointTowardZero",
                                       "ToZero",
                                       "AwayFromZero",
                                       "ToNegativeInfinity",
                                       "ToPositiveInfinity",
                                       "BankersRounding",
                                       "RoundHalfUp",
                                       "RoundHalfDown",
                                       "RoundDown",
                                       "RoundUp"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Invalid Decimal diagnostic rounding strategy");
    }
    writer.raw(names[index]);
}
}
