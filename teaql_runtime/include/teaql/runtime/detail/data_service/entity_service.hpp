#pragma once
#include <teaql/runtime/detail/data_service/data_service.hpp>
#include <teaql/runtime/relation.hpp>
#include <teaql/runtime/purposed_query.hpp>
#include <teaql/runtime/entity_status.hpp>
#include <teaql/runtime/detail/ledger_execution_outcome.hpp>

namespace teaql::detail {
Result<SqlIntentRedactions, RuntimeError> graph_plan_privacy(const UserContext&,
                                                             const GraphMutationPlan&);
Result<SqlIntentRedactions, RuntimeError> entity_ledger_privacy(const UserContext&,
                                                                const EntityRuntimeState&);
}

namespace teaql {
namespace core {
struct EntityGraph;
}

namespace detail {
struct EntityDataServiceAccess;
Result<GraphNode, RuntimeError>
graph_node_from_values(const UserContext&, std::string_view, MutationValues);
void record_query_metadata(const UserContext&, SqlIntentRedactions*, const ExecutionMetadata&);
}

// A stream is Context-owned. Copying this handle does not extend that lifetime.
template <class Error>
using EntityQueryStream = ResourceBorrow<QueryStream<DataServiceError<Error>>>;

template <Entity T, class Error>
using TypedEntityQueryStream = ResourceBorrow<AsyncStream<Result<T, DataServiceError<Error>>>>;

template <class E>
class EntityDataService {
public:
    using allocator_type = Allocator;
    using Error = typename E::Error;
    static_assert(std::derived_from<E, QueryExecutor<Error>> &&
                  std::derived_from<E, MutationExecutor<Error>>);

    EntityDataService(std::allocator_arg_t,
                      Allocator a,
                      const UserContext& context,
                      std::string_view entity,
                      const ResourceBorrow<E>& executor)
        : entity_(entity, a.resource()), data_service_(std::allocator_arg, a, context, executor),
          trace_context_(a.resource()) {}

    EntityDataService(std::allocator_arg_t, Allocator a, const EntityDataService& other)
        : entity_(other.entity_, a.resource()),
          data_service_(std::allocator_arg, a, other.data_service_),
          trace_context_(other.trace_context_, a.resource()),
          query_intent_(detail::clone_in(a.resource(), other.query_intent_)),
          query_log_intent_(other.query_log_intent_) {}

    EntityDataService(const EntityDataService&) = delete;
    EntityDataService(EntityDataService&&) noexcept = default;

    EntityDataService& operator=(EntityDataService&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return entity_.get_allocator().resource();
    }

    template <class Owner>
    static EntityDataService&
    for_executor(const UserContext& context, std::string_view entity, Owner& owner, E& executor) {
        detail::ContextEntityAccess::ensure_open(context);
        auto r = context.memory_resource();
        auto entry = detail::ContextResources::entry(
            detail::resource_arena(r), owner.memory_resource(), executor);
        ResourceBorrow<E> borrow(std::allocator_arg, Allocator(r), executor, *entry.owner);
        return context.template make<EntityDataService>(context, entity, borrow);
    }

    EntityDataService& with_trace_context(std::span<const TraceNode> trace) & {
        TraceChain copy(trace.begin(), trace.end(), memory_resource());
        trace_context_.swap(copy);
        return *this;
    }

    std::string_view entity() const {
        return entity_;
    }

    SelectQuery select() const {
        return SelectQuery(std::allocator_arg, Allocator(memory_resource()), entity_);
    }

    DataServiceOperationResult<GraphMutationPlan, DataServiceError<Error>>&
        plan_graph(GraphNode) const;

    template <Entity T>
    Result<GraphNode, RuntimeError> graph_node_from_entity(T entity) const {
        auto r = memory_resource();
        using R = Result<GraphNode, RuntimeError>;
        Arena::Quiescence active;
        active.attach(detail::resource_arena(r), nullptr, [](void*) noexcept {
            std::terminate();
        });
        auto descriptor = T::entity_descriptor(r);
        if (descriptor.name != entity_) {
            String message("entity data service ", r);
            message += entity_;
            message += " cannot extract graph root ";
            message += descriptor.name;
            return R::err(r, RuntimeError::graph(r, message));
        }
        auto dirty = detail::clone_in(r, entity.dirty_fields());
        auto original = detail::clone_in(r, entity.original_values());
        bool removed = entity.is_marked_as_delete();
        auto comment = detail::clone_in(r, entity.get_comment());
        auto node = detail::graph_node_from_values(
            context(), descriptor.name, std::move(entity).into_values());
        if (!node) {
            return node;
        }
        node.value().dirty_fields = std::move(dirty);
        node.value().original_values = std::move(original);
        if (removed) {
            node.value().operation = GraphOperation::Remove;
            node.value().relations.clear();
        }
        if (comment) {
            node.value().set_comment(*comment);
        }
        return node;
    }

    DataServiceOperationResult<Vector<CompactRow>, DataServiceError<Error>>&
    fetch_all(const PurposedSelectQuery&) const;
    DataServiceOperationResult<Vector<CompactRow>, DataServiceError<Error>>&
        fetch_all_owned(PurposedSelectQuery) const;
    DataServiceOperationResult<EntityQueryStream<Error>, DataServiceError<Error>>&
    fetch_stream(const PurposedSelectQuery&) const
        requires std::derived_from<E, StreamQueryExecutor<Error>>;
    DataServiceOperationResult<Vector<CompactRow>, DataServiceError<Error>>&
    fetch_all_with_relation_aggregates(const PurposedSelectQuery&,
                                       std::span<const RelationAggregate>) const;
    DataServiceOperationResult<SmartList<CompactRow>, DataServiceError<Error>>&
    fetch_smart_list(const PurposedSelectQuery&) const;
    DataServiceOperationResult<SmartList<CompactRow>, DataServiceError<Error>>&
    fetch_smart_list_with_relation_aggregates(const PurposedSelectQuery&,
                                              std::span<const RelationAggregate>) const;
    template <Entity T>
    DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>&
    fetch_entities(const PurposedSelectQuery&) const;
    template <Entity T>
    DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>&
    fetch_enhanced_entities(const PurposedSelectQuery&) const;
    template <Entity T>
    DataServiceOperationResult<std::optional<T>, DataServiceError<Error>>&
    fetch_entity(const PurposedSelectQuery&) const;
    template <Entity T>
    DataServiceOperationResult<TypedEntityQueryStream<T, Error>, DataServiceError<Error>>&
    fetch_entity_stream(const PurposedSelectQuery&) const
        requires std::derived_from<E, StreamQueryExecutor<Error>>;
    template <Entity T>
    DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>&
    fetch_enhanced_entities_with_relation_aggregates(const PurposedSelectQuery&,
                                                     std::span<const RelationAggregate>) const;
    template <Entity T>
    DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>&
        fetch_enhanced_entities_with_relation_aggregates_owned(
            PurposedSelectQuery, std::span<const RelationAggregate>) const;

    InsertCommand insert_command() const {
        return InsertCommand(std::allocator_arg, Allocator(memory_resource()), entity_);
    }

    Result<InsertCommand, RuntimeError> prepare_insert_command(const InsertCommand&) const;

    template <class T>
    UpdateCommand update_command(T&& id) const {
        return UpdateCommand(
            std::allocator_arg, Allocator(memory_resource()), entity_, std::forward<T>(id));
    }

    template <class T>
    DeleteCommand delete_command(T&& id) const {
        return DeleteCommand(
            std::allocator_arg, Allocator(memory_resource()), entity_, std::forward<T>(id));
    }

    template <class T>
    RecoverCommand recover_command(T&& id, std::int64_t version) const {
        return RecoverCommand(std::allocator_arg,
                              Allocator(memory_resource()),
                              entity_,
                              std::forward<T>(id),
                              version);
    }

    Result<UpdateCommand, RuntimeError> prepare_update_command(const UpdateCommand& source) const {
        auto r = memory_resource();
        using R = Result<UpdateCommand, RuntimeError>;
        Arena::Quiescence active;
        active.attach(detail::resource_arena(r), nullptr, [](void*) noexcept {
            std::terminate();
        });
        auto command = detail::clone_in(r, source);
        if (auto b = behavior()) {
            auto result = b->get().before_update(r, context(), command);
            if (!result) {
                return R::err(r, std::move(result).error());
            }
        }
        if (auto* policy = detail::ContextEntityAccess::request_policy(context())) {
            auto result = policy->enforce_update(r, context(), command);
            if (!result) {
                return R::err(r, std::move(result).error());
            }
        }
        return R::ok(r, std::move(command));
    }

    Vector<String> relation_loads() const {
        Arena::Quiescence active;
        active.attach(detail::resource_arena(memory_resource()), nullptr, [](void*) noexcept {
            std::terminate();
        });
        auto b = behavior();
        return b ? b->get().relation_loads(memory_resource(), context())
                 : Vector<String>(memory_resource());
    }

    detail::RelationPlansResult relation_plans() const {
        return detail::build_relation_plans(context(), entity_, relation_loads());
    }

    Result<SelectQuery, RuntimeError> relation_query(std::string_view relation,
                                                     std::span<const CompactRow> parents) const {
        auto r = memory_resource();
        using R = Result<SelectQuery, RuntimeError>;
        auto plans = relation_plans();
        if (!plans) {
            return R::err(r, std::move(plans).error());
        }
        for (const auto& plan : plans.value()) {
            if (plan.relation_name == relation) {
                return R::ok(r, detail::query_for_relation_plan(context(), plan, parents, false));
            }
        }
        return R::err(r, RuntimeError::missing_relation(r, entity_, relation));
    }

private:
    friend struct detail::EntityDataServiceAccess;
    String entity_;
    detail::ContextDataService<E> data_service_;
    TraceChain trace_context_;
    std::optional<QueryIntent> query_intent_;
    // Present only on a private query tree, never on UserContext or a wire query.
    SqlIntentRedactions* query_log_intent_ = nullptr;

    const UserContext& context() const {
        return data_service_.metadata.context();
    }

    std::optional<EntityBehaviorBorrow> behavior() const {
        if (detail::is_generated_schema_bootstrap(context())) {
            return {};
        }
        return context().entity_data_service_behavior(entity_);
    }

    EntityDataService& with_query_intent(const QueryIntent& intent) & {
        intent.require_valid();
        query_intent_.emplace(std::allocator_arg, Allocator(memory_resource()), intent);
        return *this;
    }

    Result<QueryIntent, RuntimeError> request_intent_for(const SelectQuery& query) const {
        auto r = memory_resource();
        using R = Result<QueryIntent, RuntimeError>;
        if (query_intent_) {
            return R::ok(r, detail::clone_in(r, *query_intent_));
        }
        auto intent = QueryIntent::from_optional(
            r,
            query.comment_text ? std::optional<std::string_view>(*query.comment_text)
                               : std::nullopt,
            query.purpose_text ? std::optional<std::string_view>(*query.purpose_text)
                               : std::nullopt);
        if (!intent) {
            return R::err(r, RuntimeError::request_intent(r, intent.error()));
        }
        return R::ok(r, std::move(intent).value());
    }

    Result<SelectQuery, RuntimeError> prepare_select_query(const SelectQuery& query) const {
        auto r = memory_resource();
        auto intent = request_intent_for(query);
        if (!intent) {
            return Result<SelectQuery, RuntimeError>::err(r, std::move(intent).error());
        }
        auto copied = detail::clone_in(r, query);
        copied.comment_text.emplace(intent.value().comment(), r);
        copied.purpose_text.emplace(intent.value().purpose(), r);
        return detail::prepare_select_query(context(), trace_context_, copied);
    }

    EntityDataService scoped_data_service(std::string_view entity) const {
        auto r = memory_resource();
        EntityDataService scoped(
            std::allocator_arg, Allocator(r), context(), entity, data_service_.executor);
        if (auto* mutation = data_service_.mutation_intent()) {
            scoped.data_service_.with_mutation_intent(*mutation, data_service_.mutation_privacy());
        }
        if (query_intent_) {
            scoped.with_query_intent(*query_intent_);
        }
        if (query_log_intent_) {
            scoped.query_log_intent_ =
                &context().template make<SqlIntentRedactions>(*query_log_intent_);
            scoped.data_service_.metadata.with_query_privacy(*scoped.query_log_intent_);
        }
        return scoped;
    }

    EntityDataService query_scoped_service() const {
        auto scoped = scoped_data_service(entity_);
        TraceChain trace(trace_context_, memory_resource());
        scoped.trace_context_.swap(trace);
        if (!scoped.query_log_intent_) {
            scoped.query_log_intent_ = &context().template make<SqlIntentRedactions>();
        }
        scoped.data_service_.metadata.with_query_privacy(*scoped.query_log_intent_);
        return scoped;
    }

    void record_query_metadata(const ExecutionMetadata& value) const {
        detail::record_query_metadata(context(), query_log_intent_, value);
    }

    std::optional<ExecutionObserver> query_diagnostic_observer() const {
        if (!data_service_.metadata.capture_execution_metadata()) {
            return {};
        }
        const UserContext* owner = &context();
        auto* intent = query_log_intent_;
        return ExecutionObserver(std::allocator_arg,
                                 Allocator(memory_resource()),
                                 [owner, intent](ExecutionMetadata value) {
                                     detail::record_query_metadata(*owner, intent, value);
                                 });
    }
};

template <class E>
Result<ResourceBorrow<EntityDataService<E>>, ContextError>
UserContext::entity_data_service(std::string_view entity) const {
    auto r = memory_resource();
    using R = Result<ResourceBorrow<EntityDataService<E>>, ContextError>;
    if (!has_entity_data_service(entity)) {
        return R::err(r,
                      ContextError(std::allocator_arg,
                                   Allocator(r),
                                   ContextError::Kind::MissingEntityDataService,
                                   entity));
    }
    if (!detail::ContextEntityAccess::has_metadata(*this)) {
        return R::err(
            r,
            ContextError(
                std::allocator_arg, Allocator(r), ContextError::Kind::MissingResource, "metadata"));
    }
    auto executor = detail::ContextEntityAccess::require_executor<E>(*this);
    if (!executor) {
        return R::err(r, std::move(executor).error());
    }
    auto& service = make<EntityDataService<E>>(*this, entity, executor.value());
    return R::ok(r,
                 ResourceBorrow<EntityDataService<E>>(
                     std::allocator_arg, Allocator(r), service, detail::resource_arena(r)));
}

namespace detail {
struct EntityDataServiceAccess {
    template <class E>
    static auto
    insert_graph_node_scoped(const EntityDataService<E>&, GraphNode, const ScopedCommentNode*)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto
    upsert_graph_node_scoped(const EntityDataService<E>&, GraphNode, const ScopedCommentNode*)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto
    validate_reference_node(const EntityDataService<E>&, GraphNode, std::span<const TraceNode>)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto
    validate_remove_node(const EntityDataService<E>&, const GraphNode&, std::span<const TraceNode>)
        -> AsyncOperation<Result<std::monostate, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto graph_update_command(const EntityDataService<E>&,
                                     GraphNode&,
                                     const EntityDescriptor&,
                                     const PropertyDescriptor&,
                                     const Value&)
        -> Result<UpdateCommand, DataServiceError<typename E::Error>>;
    template <class E>
    static auto
    delete_graph_node(const EntityDataService<E>&, const GraphNode&, const ScopedCommentNode*)
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto save_entity_graph_from_internal(const EntityDataService<E>&, core::EntityGraph)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E, Entity T>
    static auto save_entity_graph_internal(const EntityDataService<E>&, T)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E, Entity T>
    static auto save_entity_internal(const EntityDataService<E>&, T, EntityStatus)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E, Entity T>
    static auto save_entity_with_comment_internal(const EntityDataService<E>&,
                                                  T,
                                                  EntityStatus,
                                                  std::string_view)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E, Entity T>
    static auto
    save_entity_graph_with_comment_internal(const EntityDataService<E>&, T, std::string_view)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E, Entity T>
    static auto
    create_entity_graph_with_comment_internal(const EntityDataService<E>&, T, std::string_view)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto execute_ledger_plan_internal(const EntityDataService<E>&,
                                             const EntityRuntimeState&,
                                             const std::pmr::map<EntityKey, ObjectLocation>&)
        -> AsyncOperation<Result<LedgerExecutionOutcome, DataServiceError<typename E::Error>>>&;
    template <class E>
    static Result<Vector<EntityKey>, RuntimeError>
    order_new_ledger_keys(const EntityDataService<E>&,
                          std::span<const EntityKey>,
                          const std::pmr::map<EntityKey, EntityValues>&);
    template <class E>
    static auto execute_graph_plan_internal(const EntityDataService<E>&, GraphMutationPlan)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto save_graph_internal(const EntityDataService<E>&, GraphNode)
        -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto fetch_graph_current_row_internal(const EntityDataService<E>&,
                                                 std::string_view,
                                                 std::string_view,
                                                 const Value&,
                                                 std::span<const TraceNode>)
        -> AsyncOperation<Result<std::optional<CompactRow>, DataServiceError<typename E::Error>>>&;

    template <class E>
    static auto behavior(const EntityDataService<E>& service) {
        return service.behavior();
    }
    template <class E>
    static auto insert_internal(const EntityDataService<E>&, const InsertCommand&)
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto update_internal(const EntityDataService<E>&, const UpdateCommand&)
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto delete_internal(const EntityDataService<E>&, const DeleteCommand&)
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto delete_scoped_internal(const EntityDataService<E>&,
                                       const DeleteCommand&,
                                       std::span<const TraceNode>)
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto recover_internal(const EntityDataService<E>&, const RecoverCommand&)
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto execute_prepared_insert(const EntityDataService<E>&,
                                        const InsertCommand&,
                                        std::span<const TraceNode> = {})
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto execute_prepared_update(const EntityDataService<E>&,
                                        const UpdateCommand&,
                                        std::span<const TraceNode> = {})
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto execute_prepared_batch_insert(const EntityDataService<E>&,
                                              const BatchInsertCommand&)
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;
    template <class E>
    static auto execute_prepared_batch_update(const EntityDataService<E>&,
                                              const BatchUpdateCommand&)
        -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>&;

    template <class E>
    static auto load_dynamic_fields_into_rows(const EntityDataService<E>&,
                                              std::span<CompactRow>,
                                              std::string_view,
                                              const DynamicFieldSelection&,
                                              const QueryIntent&)
        -> AsyncOperation<Result<std::monostate, DataServiceError<typename E::Error>>>&;

    template <class E>
    static bool has_intent(const EntityDataService<E>& service) {
        return service.query_log_intent_ != nullptr;
    }

    template <class E>
    static bool has_request_intent(const EntityDataService<E>& service) {
        return service.query_intent_.has_value();
    }

    template <class E>
    static const UserContext& context(const EntityDataService<E>& service) {
        return service.context();
    }

    template <class E>
    static const DynamicFieldStore* dynamic_field_store(const EntityDataService<E>& service) {
        return service.data_service_.executor.get().dynamic_field_store();
    }

    template <class E>
    static void extend_intent(const EntityDataService<E>& service,
                              const SqlIntentRedactions& value) {
        if (service.query_log_intent_) {
            service.query_log_intent_->extend(value);
        }
    }

    template <class E>
    static AsyncOperation<Result<Vector<CompactRow>, DataServiceError<typename E::Error>>>&
    fetch_all(const EntityDataService<E>&, const SelectQuery&);

    template <class E, Entity T>
    static auto fetch_entities_with_relation_aggregates(const EntityDataService<E>&,
                                                        const SelectQuery&,
                                                        std::span<const RelationAggregate>)
        -> AsyncOperation<Result<SmartList<T>, DataServiceError<typename E::Error>>>&;

    template <class E>
    static void bind_query_intent(EntityDataService<E>& service, const QueryIntent& intent) {
        service.with_query_intent(intent);
    }

    template <class E>
    static RuntimeHookResult bind_mutation_intent(EntityDataService<E>& service,
                                                  std::optional<std::string_view> comment) {
        auto r = service.memory_resource();
        auto mutation = MutationIntent::from_optional(r, comment);
        if (!mutation) {
            return RuntimeHookResult::err(r, RuntimeError::request_intent(r, mutation.error()));
        }
        auto intent = QueryIntent::create(
            r, mutation.value().comment(), "runtime: load state for an audited graph mutation");
        service.with_query_intent(intent.value());
        service.data_service_.with_mutation_intent(mutation.value(),
                                                   service.data_service_.mutation_privacy());
        return RuntimeHookResult::ok(r, {});
    }

    template <class E>
    static RuntimeHookResult bind_mutation_privacy(EntityDataService<E>& service,
                                                   const SqlIntentRedactions& redactions) {
        auto r = service.memory_resource();
        auto& retained = service.context().template make<SqlIntentRedactions>(redactions);
        service.data_service_.with_mutation_privacy(&retained);
        service.query_log_intent_ = &retained;
        return RuntimeHookResult::ok(r, {});
    }

    template <class E>
    static RuntimeHookResult bind_plan_privacy(EntityDataService<E>& service,
                                               const GraphMutationPlan& plan) {
        auto collected = graph_plan_privacy(service.context(), plan);
        if (!collected) {
            return RuntimeHookResult::err(service.memory_resource(), std::move(collected).error());
        }
        return bind_mutation_privacy(service, collected.value());
    }

    template <class E>
    static RuntimeHookResult bind_ledger_privacy(EntityDataService<E>& service,
                                                 const EntityRuntimeState& root) {
        auto collected = entity_ledger_privacy(service.context(), root);
        if (!collected) {
            return RuntimeHookResult::err(service.memory_resource(), std::move(collected).error());
        }
        return bind_mutation_privacy(service, collected.value());
    }

    template <class E>
    static auto request_intent(const EntityDataService<E>& service, const SelectQuery& query) {
        return service.request_intent_for(query);
    }

    template <class E>
    static auto prepare_select(const EntityDataService<E>& service, const SelectQuery& query) {
        return service.prepare_select_query(query);
    }

    template <class E>
    static auto scoped(const EntityDataService<E>& service, std::string_view entity) {
        return service.scoped_data_service(entity);
    }

    template <class E>
    static auto query_scoped(const EntityDataService<E>& service) {
        return service.query_scoped_service();
    }

    static const SelectQuery* diagnostic_source(const PurposedSelectQuery& query) {
        return query.diagnostic_source_ ? &*query.diagnostic_source_ : nullptr;
    }

    template <class E>
    static auto query_scoped(const EntityDataService<E>& service, const SelectQuery& source) {
        auto r = service.memory_resource();
        using R = Result<EntityDataService<E>, RuntimeError>;
        auto intent = service.request_intent_for(source);
        if (!intent) {
            return R::err(r, std::move(intent).error());
        }
        auto scoped = service.query_scoped_service();
        scoped.with_query_intent(intent.value());
        if (scoped.data_service_.metadata.capture_execution_metadata()) {
            auto tree = clone_in(r, source);
            expand_query_facet_diagnostics(tree);
            Vector<const SelectQuery*> pending(r);
            pending.push_back(&tree);
            while (!pending.empty()) {
                auto* query = pending.back();
                pending.pop_back();
                scoped.query_log_intent_->extend(
                    scoped.data_service_.executor.get().query_log_intent(r, *query));
                for (const auto& relation : query->relations) {
                    if (relation.query) {
                        pending.push_back(&*relation.query);
                    }
                }
                for (const auto& child : query->child_enhancements) {
                    pending.push_back(&child);
                }
                for (const auto& aggregate : query->relation_aggregates) {
                    pending.push_back(&aggregate.query);
                }
                for (const auto& group : query->object_group_bys) {
                    pending.push_back(&group.query);
                }
            }
        }
        return R::ok(r, std::move(scoped));
    }

    template <class E>
    static void record(const EntityDataService<E>& service, const ExecutionMetadata& metadata) {
        service.record_query_metadata(metadata);
    }

    template <class E>
    static auto observer(const EntityDataService<E>& service) {
        return service.query_diagnostic_observer();
    }

    template <class E>
    static auto intent(const EntityDataService<E>& service) {
        return service.query_log_intent_
                   ? std::optional<SqlIntentRedactions>(
                         clone_in(service.memory_resource(), *service.query_log_intent_))
                   : std::nullopt;
    }

    template <class E>
    static void capture_readback_target(EntityDataService<E>& scoped, const Value& id) {
        if (!scoped.query_log_intent_) {
            scoped.query_log_intent_ = &scoped.context().template make<SqlIntentRedactions>();
        }
        scoped.query_log_intent_->capture_target_id(id);
    }

    template <class E>
    static const ContextDataService<E>& context_service(const EntityDataService<E>& service) {
        return service.data_service_;
    }

    template <class E>
    static std::span<const TraceNode> trace(const EntityDataService<E>& service) {
        return service.trace_context_;
    }

    template <class E>
    static EntityDataService<E> relation_child(const EntityDataService<E>& service,
                                               const RelationLoadPlan& plan) {
        auto r = service.memory_resource();
        TraceChain trace(service.trace_context_, r);
        String comment(plan.parent_entity, r);
        comment += '.';
        comment += plan.relation_name;
        trace.emplace_back(TraceKind::Relation, plan.relation_name, comment);
        auto child = service.scoped_data_service(plan.target_entity);
        child.with_trace_context(trace);
        return child;
    }

    template <class E>
    static Result<std::optional<FlatRelationPlans>, RuntimeError>
    flat_relation_plans(const EntityDataService<E>& service, const SelectQuery& query) {
        using Plans = FlatRelationPlans;
        using R = Result<std::optional<Plans>, RuntimeError>;
        auto r = service.memory_resource();
        auto requested =
            build_relation_plans_from_loads(service.context(), query.entity, query.relations);
        if (!requested) {
            return R::err(r, std::move(requested).error());
        }
        auto behaviors = service.relation_plans();
        if (!behaviors) {
            return R::err(r, std::move(behaviors).error());
        }
        auto supported = [&](auto&& self, const RelationLoadPlan& plan) -> bool {
            if (!ContextEntityAccess::has_entity_graph_decoder(service.context(),
                                                               plan.target_entity)) {
                return false;
            }
            for (const auto& child : plan.children) {
                if (!self(self, child)) {
                    return false;
                }
            }
            return true;
        };
        for (const auto& plan : requested.value()) {
            if (!supported(supported, plan)) {
                return R::ok(r, {});
            }
        }
        for (const auto& plan : behaviors.value()) {
            if (!supported(supported, plan)) {
                return R::ok(r, {});
            }
        }
        return R::ok(r,
                     Plans(std::allocator_arg,
                           Allocator(r),
                           std::move(requested).value(),
                           std::move(behaviors).value()));
    }
};
}
}
