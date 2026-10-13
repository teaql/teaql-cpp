#pragma once
#include <teaql/runtime/data_service.hpp>

namespace teaql::detail {
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
}
