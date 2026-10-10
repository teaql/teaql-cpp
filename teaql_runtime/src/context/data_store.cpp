#include <teaql/runtime/context.hpp>
#include <teaql/runtime/data_store.hpp>

namespace teaql::detail {
template <class T>
class ContextDataStoreOperation {
public:
    using allocator_type = Allocator;
    enum class Kind { Get, Put, Clear };

    ContextDataStoreOperation(std::allocator_arg_t,
                              Allocator a,
                              const UserContext& context,
                              Kind kind,
                              std::string_view key,
                              Value value,
                              std::optional<std::uint64_t> timeout)
        : context_(context), kind_(kind), key_(key, a.resource()),
          value_(a.resource(), std::move(value)), timeout_(timeout) {}

    std::optional<T> poll(WakeSignal& wake) {
        auto r = key_.get_allocator().resource();
        if (!pending_) {
            auto* store = context_.get_resource<DataStore>();
            if (!store) {
                return std::optional<T>(std::in_place);
            }
            if constexpr (std::same_as<T, std::optional<Value>>) {
                pending_ = &store->get(r, key_);
            } else {
                if (kind_ == Kind::Put) {
                    pending_ = &store->put(r, key_, Value(r, value_), timeout_);
                } else {
                    pending_ = &store->remove(r, key_);
                }
            }
            if (pending_->memory_resource() != r) {
                pending_->cancel();
                pending_ = nullptr;
                throw std::invalid_argument("DataStore operation belongs to another Context");
            }
            forward_.attach(pending_->wake_signal(), wake);
        }
        auto* result = pending_->poll();
        if (!result) {
            return {};
        }
        return std::optional<T>(std::in_place, clone_in(r, *result));
    }

    void cancel_and_wait() noexcept {
        forward_.reset();
        if (pending_) {
            pending_->cancel();
            pending_ = nullptr;
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

private:
    const UserContext& context_;
    Kind kind_;
    String key_;
    Value value_;
    std::optional<std::uint64_t> timeout_;
    AsyncOperation<T>* pending_ = nullptr;
    WakeSignal::Forward forward_;
};
}

namespace teaql {
AsyncOperation<std::optional<Value>>& UserContext::get_in_store(std::string_view key) const {
    using State = detail::ContextDataStoreOperation<std::optional<Value>>;
    return make_operation<std::optional<Value>, State>(
        memory_resource(), *this, State::Kind::Get, key, Value(memory_resource()), std::nullopt);
}

AsyncOperation<std::monostate>& UserContext::put_in_store(
    std::string_view key, Value value, std::optional<std::uint64_t> timeout) const {
    using State = detail::ContextDataStoreOperation<std::monostate>;
    return make_operation<std::monostate, State>(
        memory_resource(), *this, State::Kind::Put, key, std::move(value), timeout);
}

AsyncOperation<std::monostate>& UserContext::clear_in_store(std::string_view key) const {
    using State = detail::ContextDataStoreOperation<std::monostate>;
    return make_operation<std::monostate, State>(
        memory_resource(), *this, State::Kind::Clear, key, Value(memory_resource()), std::nullopt);
}
}
