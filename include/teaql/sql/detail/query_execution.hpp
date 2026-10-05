#pragma once
#include <teaql/sql/error.hpp>
#include <teaql/sql/transport.hpp>
#include <teaql/sql/detail/query_plan.hpp>
#include <teaql/sql/detail/diagnostic.hpp>

namespace teaql {
namespace detail {
inline ExecutionMetadata query_diagnostic_metadata(Memory r,
                                                   const SqlDialect& dialect,
                                                   const CompiledQuery& compiled,
                                                   const QueryRequest& request) {
    auto now = ExecutionMetadata::SystemTime::clock::now();
    ExecutionMetadata metadata(
        std::allocator_arg, Allocator(r), DataServiceOperation::Query, now, now);
    metadata.sql_log = clone_in(r, compiled.log_context);
    switch (dialect.kind()) {
    case DatabaseKind::Sqlite:
        metadata.backend = "sqlite";
        break;
    case DatabaseKind::PostgreSql:
        metadata.backend = "postgresql";
        break;
    case DatabaseKind::MySql:
        metadata.backend = "mysql";
        break;
    }
    for (const auto& node : request.trace_chain) {
        metadata.trace_chain.emplace_back(node);
    }
    metadata.comment = clone_in(r, request.comment);
    metadata.parameterized_query.emplace(compiled.sql, r);
    for (const auto& param : compiled.params) {
        metadata.params.emplace_back(param);
    }
    // capture_debug_query never authorizes eager literal expansion.
    return metadata;
}

template <class E>
class SqlQueryOperation {
public:
    using allocator_type = Allocator;
    using Error = SqlExecutorError<E>;
    using Output = Result<QueryResult, Error>;

    SqlQueryOperation(std::allocator_arg_t,
                      Allocator a,
                      const SqlDialect& dialect,
                      SqlTransport<E>& transport,
                      const SchemaProvider& schema,
                      SqlExecutorState& cache,
                      QueryRequest request,
                      std::optional<ExecutionObserver> observer,
                      bool allow_probes)
        : resource_(a.resource()), dialect_(dialect), transport_(transport), schema_(schema),
          cache_(cache), allow_probes_(allow_probes),
          request_(std::allocator_arg, a, std::move(request)),
          observer_(move_in(a.resource(), std::move(observer))) {}

    std::optional<Output> poll(WakeSignal& wake) {
        try {
            if (!initialized_) {
                try {
                    initialize();
                } catch (const SqlCompileError& e) {
                    return Output::err(resource_,
                                       Error::compile(resource_, clone_in(resource_, e)));
                }
                initialized_ = true;
                pending_ = repeated_ ? &transport_.fetch_repeated_compact_sql(
                                           resource_, *compiled_, param_index_, probe_values_)
                                     : &transport_.fetch_all_compact_sql(resource_, *compiled_);
                if (pending_->memory_resource() != resource_) {
                    throw std::invalid_argument(
                        "SQL transport returned an operation in a different Context");
                }
            }
            forwarding_.attach(pending_->wake_signal(), wake);
            auto* result = pending_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            pending_ = nullptr;
            if (!*result) {
                if (diagnostic_) {
                    diagnostic_->fail();
                }
                return Output::err(
                    resource_, Error::transport(resource_, clone_in(resource_, result->error())));
            }
            Vector<CompactRow> rows(result->value(), resource_);
            auto metadata =
                diagnostic_ ? diagnostic_->success() : std::optional<ExecutionMetadata>{};
            if (!metadata) {
                metadata.emplace(ExecutionMetadata::unrecorded_query(resource_, rows.size()));
            }
            metadata->result_count = rows.size();
            QueryResult out(std::allocator_arg, Allocator(resource_), std::move(*metadata));
            out.rows = std::move(rows);
            return Output::ok(resource_, std::move(out));
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
        compiled_.reset();
    }

    void cancel_and_wait() noexcept {
        stop();
        if (diagnostic_) {
            diagnostic_->cancel();
        }
        compiled_.reset();
    }

private:
    void stop() noexcept {
        if (pending_) {
            pending_->cancel();
        }
        forwarding_.reset();
        pending_ = nullptr;
    }

    void initialize() {
        auto* entity = cache_.entity_descriptor(schema_, request_.query.entity);
        if (!entity) {
            throw SqlCompileError(std::allocator_arg,
                                  Allocator(resource_),
                                  SqlCompileError::Code::UnknownEntity,
                                  request_.query.entity);
        }
        if (allow_probes_ && !request_.capture_execution_metadata &&
            dialect_.prefers_small_parent_relation_probes() &&
            !request_.query.top_n_probe_parent_threshold_value) {
            if (auto* values = partition_probe_values(request_.query);
                values && values->size() >= 2) {
                auto first = scalar_partition_probe_query(resource_, request_.query, (*values)[0]);
                auto second = scalar_partition_probe_query(resource_, request_.query, (*values)[1]);
                if (first && second) {
                    auto compiled =
                        cache_.compile_select_cached(resource_, dialect_, *entity, *first);
                    auto params = collect_select_params(
                        resource_, *entity, *second, dialect_.large_in_uses_array_param());
                    for (std::size_t i = 0; i < std::min(compiled.params.size(), params.size());
                         ++i) {
                        if (compiled.params[i] != params[i]) {
                            compiled_.emplace(std::move(compiled));
                            param_index_ = i;
                            probe_values_ = *values;
                            repeated_ = true;
                            return;
                        }
                    }
                }
            }
        }
        compiled_.emplace(
            cache_.compile_select_cached(resource_, dialect_, *entity, request_.query));
        std::optional<ExecutionMetadata> metadata;
        if (request_.capture_execution_metadata) {
            metadata.emplace(query_diagnostic_metadata(resource_, dialect_, *compiled_, request_));
        }
        diagnostic_.emplace(
            std::allocator_arg, Allocator(resource_), std::move(metadata), observer_);
    }

    Memory resource_;
    const SqlDialect& dialect_;
    SqlTransport<E>& transport_;
    const SchemaProvider& schema_;
    SqlExecutorState& cache_;
    bool allow_probes_;
    QueryRequest request_;
    std::optional<ExecutionObserver> observer_;
    bool initialized_ = false, repeated_ = false;
    std::optional<CompiledQuery> compiled_;
    std::size_t param_index_ = 0;
    std::span<const Value> probe_values_;
    DataServiceOperationResult<Vector<CompactRow>, E>* pending_ = nullptr;
    WakeSignal::Forward forwarding_;
    std::optional<StatementDiagnostic> diagnostic_;
};
}
}
