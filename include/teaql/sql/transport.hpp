#pragma once
#include <teaql/sql/types.hpp>
#include <teaql/data_service/executor.hpp>

namespace teaql {
template <class E>
class SqlTransport {
public:
    using Error = E;
    using Rows = Vector<CompactRow>;
    virtual ~SqlTransport() = default;
    // Transport and borrowed query must outlive their pending operation. The
    // result/operation belongs to the explicitly supplied Context arena.
    virtual DataServiceOperationResult<Rows, E>& fetch_all_compact_sql(Memory,
                                                                       const CompiledQuery&) = 0;
    virtual DataServiceOperationResult<Rows, E>& fetch_repeated_compact_sql(Memory,
                                                                            const CompiledQuery&,
                                                                            std::size_t param_index,
                                                                            std::span<const Value>);
    virtual DataServiceOperationResult<std::uint64_t, E>& execute_sql(Memory,
                                                                      const CompiledQuery&) = 0;
};

template <class E>
class StreamingSqlTransport : public virtual SqlTransport<E> {
public:
    virtual QueryStream<E>& stream_sql(Memory, CompiledQuery, std::size_t chunk_size) = 0;
};

// SQL transactions remain distinct from data-service transactions. The same
// consuming terminal behavior is adapted to an invalidated Context borrow.
template <class E>
class SqlTransaction {
public:
    explicit SqlTransaction(Memory r) : resource_(r), pending_(r) {}

    SqlTransaction(const SqlTransaction&) = delete;
    SqlTransaction& operator=(const SqlTransaction&) = delete;
    virtual ~SqlTransaction() = default;

    DataServiceOperationResult<std::monostate, E>& commit_sql(Memory r) {
        require_domain(r);
        consume();
        return commit_sql_transaction(r);
    }

    DataServiceOperationResult<std::monostate, E>& rollback_sql(Memory r) {
        require_domain(r);
        consume();
        return rollback_sql_transaction(r);
    }

    bool consumed() const noexcept {
        return consumed_;
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

protected:
    template <class T>
    AsyncOperation<T>& track(AsyncOperation<T>& operation) {
        return pending_.track(operation);
    }

    void require_active() const {
        if (consumed_)
            throw std::logic_error("SQL transaction has been consumed");
    }

    void require_domain(Memory r) const {
        if (r != resource_)
            throw std::invalid_argument("SQL transaction work must remain in its owning Context");
    }

    virtual DataServiceOperationResult<std::monostate, E>& commit_sql_transaction(Memory) = 0;
    virtual DataServiceOperationResult<std::monostate, E>& rollback_sql_transaction(Memory) = 0;

private:
    void consume() {
        require_active();
        if (pending_.any())
            throw std::logic_error("SQL transaction still has pending operations");
        consumed_ = true;
    }

    Memory resource_;
    detail::PendingOperations pending_;
    bool consumed_ = false;
};

// C++ carrier for the source associated Tx bound: SqlTransport + SqlTransaction.
// Guard all public SQL entry points so consumed borrows cannot be reused.
template <class E>
class SqlTransportTransaction : public virtual SqlTransport<E>, public SqlTransaction<E> {
public:
    explicit SqlTransportTransaction(Memory r) : SqlTransaction<E>(r) {}

    using Rows = typename SqlTransport<E>::Rows;

    DataServiceOperationResult<Rows, E>& fetch_all_compact_sql(Memory r,
                                                               const CompiledQuery& query) final {
        this->require_domain(r);
        this->require_active();
        return this->track(fetch_all_compact_sql_transaction(r, query));
    }

    DataServiceOperationResult<Rows, E>&
    fetch_repeated_compact_sql(Memory r,
                               const CompiledQuery& query,
                               std::size_t index,
                               std::span<const Value> values) final {
        this->require_domain(r);
        this->require_active();
        return this->track(fetch_repeated_compact_sql_transaction(r, query, index, values));
    }

    DataServiceOperationResult<std::uint64_t, E>& execute_sql(Memory r,
                                                              const CompiledQuery& query) final {
        this->require_domain(r);
        this->require_active();
        return this->track(execute_sql_transaction(r, query));
    }

protected:
    virtual DataServiceOperationResult<Rows, E>&
    fetch_all_compact_sql_transaction(Memory, const CompiledQuery&) = 0;

    virtual DataServiceOperationResult<Rows, E>& fetch_repeated_compact_sql_transaction(
        Memory r, const CompiledQuery& query, std::size_t index, std::span<const Value> values) {
        return SqlTransport<E>::fetch_repeated_compact_sql(r, query, index, values);
    }

    virtual DataServiceOperationResult<std::uint64_t, E>&
    execute_sql_transaction(Memory, const CompiledQuery&) = 0;
};

template <class E>
class SqlTransactionHandle {
public:
    using allocator_type = Allocator;

    SqlTransactionHandle(std::allocator_arg_t, Allocator a, SqlTransportTransaction<E>& transaction)
        : transaction_(&transaction) {
        if (a.resource() != transaction.memory_resource())
            throw std::invalid_argument("SQL transaction borrow must belong to its owning Context");
    }

    SqlTransactionHandle(std::allocator_arg_t, Allocator a, const SqlTransactionHandle& other)
        : SqlTransactionHandle(std::allocator_arg, a, *other.transaction_) {}

    SqlTransactionHandle(std::allocator_arg_t, Allocator a, SqlTransactionHandle&& other)
        : SqlTransactionHandle(std::allocator_arg, a, std::as_const(other)) {}

    SqlTransactionHandle(const SqlTransactionHandle&) = delete;
    SqlTransactionHandle(SqlTransactionHandle&&) noexcept = default;

    SqlTransportTransaction<E>& get() const {
        return *transaction_;
    }

    Memory memory_resource() const noexcept {
        return transaction_->memory_resource();
    }

private:
    SqlTransportTransaction<E>* transaction_;
};

template <class E>
class SqlTransactionTransport : public virtual SqlTransport<E> {
public:
    virtual DataServiceOperationResult<SqlTransactionHandle<E>, E>& begin_sql(Memory) = 0;
};

namespace detail {
template <class E>
class RepeatedSqlFetch {
public:
    using allocator_type = Allocator;
    using Rows = Vector<CompactRow>;
    using Output = Result<Rows, E>;

    RepeatedSqlFetch(std::allocator_arg_t,
                     Allocator a,
                     SqlTransport<E>& transport,
                     const CompiledQuery& query,
                     std::size_t index,
                     std::span<const Value> values)
        : resource_(a.resource()), transport_(transport), template_(std::allocator_arg, a, query),
          index_(index), values_(values.begin(), values.end(), a.resource()), rows_(a.resource()) {}

    std::optional<Output> poll(WakeSignal& wake) {
        while (next_ < values_.size()) {
            if (!pending_) {
                query_.emplace(std::allocator_arg, Allocator(resource_), template_);
                query_->params.at(index_) = Value(resource_, values_[next_]);
                pending_ = &transport_.fetch_all_compact_sql(resource_, *query_);
                if (pending_->memory_resource() != resource_)
                    throw std::invalid_argument(
                        "SQL transport returned an operation in a different Context");
            }
            forwarding_.attach(pending_->wake_signal(), wake);
            auto* result = pending_->poll();
            if (!result)
                return {};
            forwarding_.reset();
            pending_ = nullptr;
            if (!*result)
                return Output::err(resource_, detail::clone_in(resource_, result->error()));
            for (const auto& row : result->value())
                rows_.emplace_back(row);
            query_.reset();
            ++next_;
        }
        return Output::ok(resource_, std::move(rows_));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        query_.reset();
    }

    void cancel_and_wait() noexcept {
        // Keep the query and parent wake alive through native cancellation.
        if (pending_)
            pending_->cancel();
        forwarding_.reset();
        pending_ = nullptr;
        query_.reset();
    }

private:
    Memory resource_;
    SqlTransport<E>& transport_;
    CompiledQuery template_;
    std::size_t index_, next_ = 0;
    Vector<Value> values_;
    Rows rows_;
    std::optional<CompiledQuery> query_;
    DataServiceOperationResult<Rows, E>* pending_ = nullptr;
    WakeSignal::Forward forwarding_;
};
}

template <class E>
DataServiceOperationResult<typename SqlTransport<E>::Rows, E>&
SqlTransport<E>::fetch_repeated_compact_sql(Memory r,
                                            const CompiledQuery& query,
                                            std::size_t index,
                                            std::span<const Value> values) {
    // Copy retained inputs into the explicit destination domain; the underlying
    // transport is a borrow. Iterate lazily, preserve input/row order, stop at
    // the first typed transport error. Empty values do not index the template.
    using State = detail::RepeatedSqlFetch<E>;
    return make_operation<Result<Rows, E>, State>(r, *this, query, index, values);
}
}
