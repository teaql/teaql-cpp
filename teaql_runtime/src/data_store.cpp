#include <teaql/runtime/data_store.hpp>
#include <teaql/runtime/resources.hpp>

namespace teaql::detail {
namespace {
SystemTime store_now() {
    auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    auto seconds = std::chrono::floor<std::chrono::seconds>(elapsed);
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed - seconds);
    return SystemTime::from_unix_seconds(seconds.count(), nanos.count());
}
}

class DataStoreOperation {
public:
    using allocator_type = Allocator;
    enum class Kind { Get, Put, Remove };

    DataStoreOperation(std::allocator_arg_t,
                       Allocator a,
                       const InMemoryDataStore& store,
                       Kind kind,
                       std::string_view key,
                       Value value,
                       std::optional<std::uint64_t> timeout)
        : store_(std::allocator_arg, a, store, resource_arena(store.memory_resource())),
          kind_(kind), key_(key, a.resource()), value_(a.resource(), std::move(value)),
          timeout_(timeout) {}

    std::optional<std::optional<Value>> poll_get(WakeSignal&) {
        auto& store = store_.get();
        check_poison(store);
        auto found = store.entries_.find(key_);
        if (found == store.entries_.end() ||
            (found->second.expires_at && store_now() > *found->second.expires_at)) {
            return std::optional<std::optional<Value>>(std::in_place, std::nullopt);
        }
        return std::optional<std::optional<Value>>(
            std::in_place, Value(key_.get_allocator().resource(), found->second.value));
    }

    std::optional<std::monostate> poll(WakeSignal&) {
        auto& store = store_.get();
        check_poison(store);
        if (kind_ == Kind::Remove) {
            store.entries_.erase(key_);
        } else {
            std::optional<SystemTime> expiry;
            if (timeout_) {
                expiry = store_now().checked_add_seconds(*timeout_);
                if (!expiry) {
                    store.poisoned_ = true;
                    throw std::out_of_range("DataStore timeout exceeds Rust Instant");
                }
            }
            auto owner = store.memory_resource();
            InMemoryDataStore::Entry entry(std::allocator_arg, Allocator(owner), value_, expiry);
            store.entries_.insert_or_assign(String(key_, owner), std::move(entry));
        }
        return std::monostate{};
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}

private:
    static void check_poison(const InMemoryDataStore& store) {
        if (store.poisoned_) {
            throw std::logic_error("DataStore lock poisoned");
        }
    }

    ResourceBorrow<const InMemoryDataStore> store_;
    Kind kind_;
    String key_;
    Value value_;
    std::optional<std::uint64_t> timeout_;
};

class DataStoreGet final : public DataStoreOperation {
public:
    using DataStoreOperation::DataStoreOperation;

    std::optional<std::optional<Value>> poll(WakeSignal& wake) {
        return poll_get(wake);
    }
};
}

namespace teaql {
InMemoryDataStore::InMemoryDataStore(std::allocator_arg_t,
                                     Allocator a,
                                     const InMemoryDataStore& other)
    : entries_(other.entries_, a.resource()), poisoned_(other.poisoned_) {}

AsyncOperation<std::optional<Value>>& InMemoryDataStore::get(Memory r, std::string_view key) const {
    return make_operation<std::optional<Value>, detail::DataStoreGet>(
        r, *this, detail::DataStoreOperation::Kind::Get, key, Value(r), std::nullopt);
}

AsyncOperation<std::monostate>& InMemoryDataStore::put(Memory r,
                                                       std::string_view key,
                                                       Value value,
                                                       std::optional<std::uint64_t> timeout) const {
    return make_operation<std::monostate, detail::DataStoreOperation>(
        r, *this, detail::DataStoreOperation::Kind::Put, key, std::move(value), timeout);
}

AsyncOperation<std::monostate>& InMemoryDataStore::remove(Memory r, std::string_view key) const {
    return make_operation<std::monostate, detail::DataStoreOperation>(
        r, *this, detail::DataStoreOperation::Kind::Remove, key, Value(r), std::nullopt);
}
}
