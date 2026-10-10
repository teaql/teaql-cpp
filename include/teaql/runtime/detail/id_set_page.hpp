#pragma once
#include <teaql/runtime/detail/entity_query.hpp>
#include <teaql/runtime/detail/id_set_build_lock.hpp>

namespace teaql::detail {
struct IdSetPage {
    using allocator_type = Allocator;
    SelectQuery query;
    std::optional<std::uint64_t> total_count;

    IdSetPage(std::allocator_arg_t,
              Allocator a,
              const SelectQuery& query,
              std::optional<std::uint64_t> count)
        : query(std::allocator_arg, a, query), total_count(count) {}

    IdSetPage(std::allocator_arg_t, Allocator a, const IdSetPage& other)
        : IdSetPage(std::allocator_arg, a, other.query, other.total_count) {}

    IdSetPage(std::allocator_arg_t, Allocator a, IdSetPage&& other)
        : IdSetPage(move_to(a.resource(), std::move(other))) {}

    IdSetPage(const IdSetPage&) = delete;
    IdSetPage(IdSetPage&&) noexcept = default;

    Memory memory_resource() const {
        return query.memory_resource();
    }
};

template <class E>
class IdSetPageOperation {
    using R = Result<IdSetPage, DataServiceError<typename E::Error>>;
    using Read = PaginationResult<std::optional<RetainedIdSet>>;

public:
    using allocator_type = Allocator;

    IdSetPageOperation(std::allocator_arg_t,
                       Allocator a,
                       const EntityDataService<E>& service,
                       const SelectQuery& query)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query),
          key_(a.resource()), retained_(std::allocator_arg, a) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (phase_ == Phase::Prepare) {
            if (!query_.id_set_pagination) {
                return fallback("ID_SET_DISABLED");
            }
            if (!query_.slice || !query_.slice->limit || query_.continuous_page_fetch ||
                query_.partition_field || !query_.aggregates.empty() ||
                !query_.group_by_fields.empty() || query_.having_predicate || query_.raw_sql_text ||
                std::any_of(query_.order_by.begin(), query_.order_by.end(), [](const auto& order) {
                    return bool(order.expr);
                })) {
                return fallback("ID_SET_FALLBACK_UNSUPPORTED_SHAPE");
            }
            if (std::none_of(query_.order_by.begin(), query_.order_by.end(), [](const auto& order) {
                    return order.field == "id" && !order.expr;
                })) {
                query_.order_asc("id");
            }
            std::optional<PaginationKeyRoot> root;
            if (auto* value = ContextPaginationAccess::active_root(context())) {
                root = PaginationKeyRoot{value->entity_type, value->id};
            }
            key_ = id_set_query_key(r,
                                    query_,
                                    query_.id_set_pagination->namespace_name,
                                    context().user_identifier(),
                                    root);
            auto& store = ContextPaginationAccess::id_set_store(context());
            store_.emplace(
                std::allocator_arg, Allocator(r), store, resource_arena(store.memory_resource()));
            start_read();
            phase_ = Phase::Read;
        }
        for (;;) {
            if (phase_ == Phase::Read || phase_ == Phase::Recheck) {
                forwarding_.attach(read_->wake_signal(), wake);
                auto* result = read_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                read_ = nullptr;
                if (!*result) {
                    return fallback("ID_SET_FALLBACK_STORE_UNAVAILABLE");
                }
                if (result->value()) {
                    retained_ = clone_in(r, *result->value());
                    observe("ID_SET_HIT", retained_.ids.size());
                    return page();
                }
                if (phase_ == Phase::Read) {
                    lock_ = &ContextPaginationAccess::build_lock(context(), key_);
                    lock_work_ = &lock_->lock();
                    phase_ = Phase::Lock;
                } else {
                    auto ids = clone_in(r, query_);
                    ids.projection.clear();
                    ids.project("id");
                    ids.expr_projection.clear();
                    ids.relations.clear();
                    ids.child_enhancements.clear();
                    ids.dynamic_properties.clear();
                    ids.raw_projections.clear();
                    ids.object_group_bys.clear();
                    ids.id_set_pagination.reset();
                    ids.continuous_page_fetch.reset();
                    const auto max_ids = query_.id_set_pagination->max_ids;
                    const auto limit = max_ids == UINT64_MAX ? max_ids : max_ids + 1;
                    ids.hard_limit_value = limit;
                    ids.slice = Slice{.limit = limit, .offset = 0};
                    rows_ = &make_operation<RelationRowsResult<E>, PreparedEntityQueryOperation<E>>(
                        r, service_, ids, true);
                    phase_ = Phase::Build;
                }
            }
            if (phase_ == Phase::Lock) {
                forwarding_.attach(lock_work_->wake_signal(), wake);
                auto* result = lock_work_->poll();
                if (!result) {
                    return {};
                }
                guard_ = &result->get();
                forwarding_.reset();
                lock_work_ = nullptr;
                start_read();
                phase_ = Phase::Recheck;
                continue;
            }
            if (phase_ == Phase::Build) {
                forwarding_.attach(rows_->wake_signal(), wake);
                auto* result = rows_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                auto* completed = std::exchange(rows_, nullptr);
                if (!*result) {
                    return R::err(r, copy_or_take_error(*completed, *result));
                }
                retained_.query_key = key_;
                retained_.ids.reserve(result->value().size());
                for (const auto& row : result->value()) {
                    auto* value = row.get("id");
                    auto id = value ? value->try_u64() : std::nullopt;
                    if (!id) {
                        return fallback("ID_SET_FALLBACK_UNSUPPORTED_SHAPE");
                    }
                    retained_.ids.push_back(*id);
                }
                if (retained_.ids.size() > query_.id_set_pagination->max_ids) {
                    auto max_ids = query_.id_set_pagination->max_ids;
                    return fallback("ID_SET_FALLBACK_LIMIT_EXCEEDED",
                                    max_ids == UINT64_MAX ? max_ids : max_ids + 1);
                }
                auto expiry =
                    SystemTime::now().checked_add_seconds(query_.id_set_pagination->ttl_seconds);
                if (!expiry) {
                    throw std::overflow_error("Retained ID set expiry overflow");
                }
                retained_.expires_at = *expiry;
                write_ = &store_->get().put(r, retained_);
                require_domain(*write_);
                phase_ = Phase::Store;
            }
            if (phase_ == Phase::Store) {
                forwarding_.attach(write_->wake_signal(), wake);
                auto* result = write_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                write_ = nullptr;
                if (!*result) {
                    return fallback("ID_SET_FALLBACK_STORE_UNAVAILABLE");
                }
                observe("ID_SET_BUILD", retained_.ids.size());
                return page();
            }
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        read_ = nullptr;
        write_ = nullptr;
        rows_ = nullptr;
        lock_work_ = nullptr;
        release_lock();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (read_) {
            read_->cancel();
        }
        if (write_) {
            write_->cancel();
        }
        if (rows_) {
            rows_->cancel();
        }
        if (lock_work_) {
            lock_work_->cancel();
        }
        finish_and_wait();
    }

private:
    enum class Phase { Prepare, Read, Lock, Recheck, Build, Store };

    const UserContext& context() const {
        return EntityDataServiceAccess::context(service_);
    }

    void observe(std::string_view plan, std::optional<std::uint64_t> count = {}) {
        ContextPaginationAccess::observe_id_set(context(), plan, count);
    }

    R fallback(std::string_view plan, std::optional<std::uint64_t> count = {}) {
        observe(plan, count);
        return R::ok(
            service_.memory_resource(),
            IdSetPage(
                std::allocator_arg, Allocator(service_.memory_resource()), query_, std::nullopt));
    }

    template <class T>
    void require_domain(AsyncOperation<T>& work) const {
        if (work.memory_resource() != service_.memory_resource()) {
            work.cancel();
            throw std::invalid_argument("ID set work must belong to its UserContext");
        }
    }

    void start_read() {
        read_ = &store_->get().get(service_.memory_resource(), key_);
        require_domain(*read_);
    }

    R page() {
        auto r = service_.memory_resource();
        const auto max = std::numeric_limits<std::size_t>::max();
        const auto start = std::size_t(std::min<std::uint64_t>(query_.slice->offset, max));
        const auto size = std::size_t(std::min<std::uint64_t>(*query_.slice->limit, max));
        const auto end = std::min(start > max - size ? max : start + size, retained_.ids.size());
        Vector<Value> ids(r);
        if (start < end) {
            ids.reserve(end - start);
            for (auto i = start; i != end; ++i) {
                ids.emplace_back(retained_.ids[i]);
            }
        }
        query_.slice = Slice{.limit = std::uint64_t(size), .offset = 0};
        query_.id_set_pagination.reset();
        query_.and_filter(Expr::in_list(r, "id", ids));
        return R::ok(r, IdSetPage(std::allocator_arg, Allocator(r), query_, retained_.ids.size()));
    }

    void release_lock() noexcept {
        if (guard_) {
            guard_->release();
            guard_ = nullptr;
        }
        if (lock_) {
            lock_->reset();
            lock_ = nullptr;
        }
    }

    EntityDataService<E> service_;
    SelectQuery query_;
    String key_;
    RetainedIdSet retained_;
    std::optional<ResourceBorrow<IdSetStore>> store_;
    AsyncOperation<Read>* read_ = nullptr;
    AsyncOperation<PaginationWriteResult>* write_ = nullptr;
    AsyncOperation<RelationRowsResult<E>>* rows_ = nullptr;
    IdSetBuildLock* lock_ = nullptr;
    AsyncOperation<ResourceBorrow<IdSetBuildGuard>>* lock_work_ = nullptr;
    IdSetBuildGuard* guard_ = nullptr;
    WakeSignal::Forward forwarding_;
    Phase phase_ = Phase::Prepare;
};

template <class E>
auto& prepare_id_set_page(const EntityDataService<E>& service, const SelectQuery& query) {
    using R = Result<IdSetPage, DataServiceError<typename E::Error>>;
    return make_operation<R, IdSetPageOperation<E>>(service.memory_resource(), service, query);
}
}
