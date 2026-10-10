#pragma once
#include <teaql/async/operation.hpp>
#include <teaql/core/value.hpp>
#include <teaql/runtime/system_time.hpp>
#include <map>

namespace teaql {
// Core asynchronous value store, independent from UserContext's local values
// and from aggregation/cursor/ID-set storage. No external cache integration.
class DataStore {
public:
    virtual ~DataStore() = default;
    virtual AsyncOperation<std::optional<Value>>& get(Memory destination,
                                                      std::string_view key) const = 0;
    virtual AsyncOperation<std::monostate>&
    put(Memory operation,
        std::string_view key,
        Value,
        std::optional<std::uint64_t> timeout_seconds) const = 0;
    virtual AsyncOperation<std::monostate>& remove(Memory operation,
                                                   std::string_view key) const = 0;
};

namespace detail {
class DataStoreOperation;
}

class InMemoryDataStore final : public DataStore {
public:
    using allocator_type = Allocator;

    explicit InMemoryDataStore(std::allocator_arg_t, Allocator a) : entries_(a.resource()) {}

    InMemoryDataStore(std::allocator_arg_t, Allocator, const InMemoryDataStore&);
    InMemoryDataStore(const InMemoryDataStore&) = delete;
    AsyncOperation<std::optional<Value>>& get(Memory, std::string_view) const override;
    AsyncOperation<std::monostate>&
        put(Memory, std::string_view, Value, std::optional<std::uint64_t>) const override;
    AsyncOperation<std::monostate>& remove(Memory, std::string_view) const override;

    Memory memory_resource() const {
        return entries_.get_allocator().resource();
    }

private:
    friend class detail::DataStoreOperation;

    struct Entry {
        using allocator_type = Allocator;
        Value value;
        std::optional<SystemTime> expires_at;

        Entry(std::allocator_arg_t, Allocator a, const Value& v, std::optional<SystemTime> expiry)
            : value(a.resource(), v), expires_at(expiry) {}

        Entry(std::allocator_arg_t, Allocator a, const Entry& other)
            : Entry(std::allocator_arg, a, other.value, other.expires_at) {}

        Entry(std::allocator_arg_t, Allocator a, Entry&& other)
            : Entry(std::allocator_arg, a, std::as_const(other)) {}

        Entry(Entry&&) noexcept = default;

        Entry& operator=(Entry&& other) {
            return detail::assign_scoped(*this, std::move(other));
        }

        Memory memory_resource() const {
            return value.memory_resource();
        }
    };

    mutable std::pmr::map<String, Entry, std::less<>> entries_;
    // The original write-lock is poisoned if Instant addition panics.
    mutable bool poisoned_ = false;
};
}
