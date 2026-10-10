#pragma once
#include <teaql/memory/types.hpp>

namespace teaql::detail {
// JSON's exponent thresholds differ from Rust's always-expanded f64 Display.
void append_float(String&, double, bool json);
void append_float(String&, float, bool json);
}
