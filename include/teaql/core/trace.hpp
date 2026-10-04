#pragma once
#include <teaql/core/value.hpp>

namespace teaql {
enum class TraceKind { Operation, Request, Relation, Entity, Provider, Sql, Comment, Purpose, AuditReason };
struct TraceNode {
    using allocator_type = Allocator;
    TraceKind kind;
    String entity_type;
    std::optional<std::uint64_t> entity_id;
    String comment;
    TraceNode(std::allocator_arg_t, Allocator a, TraceKind kind, std::string_view entity,
              std::string_view comment, std::optional<std::uint64_t> id = {})
        : kind(kind), entity_type(entity, a.resource()), entity_id(id), comment(comment, a.resource()) {}
    TraceNode(std::allocator_arg_t, Allocator a, const TraceNode& other)
        : TraceNode(std::allocator_arg, a, other.kind, other.entity_type, other.comment, other.entity_id) {}
    TraceNode(const TraceNode&) = delete;
    TraceNode(TraceNode&&) noexcept = default;
    bool operator==(const TraceNode& other) const {
        return kind == other.kind && entity_type == other.entity_type && entity_id == other.entity_id && comment == other.comment;
    }
};
using TraceChain = Vector<TraceNode>;
}
