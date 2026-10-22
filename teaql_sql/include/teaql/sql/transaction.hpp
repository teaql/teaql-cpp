#pragma once
#include <teaql/sql/detail/query_execution.hpp>
#include <teaql/sql/detail/stream_execution.hpp>
#include <teaql/sql/detail/mutation_execution.hpp>

namespace teaql {
namespace detail {
// The SQL terminal operation has already consumed the provider transaction.
// Mapping keeps both operation/result in the same explicit Context.
template <class E>
class SqlTransactionTerminal {
public:
    using allocator_type = Allocator;
    using Output = Result<std::monostate, SqlExecutorError<E>>;

    SqlTransactionTerminal(std::allocator_arg_t,
                           Allocator a,
                           DataServiceOperationResult<std::monostate, E>& pending)
        : resource_(a.resource()), pending_(&pending) {
        if (pending.memory_resource() != resource_) {
            pending.cancel();
            throw std::invalid_argument("SQL terminal operation belongs to a different Context");
        }
    }

    std::optional<Output> poll(WakeSignal& wake) {
        forwarding_.attach(pending_->wake_signal(), wake);
        auto* result = pending_->poll();
        if (!result) {
            return {};
        }
        forwarding_.reset();
        auto* completed = std::exchange(pending_, nullptr);
        if (!*result) {
            return Output::err(
                resource_,
                SqlExecutorError<E>::transport(resource_, copy_or_take_error(*completed, *result)));
        }
        return Output::ok(resource_, std::monostate{});
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        if (pending_) {
            pending_->cancel();
        }
        forwarding_.reset();
        pending_ = nullptr;
    }

private:
    Memory resource_;
    DataServiceOperationResult<std::monostate, E>* pending_;
    WakeSignal::Forward forwarding_;
};
}

// The Context owns the adapter and native transaction. Cache/dialect/schema are
// explicit borrows, usually from GlobalContext; they must outlive pending work.
template <class E>
class SqlDataServiceTransaction : public DataServiceTransaction<SqlExecutorError<E>>,
                                  public GuardedMutationExecutor<SqlExecutorError<E>>,
                                  public StreamQueryExecutor<SqlExecutorError<E>> {
public:
    using allocator_type = Allocator;
    using Error = SqlExecutorError<E>;
    const SqlDialect& dialect;
    SqlTransportTransaction<E>& transport;
    const SchemaProvider& schema_provider;
    detail::SqlExecutorState& state;

    SqlDataServiceTransaction(std::allocator_arg_t,
                              Allocator a,
                              const SqlDialect& dialect,
                              SqlTransportTransaction<E>& transport,
                              const SchemaProvider& schema,
                              detail::SqlExecutorState& state)
        : DataServiceTransaction<Error>(a.resource()), dialect(dialect), transport(transport),
          schema_provider(schema), state(state) {
        if (a.resource() != transport.memory_resource()) {
            throw std::invalid_argument("SQL transaction and adapter must share a Context");
        }
    }

    DataServiceCapabilities capabilities() const override {
        DataServiceCapabilities c;
        c.query = true;
        c.mutation = true;
        c.batch_mutation = true;
        c.small_parent_relation_probes = dialect.prefers_small_parent_relation_probes();
        return c;
    }

    const DynamicFieldStore* dynamic_field_store() const override {
        return transport.dynamic_field_store();
    }

    SqlIntentRedactions query_log_intent(Memory r, const SelectQuery& query) const override {
        this->require_active();
        // Pure diagnostic values can be explicitly copied to another domain.
        // Only native transaction operations must stay in the owning Context.
        if (auto* entity = state.entity_descriptor(schema_provider, query.entity)) {
            try {
                auto q = state.compile_select_cached(r, dialect, *entity, query);
                return SqlIntentRedactions::from_bindings(r, q.log_context, q.params, q.sql);
            } catch (const SqlCompileError&) {
            }
        }
        return SqlIntentRedactions::from_unclassified_query(r, query);
    }

    DataServiceOperationResult<MutationResult, Error>&
    mutate_guarded(Memory r, GuardedMutationRequest request) override {
        return mutate_guarded_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<MutationResult, Error>&
    mutate_guarded_observed(Memory r,
                            GuardedMutationRequest request,
                            std::optional<ExecutionObserver> observer) override {
        this->require_domain(r);
        this->require_active();
        return this->track(detail::execute_mutation(r,
                                                    dialect,
                                                    static_cast<SqlTransport<E>&>(transport),
                                                    schema_provider,
                                                    state,
                                                    std::move(request.mutation),
                                                    true,
                                                    std::move(observer),
                                                    std::optional<Expr>(std::move(request.guard))));
    }

    QueryStream<Error>& query_stream(Memory r, QueryRequest request, std::size_t size) override {
        this->require_domain(r);
        this->require_active();
        return this->track(detail::buffered_transaction_stream<E>(
            r, dialect, transport, schema_provider, state, std::move(request), size, {}));
    }

    QueryStream<Error>& query_stream_observed(Memory r,
                                              QueryRequest request,
                                              std::size_t size,
                                              ExecutionObserver observer) override {
        this->require_domain(r);
        this->require_active();
        return this->track(detail::buffered_transaction_stream<E>(r,
                                                                  dialect,
                                                                  transport,
                                                                  schema_provider,
                                                                  state,
                                                                  std::move(request),
                                                                  size,
                                                                  std::move(observer)));
    }

protected:
    TransactionResourceScope do_bind_transaction_resources(Memory r) override {
        return transport.bind_transaction_resources(r);
    }

    void cancel_transaction() noexcept override {
        transport.cancel_sql();
    }

    DataServiceOperationResult<QueryResult, Error>&
    query_transaction(Memory r, QueryRequest request) override {
        return query_transaction_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<QueryResult, Error>& query_transaction_observed(
        Memory r, QueryRequest request, std::optional<ExecutionObserver> observer) override {
        using Operation = detail::SqlQueryOperation<E>;
        return make_operation<typename Operation::Output, Operation>(r,
                                                                     dialect,
                                                                     transport,
                                                                     schema_provider,
                                                                     state,
                                                                     std::move(request),
                                                                     std::move(observer),
                                                                     false);
    }

    DataServiceOperationResult<MutationResult, Error>&
    mutate_transaction(Memory r, MutationRequest request) override {
        return mutate_transaction_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<MutationResult, Error>& mutate_transaction_observed(
        Memory r, MutationRequest request, std::optional<ExecutionObserver> observer) override {
        return detail::execute_mutation(r,
                                        dialect,
                                        static_cast<SqlTransport<E>&>(transport),
                                        schema_provider,
                                        state,
                                        std::move(request),
                                        true,
                                        std::move(observer));
    }

    DataServiceOperationResult<std::monostate, Error>& commit_transaction(Memory r) override {
        return terminal(r, transport.commit_sql(r));
    }

    DataServiceOperationResult<std::monostate, Error>& rollback_transaction(Memory r) override {
        return terminal(r, transport.rollback_sql(r));
    }

private:
    DataServiceOperationResult<std::monostate, Error>&
    terminal(Memory r, DataServiceOperationResult<std::monostate, E>& operation) {
        using State = detail::SqlTransactionTerminal<E>;
        try {
            return make_operation<typename State::Output, State>(r, operation);
        } catch (...) {
            operation.cancel();
            throw;
        }
    }
};

namespace detail {
template <class E>
class SqlBeginOperation {
public:
    using allocator_type = Allocator;
    using Error = SqlExecutorError<E>;
    using Output = Result<TransactionHandle<Error>, Error>;

    SqlBeginOperation(std::allocator_arg_t,
                      Allocator a,
                      const SqlDialect& dialect,
                      SqlTransactionTransport<E>& transport,
                      const SchemaProvider& schema,
                      SqlExecutorState& cache)
        : resource_(a.resource()), dialect_(dialect), transport_(transport), schema_(schema),
          cache_(cache) {}

    std::optional<Output> poll(WakeSignal& wake) {
        if (!pending_) {
            pending_ = &transport_.begin_sql(resource_);
            if (pending_->memory_resource() != resource_) {
                throw std::invalid_argument("SQL begin operation belongs to a different Context");
            }
        }
        forwarding_.attach(pending_->wake_signal(), wake);
        auto* result = pending_->poll();
        if (!result) {
            return {};
        }
        forwarding_.reset();
        auto* completed = std::exchange(pending_, nullptr);
        if (!*result) {
            return Output::err(
                resource_, Error::transport(resource_, copy_or_take_error(*completed, *result)));
        }
        auto& native = result->value().get();
        try {
            auto& transaction = async_arena(resource_).template make<SqlDataServiceTransaction<E>>(
                dialect_, native, schema_, cache_);
            return Output::ok(
                resource_,
                TransactionHandle<Error>(std::allocator_arg, Allocator(resource_), transaction));
        } catch (...) {
            native.cancel_sql();
            throw;
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        if (pending_) {
            pending_->cancel();
        }
        forwarding_.reset();
        pending_ = nullptr;
    }

private:
    Memory resource_;
    const SqlDialect& dialect_;
    SqlTransactionTransport<E>& transport_;
    const SchemaProvider& schema_;
    SqlExecutorState& cache_;
    DataServiceOperationResult<SqlTransactionHandle<E>, E>* pending_ = nullptr;
    WakeSignal::Forward forwarding_;
};

// Rust exposes TransactionExecutor only when T: SqlTransactionTransport. Use
// the corresponding C++ type bound, without exposing an unsupported begin SPI.
template <class E, class T, bool Enabled = std::derived_from<T, SqlTransactionTransport<E>>>
class SqlTransactionExecutorSupport {
public:
    SqlTransactionExecutorSupport(const SqlDialect&, T&, const SchemaProvider&, SqlExecutorState&) {
    }
};

template <class E, class T>
class SqlTransactionExecutorSupport<E, T, true> : public TransactionExecutor<SqlExecutorError<E>> {
public:
    SqlTransactionExecutorSupport(const SqlDialect& dialect,
                                  T& transport,
                                  const SchemaProvider& schema,
                                  SqlExecutorState& cache)
        : dialect_(dialect), transport_(transport), schema_(schema), cache_(cache) {}

    DataServiceOperationResult<TransactionHandle<SqlExecutorError<E>>, SqlExecutorError<E>>&
    begin(Memory r) override {
        using State = SqlBeginOperation<E>;
        return make_operation<typename State::Output, State>(
            r, dialect_, transport_, schema_, cache_);
    }

private:
    const SqlDialect& dialect_;
    T& transport_;
    const SchemaProvider& schema_;
    SqlExecutorState& cache_;
};
}
}
