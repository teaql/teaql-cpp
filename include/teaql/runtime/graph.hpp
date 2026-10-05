#pragma once
#include <teaql/core/mutation.hpp>
#include <teaql/core/row.hpp>
#include <teaql/runtime/resources.hpp>
#include <set>
#include <unordered_set>

namespace teaql {
// Mutable checker/planner state is distinct from commands and loaded snapshots.
struct EntityValues {
    using allocator_type = Allocator;
    Record fields;

    EntityValues(std::allocator_arg_t, Allocator a) : fields(a.resource()) {}

    EntityValues(std::allocator_arg_t, Allocator a, const Record& values)
        : fields(values, a.resource()) {}

    EntityValues(std::allocator_arg_t, Allocator a, const EntityValues& other)
        : EntityValues(std::allocator_arg, a, other.fields) {}

    EntityValues(std::allocator_arg_t, Allocator a, const MutationValues& other)
        : EntityValues(std::allocator_arg, a, other.fields) {}

    EntityValues(std::allocator_arg_t, Allocator a, const CompactRow& other)
        : fields(other.into_map(a.resource())) {}

    EntityValues(std::allocator_arg_t, Allocator a, EntityValues&& other)
        : EntityValues(detail::move_to(a.resource(), std::move(other))) {}

    EntityValues(const EntityValues&) = delete;
    EntityValues(EntityValues&&) noexcept = default;

    EntityValues& operator=(EntityValues&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return fields.get_allocator().resource();
    }

    Record into_map(Memory r) const {
        return clone_record(r, fields);
    }

    MutationValues into_mutation_values(Memory r) const {
        MutationValues result(r);
        result.fields = clone_record(r, fields);
        return result;
    }

    auto begin() {
        return fields.begin();
    }

    auto end() {
        return fields.end();
    }

    auto begin() const {
        return fields.begin();
    }

    auto end() const {
        return fields.end();
    }

    bool operator==(const EntityValues&) const = default;
};
enum class GraphOperation { Upsert, Create, Reference, Remove };
enum class GraphMutationKind { Create, Update, Delete, Reference };

inline GraphMutationKind graph_mutation_kind_for_update(bool update) {
    return update ? GraphMutationKind::Update : GraphMutationKind::Create;
}

struct GraphNode {
    using allocator_type = Allocator;
    String entity;
    EntityValues values;
    std::pmr::map<String, Vector<GraphNode>, std::less<>> relations;
    GraphOperation operation = GraphOperation::Upsert;
    std::optional<String> comment;
    std::optional<std::pmr::set<String, std::less<>>> dirty_fields;
    std::optional<EntitySnapshot> original_values;

    GraphNode(std::allocator_arg_t, Allocator a, std::string_view entity)
        : entity(entity, a.resource()), values(std::allocator_arg, a), relations(a.resource()) {}

    GraphNode(std::allocator_arg_t, Allocator a, const GraphNode& other)
        : entity(other.entity, a.resource()), values(std::allocator_arg, a, other.values),
          relations(other.relations, a.resource()), operation(other.operation),
          comment(detail::clone_in(a.resource(), other.comment)),
          dirty_fields(detail::clone_in(a.resource(), other.dirty_fields)),
          original_values(detail::clone_in(a.resource(), other.original_values)) {}

    GraphNode(std::allocator_arg_t, Allocator a, GraphNode&& other)
        : GraphNode(detail::move_to(a.resource(), std::move(other))) {}

    GraphNode(const GraphNode&) = delete;
    GraphNode(GraphNode&&) noexcept = default;

    GraphNode& operator=(GraphNode&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entity.get_allocator().resource();
    }

    GraphNode& with_operation(GraphOperation value) {
        operation = value;
        return *this;
    }

    GraphNode& reference() {
        return with_operation(GraphOperation::Reference);
    }

    GraphNode& remove() {
        return with_operation(GraphOperation::Remove);
    }

    template <class T>
    GraphNode& value(std::string_view field, T&& v) {
        values.fields.insert_or_assign(String(field, memory_resource()),
                                       Value(memory_resource(), std::forward<T>(v)));
        return *this;
    }

    GraphNode& relation(std::string_view name, const GraphNode& node) {
        relations.try_emplace(String(name, memory_resource())).first->second.emplace_back(node);
        return *this;
    }

    GraphNode& with_relations(std::string_view name, std::span<const GraphNode> nodes) {
        auto& target = relations.try_emplace(String(name, memory_resource())).first->second;
        for (const auto& node : nodes)
            target.emplace_back(node);
        return *this;
    }

    const Value* id() const {
        auto found = values.fields.find(std::string_view("id"));
        return found == values.fields.end() ? nullptr : &found->second;
    }

    void set_comment(std::string_view value) {
        String copy(value, memory_resource());
        comment.emplace(std::move(copy));
    }

    GraphNode& with_comment(std::string_view value) {
        set_comment(value);
        return *this;
    }

    bool operator==(const GraphNode& other) const {
        return entity == other.entity && values == other.values && relations == other.relations &&
               operation == other.operation && comment == other.comment &&
               dirty_fields == other.dirty_fields &&
               ((!original_values && !other.original_values) ||
                (original_values && other.original_values &&
                 original_values->fields == other.original_values->fields));
    }
};

// Persistent trace nodes live in a Context arena. Parent links are borrows;
// allocator-aware cross-domain copies recursively copy the parent chain.
class TraceScopeToken {
public:
    using allocator_type = Allocator;
    TraceNode track;
    std::uint64_t node_index;
    TraceScopeToken(std::allocator_arg_t,
                    Allocator,
                    const TraceScopeToken* parent,
                    const TraceNode&,
                    std::uint64_t);
    TraceScopeToken(std::allocator_arg_t, Allocator, const TraceScopeToken&);
    TraceScopeToken(const TraceScopeToken&) = delete;

    const TraceScopeToken* parent() const noexcept {
        return parent_;
    }

    Memory memory_resource() const noexcept {
        return track.comment.get_allocator().resource();
    }

    TraceChain recover_trace_chain(Memory) const;
    bool operator==(const TraceScopeToken&) const;

private:
    const TraceScopeToken* parent_;
};

namespace detail {
const TraceScopeToken* clone_trace_scope(Memory, const TraceScopeToken*);
}

struct GraphMutationPlanItem {
    using allocator_type = Allocator;
    String entity;
    GraphMutationKind kind;
    MutationValues values;
    Vector<String> update_fields;
    std::uint64_t item_index;
    const TraceScopeToken* scope_token;
    std::optional<EntitySnapshot> old_values;
    GraphMutationPlanItem(std::allocator_arg_t,
                          Allocator,
                          std::string_view,
                          GraphMutationKind,
                          const MutationValues&,
                          std::span<const String>,
                          std::uint64_t,
                          const TraceScopeToken*,
                          const std::optional<EntitySnapshot>&);
    GraphMutationPlanItem(std::allocator_arg_t, Allocator, const GraphMutationPlanItem&);

    GraphMutationPlanItem(std::allocator_arg_t, Allocator a, GraphMutationPlanItem&& other)
        : GraphMutationPlanItem(detail::move_to(a.resource(), std::move(other))) {}

    GraphMutationPlanItem(const GraphMutationPlanItem&) = delete;
    GraphMutationPlanItem(GraphMutationPlanItem&&) noexcept = default;

    GraphMutationPlanItem& operator=(GraphMutationPlanItem&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entity.get_allocator().resource();
    }

    bool operator==(const GraphMutationPlanItem&) const;
};

struct GraphMutationBatch {
    using allocator_type = Allocator;
    String entity;
    GraphMutationKind kind;
    Vector<String> update_fields;
    Vector<GraphMutationPlanItem> items;

    GraphMutationBatch(std::allocator_arg_t,
                       Allocator a,
                       std::string_view entity,
                       GraphMutationKind kind,
                       std::span<const String> fields)
        : entity(entity, a.resource()), kind(kind),
          update_fields(fields.begin(), fields.end(), a.resource()), items(a.resource()) {}

    GraphMutationBatch(std::allocator_arg_t, Allocator a, const GraphMutationBatch& other)
        : entity(other.entity, a.resource()), kind(other.kind),
          update_fields(other.update_fields, a.resource()), items(other.items, a.resource()) {}

    GraphMutationBatch(std::allocator_arg_t, Allocator a, GraphMutationBatch&& other)
        : GraphMutationBatch(detail::move_to(a.resource(), std::move(other))) {}

    GraphMutationBatch(const GraphMutationBatch&) = delete;
    GraphMutationBatch(GraphMutationBatch&&) noexcept = default;

    GraphMutationBatch& operator=(GraphMutationBatch&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entity.get_allocator().resource();
    }

    bool operator==(const GraphMutationBatch&) const = default;
};

namespace detail {
struct GraphVisitedHash {
    std::size_t operator()(const std::pair<String, String>& key) const noexcept {
        auto first = std::hash<std::string_view>{}(key.first),
             second = std::hash<std::string_view>{}(key.second);
        return first ^ (second + std::size_t(0x9e3779b9) + (first << 6) + (first >> 2));
    }
};
}

struct GraphMutationPlan {
    using allocator_type = Allocator;
    std::optional<GraphNode> planned_root;
    Vector<GraphMutationPlanItem> items;
    Vector<GraphMutationBatch> batches;
    std::uint64_t next_item_index = 0;
    std::pmr::unordered_set<std::pair<String, String>, detail::GraphVisitedHash> visited_nodes;

    GraphMutationPlan(std::allocator_arg_t, Allocator a)
        : items(a.resource()), batches(a.resource()), visited_nodes(a.resource()) {}

    GraphMutationPlan(std::allocator_arg_t, Allocator a, const GraphMutationPlan& other)
        : planned_root(detail::clone_in(a.resource(), other.planned_root)),
          items(other.items, a.resource()), batches(other.batches, a.resource()),
          next_item_index(other.next_item_index), visited_nodes(other.visited_nodes, a.resource()) {
    }

    GraphMutationPlan(std::allocator_arg_t, Allocator a, GraphMutationPlan&& other)
        : GraphMutationPlan(detail::move_to(a.resource(), std::move(other))) {}

    GraphMutationPlan(const GraphMutationPlan&) = delete;
    GraphMutationPlan(GraphMutationPlan&&) noexcept = default;

    GraphMutationPlan& operator=(GraphMutationPlan&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return items.get_allocator().resource();
    }

    void push(std::string_view,
              GraphMutationKind,
              const MutationValues&,
              std::span<const String>,
              const TraceScopeToken* = nullptr,
              const std::optional<EntitySnapshot>& = {});
    void rebuild_batches();
    using Counts = std::pmr::map<std::pair<String, GraphMutationKind>, std::size_t>;
    Counts grouped_counts(Memory) const;

    std::size_t batch_count() const noexcept {
        return batches.size();
    }

    std::size_t len() const noexcept {
        return items.size();
    }

    bool is_empty() const noexcept {
        return items.empty();
    }

    bool operator==(const GraphMutationPlan&) const = default;
};

Vector<String> sorted_update_fields(Memory, const EntityValues&, std::span<const String> excluded);

// A synchronous stack cons-list, distinct from the persistent arena token.
struct ScopedCommentNode {
    using allocator_type = Allocator;
    const ScopedCommentNode* parent;
    TraceNode track;

    ScopedCommentNode(std::allocator_arg_t,
                      Allocator a,
                      const ScopedCommentNode* parent,
                      const TraceNode& track)
        : parent(parent), track(std::allocator_arg, a, track) {}

    ScopedCommentNode(const ScopedCommentNode&) = delete;
    TraceChain to_trace_chain(Memory) const;
};
}
