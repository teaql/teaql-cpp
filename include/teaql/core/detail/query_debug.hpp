#pragma once
#include <teaql/core/query.hpp>

namespace teaql::detail {
// Exact non-pretty Rust Debug shapes used by the pinned runtime's cache keys.
// These contain query values and must not be used as public log projections.
String debug_query(Memory, const SelectQuery&);
String debug_expr(Memory, const Expr&);
}
