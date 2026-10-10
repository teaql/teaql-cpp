#pragma once
#include <teaql/provider_sqlite/error.hpp>
#include <teaql/sql/error_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const MutationExecutorError& error) {
    constexpr std::string_view names[]{
        "Sqlite", "SqlCompile", "UnsupportedValue", "UnsupportedColumnType", "Bind", "Lock"};
    auto kind = static_cast<std::size_t>(error.kind());
    if (kind >= std::size(names)) {
        throw std::invalid_argument("Unknown provider error diagnostic variant");
    }
    writer.tuple(names[kind], [&](auto& out) {
        if (error.kind() == MutationExecutorError::Kind::Sqlite) {
            write_debug(out, error.driver_error());
        } else if (error.kind() == MutationExecutorError::Kind::SqlCompile) {
            write_debug(out, error.compile_error());
        } else {
            out.text(error.detail());
        }
    });
}
}
