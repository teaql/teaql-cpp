#pragma once
#include <teaql/core/value.hpp>

namespace teaql::detail {
String debug_text(Memory, std::string_view);
String debug_value(Memory, const Value&);
}
