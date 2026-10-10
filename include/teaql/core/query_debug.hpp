#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/core/meta_debug.hpp>
#include <teaql/core/query.hpp>

namespace teaql {
// Raw model diagnostics. Never substitute these for privacy-filtered SQL logs.
void write_debug(DebugWriter&, BinaryOp);
void write_debug(DebugWriter&, ExprFunction);
void write_debug(DebugWriter&, SortDirection);
void write_debug(DebugWriter&, AggregateFunction);
void write_debug(DebugWriter&, TraceKind);
void write_debug(DebugWriter&, const Expr&);
void write_debug(DebugWriter&, const NamedExpr&);
void write_debug(DebugWriter&, const OrderBy&);
void write_debug(DebugWriter&, const Aggregate&);
void write_debug(DebugWriter&, const Slice&);
void write_debug(DebugWriter&, const RelationLoad&);
void write_debug(DebugWriter&, const RelationAggregate&);
void write_debug(DebugWriter&, const RawSqlProjection&);
void write_debug(DebugWriter&, const ObjectGroupBy&);
void write_debug(DebugWriter&, const AggregationCacheOptions&);
void write_debug(DebugWriter&, const StreamConfig&);
void write_debug(DebugWriter&, const ContinuousPageFetchOptions&);
void write_debug(DebugWriter&, const IdSetPaginationOptions&);
void write_debug(DebugWriter&, const TraceNode&);
void write_debug(DebugWriter&, const SelectQuery&);
}
