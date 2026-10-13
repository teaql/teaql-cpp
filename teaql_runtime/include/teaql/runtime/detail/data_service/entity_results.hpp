#pragma once
#include <teaql/runtime/detail/data_service/enhanced_entities.hpp>
#include <teaql/runtime/detail/data_service/entity_stream.hpp>
#include <teaql/runtime/detail/read_hydration.hpp>

namespace teaql::detail {
template <Entity T, class Error>
class EntityOneOperation {
public:
    using allocator_type = Allocator;
    using Input = AsyncOperation<Result<SmartList<T>, DataServiceError<Error>>>;
    using Output = Result<std::optional<T>, DataServiceError<Error>>;

    EntityOneOperation(std::allocator_arg_t, Allocator a, Input& input)
        : resource_(a.resource()), input_(input) {}

    std::optional<Output> poll(WakeSignal& wake) {
        forwarding_.attach(input_.wake_signal(), wake);
        auto* result = input_.poll();
        if (!result) {
            return {};
        }
        forwarding_.reset();
        if (!*result) {
            return Output::err(resource_, copy_or_take_error(input_, *result));
        }
        auto* value = result->value().get(0);
        return Output::ok(
            resource_, value ? std::optional<T>(clone_in(resource_, *value)) : std::optional<T>{});
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
    WakeSignal::Forward forwarding_;
};

// Decode one bounded chunk before delivering any of its entities. A malformed
// row rejects the whole chunk, matching the generated Rust stream boundary.
// Delivered entities and their runtime state belong to the executing Context.
template <Entity T, class Error>
class TypedEntityStreamItems {
public:
    using allocator_type = Allocator;
    using Item = Result<T, DataServiceError<Error>>;

    TypedEntityStreamItems(std::allocator_arg_t,
                           Allocator a,
                           const EntityQueryStream<Error>& source)
        : resource_(a.resource()), source_(std::allocator_arg, a, source),
          root_(std::allocator_arg, a) {}

    std::optional<std::optional<Item>> poll_next(WakeSignal& wake) {
        if (ended_) {
            return std::optional<Item>{};
        }
        auto& source = source_.get();
        for (;;) {
            if (entities_ && index_ < entities_->size()) {
                auto value = std::move((*entities_)[index_++]);
                if (index_ == entities_->size()) {
                    entities_.reset();
                }
                return std::optional<Item>(Item::ok(resource_, std::move(value)));
            }
            entities_.reset();
            forwarding_.attach(source.wake_signal(), wake);
            auto next = source.poll_next();
            using Kind = typename StreamPoll<Result<StreamChunk, DataServiceError<Error>>>::Kind;
            if (next.kind == Kind::Pending) {
                return {};
            }
            forwarding_.reset();
            if (next.kind == Kind::End) {
                ended_ = true;
                return std::optional<Item>{};
            }
            if (!*next.item) {
                auto error = copy_or_take_error(source, *next.item);
                stop();
                return std::optional<Item>(Item::err(resource_, std::move(error)));
            }
            auto item = source.take_item();
            auto decoded = decode_compact_rows_with_read_metadata<T>(
                resource_, std::move(item).value().rows, root_);
            if (!decoded) {
                auto error = DataServiceError<Error>::entity(resource_, std::move(decoded).error());
                stop();
                return std::optional<Item>(Item::err(resource_, std::move(error)));
            }
            entities_.emplace(std::move(decoded).value());
            index_ = 0;
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        entities_.reset();
    }

    void cancel_and_wait() noexcept {
        stop();
    }

private:
    void stop() noexcept {
        forwarding_.reset();
        source_.get().cancel();
        entities_.reset();
        ended_ = true;
    }

    Memory resource_;
    EntityQueryStream<Error> source_;
    EntityRuntimeState root_;
    std::optional<Vector<T>> entities_;
    std::size_t index_ = 0;
    bool ended_ = false;
    WakeSignal::Forward forwarding_;
};

template <Entity T, class Error>
class TypedEntityStreamOperation {
public:
    using allocator_type = Allocator;
    using Input = AsyncOperation<Result<EntityQueryStream<Error>, DataServiceError<Error>>>;
    using Output = Result<TypedEntityQueryStream<T, Error>, DataServiceError<Error>>;

    TypedEntityStreamOperation(std::allocator_arg_t, Allocator a, Input& input)
        : resource_(a.resource()), input_(input) {}

    std::optional<Output> poll(WakeSignal& wake) {
        forwarding_.attach(input_.wake_signal(), wake);
        auto* result = input_.poll();
        if (!result) {
            return {};
        }
        forwarding_.reset();
        if (!*result) {
            return Output::err(resource_, copy_or_take_error(input_, *result));
        }
        source_ = &result->value().get();
        auto& stream =
            make_stream<Result<T, DataServiceError<Error>>, TypedEntityStreamItems<T, Error>>(
                resource_, result->value());
        return Output::ok(
            resource_,
            TypedEntityQueryStream<T, Error>(
                std::allocator_arg, Allocator(resource_), stream, resource_arena(resource_)));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        source_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (source_) {
            source_->cancel();
            source_ = nullptr;
        }
        input_.cancel();
    }

private:
    Memory resource_;
    Input& input_;
    QueryStream<DataServiceError<Error>>* source_ = nullptr;
    WakeSignal::Forward forwarding_;
};
}

namespace teaql {
template <class E>
template <Entity T>
auto EntityDataService<E>::fetch_entity(const PurposedSelectQuery& query) const
    -> DataServiceOperationResult<std::optional<T>, DataServiceError<Error>>& {
    auto r = memory_resource();
    auto limited = detail::clone_in(r, query);
    limited.limit(1);
    auto& source = fetch_enhanced_entities<T>(limited);
    using State = detail::EntityOneOperation<T, Error>;
    return make_operation<typename State::Output, State>(r, source);
}

template <class E>
template <Entity T>
auto EntityDataService<E>::fetch_entity_stream(const PurposedSelectQuery& query) const
    -> DataServiceOperationResult<TypedEntityQueryStream<T, Error>, DataServiceError<Error>>&
    requires std::derived_from<E, StreamQueryExecutor<Error>>
{
    auto& source = fetch_stream(query);
    using State = detail::TypedEntityStreamOperation<T, Error>;
    return make_operation<typename State::Output, State>(memory_resource(), source);
}
}
