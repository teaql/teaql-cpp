#pragma once
#include <teaql/runtime/detail/data_service.hpp>
#include <teaql/runtime/telemetry.hpp>

namespace teaql::detail {
// The independent observe adapter also serves the later mutation entry points.
// Scope activation surrounds each poll, never the suspended interval.
template <class T, class E>
class ObservedDataServiceOperation {
    using R = Result<T, DataServiceError<typename E::Error>>;

public:
    using allocator_type = Allocator;

    ObservedDataServiceOperation(std::allocator_arg_t,
                                 Allocator a,
                                 const UserContext& context,
                                 std::string_view family,
                                 const ContextFunction<String()>& name,
                                 std::string_view entity,
                                 AsyncOperation<R>& work)
        : context_(std::allocator_arg, a, context, resource_arena(context.memory_resource())),
          family_(family, a.resource()), name_(std::allocator_arg, a, name),
          entity_(entity, a.resource()), work_(&work) {
        if (work.memory_resource() != a.resource()) {
            throw std::invalid_argument("Observed work must belong to its result Context");
        }
    }

    std::optional<R> poll(WakeSignal& wake) {
        auto r = context_.memory_resource();
        const auto& context = context_.get();
        if (!started_) {
            started_ = true;
            if (!context.runtime_telemetry_is_noop()) {
                RuntimeOperation operation(std::allocator_arg, Allocator(r), family_, name_());
                operation.attribute("teaql.entity.type", std::string_view(entity_));
                scope_ = &context.start_runtime_operation(operation);
            }
        }
        forwarding_.attach(work_->wake_signal(), wake);
        auto work = [&]() -> std::optional<R> {
            if (auto* result = work_->poll()) {
                return clone_in(r, *result);
            }
            return {};
        };
        if (!scope_) {
            return work();
        }
        auto result = scope_->poll_with_context([&]() -> std::optional<R> {
            if (!provider_scope_) {
                auto kind = resource_type_name<E>();
                String name(kind, r);
                name += '.';
                name += family_;
                RuntimeOperation operation(std::allocator_arg, Allocator(r), "provider", name);
                operation.attribute("teaql.provider.kind", kind)
                    .attribute("teaql.provider.operation", std::string_view(family_));
                provider_scope_ = &context.start_runtime_operation(operation);
            }
            auto result = provider_scope_->poll_with_context(work);
            if (result) {
                complete(*provider_scope_, bool(*result), r);
            }
            return result;
        });
        if (result) {
            complete(*scope_, bool(*result), r);
        }
        return result;
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        work_ = nullptr;
        close_scopes();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (work_) {
            work_->cancel();
            work_ = nullptr;
        }
        close_scopes();
    }

private:
    static void complete(FailOpenRuntimeTelemetryScope& scope, bool success, Memory r) {
        if (success) {
            scope.success(RuntimeAttributes(r));
        } else {
            scope.failure("data_service_error");
        }
        scope.close();
    }

    void close_scopes() noexcept {
        if (provider_scope_) {
            provider_scope_->close();
        }
        if (scope_) {
            scope_->close();
        }
    }

    ResourceBorrow<const UserContext> context_;
    String family_;
    ContextFunction<String()> name_;
    String entity_;
    AsyncOperation<R>* work_;
    WakeSignal::Forward forwarding_;
    FailOpenRuntimeTelemetryScope* scope_ = nullptr;
    FailOpenRuntimeTelemetryScope* provider_scope_ = nullptr;
    bool started_ = false;
};

struct QueryOperationName {
    using allocator_type = Allocator;
    String entity;

    QueryOperationName(std::allocator_arg_t, Allocator a, std::string_view name)
        : entity(name, a.resource()) {}

    QueryOperationName(std::allocator_arg_t, Allocator a, const QueryOperationName& other)
        : entity(other.entity, a.resource()) {}

    String operator()() const {
        String name(entity, entity.get_allocator());
        name += ".list";
        return name;
    }
};

template <class E, class T, bool List>
class ContextQueryOperation {
    using Output = std::conditional_t<List, SmartList<T>, Vector<CompactRow>>;
    using R = Result<Output, DataServiceError<typename E::Error>>;

public:
    using allocator_type = Allocator;

    ContextQueryOperation(std::allocator_arg_t,
                          Allocator a,
                          const ContextDataService<E>& service,
                          const SelectQuery& query)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = query_.memory_resource();
        if (!child_) {
            // Source fetch_all owns and resolves its comment when first polled;
            // the list/entity compatibility entry points retain the input comment.
            if constexpr (!List) {
                query_.comment_text = service_.resolve_final_comment(
                    query_.trace_chain,
                    query_.comment_text ? std::optional<std::string_view>(*query_.comment_text)
                                        : std::nullopt);
            }
            auto base = service_.data_service();
            auto& work = [&]() -> AsyncOperation<R>& {
                if constexpr (!List) {
                    return base.fetch_all(query_);
                } else if constexpr (std::same_as<T, CompactRow>) {
                    return base.fetch_smart_list(query_);
                } else {
                    return base.template fetch_entities<T>(query_);
                }
            }();
            child_ = &service_.observe(
                "query",
                QueryOperationName(std::allocator_arg, Allocator(r), query_.entity),
                query_.entity,
                work);
        }
        forwarding_.attach(child_->wake_signal(), wake);
        if (auto* result = child_->poll()) {
            return clone_in(r, *result);
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
    ContextDataService<E> service_;
    SelectQuery query_;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
template <class T, class Name>
auto ContextDataService<E>::observe(std::string_view family,
                                    Name&& name,
                                    std::string_view entity,
                                    DataServiceOperationResult<T, DataServiceError<Error>>& work)
    const -> DataServiceOperationResult<T, DataServiceError<Error>>& {
    using R = Result<T, DataServiceError<Error>>;
    auto r = memory_resource();
    ContextFunction<String()> factory(std::allocator_arg, Allocator(r), std::forward<Name>(name));
    return make_operation<R, ObservedDataServiceOperation<T, E>>(
        r, metadata.context(), family, factory, entity, work);
}

template <class E>
auto ContextDataService<E>::fetch_all(const SelectQuery& query) const
    -> DataServiceOperationResult<Vector<CompactRow>, DataServiceError<Error>>& {
    using R = Result<Vector<CompactRow>, DataServiceError<Error>>;
    return make_operation<R, ContextQueryOperation<E, CompactRow, false>>(
        memory_resource(), *this, query);
}

template <class E>
auto ContextDataService<E>::fetch_smart_list(const SelectQuery& query) const
    -> DataServiceOperationResult<SmartList<CompactRow>, DataServiceError<Error>>& {
    using R = Result<SmartList<CompactRow>, DataServiceError<Error>>;
    return make_operation<R, ContextQueryOperation<E, CompactRow, true>>(
        memory_resource(), *this, query);
}

template <class E>
template <Entity T>
auto ContextDataService<E>::fetch_entities(const SelectQuery& query) const
    -> DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>& {
    using R = Result<SmartList<T>, DataServiceError<Error>>;
    return make_operation<R, ContextQueryOperation<E, T, true>>(memory_resource(), *this, query);
}
}
