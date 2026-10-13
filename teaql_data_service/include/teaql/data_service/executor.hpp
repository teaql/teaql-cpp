#pragma once
#include <teaql/data_service/types.hpp>
#include <teaql/data_service/dynamic_fields.hpp>
#include <teaql/async/operation.hpp>
#include <teaql/async/pending.hpp>
#include <teaql/async/stream.hpp>
#include <teaql/core/result.hpp>

namespace teaql {
// Runtime-only callback. Invoke on the Context owner thread after native work
// has returned; never from a worker touching the thread-confined arena.
using ExecutionObserver = ContextFunction<void(ExecutionMetadata)>;
template <class E>
using QueryStream = AsyncStream<Result<StreamChunk, E>>;
template <class T, class E>
using DataServiceOperationResult = AsyncOperation<Result<T, E>>;

template <class E>
class DataServiceExecutor {
public:
    using Error = E;
    virtual ~DataServiceExecutor() = default;
    virtual DataServiceCapabilities capabilities() const = 0;
};

template <class E>
class QueryExecutor : public virtual DataServiceExecutor<E> {
public:
    virtual const DynamicFieldStore* dynamic_field_store() const {
        return nullptr;
    }

    // Pure diagnostic fallback. No I/O and no SQL log emission.
    virtual SqlIntentRedactions query_log_intent(Memory r, const SelectQuery& query) const {
        return SqlIntentRedactions::from_unclassified_query(r, query);
    }

    virtual DataServiceOperationResult<QueryResult, E>& query(Memory, QueryRequest) = 0;

    virtual DataServiceOperationResult<QueryResult, E>&
    query_observed(Memory r, QueryRequest request, std::optional<ExecutionObserver>) {
        return query(r, std::move(request));
    }
};

template <class E>
class StreamQueryExecutor : public virtual DataServiceExecutor<E> {
public:
    virtual Result<QueryStream<E>*, DynamicFieldError>
    query_stream_with_dynamic_fields(Memory r,
                                     QueryRequest,
                                     std::size_t,
                                     DynamicFieldStreamPlan,
                                     std::optional<ExecutionObserver>) {
        return Result<QueryStream<E>*, DynamicFieldError>::err(
            r, DynamicFieldError(r, "DYNAMIC_FIELD_STREAM_UNSUPPORTED", "stream"));
    }

    virtual QueryStream<E>& query_stream(Memory, QueryRequest, std::size_t chunk_size) = 0;

    virtual QueryStream<E>& query_stream_observed(Memory r,
                                                  QueryRequest request,
                                                  std::size_t chunk_size,
                                                  ExecutionObserver) {
        return query_stream(r, std::move(request), chunk_size);
    }
};

template <class E>
class MutationExecutor : public virtual DataServiceExecutor<E> {
public:
    virtual DataServiceOperationResult<MutationResult, E>& mutate(Memory, MutationRequest) = 0;

    virtual DataServiceOperationResult<MutationResult, E>&
    mutate_observed(Memory r, MutationRequest request, std::optional<ExecutionObserver>) {
        return mutate(r, std::move(request));
    }
};

template <class E>
class GuardedMutationExecutor : public virtual MutationExecutor<E> {
public:
    // Guard is enforced in the same datastore statement as the mutation.
    virtual DataServiceOperationResult<MutationResult, E>&
        mutate_guarded(Memory, GuardedMutationRequest) = 0;

    virtual DataServiceOperationResult<MutationResult, E>& mutate_guarded_observed(
        Memory r, GuardedMutationRequest request, std::optional<ExecutionObserver>) {
        return mutate_guarded(r, std::move(request));
    }
};

template <class E>
class Transaction {
public:
    explicit Transaction(Memory r) : resource_(r), pending_(r) {}

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    virtual ~Transaction() = default;

    // Rust commit/rollback consume self. A C++ Context borrow remains in place,
    // but terminal invocation consumes access even if the returned future fails.
    DataServiceOperationResult<std::monostate, E>& commit(Memory r) {
        require_domain(r);
        consume();
        return pending_.track(commit_transaction(r));
    }

    DataServiceOperationResult<std::monostate, E>& rollback(Memory r) {
        require_domain(r);
        consume();
        return pending_.track(rollback_transaction(r));
    }

    bool consumed() const noexcept {
        return consumed_;
    }

    // Explicit equivalent of dropping an owned Rust transaction. No allocation
    // or new async work is permitted while a Context is quiescing.
    void cancel() noexcept {
        if (auto* owner = dynamic_cast<Arena*>(resource_)) {
            owner->check_thread();
        }
        if (!cancelled_) {
            cancelled_ = true;
            consumed_ = true;
            pending_.cancel_all();
            cancel_transaction();
        }
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

protected:
    template <class T>
    AsyncOperation<T>& track(AsyncOperation<T>& operation) {
        return pending_.track(operation);
    }

    template <class T>
    AsyncStream<T>& track(AsyncStream<T>& stream) {
        return pending_.track(stream);
    }

    void require_active() const {
        if (consumed_) {
            throw std::logic_error("Transaction has been consumed");
        }
    }

    void require_domain(Memory r) const {
        if (r != resource_) {
            throw std::invalid_argument("Transaction work must remain in its owning Context");
        }
    }

    virtual DataServiceOperationResult<std::monostate, E>& commit_transaction(Memory) = 0;
    virtual DataServiceOperationResult<std::monostate, E>& rollback_transaction(Memory) = 0;
    virtual void cancel_transaction() noexcept = 0;

private:
    void consume() {
        require_active();
        if (pending_.any()) {
            throw std::logic_error("Transaction still has pending operations");
        }
        consumed_ = true;
    }

    Memory resource_;
    detail::PendingOperations pending_;
    bool consumed_ = false;
    bool cancelled_ = false;
};

// The associated transaction satisfies query + mutation + terminal transaction
// contracts. Its concrete provider implements the protected execution hooks.
template <class E>
class DataServiceTransaction : public QueryExecutor<E>,
                               public virtual MutationExecutor<E>,
                               public Transaction<E> {
public:
    explicit DataServiceTransaction(Memory r) : Transaction<E>(r) {}

    DataServiceOperationResult<QueryResult, E>& query(Memory r, QueryRequest request) final {
        this->require_domain(r);
        this->require_active();
        return this->track(query_transaction(r, std::move(request)));
    }

    DataServiceOperationResult<MutationResult, E>& mutate(Memory r, MutationRequest request) final {
        this->require_domain(r);
        this->require_active();
        return this->track(mutate_transaction(r, std::move(request)));
    }

    DataServiceOperationResult<QueryResult, E>& query_observed(
        Memory r, QueryRequest request, std::optional<ExecutionObserver> observer) final {
        this->require_domain(r);
        this->require_active();
        return this->track(query_transaction_observed(r, std::move(request), std::move(observer)));
    }

    DataServiceOperationResult<MutationResult, E>& mutate_observed(
        Memory r, MutationRequest request, std::optional<ExecutionObserver> observer) final {
        this->require_domain(r);
        this->require_active();
        return this->track(mutate_transaction_observed(r, std::move(request), std::move(observer)));
    }

protected:
    virtual DataServiceOperationResult<QueryResult, E>& query_transaction(Memory, QueryRequest) = 0;
    virtual DataServiceOperationResult<MutationResult, E>& mutate_transaction(Memory,
                                                                              MutationRequest) = 0;

    virtual DataServiceOperationResult<QueryResult, E>&
    query_transaction_observed(Memory r, QueryRequest request, std::optional<ExecutionObserver>) {
        return query_transaction(r, std::move(request));
    }

    virtual DataServiceOperationResult<MutationResult, E>& mutate_transaction_observed(
        Memory r, MutationRequest request, std::optional<ExecutionObserver>) {
        return mutate_transaction(r, std::move(request));
    }
};

template <class E>
class TransactionHandle {
public:
    using allocator_type = Allocator;

    TransactionHandle(std::allocator_arg_t, Allocator a, DataServiceTransaction<E>& transaction)
        : transaction_(&transaction) {
        if (a.resource() != transaction.memory_resource()) {
            throw std::invalid_argument("Transaction borrow must belong to its owning Context");
        }
    }

    TransactionHandle(std::allocator_arg_t, Allocator a, const TransactionHandle& other)
        : TransactionHandle(std::allocator_arg, a, *other.transaction_) {}

    TransactionHandle(std::allocator_arg_t, Allocator a, TransactionHandle&& other)
        : TransactionHandle(std::allocator_arg, a, std::as_const(other)) {}

    TransactionHandle(const TransactionHandle&) = delete;
    TransactionHandle(TransactionHandle&&) noexcept = default;

    DataServiceTransaction<E>& get() const {
        return *transaction_;
    }

    // Rust's associated concrete transaction retains its additional SPIs.
    // Recover one explicitly from this erased Context borrow; never emulate a
    // guard with a pre-read or advertise it on every generic transaction.
    template <class Capability>
        requires std::derived_from<Capability, DataServiceExecutor<E>>
    Capability& get_as() const {
        auto* capability = dynamic_cast<Capability*>(transaction_);
        if (!capability) {
            throw std::logic_error("Transaction does not implement the requested capability");
        }
        return *capability;
    }

    Memory memory_resource() const noexcept {
        return transaction_->memory_resource();
    }

private:
    DataServiceTransaction<E>* transaction_;
};

template <class E>
class TransactionExecutor : public virtual DataServiceExecutor<E> {
public:
    virtual DataServiceOperationResult<TransactionHandle<E>, E>& begin(Memory) = 0;
};

template <class E>
class SchemaExecutor : public virtual DataServiceExecutor<E> {
public:
    virtual DataServiceOperationResult<SchemaResult, E>& ensure_schema(Memory, SchemaRequest) = 0;
};

template <class E>
class IdGeneratorExecutor : public virtual DataServiceExecutor<E> {
public:
    virtual DataServiceOperationResult<std::uint64_t, E>& next_id(Memory,
                                                                  std::string_view entity) = 0;
};

class SchemaProvider {
public:
    virtual ~SchemaProvider() = default;
    // Returned descriptors borrow the provider's explicitly owned Context.
    virtual const EntityDescriptor* get_entity(std::string_view name) const = 0;
};
}
