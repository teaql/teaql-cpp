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
std::string_view mutation_backend_name(const SqlDialect&);
ExecutionMetadata mutation_metadata(Memory,
                                    std::string_view backend,
                                    const CompiledQuery&,
                                    const MutationRequest&,
                                    DataServiceOperation);

// Immutable Context-owned batch ancestry; pointers never extend a Context lifetime.
struct BatchIntentScope {
    using allocator_type = Allocator;
    const BatchIntentScope* parent;
    MutationIntent intent;

    BatchIntentScope(std::allocator_arg_t,
                     Allocator a,
                     const BatchIntentScope* parent,
                     const MutationIntent& intent)
        : parent(parent), intent(std::allocator_arg, a, intent) {}
};

TraceChain mutation_statement_trace(Memory, const MutationRequest&, const BatchIntentScope*);
SqlIntentRedactions batch_mutation_redactions(Memory,
                                              const MutationRequest&,
                                              const SchemaProvider&,
                                              const SqlExecutorState&);

struct MutationExecutionScope {
    using allocator_type = Allocator;
    MutationIntent root_intent;
    const BatchIntentScope* batch = nullptr;
    const SqlIntentRedactions* redactions;

    MutationExecutionScope(std::allocator_arg_t,
                           Allocator a,
                           const MutationIntent& intent,
                           const SqlIntentRedactions& redactions)
        : root_intent(std::allocator_arg, a, intent), redactions(&redactions) {}

    MutationExecutionScope(std::allocator_arg_t, Allocator a, const MutationExecutionScope& other)
        : root_intent(std::allocator_arg, a, other.root_intent), batch(other.batch),
          redactions(other.redactions) {}
};

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
                if constexpr (std::same_as<T, std::uint64_t>) {
                    owner_.intent = SqlIntentRedactions::from_bindings(
                        resource_, query_.log_context, query_.params, query_.sql);
                }
                if (owner_.observer || !std::same_as<T, std::uint64_t>) {
                    diagnostic_.emplace(std::allocator_arg,
                                        Allocator(resource_),
                                        owner_.metadata(query_,
                                                        std::same_as<T, std::uint64_t>
                                                            ? owner_.operation
                                                            : DataServiceOperation::Query),
                                        owner_.observer);
                }
                if constexpr (std::same_as<T, std::uint64_t>) {
                    pending_ = &owner_.inner.execute_sql(resource_, query_);
                } else {
                    pending_ = &owner_.inner.fetch_all_compact_sql(resource_, query_);
                }
                if (pending_->memory_resource() != resource_) {
                    throw std::invalid_argument(
                        "SQL transport returned work in a different Context");
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
                if (diagnostic_) {
                    diagnostic_->fail();
                }
                return Output::err(resource_, copy_or_take_error(*completed, *result));
            }
            auto value = clone_in(resource_, result->value());
            if (diagnostic_) {
                if (auto metadata = diagnostic_->success()) {
                    if constexpr (std::same_as<T, std::uint64_t>) {
                        metadata->affected_rows = value;
                    } else {
                        metadata->result_count = value.size();
                        owner_.successful_readbacks.emplace_back(*metadata);
                    }
                    if (owner_.observer) {
                        (*owner_.observer)(std::move(*metadata));
                    }
                }
            }
            return Output::ok(resource_, std::move(value));
        } catch (...) {
            stop();
            if (diagnostic_) {
                diagnostic_->fail();
            }
            throw;
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        stop();
        if (diagnostic_) {
            diagnostic_->cancel();
        }
    }

private:
    void stop() noexcept {
        if (pending_) {
            pending_->cancel();
        }
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
    String backend;
    const MutationRequest& request;
    DataServiceOperation operation;
    std::optional<ExecutionObserver> observer;
    SqlIntentRedactions intent;
    TraceChain statement_trace;
    String root_comment;
    const SqlIntentRedactions* inherited_redactions;
    Vector<ExecutionMetadata> successful_readbacks;

    ObservedSqlTransport(std::allocator_arg_t,
                         Allocator a,
                         SqlTransport<E>& inner,
                         const SqlDialect& dialect,
                         const MutationRequest& request,
                         std::optional<ExecutionObserver> observer,
                         const MutationExecutionScope& scope)
        : inner(inner), backend(mutation_backend_name(dialect), a.resource()), request(request),
          operation(mutation_operation(request)),
          observer(move_in(a.resource(), std::move(observer))), intent(a.resource()),
          statement_trace(mutation_statement_trace(a.resource(), request, scope.batch)),
          root_comment(scope.root_intent.comment(), a.resource()),
          inherited_redactions(scope.redactions), successful_readbacks(a.resource()) {}

    ExecutionMetadata metadata(const CompiledQuery& query, DataServiceOperation op) const {
        auto r = intent.memory_resource();
        auto m = mutation_metadata(r, backend, query, request, op);
        m.trace_chain = clone_in(r, statement_trace);
        m.comment.emplace(root_comment, r);
        if (op == DataServiceOperation::Query) {
            TraceChain trace(r);
            auto root = statement_trace.empty()
                            ? std::string_view("unknown")
                            : std::string_view(statement_trace.front().entity_type);
            trace.emplace_back(TraceKind::Comment, root, root_comment);
            trace.emplace_back(TraceKind::Purpose, root, "verify the persisted mutation result");
            for (const auto& node : statement_trace) {
                if (node.kind != TraceKind::Comment && node.kind != TraceKind::Purpose) {
                    trace.emplace_back(node);
                }
            }
            m.trace_chain.swap(trace);
            m.sql_log.intent_redactions = clone_in(r, intent);
        }
        if (auto* id = mutation_target_id(request)) {
            m.sql_log.intent_redactions.capture_target_id(*id);
        }
        m.sql_log.intent_redactions.extend(*inherited_redactions);
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
                          std::optional<Expr> guard,
                          const MutationExecutionScope& scope)
        : resource_(a.resource()), dialect_(dialect), transport_(transport), schema_(schema),
          cache_(cache), request_(std::allocator_arg, a, std::move(request)), readback_(readback),
          observer_(move_in(a.resource(), std::move(observer))),
          guard_(move_in(a.resource(), std::move(guard))), scope_(std::allocator_arg, a, scope) {}

    std::optional<Output> poll(WakeSignal& wake) {
        try {
            if (!initialized_) {
                initialize();
                initialized_ = true;
            }
            if (auto* batch = std::get_if<MutationRequest::Batch>(&request_.data)) {
                return poll_batch(wake, *batch);
            }
            if (write_) {
                forwarding_.attach(write_->wake_signal(), wake);
                auto* result = write_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                auto* completed = std::exchange(write_, nullptr);
                if (!*result) {
                    return Output::err(
                        resource_,
                        Error::transport(resource_, copy_or_take_error(*completed, *result)));
                }
                metadata_->ended_at = ExecutionMetadata::SystemTime::clock::now();
                metadata_->affected_rows = result->value();
                metadata_->sql_log.execution_outcome = SqlExecutionOutcome::Success;
                if (readback_ && result->value() == 1) {
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
                        } else {
                            query.filter(std::move(filter));
                        }
                        compiled_.emplace(
                            cache_.compile_select_cached(resource_, dialect_, *entity_, query));
                        read_ = &observed_->fetch_all_compact_sql(resource_, *compiled_);
                    }
                }
            }
            std::optional<EntitySnapshot> snapshot;
            if (read_) {
                forwarding_.attach(read_->wake_signal(), wake);
                auto* result = read_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                auto* completed = std::exchange(read_, nullptr);
                if (!*result) {
                    return Output::err(
                        resource_,
                        Error::transport(resource_, copy_or_take_error(*completed, *result)));
                }
                if (result->value().size() != 1) {
                    String error("persisted ", resource_);
                    error += mutation_entity_name(request_);
                    error += " record could not be read back";
                    return Output::err(resource_, Error::persisted_record(resource_, error));
                }
                snapshot.emplace(resource_);
                snapshot->fields = result->value()[0].into_map(resource_);
            }
            if (guard_) {
                metadata_->backend = mutation_backend_name(dialect_);
            }
            if (!observed_->successful_readbacks.empty()) {
                auto write = clone_in(resource_, *metadata_);
                metadata_->statements.clear();
                metadata_->statements.emplace_back(std::move(write));
                for (const auto& readback : observed_->successful_readbacks) {
                    metadata_->statements.emplace_back(readback);
                }
                observed_->successful_readbacks.clear();
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
        if (child_) {
            child_->cancel();
        }
        if (write_) {
            write_->cancel();
        }
        if (read_) {
            read_->cancel();
        }
        forwarding_.reset();
        child_ = nullptr;
        write_ = nullptr;
        read_ = nullptr;
    }

    void initialize() {
        if (std::holds_alternative<MutationRequest::Batch>(request_.data)) {
            if (guard_) {
                throw SqlCompileError(std::allocator_arg,
                                      Allocator(resource_),
                                      SqlCompileError::Code::InvalidFunctionArguments,
                                      "guarded mutation supports update, delete, and recover only");
            }
            auto now = ExecutionMetadata::SystemTime::clock::now();
            metadata_.emplace(
                std::allocator_arg, Allocator(resource_), DataServiceOperation::Batch, now, now);
            metadata_->affected_rows = 0;
            metadata_->comment.emplace(scope_.root_intent.comment(), resource_);
            if (!scope_.batch || scope_.batch->intent.comment() != request_.comment()) {
                scope_.batch = &async_arena(resource_).template make<BatchIntentScope>(
                    scope_.batch, request_.intent());
            }
            return;
        }
        if (guard_ && std::holds_alternative<InsertCommand>(request_.data)) {
            throw SqlCompileError(std::allocator_arg,
                                  Allocator(resource_),
                                  SqlCompileError::Code::InvalidFunctionArguments,
                                  "guarded mutation supports update, delete, and recover only");
        }
        auto name = mutation_entity_name(request_);
        entity_ = cache_.entity_descriptor(schema_, name);
        if (!entity_) {
            throw SqlCompileError(std::allocator_arg,
                                  Allocator(resource_),
                                  SqlCompileError::Code::UnknownEntity,
                                  name);
        }
        // Guarded source execution captures its observed transport before
        // compiling the statement; ordinary leaves compile before capture.
        if (guard_) {
            observed_ = &async_arena(resource_).template make<ObservedSqlTransport<E>>(
                transport_, dialect_, request_, observer_, scope_);
        }
        compiled_.emplace(compile_mutation(resource_, dialect_, *entity_, request_, guard_));
        if (!observed_) {
            observed_ = &async_arena(resource_).template make<ObservedSqlTransport<E>>(
                transport_, dialect_, request_, observer_, scope_);
        }
        metadata_.emplace(guard_ ? mutation_metadata(resource_,
                                                     std::string_view{},
                                                     *compiled_,
                                                     request_,
                                                     mutation_operation(request_))
                                 : observed_->metadata(*compiled_, mutation_operation(request_)));
        if (guard_) {
            metadata_->trace_chain = clone_in(resource_, observed_->statement_trace);
            metadata_->comment.emplace(scope_.root_intent.comment(), resource_);
            metadata_->sql_log.intent_redactions.extend(*scope_.redactions);
        }
        write_ = &observed_->execute_sql(resource_, *compiled_);
    }

    std::optional<Output> poll_batch(WakeSignal& wake, const MutationRequest::Batch& batch) {
        while (index_ < batch.size()) {
            if (!child_) {
                child_ = &make_operation<Output, MutationTreeOperation>(
                    resource_,
                    dialect_,
                    transport_,
                    schema_,
                    cache_,
                    clone_in(resource_, batch[index_]),
                    readback_,
                    observer_,
                    std::optional<Expr>{},
                    scope_);
            }
            forwarding_.attach(child_->wake_signal(), wake);
            auto* result = child_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            auto* completed = std::exchange(child_, nullptr);
            if (!*result) {
                return Output::err(resource_, copy_or_take_error(*completed, *result));
            }
            const auto& value = result->value();
            if (value.affected_rows >
                std::numeric_limits<std::uint64_t>::max() - *metadata_->affected_rows) {
                throw std::overflow_error("batch affected row count overflow");
            }
            *metadata_->affected_rows += value.affected_rows;
            if (value.metadata.parameterized_query) {
                if (!metadata_->parameterized_query) {
                    metadata_->parameterized_query.emplace(resource_);
                } else {
                    *metadata_->parameterized_query += "; ";
                }
                *metadata_->parameterized_query += *value.metadata.parameterized_query;
            }
            for (const auto& param : value.metadata.params) {
                metadata_->params.emplace_back(param);
            }
            metadata_->statements.emplace_back(value.metadata);
            ++index_;
        }
        metadata_->backend = mutation_backend_name(dialect_);
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
    MutationExecutionScope scope_;
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
                request_.require_valid_intent();
                journal_ = &async_arena(resource_).template make<FailureJournal>(observer_);
                // The guarded source entry does not inherit request sidecars.
                // Ordinary batches classify through the same descriptor cache
                // used to compile each leaf, including a previously cached schema.
                auto redactions =
                    guard_ ? SqlIntentRedactions(resource_)
                           : batch_mutation_redactions(resource_, request_, schema_, cache_);
                auto& retained = async_arena(resource_).template make<SqlIntentRedactions>(
                    std::move(redactions));
                MutationExecutionScope scope(
                    std::allocator_arg, Allocator(resource_), request_.intent(), retained);
                pending_ = &make_operation<Output, MutationTreeOperation<E>>(resource_,
                                                                             dialect_,
                                                                             transport_,
                                                                             schema_,
                                                                             cache_,
                                                                             std::move(request_),
                                                                             readback_,
                                                                             journal_->recorder(),
                                                                             std::move(guard_),
                                                                             scope);
            }
            forwarding_.attach(pending_->wake_signal(), wake);
            auto* result = pending_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            auto* completed = std::exchange(pending_, nullptr);
            auto output = copy_or_take_completed(*completed, *result);
            if (output) {
                journal_->disarm();
            } else {
                journal_->flush();
            }
            return output;
        } catch (...) {
            stop();
            if (journal_) {
                journal_->flush();
            }
            throw;
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        stop();
        if (journal_) {
            journal_->flush();
        }
    }

private:
    void stop() noexcept {
        if (pending_) {
            pending_->cancel();
        }
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
