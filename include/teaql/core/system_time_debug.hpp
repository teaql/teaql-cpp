#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/core/system_time.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, SystemTime time) {
    writer.structure("SystemTime", [&](auto& fields) {
        fields.field("tv_sec", [&](auto& out) {
            out.integer(time.unix_seconds());
        });
        fields.field("tv_nsec", [&](auto& out) {
            out.integer(time.subsec_nanos());
        });
    });
}

inline void write_debug(DebugWriter& writer, SystemDuration duration) {
    if (duration.nanos >= 1'000'000'000) {
        throw std::invalid_argument("SystemDuration nanoseconds must be normalized");
    }
    std::uint64_t integral;
    std::uint32_t fraction;
    unsigned width;
    std::string_view unit;
    if (duration.seconds) {
        integral = duration.seconds;
        fraction = duration.nanos;
        width = 9;
        unit = "s";
    } else if (duration.nanos >= 1'000'000) {
        integral = duration.nanos / 1'000'000;
        fraction = duration.nanos % 1'000'000;
        width = 6;
        unit = "ms";
    } else if (duration.nanos >= 1'000) {
        integral = duration.nanos / 1'000;
        fraction = duration.nanos % 1'000;
        width = 3;
        unit = "µs";
    } else {
        integral = duration.nanos;
        fraction = 0;
        width = 0;
        unit = "ns";
    }
    auto precision = writer.options().precision;
    auto places = precision.value_or(width);
    auto kept = static_cast<unsigned>(std::min<std::size_t>(places, width));
    std::uint32_t divisor = 1;
    for (unsigned i = kept; i < width; ++i) {
        divisor *= 10;
    }
    auto rounded = fraction / divisor;
    auto remainder = fraction % divisor;
    auto last_odd = kept ? (rounded & 1) : (integral & 1);
    if (remainder > divisor / 2 || (divisor > 1 && remainder == divisor / 2 && last_odd)) {
        ++rounded;
    }
    std::uint32_t scale = 1;
    for (unsigned i = 0; i < kept; ++i) {
        scale *= 10;
    }
    bool overflow = false;
    if (rounded == scale) {
        rounded = 0;
        overflow = integral == UINT64_MAX;
        if (!overflow) {
            ++integral;
        }
    }
    String output(writer.memory_resource());
    if (writer.options().sign_plus) {
        output += '+';
    }
    char digits[24];
    if (overflow) {
        output += "18446744073709551616";
    } else {
        auto end = std::to_chars(digits, digits + sizeof(digits), integral).ptr;
        output.append(digits, end);
    }
    String fractional(writer.memory_resource());
    if (kept) {
        auto end = std::to_chars(digits, digits + sizeof(digits), rounded).ptr;
        fractional.append(kept - static_cast<unsigned>(end - digits), '0');
        fractional.append(digits, end);
    }
    if (precision) {
        fractional.append(places - kept, '0');
    } else {
        while (!fractional.empty() && fractional.back() == '0') {
            fractional.pop_back();
        }
    }
    if (!fractional.empty()) {
        output += '.';
        output += fractional;
    }
    output += unit;
    writer.padded_text(output);
}

inline void write_debug(DebugWriter& writer, const SystemTimeError& error) {
    writer.tuple("SystemTimeError", [&](auto& out) {
        write_debug(out, error.duration());
    });
}

inline void write_debug(DebugWriter& writer, const SystemTimeDifference& difference) {
    if (difference.is_ok()) {
        writer.tuple("Ok", [&](auto& out) {
            write_debug(out, difference.value());
        });
    } else {
        writer.tuple("Err", [&](auto& out) {
            write_debug(out, difference.error());
        });
    }
}
}
