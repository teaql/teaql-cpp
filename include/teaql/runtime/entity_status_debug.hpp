#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/runtime/entity_status.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, EntityStatus status) {
    writer.raw(to_string(status));
}

inline void write_debug(DebugWriter& writer, EntityAction action) {
    writer.raw(to_string(action));
}
}
