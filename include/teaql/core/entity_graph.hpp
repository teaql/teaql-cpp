#pragma once
#include <teaql/core/entity.hpp>

namespace teaql {
// Annotated save input, distinct from the runtime identity/loading graph.
enum class EntityGraphOperation { Save, Delete };

struct EntityGraphNode {
    using allocator_type = Allocator;
    String entity_type;
    MutationValues values;
    std::optional<String> comment;
    EntityGraphOperation operation = EntityGraphOperation::Save;
    Vector<std::pair<String, EntityGraphNode>> children;

    EntityGraphNode(std::allocator_arg_t,
                    Allocator a,
                    std::string_view type,
                    const MutationValues& mutation)
        : entity_type(type, a.resource()), values(a.resource(), mutation), children(a.resource()) {}

    EntityGraphNode(std::allocator_arg_t, Allocator a, const EntityGraphNode& other);

    EntityGraphNode(std::allocator_arg_t, Allocator a, EntityGraphNode&& other)
        : EntityGraphNode(detail::move_to(a.resource(), std::move(other))) {}

    EntityGraphNode(const EntityGraphNode&) = delete;
    EntityGraphNode(EntityGraphNode&&) noexcept = default;

    EntityGraphNode& operator=(EntityGraphNode&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return entity_type.get_allocator().resource();
    }
};

inline EntityGraphNode::EntityGraphNode(std::allocator_arg_t,
                                        Allocator a,
                                        const EntityGraphNode& other)
    : EntityGraphNode(std::allocator_arg, a, other.entity_type, other.values) {
    if (other.comment) {
        comment.emplace(*other.comment, a.resource());
    }
    operation = other.operation;
    for (const auto& [relation, node] : other.children) {
        children.emplace_back(relation, node);
    }
}
class EntityGraphBuilder;

struct EntityGraph {
    using allocator_type = Allocator;
    EntityGraphNode root;

    EntityGraph(std::allocator_arg_t, Allocator a, EntityGraphNode&& node)
        : root(std::allocator_arg, a, std::move(node)) {}

    EntityGraph(std::allocator_arg_t, Allocator a, const EntityGraph& other)
        : root(std::allocator_arg, a, other.root) {}

    EntityGraph(std::allocator_arg_t, Allocator a, EntityGraph&& other)
        : root(std::allocator_arg, a, std::move(other.root)) {}

    EntityGraph(const EntityGraph&) = delete;
    EntityGraph(EntityGraph&&) noexcept = default;

    Memory memory_resource() const {
        return root.memory_resource();
    }

    template <Entity T>
    static EntityGraphBuilder from_entity(Memory r, T entity);
};

class EntityGraphBuilder {
public:
    using allocator_type = Allocator;

    EntityGraphBuilder(std::allocator_arg_t, Allocator a, EntityGraphNode&& node)
        : node_(std::allocator_arg, a, std::move(node)) {}

    EntityGraphBuilder(std::allocator_arg_t, Allocator a, const EntityGraphBuilder& other)
        : node_(std::allocator_arg, a, other.node_) {}

    EntityGraphBuilder(const EntityGraphBuilder&) = delete;
    EntityGraphBuilder(EntityGraphBuilder&&) noexcept = default;

    Memory memory_resource() const {
        return node_.memory_resource();
    }

    EntityGraphBuilder& comment(std::string_view text) & {
        String copy(text, memory_resource());
        node_.comment.emplace(std::move(copy));
        return *this;
    }

    EntityGraphBuilder&& comment(std::string_view text) && {
        comment(text);
        return std::move(*this);
    }

    EntityGraphBuilder& delete_() & {
        node_.operation = EntityGraphOperation::Delete;
        return *this;
    }

    EntityGraphBuilder&& delete_() && {
        delete_();
        return std::move(*this);
    }

    EntityGraphBuilder& child(std::string_view relation, EntityGraphBuilder child) & {
        node_.children.emplace_back(String(relation, memory_resource()), std::move(child.node_));
        return *this;
    }

    EntityGraphBuilder&& child(std::string_view relation, EntityGraphBuilder child) && {
        this->child(relation, std::move(child));
        return std::move(*this);
    }

    EntityGraph build() && {
        return EntityGraph(std::allocator_arg, Allocator(memory_resource()), std::move(node_));
    }

private:
    EntityGraphNode node_;
};

template <Entity T>
EntityGraphBuilder EntityGraph::from_entity(Memory r, T entity) {
    auto descriptor = T::entity_descriptor(r);
    auto values = std::move(entity).into_values();
    return EntityGraphBuilder(
        std::allocator_arg,
        Allocator(r),
        EntityGraphNode(std::allocator_arg, Allocator(r), descriptor.name, values));
}
}
