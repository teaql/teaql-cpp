#pragma once
#include <teaql/runtime/detail/data_service/entity_service.hpp>
#include <teaql/runtime/detail/data_service/diagnostic_query.hpp>
#include <teaql/runtime/detail/data_service/relation_enhancement.hpp>
#include <teaql/runtime/detail/context/pagination.hpp>
#include <teaql/runtime/detail/query_keys.hpp>
#include <teaql/runtime/cache.hpp>
#include <charconv>
#include <limits>

namespace teaql::detail {
template <class E>
class PreparedEntityQueryOperation {
    using R = RelationRowsResult<E>;
    using Q = Result<QueryResult, typename E::Error>;

public:
    using allocator_type = Allocator;

    PreparedEntityQueryOperation(std::allocator_arg_t,
                                 Allocator a,
                                 const EntityDataService<E>& service,
                                 const SelectQuery& query,
                                 bool owned,
                                 bool compact = false)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query),
          key_(a.resource()), owned_(owned), compact_(compact) {}

    std::optional<R> poll(WakeSignal& wake) {
        const auto& cs = EntityDataServiceAccess::context_service(service_);
        auto r = service_.memory_resource();
        if (!started_) {
            started_ = true;
            if (!compact_ && query_.aggregation_cache && query_.aggregation_cache->enabled) {
                auto& mutable_context = const_cast<UserContext&>(cs.metadata.context());
                if (mutable_context.template get_resource<AggregationCacheBackend>()) {
                    auto base =
                        mutable_context.template require_resource<AggregationCacheBackend>();
                    cache_.emplace(std::move(base).value());
                } else if (auto* concrete =
                               mutable_context.template get_resource<InMemoryAggregationCache>()) {
                    auto& value = *concrete;
                    cache_.emplace(std::allocator_arg,
                                   Allocator(r),
                                   value,
                                   resource_arena(value.memory_resource()));
                }
            }
            if (cache_) {
                key_ = aggregation_cache_key(r,
                                             cache_->get().name_space(),
                                             aggregation_cache_namespace(r, query_.entity),
                                             query_);
                String name(query_.entity, r);
                name += ".aggregation.get";
                RuntimeOperation operation(std::allocator_arg, Allocator(r), "cache", name);
                operation.attribute("teaql.cache.operation", "get");
                cache_scope_ = &cs.metadata.context().start_runtime_operation(operation);
            }
        }
        if (cache_scope_) {
            auto result = cache_scope_->poll_with_context([&] {
                return poll_body(wake);
            });
            if (result) {
                if (*result) {
                    finish_cache(cache_outcome_);
                } else {
                    cache_scope_->failure("cache_load_error");
                    cache_scope_->close();
                }
            }
            return result;
        }
        return poll_body(wake);
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        child_ = nullptr;
        close_scopes();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
        close_scopes();
    }

private:
    std::optional<R> poll_body(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& cs = EntityDataServiceAccess::context_service(service_);
        if (!child_) {
            if (cache_) {
                auto rows =
                    cache_->get().get(r, key_, query_.aggregation_cache->cache_expired_millis);
                if (rows) {
                    if (EntityDataServiceAccess::has_intent(service_)) {
                        auto intent = cs.executor.get().query_log_intent(r, query_);
                        EntityDataServiceAccess::extend_intent(service_, intent);
                    }
                    cache_outcome_ = "hit";
                    attach_property_definitions(*rows);
                    return R::ok(r, std::move(*rows));
                }
            }
            // Prepared cache hits return before request construction in the source.
            // Public entry points already validate intent before preparation.
            auto intent = EntityDataServiceAccess::request_intent(service_, query_);
            if (!intent) {
                return R::err(
                    r, DataServiceError<typename E::Error>::runtime(r, std::move(intent).error()));
            }
            EntityDataServiceAccess::bind_query_intent(service_, intent.value());
            if (cache_) {
                auto kind = resource_type_name<E>();
                String name(kind, r);
                name += ".query";
                RuntimeOperation operation(std::allocator_arg, Allocator(r), "provider", name);
                operation.attribute("teaql.provider.kind", kind)
                    .attribute("teaql.provider.operation", "query");
                provider_scope_ = &cs.metadata.context().start_runtime_operation(operation);
            }
        }
        auto request_work = [&]() -> const Q* {
            if (!child_) {
                auto capture = cs.metadata.capture_execution_metadata();
                auto intent = EntityDataServiceAccess::request_intent(service_, query_);
                QueryRequest request(std::allocator_arg,
                                     Allocator(r),
                                     query_,
                                     intent.value(),
                                     cs.metadata.capture_query_debug(),
                                     capture);
                if (cache_ || capture) {
                    TraceChain trace(query_.trace_chain, r);
                    request.trace_chain.swap(trace);
                } else {
                    request.trace_chain.clear();
                }
                if (owned_ &&
                    (compact_ || !query_.aggregation_cache || !query_.aggregation_cache->enabled)) {
                    request.query.trace_chain.clear();
                }
                auto& work = cs.executor.get().query_observed(
                    r, std::move(request), EntityDataServiceAccess::observer(service_));
                if (work.memory_resource() != r) {
                    work.cancel();
                    throw std::invalid_argument(
                        "Entity query result must belong to its UserContext");
                }
                child_ = &work;
            }
            forwarding_.attach(child_->wake_signal(), wake);
            return child_->poll();
        };
        const auto* result =
            provider_scope_ ? provider_scope_->poll_with_context(request_work) : request_work();
        if (!result) {
            return {};
        }
        if (!*result) {
            if (provider_scope_) {
                provider_scope_->failure("data_service_error");
                provider_scope_->close();
            }
            return R::err(r,
                          DataServiceError<typename E::Error>::executor(
                              r, copy_or_take_error(*child_, *result)));
        }
        if (provider_scope_) {
            provider_scope_->success(RuntimeAttributes(r));
            provider_scope_->close();
        }
        EntityDataServiceAccess::record(service_, result->value().metadata);
        Vector<CompactRow> rows(result->value().rows, r);
        if (cache_) {
            cache_->get().put(key_, rows);
            cache_outcome_ = "miss";
        }
        attach_property_definitions(rows);
        return R::ok(r, std::move(rows));
    }

    void attach_property_definitions(std::span<CompactRow> rows) const {
        if (query_.dynamic_property_definitions) {
            CompactRow::attach_dynamic_property_definitions(rows,
                                                            *query_.dynamic_property_definitions);
        }
    }

    void finish_cache(std::string_view outcome) {
        RuntimeAttributes attributes(service_.memory_resource());
        attributes.emplace(String("teaql.cache.result", service_.memory_resource()),
                           std::make_obj_using_allocator<RuntimeAttributeValue>(
                               Allocator(service_.memory_resource()), outcome));
        cache_scope_->success(attributes);
        cache_scope_->close();
    }

    void close_scopes() noexcept {
        if (provider_scope_) {
            provider_scope_->close();
        }
        if (cache_scope_) {
            cache_scope_->close();
        }
    }

    EntityDataService<E> service_;
    SelectQuery query_;
    String key_;
    std::optional<ResourceBorrow<AggregationCacheBackend>> cache_;
    AsyncOperation<Result<QueryResult, typename E::Error>>* child_ = nullptr;
    FailOpenRuntimeTelemetryScope* cache_scope_ = nullptr;
    FailOpenRuntimeTelemetryScope* provider_scope_ = nullptr;
    WakeSignal::Forward forwarding_;
    std::string_view cache_outcome_;
    bool started_ = false;
    bool owned_;
    bool compact_;
};

template <class E>
class EntityRowsOperation {
    using R = RelationRowsResult<E>;
    using H = RelationHydrationResult<E>;

public:
    using allocator_type = Allocator;

    EntityRowsOperation(std::allocator_arg_t,
                        Allocator a,
                        const EntityDataService<E>& service,
                        const SelectQuery& query,
                        std::span<const RelationAggregate> aggregates,
                        bool aggregate_entry = false,
                        bool prepared = false,
                        bool compact_entry = false)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query),
          aggregates_(aggregates.begin(), aggregates.end(), a.resource()), rows_(a.resource()),
          aggregate_keys_(a.resource()), cursor_key_(a.resource()), cursor_id_(a.resource()),
          fetch_(std::allocator_arg,
                 a,
                 [](const EntityDataService<E>& repo, SelectQuery query) -> AsyncOperation<R>& {
                     return EntityDataServiceAccess::fetch_all(repo, query);
                 }),
          aggregate_entry_(aggregate_entry), prepared_(prepared), compact_entry_(compact_entry) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        while (true) {
            if (phase_ == Phase::Prepare) {
                if (aggregate_entry_ && !prepared_) {
                    if (auto error = scope_query()) {
                        return R::err(
                            r, DataServiceError<typename E::Error>::runtime(r, std::move(*error)));
                    }
                }
                if (!prepared_) {
                    auto prepared = EntityDataServiceAccess::prepare_select(service_, query_);
                    if (!prepared) {
                        return R::err(r,
                                      DataServiceError<typename E::Error>::runtime(
                                          r, std::move(prepared).error()));
                    }
                    query_ = std::move(prepared).value();
                }
                const bool enhances =
                    prepared_ || aggregate_entry_ || compact_entry_ ||
                    query_.continuous_page_fetch || !query_.object_group_bys.empty() ||
                    !query_.child_enhancements.empty() || !query_.relations.empty();
                if (enhances && !query_.relation_aggregates.empty()) {
                    Vector<RelationAggregate> combined(query_.relation_aggregates, r);
                    for (const auto& aggregate : aggregates_) {
                        if (std::find(combined.begin(), combined.end(), aggregate) ==
                            combined.end()) {
                            combined.emplace_back(aggregate);
                        }
                    }
                    aggregates_.swap(combined);
                    query_.relation_aggregates.clear();
                    aggregate_entry_ = true;
                }
                if (prepared_ || aggregate_entry_ || query_.continuous_page_fetch ||
                    !query_.object_group_bys.empty() || !query_.child_enhancements.empty() ||
                    !query_.relations.empty()) {
                    if (auto error = scope_query()) {
                        return R::err(
                            r, DataServiceError<typename E::Error>::runtime(r, std::move(*error)));
                    }
                }
                auto intent = EntityDataServiceAccess::request_intent(service_, query_);
                if (!intent) {
                    return R::err(
                        r,
                        DataServiceError<typename E::Error>::runtime(r, std::move(intent).error()));
                }
                EntityDataServiceAccess::bind_query_intent(service_, intent.value());
                try {
                    query_ = query_.prepare_for_list();
                } catch (const std::invalid_argument& error) {
                    return R::err(r,
                                  DataServiceError<typename E::Error>::runtime(
                                      r, RuntimeError::graph(r, error.what())));
                }
                phase_ = Phase::Cursor;
                prepare_cursor();
            }
            if (phase_ == Phase::Cursor) {
                if (cursor_read_) {
                    forwarding_.attach(cursor_read_->wake_signal(), wake);
                    auto* result = cursor_read_->poll();
                    if (!result) {
                        return {};
                    }
                    if (!*result) {
                        observe("OFFSET_FALLBACK:STORE_UNAVAILABLE");
                    } else if (!result->value()) {
                        observe("OFFSET_FALLBACK:CACHE_MISS");
                    } else {
                        const auto& cursor = *result->value();
                        if (cursor.entity != query_.entity || cursor.direction != direction_ ||
                            cursor.page_size != page_size_ || cursor.next_offset != offset_ ||
                            cursor.expires_at <= SystemTime::now()) {
                            observe("OFFSET_FALLBACK:CURSOR_INVALID");
                        } else {
                            query_.slice->offset = 0;
                            query_.and_filter(direction_ == SortDirection::Asc
                                                  ? Expr::gt(r, "id", cursor.boundary)
                                                  : Expr::lt(r, "id", cursor.boundary));
                            cursor_id_ = cursor.cursor_id;
                            optimized_ = true;
                            observe("CURSOR_SEEK", cursor_id_);
                        }
                    }
                    forwarding_.reset();
                    cursor_read_ = nullptr;
                }
                const bool owned = !prepared_ && !aggregate_entry_ &&
                                   !query_.continuous_page_fetch &&
                                   query_.object_group_bys.empty() &&
                                   query_.child_enhancements.empty() && query_.relations.empty();
                row_work_ =
                    &make_operation<R, PreparedEntityQueryOperation<E>>(r, service_, query_, owned);
                phase_ = Phase::Fetch;
            }
            if (phase_ == Phase::Fetch) {
                forwarding_.attach(row_work_->wake_signal(), wake);
                auto* result = row_work_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return R::err(r, copy_or_take_error(*row_work_, *result));
                }
                rows_ = Vector<CompactRow>(result->value(), r);
                forwarding_.reset();
                row_work_ = nullptr;
                if (!aggregates_.empty()) {
                    auto keys = relation_aggregate_key_rows(
                        context(), service_.entity(), rows_, aggregates_);
                    if (!keys) {
                        return R::err(r,
                                      DataServiceError<typename E::Error>::runtime(
                                          r, std::move(keys).error()));
                    }
                    aggregate_keys_ = std::move(keys).value();
                }
                phase_ = query_.continuous_page_fetch || !query_.object_group_bys.empty() ||
                                 !query_.child_enhancements.empty() || !query_.relations.empty()
                             ? Phase::Groups
                         : aggregate_entry_ ? Phase::Aggregates
                                            : Phase::Done;
            }
            if ((phase_ >= Phase::Groups && phase_ <= Phase::Relations) ||
                phase_ == Phase::Aggregates) {
                if (!enhancement_) {
                    switch (phase_) {
                    case Phase::Groups:
                        enhancement_ = &enhance_object_group_bys(
                            service_, rows_, query_.object_group_bys, query_.trace_chain, fetch_);
                        break;
                    case Phase::Children:
                        enhancement_ = &enhance_child_queries(
                            service_, rows_, query_.child_enhancements, query_.trace_chain, fetch_);
                        break;
                    case Phase::Relations: {
                        auto plans = build_relation_plans_from_loads(
                            context(), query_.entity, query_.relations);
                        if (!plans) {
                            return R::err(r,
                                          DataServiceError<typename E::Error>::runtime(
                                              r, std::move(plans).error()));
                        }
                        auto& traced = context().template make<EntityDataService<E>>(
                            EntityDataServiceAccess::scoped(service_, query_.entity));
                        TraceChain trace(EntityDataServiceAccess::trace(service_).begin(),
                                         EntityDataServiceAccess::trace(service_).end(),
                                         r);
                        for (const auto& node : query_.trace_chain) {
                            trace.emplace_back(node);
                        }
                        traced.with_trace_context(trace);
                        enhancement_ =
                            &enhance_relation_plans(traced, rows_, plans.value(), fetch_);
                        break;
                    }
                    case Phase::Aggregates:
                        enhancement_ = &enhance_relation_aggregates(service_,
                                                                    rows_,
                                                                    aggregate_keys_,
                                                                    aggregates_,
                                                                    query_.aggregation_cache,
                                                                    query_.trace_chain,
                                                                    fetch_);
                        break;
                    default:
                        throw std::logic_error("Invalid entity enhancement phase");
                    }
                }
                forwarding_.attach(enhancement_->wake_signal(), wake);
                auto* result = enhancement_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return R::err(r, copy_or_take_error(*enhancement_, *result));
                }
                forwarding_.reset();
                enhancement_ = nullptr;
                phase_ = Phase(unsigned(phase_) + 1);
                continue;
            }
            if (phase_ == Phase::Register) {
                // Publish a cursor only after all result enhancements succeed.
                if (cursor_enabled_ && rows_.size() == page_size_ && !rows_.empty()) {
                    if (auto* boundary = rows_.back().get("id")) {
#ifndef NDEBUG
                        if (offset_ > UINT64_MAX - rows_.size()) {
                            throw std::overflow_error("Continuous page next offset overflow");
                        }
#endif
                        ContinuousPageCursor cursor(std::allocator_arg, Allocator(r));
                        auto now = SystemTime::now();
                        auto expires =
                            now.checked_add_seconds(query_.continuous_page_fetch->ttl_seconds);
                        if (!expires) {
                            throw std::overflow_error("Continuous page expiry overflow");
                        }
                        cursor.cursor_id = cursor_identifier(r, now);
                        cursor.query_key = cursor_key_;
                        cursor.entity = service_.entity();
                        cursor.direction = direction_;
                        cursor.boundary = Value(r, *boundary);
                        cursor.page_size = page_size_;
                        cursor.next_offset = offset_ + rows_.size();
                        cursor.expires_at = *expires;
                        bind_cursor_store();
                        cursor_write_ = &cursor_store_->get().put(r, cursor);
                        if (cursor_write_->memory_resource() != r) {
                            throw std::invalid_argument(
                                "Cursor write must belong to its UserContext");
                        }
                    }
                }
                phase_ = Phase::Store;
            }
            if (phase_ == Phase::Store) {
                if (cursor_write_) {
                    forwarding_.attach(cursor_write_->wake_signal(), wake);
                    auto* result = cursor_write_->poll();
                    if (!result) {
                        return {};
                    }
                    if (!*result) {
                        observe("OFFSET_FALLBACK:STORE_UNAVAILABLE");
                    } else {
                        observe(optimized_ ? "CURSOR_SEEK" : "OFFSET_FALLBACK:FIRST_PAGE",
                                optimized_ ? std::optional<std::string_view>(cursor_id_)
                                           : std::nullopt);
                    }
                    forwarding_.reset();
                    cursor_write_ = nullptr;
                }
                phase_ = Phase::Done;
                continue;
            }
            if (phase_ == Phase::Done) {
                return R::ok(r, std::move(rows_));
            }
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        row_work_ = nullptr;
        enhancement_ = nullptr;
        cursor_read_ = nullptr;
        cursor_write_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (row_work_) {
            row_work_->cancel();
        }
        if (enhancement_) {
            enhancement_->cancel();
        }
        if (cursor_read_) {
            cursor_read_->cancel();
        }
        if (cursor_write_) {
            cursor_write_->cancel();
        }
        finish_and_wait();
    }

private:
    std::optional<RuntimeError> scope_query() {
        if (EntityDataServiceAccess::has_request_intent(service_)) {
            return {};
        }
        auto source = query_.clone(service_.memory_resource());
        if (EntityDataServiceAccess::context_service(service_)
                .metadata.capture_execution_metadata()) {
            for (const auto& aggregate : aggregates_) {
                source.child_enhancements.emplace_back(aggregate.query);
            }
        }
        auto scoped = EntityDataServiceAccess::query_scoped(service_, source);
        if (!scoped) {
            return std::move(scoped).error();
        }
        service_ = std::move(scoped).value();
        return {};
    }

    enum class Phase {
        Prepare,
        Cursor,
        Fetch,
        Groups,
        Children,
        Relations,
        Aggregates,
        Register,
        Store,
        Done
    };

    const UserContext& context() const {
        return EntityDataServiceAccess::context(service_);
    }

    void observe(std::string_view plan, std::optional<std::string_view> id = {}) {
        ContextPaginationAccess::observe_continuous_page(context(), plan, id);
    }

    void prepare_cursor() {
        auto r = service_.memory_resource();
        if (!query_.continuous_page_fetch) {
            // The direct owned path does not enter prepare_continuous_page.
            if (!aggregates_.empty() || !query_.object_group_bys.empty() ||
                !query_.child_enhancements.empty() || !query_.relations.empty()) {
                observe("DISABLED");
            }
            return;
        }
        if (!query_.slice || !query_.slice->limit) {
            observe("OFFSET_FALLBACK:INVALID_SLICE");
            return;
        }
        if (query_.partition_field || !query_.aggregates.empty() ||
            !query_.group_by_fields.empty()) {
            observe("OFFSET_FALLBACK:UNSUPPORTED_QUERY_SHAPE");
            return;
        }
        if (query_.order_by.size() != 1 || query_.order_by[0].field != "id" ||
            query_.order_by[0].expr) {
            observe("OFFSET_FALLBACK:ORDER_NOT_SEEKABLE_ID");
            return;
        }
        cursor_enabled_ = true;
        page_size_ = *query_.slice->limit;
        offset_ = query_.slice->offset;
        direction_ = query_.order_by[0].direction;
        cursor_key_ = continuous_page_query_key(
            r, query_, query_.continuous_page_fetch->namespace_name, context().user_identifier());
        if (!offset_) {
            observe("OFFSET_FALLBACK:FIRST_PAGE");
        } else {
            bind_cursor_store();
            cursor_read_ = &cursor_store_->get().get(r, cursor_key_, offset_);
            if (cursor_read_->memory_resource() != r) {
                throw std::invalid_argument("Cursor read must belong to its UserContext");
            }
        }
    }

    void bind_cursor_store() {
        if (!cursor_store_) {
            auto& store = ContextPaginationAccess::cursor_store(context());
            cursor_store_.emplace(std::allocator_arg,
                                  Allocator(service_.memory_resource()),
                                  store,
                                  resource_arena(store.memory_resource()));
        }
    }

    static String cursor_identifier(Memory r, SystemTime now) {
        // Render the source's u128 nanosecond identity without compiler extensions.
        std::array<std::uint32_t, 4> words{};
        if (now.unix_seconds() >= 0) {
            std::uint64_t seconds = now.unix_seconds(), carry = now.subsec_nanos();
            words[0] = std::uint32_t(seconds);
            words[1] = std::uint32_t(seconds >> 32);
            for (auto& word : words) {
                carry += std::uint64_t(word) * 1000000000u;
                word = std::uint32_t(carry);
                carry >>= 32;
            }
        }
        String result("cpg_", r);
        bool emitted = false;
        for (int i = 3; i >= 0; --i) {
            if (!emitted && !words[i] && i) {
                continue;
            }
            char buffer[8];
            auto end = std::to_chars(buffer, buffer + sizeof(buffer), words[i], 16).ptr;
            if (emitted) {
                result.append(8 - std::size_t(end - buffer), '0');
            }
            result.append(buffer, end);
            emitted = true;
        }
        return result;
    }

    EntityDataService<E> service_;
    SelectQuery query_;
    Vector<RelationAggregate> aggregates_;
    Vector<CompactRow> rows_;
    Vector<CompactRow> aggregate_keys_;
    String cursor_key_, cursor_id_;
    EntityRelationFetch<E> fetch_;
    std::optional<ResourceBorrow<ContinuousPageCursorStore>> cursor_store_;
    AsyncOperation<PaginationResult<std::optional<ContinuousPageCursor>>>* cursor_read_ = nullptr;
    AsyncOperation<PaginationWriteResult>* cursor_write_ = nullptr;
    AsyncOperation<R>* row_work_ = nullptr;
    AsyncOperation<H>* enhancement_ = nullptr;
    WakeSignal::Forward forwarding_;
    Phase phase_ = Phase::Prepare;
    std::uint64_t page_size_ = 0, offset_ = 0;
    SortDirection direction_ = SortDirection::Asc;
    bool cursor_enabled_ = false, optimized_ = false;
    bool aggregate_entry_ = false;
    bool prepared_ = false;
    bool compact_entry_ = false;
};

}

#include <teaql/runtime/detail/data_service/dynamic_entities.hpp>

namespace teaql::detail {
template <class E>
class EntityRowListOperation;

template <class E, class T>
class EntityBasicListOperation {
    using R = Result<SmartList<T>, DataServiceError<typename E::Error>>;

public:
    using allocator_type = Allocator;

    EntityBasicListOperation(std::allocator_arg_t,
                             Allocator a,
                             const EntityDataService<E>& service,
                             const SelectQuery& query)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_) {
            if constexpr (!std::same_as<T, CompactRow>) {
                child_ = &make_operation<R, DynamicEntityListOperation<E, T>>(
                    r, service_, std::move(query_));
            } else {
                auto prepared = EntityDataServiceAccess::prepare_select(service_, query_);
                if (!prepared) {
                    return R::err(r,
                                  DataServiceError<typename E::Error>::runtime(
                                      r, std::move(prepared).error()));
                }
                query_ = std::move(prepared).value();
                auto& rows = make_operation<RelationRowsResult<E>, PreparedEntityQueryOperation<E>>(
                    r, service_, query_, true, true);
                child_ = &make_operation<R, EntityRowListOperation<E>>(r, rows);
            }
        }
        forwarding_.attach(child_->wake_signal(), wake);
        if (auto* result = child_->poll()) {
            return copy_or_take_completed(*child_, *result);
        }
        return {};
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
    EntityDataService<E> service_;
    SelectQuery query_;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
class EntityRowListOperation {
    using R = Result<SmartList<CompactRow>, DataServiceError<typename E::Error>>;

public:
    using allocator_type = Allocator;

    EntityRowListOperation(std::allocator_arg_t,
                           Allocator a,
                           AsyncOperation<RelationRowsResult<E>>& child)
        : resource_(a.resource()), child_(child) {
        if (child.memory_resource() != resource_) {
            throw std::invalid_argument("Entity list work must belong to its UserContext");
        }
    }

    std::optional<R> poll(WakeSignal& wake) {
        forwarding_.attach(child_.wake_signal(), wake);
        auto* result = child_.poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(resource_, copy_or_take_error(child_, *result));
        }
        return R::ok(resource_,
                     SmartList<CompactRow>(std::allocator_arg,
                                           Allocator(resource_),
                                           std::span<const CompactRow>(result->value())));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        child_.cancel();
    }

private:
    Memory resource_;
    AsyncOperation<RelationRowsResult<E>>& child_;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::fetch_all(const EntityDataService<E>& service,
                                        const SelectQuery& query)
    -> AsyncOperation<RelationRowsResult<E>>& {
    return make_operation<RelationRowsResult<E>, EntityRowsOperation<E>>(
        service.memory_resource(), service, query, std::span<const RelationAggregate>{});
}

template <class E>
auto EntityDataServiceAccess::fetch_compact_all(const EntityDataService<E>& service,
                                                const SelectQuery& query)
    -> AsyncOperation<RelationRowsResult<E>>& {
    return make_operation<RelationRowsResult<E>, EntityRowsOperation<E>>(
        service.memory_resource(),
        service,
        query,
        std::span<const RelationAggregate>{},
        false,
        false,
        true);
}
}

namespace teaql {
template <class E>
auto EntityDataService<E>::fetch_all(const PurposedSelectQuery& query) const
    -> DataServiceOperationResult<Vector<CompactRow>, DataServiceError<Error>>& {
    return detail::execute_purposed_query<Vector<CompactRow>>(
        *this,
        query,
        {},
        +[](const EntityDataService<E>& service,
            const SelectQuery& actual,
            std::span<const RelationAggregate>) -> auto& {
            return detail::EntityDataServiceAccess::fetch_all(service, actual);
        });
}

template <class E>
auto EntityDataService<E>::fetch_all_owned(PurposedSelectQuery query) const
    -> DataServiceOperationResult<Vector<CompactRow>, DataServiceError<Error>>& {
    return fetch_all(query);
}

template <class E>
auto EntityDataService<E>::fetch_all_with_relation_aggregates(
    const PurposedSelectQuery& query, std::span<const RelationAggregate> aggregates) const
    -> DataServiceOperationResult<Vector<CompactRow>, DataServiceError<Error>>& {
    return detail::execute_purposed_query<Vector<CompactRow>>(
        *this,
        query,
        aggregates,
        +[](const EntityDataService<E>& service,
            const SelectQuery& actual,
            std::span<const RelationAggregate> items) -> auto& {
            return make_operation<detail::RelationRowsResult<E>, detail::EntityRowsOperation<E>>(
                service.memory_resource(), service, actual, items, true);
        });
}

template <class E>
auto EntityDataService<E>::fetch_smart_list(const PurposedSelectQuery& query) const
    -> DataServiceOperationResult<SmartList<CompactRow>, DataServiceError<Error>>& {
    using R = Result<SmartList<CompactRow>, DataServiceError<Error>>;
    return detail::execute_purposed_query<SmartList<CompactRow>>(
        *this,
        query,
        {},
        +[](const EntityDataService<E>& service,
            const SelectQuery& actual,
            std::span<const RelationAggregate>) -> auto& {
            return make_operation<R, detail::EntityBasicListOperation<E, CompactRow>>(
                service.memory_resource(), service, actual);
        });
}

template <class E>
auto EntityDataService<E>::fetch_smart_list_with_relation_aggregates(
    const PurposedSelectQuery& query, std::span<const RelationAggregate> aggregates) const
    -> DataServiceOperationResult<SmartList<CompactRow>, DataServiceError<Error>>& {
    using R = Result<SmartList<CompactRow>, DataServiceError<Error>>;
    return detail::execute_scoped_purposed_query<SmartList<CompactRow>>(
        *this,
        query,
        aggregates,
        +[](const EntityDataService<E>& service,
            const SelectQuery& actual,
            std::span<const RelationAggregate> items) -> auto& {
            auto& rows =
                make_operation<detail::RelationRowsResult<E>, detail::EntityRowsOperation<E>>(
                    service.memory_resource(), service, actual, items, true);
            return make_operation<R, detail::EntityRowListOperation<E>>(service.memory_resource(),
                                                                        rows);
        });
}

template <class E>
template <Entity T>
auto EntityDataService<E>::fetch_entities(const PurposedSelectQuery& query) const
    -> DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>& {
    using R = Result<SmartList<T>, DataServiceError<Error>>;
    return detail::execute_purposed_query<SmartList<T>>(
        *this,
        query,
        {},
        +[](const EntityDataService<E>& service,
            const SelectQuery& actual,
            std::span<const RelationAggregate>) -> auto& {
            return make_operation<R, detail::EntityBasicListOperation<E, T>>(
                service.memory_resource(), service, actual);
        });
}
}
