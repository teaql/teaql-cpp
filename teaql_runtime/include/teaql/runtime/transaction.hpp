#pragma once
#include <teaql/runtime/data_service.hpp>
#include <teaql/memory/once_function.hpp>

namespace teaql {
namespace detail {
struct TransactionScopeAccess;
template <class E, class T>
class ScopeExecution;
template <class E, class Request, class T>
class ScopeRequest;
template <class E, LedgerEntity T>
class ScopeAuditedSave;
}

// A Context-owned borrow of one real provider transaction. Completion consumes
// access; retaining this wrapper never extends the native transaction lifetime.
template <class E>
class [[nodiscard]] TransactionScope {
public:
    using allocator_type = Allocator;
    using Error = typename E::Error;
    using Tx = DataServiceTransaction<Error>;
    static_assert(std::derived_from<E, TransactionExecutor<Error>>);

    TransactionScope(std::allocator_arg_t, Allocator a, const UserContext& context, Tx& transaction)
        : context_(std::allocator_arg, a, context, detail::resource_arena(a.resource())),
          transaction_(std::allocator_arg, a, transaction, detail::resource_arena(a.resource())),
          flushed_ledgers_(a.resource()) {
        registration_.attach(detail::async_arena(a.resource()), this, [](void* scope) noexcept {
            static_cast<TransactionScope*>(scope)->abandon();
        });
    }

    TransactionScope(const TransactionScope&) = delete;
    TransactionScope& operator=(const TransactionScope&) = delete;

    ~TransactionScope() {
        abandon();
    }

    const UserContext& context() const {
        return context_.get();
    }

    Result<ResourceBorrow<EntityDataService<Tx>>, ContextError>
    entity_data_service(std::string_view entity) const {
        auto& user = context();
        auto r = user.memory_resource();
        using R = Result<ResourceBorrow<EntityDataService<Tx>>, ContextError>;
        if (!user.has_entity_data_service(entity)) {
            return R::err(r,
                          ContextError(std::allocator_arg,
                                       Allocator(r),
                                       ContextError::Kind::MissingEntityDataService,
                                       entity));
        }
        (void)transaction();
        auto& service = user.template make<EntityDataService<Tx>>(user, entity, transaction_);
        return R::ok(r,
                     ResourceBorrow<EntityDataService<Tx>>(
                         std::allocator_arg, Allocator(r), service, detail::resource_arena(r)));
    }

    AsyncOperation<Result<QueryResult, RuntimeError>>& query(QueryRequest request) const {
        return make_operation<Result<QueryResult, RuntimeError>,
                              detail::ScopeRequest<E, QueryRequest, QueryResult>>(
            context().memory_resource(), *this, std::move(request));
    }

    AsyncOperation<Result<MutationResult, RuntimeError>>& mutate(MutationRequest request) const {
        return make_operation<Result<MutationResult, RuntimeError>,
                              detail::ScopeRequest<E, MutationRequest, MutationResult>>(
            context().memory_resource(), *this, std::move(request));
    }

    template <LedgerEntity T>
    AsyncOperation<Result<T, RuntimeError>>& save_audited(Audited<T> audited) const {
        return make_operation<Result<T, RuntimeError>, detail::ScopeAuditedSave<E, T>>(
            context().memory_resource(), *this, std::move(audited));
    }

    template <class T, class F>
    AsyncOperation<Result<T, RuntimeError>>& execute(F&& operation) {
        (void)transaction();
        if (execution_started_) {
            throw std::logic_error("transaction scope cannot be completed twice");
        }
        execution_started_ = true;
        try {
            return make_operation<Result<T, RuntimeError>, detail::ScopeExecution<E, T>>(
                context().memory_resource(), *this, std::forward<F>(operation));
        } catch (...) {
            abandon();
            throw;
        }
    }

private:
    friend struct detail::TransactionScopeAccess;

    Tx& transaction() const {
        auto& transaction = transaction_.get();
        if (completed_ || transaction.consumed()) {
            throw std::logic_error("transaction scope cannot be used after completion");
        }
        return transaction;
    }

    void remember(const EntityRuntimeState& ledger) const {
        (void)transaction();
        for (const auto& pending : flushed_ledgers_) {
            if (pending == ledger) {
                return;
            }
        }
        flushed_ledgers_.emplace_back(ledger);
    }

    void committed() {
        for (auto& ledger : flushed_ledgers_) {
            ledger.clear_committed();
        }
        flushed_ledgers_.clear();
    }

    void abandon() noexcept {
        if (!completed_) {
            completed_ = true;
            registration_.reset();
            // Trusted shutdown uses the still-live native borrow after public
            // User access was invalidated, and before arena reclamation.
            transaction_.get().cancel();
        }
    }

    ResourceBorrow<const UserContext> context_;
    ResourceBorrow<Tx> transaction_;
    mutable Vector<EntityRuntimeState> flushed_ledgers_;
    Arena::Quiescence registration_;
    bool execution_started_ = false;
    bool completed_ = false;
};

namespace detail {
struct TransactionScopeAccess {
    template <class E>
    static auto& transaction(const TransactionScope<E>& scope) {
        return scope.transaction();
    }

    template <class E>
    static const auto& borrow(const TransactionScope<E>& scope) {
        (void)scope.transaction();
        return scope.transaction_;
    }

    template <class E>
    static void remember(const TransactionScope<E>& scope, const EntityRuntimeState& ledger) {
        scope.remember(ledger);
    }

    template <class E>
    static void committed(TransactionScope<E>& scope) {
        scope.committed();
    }

    template <class E>
    static void abandon(TransactionScope<E>& scope) noexcept {
        scope.abandon();
    }

    template <class E>
    static std::size_t pending_ledgers(const TransactionScope<E>& scope) {
        return scope.flushed_ledgers_.size();
    }
};

template <class Error>
RuntimeError transaction_error(Memory r, Error&& error) {
    using NativeError = std::remove_cvref_t<Error>;
    return RuntimeError::transaction(
        r, DataServiceError<NativeError>::executor(r, std::forward<Error>(error)).to_string(r));
}

template <class E, class Request, class T>
class ScopeRequest {
    using Error = typename E::Error;
    using R = Result<T, RuntimeError>;

public:
    using allocator_type = Allocator;

    ScopeRequest(std::allocator_arg_t,
                 Allocator a,
                 const TransactionScope<E>& scope,
                 Request request)
        : scope_(std::allocator_arg, a, scope, resource_arena(a.resource())),
          request_(std::allocator_arg, a, std::move(request)) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto& scope = scope_.get();
        auto& context = scope.context();
        auto r = context.memory_resource();
        if (!child_) {
            auto& transaction = TransactionScopeAccess::transaction(scope);
            UserContextMetadata metadata(std::allocator_arg, Allocator(r), context);
            if constexpr (std::same_as<Request, QueryRequest>) {
                child_ = &transaction.query_observed(
                    r, std::move(request_), metadata.query_diagnostic_observer(r));
            } else {
                child_ = &transaction.mutate_observed(
                    r, std::move(request_), metadata.mutation_diagnostic_observer(r));
            }
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(r, transaction_error(r, copy_or_take_error(*child_, *result)));
        }
        context.record_metadata_log(result->value().metadata);
        return R::ok(r, clone_in(r, result->value()));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
        }
    }

private:
    ResourceBorrow<const TransactionScope<E>> scope_;
    Request request_;
    AsyncOperation<Result<T, Error>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E, LedgerEntity T>
class ScopeAuditedSave {
    using R = Result<T, RuntimeError>;

public:
    using allocator_type = Allocator;

    ScopeAuditedSave(std::allocator_arg_t,
                     Allocator a,
                     const TransactionScope<E>& scope,
                     Audited<T> audited)
        : scope_(std::allocator_arg, a, scope, resource_arena(a.resource())),
          audited_(std::allocator_arg, a, std::move(audited)) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto& scope = scope_.get();
        auto& context = scope.context();
        auto r = context.memory_resource();
        if (!child_) {
            child_ = &save_audited_ledger_entity_with_executor(
                std::move(audited_), context, TransactionScopeAccess::borrow(scope));
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(r, clone_in(r, result->error()));
        }
        if (result->value().ledger) {
            TransactionScopeAccess::remember(scope, *result->value().ledger);
        }
        if constexpr (domain_copyable<T>) {
            return R::ok(r, clone_in(r, result->value().entity));
        } else {
            return R::ok(r, std::move(child_->take_result()).value().entity);
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
        }
    }

private:
    ResourceBorrow<const TransactionScope<E>> scope_;
    Audited<T> audited_;
    AsyncOperation<Result<AuditedLedgerResult<T>, RuntimeError>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E, class T>
class ScopeExecution {
    using R = Result<T, RuntimeError>;
    using Error = typename E::Error;
    using Callback = ContextOnceFunction<AsyncOperation<R>&(const TransactionScope<E>&)>;

public:
    using allocator_type = Allocator;

    template <class F>
    ScopeExecution(std::allocator_arg_t, Allocator a, TransactionScope<E>& scope, F&& callback)
        : scope_(std::allocator_arg, a, scope, resource_arena(a.resource())),
          callback_(std::allocator_arg, a, std::forward<F>(callback)) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto& scope = scope_.get();
        auto r = scope.context().memory_resource();
        if (!terminal_) {
            if (!child_) {
                auto& child = callback_(scope);
                if (child.memory_resource() != r) {
                    child.cancel();
                    throw std::invalid_argument(
                        "Transaction callback result must belong to its UserContext");
                }
                child_ = &child;
            }
            forwarding_.attach(child_->wake_signal(), wake);
            auto* result = child_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            auto& transaction = TransactionScopeAccess::transaction(scope);
            if (*result) {
                if constexpr (domain_copyable<T>) {
                    saved_.emplace(clone_in(r, result->value()));
                } else {
                    saved_.emplace(std::move(child_->take_result()).value());
                }
                terminal_ = &transaction.commit(r);
            } else {
                error_ = &result->error();
                terminal_ = &transaction.rollback(r);
            }
        }
        forwarding_.attach(terminal_->wake_signal(), wake);
        auto* result = terminal_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            if (error_) {
                String message("operation failed (", r);
                message += error_->to_string(r);
                message += "); rollback also failed (";
                message +=
                    DataServiceError<Error>::executor(r, copy_or_take_error(*terminal_, *result))
                        .to_string(r);
                message += ')';
                return R::err(r, RuntimeError::transaction(r, message));
            }
            return R::err(r, transaction_error(r, copy_or_take_error(*terminal_, *result)));
        }
        if (error_) {
            return R::err(r, clone_in(r, *error_));
        }
        TransactionScopeAccess::committed(scope);
        return R::ok(r, std::move(*saved_));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        TransactionScopeAccess::abandon(scope_.get());
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
        }
        if (terminal_) {
            terminal_->cancel();
        }
        TransactionScopeAccess::abandon(scope_.get());
    }

private:
    ResourceBorrow<TransactionScope<E>> scope_;
    Callback callback_;
    AsyncOperation<R>* child_ = nullptr;
    AsyncOperation<Result<std::monostate, Error>>* terminal_ = nullptr;
    std::optional<T> saved_;
    const RuntimeError* error_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E, class T>
class ExecuteInTransaction {
    using Error = typename E::Error;
    using R = Result<T, RuntimeError>;
    using Callback = ContextOnceFunction<AsyncOperation<R>&(const TransactionScope<E>&)>;

public:
    using allocator_type = Allocator;

    template <class F>
    ExecuteInTransaction(std::allocator_arg_t,
                         Allocator a,
                         const UserContext& context,
                         F&& callback)
        : context_(std::allocator_arg, a, context, resource_arena(a.resource())),
          callback_(std::allocator_arg, a, std::forward<F>(callback)) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto& context = context_.get();
        auto r = context.memory_resource();
        if (!child_) {
            if (!begin_) {
                auto executor = ContextEntityAccess::require_executor<E>(context);
                if (!executor) {
                    return R::err(r, RuntimeError::transaction(r, executor.error().to_string(r)));
                }
                begin_ = &executor.value().get().begin(r);
            }
            forwarding_.attach(begin_->wake_signal(), wake);
            auto* result = begin_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            if (!*result) {
                return R::err(r, transaction_error(r, copy_or_take_error(*begin_, *result)));
            }
            acquired_ = &result->value().get();
            scope_ = &context.template make<TransactionScope<E>>(context, *acquired_);
            acquired_ = nullptr;
            child_ = &scope_->template execute<T>(callback_);
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        return copy_or_take_completed(*child_, *result);
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
        }
        if (begin_) {
            begin_->cancel();
        }
        if (scope_) {
            TransactionScopeAccess::abandon(*scope_);
        }
        if (acquired_) {
            acquired_->cancel();
            acquired_ = nullptr;
        }
    }

private:
    ResourceBorrow<const UserContext> context_;
    Callback callback_;
    AsyncOperation<Result<TransactionHandle<Error>, Error>>* begin_ = nullptr;
    TransactionScope<E>* scope_ = nullptr;
    DataServiceTransaction<Error>* acquired_ = nullptr;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};
}

template <class E, class T, class F>
AsyncOperation<Result<T, RuntimeError>>& UserContext::execute_in_transaction(F&& operation) const {
    return make_operation<Result<T, RuntimeError>, detail::ExecuteInTransaction<E, T>>(
        memory_resource(), *this, std::forward<F>(operation));
}
}
