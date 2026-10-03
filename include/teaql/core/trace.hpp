#pragma once
#include <teaql/core/value.hpp>

namespace teaql {
enum class TraceKind { Operation, Request, Relation, Entity, Provider, Sql, Comment, Purpose, AuditReason };
struct TraceNode {
    TraceKind kind;
    String entity_type;
    std::optional<std::uint64_t> entity_id;
    String comment;
    TraceNode(Memory r, TraceKind kind, std::string_view entity, std::string_view comment)
        : kind(kind), entity_type(entity, r), comment(comment, r) {}
};
using TraceChain = Vector<TraceNode>;
}
