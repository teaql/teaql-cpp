#pragma once
#include <teaql/runtime/detail/data_service/graph_current_row.hpp>

namespace teaql::detail {
inline bool graph_unassigned(const Value* value) {
    if (!value || std::holds_alternative<std::monostate>(value->data)) {
        return true;
    }
    if (auto v = std::get_if<std::int64_t>(&value->data)) {
        return *v == 0;
    }
    if (auto v = std::get_if<std::uint64_t>(&value->data)) {
        return *v == 0;
    }
    return false;
}

inline const Value* graph_field(const GraphNode& node, std::string_view name) {
    auto found = node.values.fields.find(name);
    return found == node.values.fields.end() ? nullptr : &found->second;
}

inline std::optional<std::uint64_t> graph_trace_id(const GraphNode& node) {
    auto* value = node.id();
    if (value) {
        if (auto id = value->try_u64(); id && *id > 0) {
            return id;
        }
    }
    return {};
}

inline bool graph_has_comment(const GraphNode& node) {
    auto text = node.comment ? std::string_view(*node.comment) : std::string_view{};
    trim(text);
    return !text.empty();
}

inline void
ensure_graph_timestamps(GraphNode& node, const EntityDescriptor& descriptor, bool fresh) {
    auto now = Timestamp::now();
    for (auto name : {std::string_view("create_time"), std::string_view("update_time")}) {
        if ((fresh || name == "update_time") && descriptor.property_by_name(name) &&
            graph_unassigned(graph_field(node, name))) {
            node.value(name, now);
        }
    }
}

// Frames and trace links use stable User-owned storage. An explicit traversal
// stack preserves recursive source ordering without retaining native stack frames.
struct GraphPlanningFrame {
    using allocator_type = Allocator;
    enum class Phase { Start, Finalize, Children };
    GraphNode& node;
    const ScopedCommentNode* parent_scope;
    const TraceScopeToken* token;
    bool parent_is_create, force_create = false, update = false;
    Phase phase = Phase::Start;
    std::optional<ResourceBorrow<const EntityDescriptor>> descriptor;
    std::optional<ScopedCommentNode> scope;
    decltype(GraphNode::relations)::iterator relation;
    std::size_t child = 0;

    GraphPlanningFrame(std::allocator_arg_t,
                       Allocator,
                       GraphNode& node,
                       const ScopedCommentNode* scope,
                       const TraceScopeToken* token,
                       bool create)
        : node(node), parent_scope(scope), token(token), parent_is_create(create),
          relation(node.relations.begin()) {}

    const ScopedCommentNode* active_scope() const {
        return scope ? &*scope : parent_scope;
    }
};

template <class E>
class GraphPlanningOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<GraphMutationPlan, Error>;

public:
    using allocator_type = Allocator;

    GraphPlanningOperation(std::allocator_arg_t,
                           Allocator a,
                           const EntityDataService<E>& service,
                           GraphNode node)
        : service_(std::allocator_arg, a, service), plan_(std::allocator_arg, a),
          frames_(a.resource()) {
        plan_.planned_root.emplace(std::allocator_arg, a, std::move(node));
    }

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& context = EntityDataServiceAccess::context(service_);
        if (!started_) {
            started_ = true;
            auto& root = *plan_.planned_root;
            auto bound = EntityDataServiceAccess::bind_mutation_intent(
                service_,
                root.comment ? std::optional<std::string_view>(*root.comment) : std::nullopt);
            if (!bound) {
                return failed(std::move(bound).error());
            }
            if (root.entity != service_.entity()) {
                String message("entity data service ", r);
                message += service_.entity();
                message += " cannot plan graph root ";
                message += root.entity;
                return failed(RuntimeError::graph(r, message));
            }
            push(root, nullptr, nullptr, false);
        }
        while (!frames_.empty()) {
            auto& frame = *frames_.back();
            auto& node = frame.node;
            if (frame.phase == GraphPlanningFrame::Phase::Start) {
                if (node.operation == GraphOperation::Reference ||
                    node.operation == GraphOperation::Remove) {
                    if (node.operation == GraphOperation::Remove && graph_has_comment(node)) {
                        TraceNode trace(std::allocator_arg,
                                        Allocator(r),
                                        TraceKind::AuditReason,
                                        node.entity,
                                        *node.comment,
                                        graph_trace_id(node));
                        frame.token = &resource_arena(r).template make_payload<TraceScopeToken>(
                            frame.token, trace, plan_.next_item_index);
                    }
                    plan_.push(node.entity,
                               node.operation == GraphOperation::Reference
                                   ? GraphMutationKind::Reference
                                   : GraphMutationKind::Delete,
                               node.values.into_mutation_values(r),
                               {},
                               frame.token,
                               node.original_values);
                    frames_.pop_back();
                    continue;
                }
                auto descriptor = context.require_entity(node.entity);
                if (!descriptor) {
                    return failed(std::move(descriptor).error());
                }
                frame.descriptor.emplace(std::allocator_arg, Allocator(r), descriptor.value());
                if (graph_has_comment(node)) {
                    TraceNode trace(std::allocator_arg,
                                    Allocator(r),
                                    TraceKind::AuditReason,
                                    node.entity,
                                    *node.comment,
                                    graph_trace_id(node));
                    frame.scope.emplace(
                        std::allocator_arg, Allocator(r), frame.parent_scope, trace);
                }
                auto* property = frame.descriptor->get().id_property();
                auto* id = property ? graph_field(node, property->name) : nullptr;
                if (!graph_unassigned(id) &&
                    !plan_.visited_nodes.emplace(String(node.entity, r), graph_identity_key(r, *id))
                         .second) {
                    frames_.pop_back();
                    continue;
                }
                frame.force_create =
                    node.operation == GraphOperation::Create ||
                    (frame.parent_is_create && node.operation == GraphOperation::Upsert);
                frame.phase = GraphPlanningFrame::Phase::Finalize;
                if (!frame.force_create && property && !graph_unassigned(id)) {
                    TraceChain trace(r);
                    if (auto scope = frame.active_scope()) {
                        trace = scope->to_trace_chain(r);
                    }
                    child_ = &EntityDataServiceAccess::fetch_graph_current_row_internal(
                        service_, node.entity, property->name, *id, trace);
                }
            }
            if (frame.phase == GraphPlanningFrame::Phase::Finalize) {
                if (child_) {
                    forwarding_.attach(child_->wake_signal(), wake);
                    auto* result = child_->poll();
                    if (!result) {
                        return {};
                    }
                    forwarding_.reset();
                    auto* completed = std::exchange(child_, nullptr);
                    if (!*result) {
                        return R::err(r, copy_or_take_error(*completed, *result));
                    }
                    frame.update = result->value().has_value();
                }
                // Validate the metadata owner again after a suspended query.
                (void)context.entity(node.entity);
                const auto& descriptor = frame.descriptor->get();
                if (!frame.update) {
                    if (auto* id = descriptor.id_property();
                        id && graph_unassigned(graph_field(node, id->name))) {
                        auto generated = context.next_id(node.entity);
                        if (!generated) {
                            return failed(std::move(generated).error());
                        }
                        node.value(id->name, generated.value());
                    }
                    if (auto* version = descriptor.version_property();
                        version && graph_unassigned(graph_field(node, version->name))) {
                        node.value(version->name, std::int64_t(1));
                    }
                }
                ensure_graph_timestamps(node, descriptor, !frame.update);
                Vector<String> fields(r);
                if (frame.update) {
                    Vector<String> excluded(r);
                    if (auto* id = descriptor.id_property()) {
                        excluded.emplace_back(id->name);
                    }
                    if (auto* version = descriptor.version_property()) {
                        excluded.emplace_back(version->name);
                    }
                    fields = sorted_update_fields(r, node.values, excluded);
                    if (node.dirty_fields) {
                        std::erase_if(fields, [&](const String& name) {
                            return !node.dirty_fields->contains(name);
                        });
                    }
                }
                if (graph_has_comment(node)) {
                    TraceNode trace(std::allocator_arg,
                                    Allocator(r),
                                    TraceKind::AuditReason,
                                    node.entity,
                                    *node.comment,
                                    graph_trace_id(node));
                    frame.token = &resource_arena(r).template make_payload<TraceScopeToken>(
                        frame.token, trace, plan_.next_item_index);
                }
                plan_.push(node.entity,
                           graph_mutation_kind_for_update(frame.update),
                           node.values.into_mutation_values(r),
                           fields,
                           frame.token,
                           node.original_values);
                frame.phase = GraphPlanningFrame::Phase::Children;
            }
            if (frame.phase == GraphPlanningFrame::Phase::Children) {
                if (frame.relation == node.relations.end()) {
                    frames_.pop_back();
                    continue;
                }
                (void)context.entity(node.entity);
                const auto& [name, children] = *frame.relation;
                auto* relation = frame.descriptor->get().relation_by_name(name);
                if (!relation) {
                    return failed(RuntimeError::missing_relation(r, node.entity, name));
                }
                if (frame.child == children.size()) {
                    ++frame.relation;
                    frame.child = 0;
                    continue;
                }
                auto& child = frame.relation->second[frame.child++];
                if (child.entity != relation->target_entity) {
                    String message("relation ", r);
                    message += node.entity;
                    message += '.';
                    message += name;
                    message += " expects ";
                    message += relation->target_entity;
                    message += ", got ";
                    message += child.entity;
                    return failed(RuntimeError::graph(r, message));
                }
                push(child, frame.active_scope(), frame.token, frame.force_create);
            }
        }
        plan_.rebuild_batches();
        return R::ok(r, std::move(plan_));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        child_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
    }

private:
    void push(GraphNode& node,
              const ScopedCommentNode* scope,
              const TraceScopeToken* token,
              bool create) {
        frames_.push_back(
            &resource_arena(service_.memory_resource())
                 .template make_payload<GraphPlanningFrame>(node, scope, token, create));
    }

    R failed(RuntimeError error) {
        auto r = service_.memory_resource();
        return R::err(r, Error::runtime(r, std::move(error)));
    }

    EntityDataService<E> service_;
    GraphMutationPlan plan_;
    Vector<GraphPlanningFrame*> frames_;
    AsyncOperation<Result<std::optional<CompactRow>, Error>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
    bool started_ = false;
};
}

namespace teaql {
template <class E>
auto EntityDataService<E>::plan_graph(GraphNode node) const
    -> DataServiceOperationResult<GraphMutationPlan, DataServiceError<Error>>& {
    return make_operation<Result<GraphMutationPlan, DataServiceError<Error>>,
                          detail::GraphPlanningOperation<E>>(
        memory_resource(), *this, std::move(node));
}
}
