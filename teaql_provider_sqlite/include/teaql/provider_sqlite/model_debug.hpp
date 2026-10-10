#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/provider_sqlite/schema.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const SqliteDialect&) {
    writer.raw("SqliteDialect");
}

inline void write_debug(DebugWriter& writer, const SqliteSchemaProvider&) {
    writer.raw("SqliteSchemaProvider");
}
}
