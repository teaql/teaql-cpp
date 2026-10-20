#pragma once
#include <teaql/runtime/detail/query_results.hpp>
#include <teaql/core/request.hpp>
#include <teaql/runtime/detail/generated_query_facets.hpp>

namespace teaql::detail {
inline PurposedSelectQuery reauthorize_generated_query(const SelectQuery& source) {
    auto r = source.memory_resource();
    auto query = source.clone(r);
    // Generated authorization replaces the last purpose frame before the
    // runtime wrapper appends it. Earlier ancestry remains distinct.
    auto purpose =
        std::find_if(query.trace_chain.rbegin(), query.trace_chain.rend(), [](const auto& node) {
            return node.kind == TraceKind::Purpose;
        });
    if (purpose == query.trace_chain.rend() || !query.purpose_text) {
        throw std::logic_error("Generated derived query requires its authorized purpose");
    }
    query.trace_chain.erase(std::prev(purpose.base()));
    return PurposedSelectQuery(std::allocator_arg, Allocator(r), query, *query.purpose_text);
}

template <class Error, bool Count>
class GeneratedScalarQueryOperation {
public:
    using allocator_type = Allocator;
    using Scalar = std::conditional_t<Count, std::uint64_t, bool>;
    using Output = Result<Scalar, DataServiceError<Error>>;
    using Input = AsyncOperation<Result<Vector<CompactRow>, DataServiceError<Error>>>;

    GeneratedScalarQueryOperation(std::allocator_arg_t,
                                  Allocator a,
                                  Input& input,
                                  std::string_view entity)
        : resource_(a.resource()), input_(input), entity_(entity, resource_) {
        if (input.memory_resource() != resource_) {
            throw std::invalid_argument("Generated scalar query must share its execution Context");
        }
    }

    std::optional<Output> poll(WakeSignal& wake) {
        forwarding_.attach(input_.wake_signal(), wake);
        if (!input_.poll()) {
            return std::nullopt;
        }
        forwarding_.reset();
        auto rows = input_.take_result();
        if (!rows) {
            return Output::err(resource_, generated_query_error(resource_, rows.error()));
        }
        if constexpr (!Count) {
            return Output::ok(resource_, !rows.value().empty());
        } else {
            if (!rows.value().empty()) {
                if (auto* value = rows.value().front().get(request::COUNT_ALIAS)) {
                    if (auto count = value->try_u64()) {
                        return Output::ok(resource_, *count);
                    }
                }
            }
            String message("count result for ", resource_);
            message += entity_;
            message += " is missing or not numeric";
            return Output::err(resource_,
                               DataServiceError<Error>::runtime(
                                   resource_, RuntimeError::graph(resource_, message)));
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        input_.cancel();
    }

private:
    Memory resource_;
    Input& input_;
    String entity_;
    WakeSignal::Forward forwarding_;
};

template <bool Count, class E>
auto& execute_generated_scalar_query(const EntityDataService<E>& service,
                                     const PurposedSelectQuery& source) {
    auto r = service.memory_resource();
    auto query = source.as_query().clone(r);
    if constexpr (Count) {
        query.projection.clear();
        query.expr_projection.clear();
        query.order_by.clear();
        query.slice.reset();
        query.count(request::COUNT_ALIAS);
    } else {
        query.limit(1);
    }
    query.relations.clear();
    auto authorized = reauthorize_generated_query(query);
    auto& rows = service.fetch_all(authorized);
    using State = GeneratedScalarQueryOperation<typename E::Error, Count>;
    return make_operation<typename State::Output, State>(r, rows, query.entity);
}

template <Entity T, class E>
class GeneratedPageQueryOperation {
public:
    using allocator_type = Allocator;
    using Error = DataServiceError<typename E::Error>;
    using Output = Result<SmartList<T>, Error>;
    using CountResult = Result<std::uint64_t, Error>;

    GeneratedPageQueryOperation(std::allocator_arg_t,
                                Allocator a,
                                const EntityDataService<E>& service,
                                const PurposedSelectQuery& source,
                                std::uint64_t offset,
                                std::uint64_t limit)
        : service_(std::allocator_arg, a, service), source_(std::allocator_arg, a, source),
          offset_(offset), limit_(limit), list_first_(bool(source.as_query().id_set_pagination)) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        for (;;) {
            if (!list_ && (list_first_ || count_)) {
                auto query = source_.as_query().clone(r);
                query.page(offset_, limit_);
                auto authorized = reauthorize_generated_query(query);
                list_ = &execute_generated_list_query<T>(service_, authorized);
            }
            if (list_ && !rows_) {
                forwarding_.attach(list_->wake_signal(), wake);
                if (!list_->poll()) {
                    return std::nullopt;
                }
                forwarding_.reset();
                auto result = list_->take_result();
                if (!result) {
                    return Output::err(r, std::move(result).error());
                }
                rows_.emplace(std::move(result).value());
                if (list_first_ && rows_->total_count) {
                    return Output::ok(r, std::move(*rows_));
                }
            }
            if (!count_) {
                if (!count_operation_) {
                    count_operation_ = &execute_generated_scalar_query<true>(service_, source_);
                }
                forwarding_.attach(count_operation_->wake_signal(), wake);
                if (!count_operation_->poll()) {
                    return std::nullopt;
                }
                forwarding_.reset();
                auto result = count_operation_->take_result();
                if (!result) {
                    return Output::err(r, std::move(result).error());
                }
                count_ = result.value();
            }
            if (rows_) {
                rows_->total_count = *count_;
                return Output::ok(r, std::move(*rows_));
            }
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (count_operation_) {
            count_operation_->cancel();
        }
        if (list_) {
            list_->cancel();
        }
    }

private:
    EntityDataService<E> service_;
    PurposedSelectQuery source_;
    std::uint64_t offset_, limit_;
    bool list_first_;
    std::optional<std::uint64_t> count_;
    std::optional<SmartList<T>> rows_;
    AsyncOperation<CountResult>* count_operation_ = nullptr;
    AsyncOperation<Output>* list_ = nullptr;
    WakeSignal::Forward forwarding_;
};
}
