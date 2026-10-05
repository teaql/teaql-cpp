#pragma once
#include <teaql/sql/error.hpp>
#include <teaql/sql/transport.hpp>
#include <teaql/sql/detail/query_plan.hpp>
#include <teaql/sql/detail/diagnostic.hpp>
#include <limits>

namespace teaql::detail {
std::string_view mutation_entity_name(const MutationRequest&);
DataServiceOperation mutation_operation(const MutationRequest&);
const Value* mutation_target_id(const MutationRequest&);
const Value* mutation_persisted_id(const MutationRequest&);
CompiledQuery compile_mutation(Memory,
                               const SqlDialect&,
                               const EntityDescriptor&,
                               const MutationRequest&,
                               const std::optional<Expr>& guard);
ExecutionMetadata mutation_metadata(
    Memory, const SqlDialect&, const CompiledQuery&, const MutationRequest&, DataServiceOperation);

template <class E>
class ObservedSqlTransport;

template <class E, class T>
class ObservedSqlCall {
public:
    using allocator_type = Allocator;
    using Output = Result<T, E>;

    ObservedSqlCall(std::allocator_arg_t,
                    Allocator a,
                    ObservedSqlTransport<E>& owner,
                    const CompiledQuery& query)
        : resource_(a.resource()), owner_(owner), query_(std::allocator_arg, a, query) {}

    std::optional<Output> poll(WakeSignal& wake) {
        try {
            if (!pending_) {
                if (owner_.observer) {
                    if constexpr (std::same_as<T, std::uint64_t>)
                        owner_.intent = SqlIntentRedactions::from_bindings(
                            resource_, query_.log_context, query_.params, query_.sql);
                    diagnostic_.emplace(std::allocator_arg,
                                        Allocator(resource_),
                                        owner_.metadata(query_,
                                                        std::same_as<T, std::uint64_t>
                                                            ? owner_.operation
                                                            : DataServiceOperation::Query),
                                        owner_.observer);
                }
                if constexpr (std::same_as<T, std::uint64_t>)
                    pending_ = &owner_.inner.execute_sql(resource_, query_);
                else
                    pending_ = &owner_.inner.fetch_all_compact_sql(resource_, query_);
                if (pending_->memory_resource() != resource_)
                    throw std::invalid_argument(
                        "SQL transport returned work in a different Context");
            }
            forwarding_.attach(pending_->wake_signal(), wake);
            auto* result = pending_->poll();
            if (!result)
                return {};
            forwarding_.reset();
            pending_ = nullptr;
            if (!*result) {
                if (diagnostic_)
                    diagnostic_->fail();
                return Output::err(resource_, clone_in(resource_, result->error()));
            }
            auto value = clone_in(resource_, result->value());
            if (diagnostic_)
                if (auto metadata = diagnostic_->success()) {
                    if constexpr (std::same_as<T, std::uint64_t>)
                        metadata->affected_rows = value;
                    else
                        metadata->result_count = value.size();
                    (*owner_.observer)(std::move(*metadata));
                }
            return Output::ok(resource_, std::move(value));
        } catch (...) {
            stop();
            if (diagnostic_)
                diagnostic_->fail();
            throw;
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        stop();
        if (diagnostic_)
            diagnostic_->cancel();
    }

private:
    void stop() noexcept {
        if (pending_)
            pending_->cancel();
        forwarding_.reset();
        pending_ = nullptr;
    }

    Memory resource_;
    ObservedSqlTransport<E>& owner_;
    CompiledQuery query_;
    DataServiceOperationResult<T, E>* pending_ = nullptr;
    WakeSignal::Forward forwarding_;
    std::optional<StatementDiagnostic> diagnostic_;
};

// One adapter per leaf mutation. Its provenance and target id never enter an
// ambient Context cache; readback inherits only this mutation's intent.
template <class E>
class ObservedSqlTransport : public SqlTransport<E> {
public:
    using allocator_type = Allocator;
    SqlTransport<E>& inner;
    const SqlDialect& dialect;
    const MutationRequest& request;
    DataServiceOperation operation;
    std::optional<ExecutionObserver> observer;
    SqlIntentRedactions intent;

    ObservedSqlTransport(std::allocator_arg_t,
                         Allocator a,
                         SqlTransport<E>& inner,
                         const SqlDialect& dialect,
                         const MutationRequest& request,
                         std::optional<ExecutionObserver> observer)
        : inner(inner), dialect(dialect), request(request), operation(mutation_operation(request)),
          observer(move_in(a.resource(), std::move(observer))), intent(a.resource()) {}

    ExecutionMetadata metadata(const CompiledQuery& query, DataServiceOperation op) const {
        auto r = intent.memory_resource();
        auto m = mutation_metadata(r, dialect, query, request, op);
        if (op == DataServiceOperation::Query)
            m.sql_log.intent_redactions = clone_in(r, intent);
        if (auto* id = mutation_target_id(request))
            m.sql_log.intent_redactions.capture_target_id(*id);
        return m;
    }

    DataServiceOperationResult<Vector<CompactRow>, E>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& query) override {
        return make_operation<Result<Vector<CompactRow>, E>,
                              ObservedSqlCall<E, Vector<CompactRow>>>(r, *this, query);
    }

    DataServiceOperationResult<std::uint64_t, E>& execute_sql(Memory r,
                                                              const CompiledQuery& query) override {
        return make_operation<Result<std::uint64_t, E>, ObservedSqlCall<E, std::uint64_t>>(
            r, *this, query);
    }
};

template <class E>
class MutationTreeOperation {
public:
    using allocator_type = Allocator;
    using Error = SqlExecutorError<E>;
    using Output = Result<MutationResult, Error>;

    MutationTreeOperation(std::allocator_arg_t,
                          Allocator a,
                          const SqlDialect& dialect,
                          SqlTransport<E>& transport,
                          const SchemaProvider& schema,
                          SqlExecutorState& cache,
                          MutationRequest request,
                          bool readback,
                          std::optional<ExecutionObserver> observer,
                          std::optional<Expr> guard = {})
        : resource_(a.resource()), dialect_(dialect), transport_(transport), schema_(schema),
          cache_(cache), request_(std::allocator_arg, a, std::move(request)), readback_(readback),
          observer_(move_in(a.resource(), std::move(observer))),
          guard_(move_in(a.resource(), std::move(guard))) {}

    std::optional<Output> poll(WakeSignal& wake) {
        try {
            if (!initialized_) {
                initialize();
                initialized_ = true;
            }
            if (auto* batch = std::get_if<MutationRequest::Batch>(&request_.data))
                return poll_batch(wake, *batch);
            if (write_) {
                forwarding_.attach(write_->wake_signal(), wake);
                auto* result = write_->poll();
                if (!result)
                    return {};
                forwarding_.reset();
                write_ = nullptr;
                if (!*result)
                    return Output::err(
                        resource_,
                        Error::transport(resource_, clone_in(resource_, result->error())));
                metadata_->ended_at = ExecutionMetadata::SystemTime::clock::now();
                metadata_->affected_rows = result->value();
                metadata_->sql_log.execution_outcome = SqlExecutionOutcome::Success;
                if (readback_ && result->value() == 1)
                    if (auto* id = mutation_persisted_id(request_)) {
                        SelectQuery query(std::allocator_arg,
                                          Allocator(resource_),
                                          mutation_entity_name(request_));
                        auto filter = Expr::eq(resource_, "id", *id);
                        if (guard_) {
                            Expr both(resource_, Expr::Kind::And);
                            both.args.emplace_back(std::move(filter));
                            both.args.emplace_back(*guard_);
                            query.filter(std::move(both));
                        } else
                            query.filter(std::move(filter));
                        compiled_.emplace(
                            cache_.compile_select_cached(resource_, dialect_, *entity_, query));
                        read_ = &observed_->fetch_all_compact_sql(resource_, *compiled_);
                    }
            }
            std::optional<EntitySnapshot> snapshot;
            if (read_) {
                forwarding_.attach(read_->wake_signal(), wake);
                auto* result = read_->poll();
                if (!result)
                    return {};
                forwarding_.reset();
                read_ = nullptr;
                if (!*result)
                    return Output::err(
                        resource_,
                        Error::transport(resource_, clone_in(resource_, result->error())));
                if (result->value().size() != 1) {
                    String error("persisted ", resource_);
                    error += mutation_entity_name(request_);
                    error += " record could not be read back";
                    return Output::err(resource_, Error::persisted_record(resource_, error));
                }
                snapshot.emplace(resource_);
                snapshot->fields = result->value()[0].into_map(resource_);
            }
            MutationResult result(std::allocator_arg,
                                  Allocator(resource_),
                                  *metadata_->affected_rows,
                                  std::move(*metadata_));
            result.persisted_snapshot = std::move(snapshot);
            return Output::ok(resource_, std::move(result));
        } catch (const SqlCompileError& e) {
            stop();
            return Output::err(resource_, Error::compile(resource_, clone_in(resource_, e)));
        } catch (...) {
            stop();
            throw;
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        compiled_.reset();
    }

    void cancel_and_wait() noexcept {
        stop();
        compiled_.reset();
    }

private:
    void stop() noexcept {
        if (child_)
            child_->cancel();
        if (write_)
            write_->cancel();
        if (read_)
            read_->cancel();
        forwarding_.reset();
        child_ = nullptr;
        write_ = nullptr;
        read_ = nullptr;
    }

    void initialize() {
        if (std::holds_alternative<MutationRequest::Batch>(request_.data)) {
            if (guard_)
                throw SqlCompileError(std::allocator_arg,
                                      Allocator(resource_),
                                      SqlCompileError::Code::InvalidFunctionArguments,
                                      "guarded mutation supports update, delete, and recover only");
            auto now = ExecutionMetadata::SystemTime::clock::now();
            metadata_.emplace(
                std::allocator_arg, Allocator(resource_), DataServiceOperation::Batch, now, now);
            metadata_->backend = backend_name();
            metadata_->affected_rows = 0;
            return;
        }
        if (guard_ && std::holds_alternative<InsertCommand>(request_.data))
            throw SqlCompileError(std::allocator_arg,
                                  Allocator(resource_),
                                  SqlCompileError::Code::InvalidFunctionArguments,
                                  "guarded mutation supports update, delete, and recover only");
        auto name = mutation_entity_name(request_);
        entity_ = cache_.entity_descriptor(schema_, name);
        if (!entity_)
            throw SqlCompileError(std::allocator_arg,
                                  Allocator(resource_),
                                  SqlCompileError::Code::UnknownEntity,
                                  name);
        compiled_.emplace(compile_mutation(resource_, dialect_, *entity_, request_, guard_));
        observed_ = &async_arena(resource_).template make<ObservedSqlTransport<E>>(
            transport_, dialect_, request_, observer_);
        metadata_.emplace(
            guard_ ? mutation_metadata(
                         resource_, dialect_, *compiled_, request_, mutation_operation(request_))
                   : observed_->metadata(*compiled_, mutation_operation(request_)));
        write_ = &observed_->execute_sql(resource_, *compiled_);
    }

    std::string_view backend_name() const {
        switch (dialect_.kind()) {
        case DatabaseKind::Sqlite:
            return "sqlite";
        case DatabaseKind::PostgreSql:
            return "postgresql";
        case DatabaseKind::MySql:
            return "mysql";
        }
        return {};
    }

    std::optional<Output> poll_batch(WakeSignal& wake, const MutationRequest::Batch& batch) {
        while (index_ < batch.size()) {
            if (!child_)
                child_ = &make_operation<Output, MutationTreeOperation>(
                    resource_,
                    dialect_,
                    transport_,
                    schema_,
                    cache_,
                    clone_in(resource_, batch[index_]),
                    readback_,
                    observer_);
            forwarding_.attach(child_->wake_signal(), wake);
            auto* result = child_->poll();
            if (!result)
                return {};
            forwarding_.reset();
            child_ = nullptr;
            if (!*result)
                return Output::err(resource_, clone_in(resource_, result->error()));
            const auto& value = result->value();
            if (value.affected_rows >
                std::numeric_limits<std::uint64_t>::max() - *metadata_->affected_rows)
                throw std::overflow_error("batch affected row count overflow");
            *metadata_->affected_rows += value.affected_rows;
            if (value.metadata.parameterized_query) {
                if (!metadata_->parameterized_query)
                    metadata_->parameterized_query.emplace(resource_);
                else
                    *metadata_->parameterized_query += "; ";
                *metadata_->parameterized_query += *value.metadata.parameterized_query;
            }
            for (const auto& param : value.metadata.params)
                metadata_->params.emplace_back(param);
            metadata_->statements.emplace_back(value.metadata);
            ++index_;
        }
        metadata_->ended_at = ExecutionMetadata::SystemTime::clock::now();
        auto count = *metadata_->affected_rows;
        return Output::ok(
            resource_,
            MutationResult(std::allocator_arg, Allocator(resource_), count, std::move(*metadata_)));
    }

    Memory resource_;
    const SqlDialect& dialect_;
    SqlTransport<E>& transport_;
    const SchemaProvider& schema_;
    SqlExecutorState& cache_;
    MutationRequest request_;
    bool readback_, initialized_ = false;
    std::optional<ExecutionObserver> observer_;
    std::optional<Expr> guard_;
    const EntityDescriptor* entity_ = nullptr;
    ObservedSqlTransport<E>* observed_ = nullptr;
    std::optional<CompiledQuery> compiled_;
    std::optional<ExecutionMetadata> metadata_;
    DataServiceOperationResult<MutationResult, Error>* child_ = nullptr;
    DataServiceOperationResult<std::uint64_t, E>* write_ = nullptr;
    DataServiceOperationResult<Vector<CompactRow>, E>* read_ = nullptr;
    WakeSignal::Forward forwarding_;
    std::size_t index_ = 0;
};

template <class E>
class MutationExecution {
public:
    using allocator_type = Allocator;
    using Output = Result<MutationResult, SqlExecutorError<E>>;

    MutationExecution(std::allocator_arg_t,
                      Allocator a,
                      const SqlDialect& dialect,
                      SqlTransport<E>& transport,
                      const SchemaProvider& schema,
                      SqlExecutorState& cache,
                      MutationRequest request,
                      bool readback,
                      std::optional<ExecutionObserver> observer,
                      std::optional<Expr> guard)
        : resource_(a.resource()), dialect_(dialect), transport_(transport), schema_(schema),
          cache_(cache), request_(std::allocator_arg, a, std::move(request)), readback_(readback),
          observer_(move_in(a.resource(), std::move(observer))),
          guard_(move_in(a.resource(), std::move(guard))) {}

    std::optional<Output> poll(WakeSignal& wake) {
        try {
            if (!pending_) {
                journal_ = &async_arena(resource_).template make<FailureJournal>(observer_);
                pending_ = &make_operation<Output, MutationTreeOperation<E>>(resource_,
                                                                             dialect_,
                                                                             transport_,
                                                                             schema_,
                                                                             cache_,
                                                                             std::move(request_),
                                                                             readback_,
                                                                             journal_->recorder(),
                                                                             std::move(guard_));
            }
            forwarding_.attach(pending_->wake_signal(), wake);
            auto* result = pending_->poll();
            if (!result)
                return {};
            forwarding_.reset();
            pending_ = nullptr;
            auto output = clone_in(resource_, *result);
            if (output)
                journal_->disarm();
            else
                journal_->flush();
            return output;
        } catch (...) {
            stop();
            if (journal_)
                journal_->flush();
            throw;
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        stop();
        if (journal_)
            journal_->flush();
    }

private:
    void stop() noexcept {
        if (pending_)
            pending_->cancel();
        forwarding_.reset();
        pending_ = nullptr;
    }

    Memory resource_;
    const SqlDialect& dialect_;
    SqlTransport<E>& transport_;
    const SchemaProvider& schema_;
    SqlExecutorState& cache_;
    MutationRequest request_;
    bool readback_;
    std::optional<ExecutionObserver> observer_;
    std::optional<Expr> guard_;
    FailureJournal* journal_ = nullptr;
    AsyncOperation<Output>* pending_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
DataServiceOperationResult<MutationResult, SqlExecutorError<E>>&
execute_mutation(Memory r,
                 const SqlDialect& dialect,
                 SqlTransport<E>& transport,
                 const SchemaProvider& schema,
                 SqlExecutorState& cache,
                 MutationRequest request,
                 bool readback,
                 std::optional<ExecutionObserver> observer,
                 std::optional<Expr> guard = {}) {
    return make_operation<typename MutationExecution<E>::Output, MutationExecution<E>>(
        r,
        dialect,
        transport,
        schema,
        cache,
        std::move(request),
        readback,
        std::move(observer),
        std::move(guard));
}
}
