#pragma once
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/context_binding.hpp>

namespace teaql::detail {
// Repository lookup is part of generated execution. Preserve its Graph result
// boundary instead of throwing before callers receive their operation handle.
template <class T, class Error>
class GeneratedQueryLookupFailure {
public:
    using allocator_type = Allocator;
    using Output = Result<T, DataServiceError<Error>>;

    GeneratedQueryLookupFailure(std::allocator_arg_t, Allocator a, const ContextError& error)
        : result_(
              Output::err(a.resource(),
                          DataServiceError<Error>::runtime(
                              a.resource(),
                              RuntimeError::graph(a.resource(), error.to_string(a.resource()))))) {}

    std::optional<Output> poll(WakeSignal&) {
        return std::move(result_);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}

private:
    Output result_;
};

template <class T, class Error>
auto& generated_query_lookup_failure(Memory r, const ContextError& error) {
    using State = GeneratedQueryLookupFailure<T, Error>;
    return make_operation<typename State::Output, State>(r, error);
}

// The generated Rust runtime maps service failures into RuntimeError::Graph.
// Keep the C++ service-error carrier while preserving that application boundary.
template <class Error>
DataServiceError<Error> generated_query_error(Memory r, const DataServiceError<Error>& error) {
    return DataServiceError<Error>::runtime(r, RuntimeError::graph(r, error.to_string(r)));
}

template <Entity T, class Error>
class GeneratedQueryStreamItems {
public:
    using allocator_type = Allocator;
    using Item = Result<T, DataServiceError<Error>>;

    GeneratedQueryStreamItems(std::allocator_arg_t,
                              Allocator a,
                              const TypedEntityQueryStream<T, Error>& source)
        : resource_(a.resource()), source_(std::allocator_arg, a, source) {}

    std::optional<std::optional<Item>> poll_next(WakeSignal& wake) {
        auto& source = source_.get();
        forwarding_.attach(source.wake_signal(), wake);
        auto next = source.poll_next();
        using Kind = typename StreamPoll<Item>::Kind;
        if (next.kind == Kind::Pending) {
            return {};
        }
        forwarding_.reset();
        if (next.kind == Kind::End) {
            return std::optional<Item>{};
        }
        auto item = source.take_item();
        if (!item) {
            auto error = generated_query_error(resource_, item.error());
            source.cancel();
            return std::optional<Item>(Item::err(resource_, std::move(error)));
        }
        return std::optional<Item>(std::move(item));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        source_.get().cancel();
    }

private:
    Memory resource_;
    TypedEntityQueryStream<T, Error> source_;
    WakeSignal::Forward forwarding_;
};

template <class T>
T generated_query_value(Memory, T value) {
    return value;
}

template <Entity T, class Error>
TypedEntityQueryStream<T, Error> generated_query_value(Memory r,
                                                       TypedEntityQueryStream<T, Error> source) {
    auto& stream =
        make_stream<Result<T, DataServiceError<Error>>, GeneratedQueryStreamItems<T, Error>>(
            r, source);
    return TypedEntityQueryStream<T, Error>(
        std::allocator_arg, Allocator(r), stream, resource_arena(r));
}

template <class T, class Error>
class GeneratedQueryResultOperation {
public:
    using allocator_type = Allocator;
    using Output = Result<T, DataServiceError<Error>>;

    GeneratedQueryResultOperation(std::allocator_arg_t, Allocator a, AsyncOperation<Output>& input)
        : resource_(a.resource()), input_(input) {
        if (input.memory_resource() != resource_) {
            throw std::invalid_argument("Generated query result must share its execution Context");
        }
    }

    std::optional<Output> poll(WakeSignal& wake) {
        forwarding_.attach(input_.wake_signal(), wake);
        auto* result = input_.poll();
        if (!result) {
            return {};
        }
        forwarding_.reset();
        auto value = input_.take_result();
        if (!value) {
            return Output::err(resource_, generated_query_error(resource_, value.error()));
        }
        return Output::ok(resource_, generated_query_value(resource_, std::move(value).value()));
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
    AsyncOperation<Output>& input_;
    WakeSignal::Forward forwarding_;
};

template <class T, class Error>
auto& generated_query_result(AsyncOperation<Result<T, DataServiceError<Error>>>& input) {
    using Output = Result<T, DataServiceError<Error>>;
    return make_operation<Output, GeneratedQueryResultOperation<T, Error>>(input.memory_resource(),
                                                                           input);
}

// The original generated wrapper boxes its stream before repository lookup.
// Resolve and open only when the consumer requests the first item.
template <Entity T, class E>
class DeferredGeneratedQueryStreamItems {
public:
    using allocator_type = Allocator;
    using Error = DataServiceError<typename E::Error>;
    using Item = Result<T, Error>;
    using Handle = TypedEntityQueryStream<T, typename E::Error>;
    using Opening = AsyncOperation<Result<Handle, Error>>;

    DeferredGeneratedQueryStreamItems(std::allocator_arg_t,
                                      Allocator a,
                                      const ContextBinding& binding,
                                      std::string_view entity,
                                      const PurposedSelectQuery& query)
        : resource_(a.resource()), binding_(binding), entity_(entity, resource_),
          query_(std::allocator_arg, a, query) {}

    std::optional<std::optional<Item>> poll_next(WakeSignal& wake) {
        if (ended_) {
            return std::optional<Item>{};
        }
        if (!source_) {
            if (!opening_) {
                auto service = binding_.get().template entity_data_service<E>(entity_);
                if (!service) {
                    ended_ = true;
                    return std::optional<Item>(Item::err(
                        resource_,
                        Error::runtime(
                            resource_,
                            RuntimeError::graph(resource_, service.error().to_string(resource_)))));
                }
                opening_ = &generated_query_result(
                    service.value().get().template fetch_entity_stream<T>(query_));
            }
            forwarding_.attach(opening_->wake_signal(), wake);
            if (!opening_->poll()) {
                return {};
            }
            forwarding_.reset();
            auto opened = opening_->take_result();
            if (!opened) {
                ended_ = true;
                return std::optional<Item>(Item::err(resource_, std::move(opened).error()));
            }
            source_.emplace(std::allocator_arg, Allocator(resource_), std::move(opened).value());
        }
        auto& source = source_->get();
        forwarding_.attach(source.wake_signal(), wake);
        auto next = source.poll_next();
        using Kind = typename StreamPoll<Item>::Kind;
        if (next.kind == Kind::Pending) {
            return {};
        }
        forwarding_.reset();
        if (next.kind == Kind::End) {
            ended_ = true;
            return std::optional<Item>{};
        }
        auto item = source.take_item();
        if (!item) {
            ended_ = true;
        }
        return std::optional<Item>(std::move(item));
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        ended_ = true;
        if (source_) {
            source_->get().cancel();
            source_.reset();
        }
        if (opening_) {
            opening_->cancel();
            opening_ = nullptr;
        }
    }

private:
    Memory resource_;
    ContextBinding binding_;
    String entity_;
    PurposedSelectQuery query_;
    Opening* opening_ = nullptr;
    std::optional<Handle> source_;
    bool ended_ = false;
    WakeSignal::Forward forwarding_;
};

template <Entity T, class E>
class DeferredGeneratedQueryStreamOperation {
public:
    using allocator_type = Allocator;
    using Error = DataServiceError<typename E::Error>;
    using Handle = TypedEntityQueryStream<T, typename E::Error>;
    using Output = Result<Handle, Error>;
    using Item = Result<T, Error>;

    DeferredGeneratedQueryStreamOperation(std::allocator_arg_t,
                                          Allocator a,
                                          const ContextBinding& binding,
                                          std::string_view entity,
                                          const PurposedSelectQuery& query)
        : resource_(a.resource()), binding_(binding), entity_(entity, resource_),
          query_(std::allocator_arg, a, query) {}

    std::optional<Output> poll(WakeSignal&) {
        stream_ = &make_stream<Item, DeferredGeneratedQueryStreamItems<T, E>>(
            resource_, binding_, entity_, query_);
        return Output::ok(
            resource_,
            Handle(std::allocator_arg, Allocator(resource_), *stream_, resource_arena(resource_)));
    }

    void finish_and_wait() noexcept {
        stream_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        if (stream_) {
            stream_->cancel();
            stream_ = nullptr;
        }
    }

private:
    Memory resource_;
    ContextBinding binding_;
    String entity_;
    PurposedSelectQuery query_;
    AsyncStream<Item>* stream_ = nullptr;
};

template <Entity T, class E>
auto& defer_generated_query_stream(UserContext& user_context,
                                   std::string_view entity,
                                   const PurposedSelectQuery& query) {
    using State = DeferredGeneratedQueryStreamOperation<T, E>;
    return make_operation<typename State::Output, State>(
        user_context.memory_resource(), ContextBinding(user_context), entity, query);
}

}
