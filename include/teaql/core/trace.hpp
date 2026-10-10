#pragma once
#include <teaql/core/value.hpp>

namespace teaql {
enum class TraceKind {
    Operation,
    Request,
    Relation,
    Entity,
    Provider,
    Sql,
    Comment,
    Purpose,
    AuditReason
};

struct TraceNode {
    using allocator_type = Allocator;
    TraceKind kind;
    String entity_type;
    std::optional<std::uint64_t> entity_id;
    String comment;

    TraceNode(std::allocator_arg_t,
              Allocator a,
              std::string_view entity,
              std::string_view comment,
              std::optional<std::uint64_t> id = {})
        : TraceNode(std::allocator_arg, a, TraceKind::Entity, entity, comment, id) {}

    TraceNode(std::allocator_arg_t,
              Allocator a,
              TraceKind kind,
              std::string_view entity,
              std::string_view comment,
              std::optional<std::uint64_t> id = {})
        : kind(kind), entity_type(entity, a.resource()), entity_id(id),
          comment(comment, a.resource()) {}

    TraceNode(std::allocator_arg_t, Allocator a, const TraceNode& other)
        : TraceNode(std::allocator_arg,
                    a,
                    other.kind,
                    other.entity_type,
                    other.comment,
                    other.entity_id) {}

    TraceNode(std::allocator_arg_t, Allocator a, TraceNode&& other)
        : TraceNode(detail::move_to(a.resource(), std::move(other))) {}

    TraceNode(const TraceNode&) = delete;
    TraceNode(TraceNode&&) noexcept = default;

    TraceNode& operator=(TraceNode&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entity_type.get_allocator().resource();
    }

    bool operator==(const TraceNode& other) const {
        return kind == other.kind && entity_type == other.entity_type &&
               entity_id == other.entity_id && comment == other.comment;
    }
};

using TraceChain = Vector<TraceNode>;
}
