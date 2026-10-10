#pragma once
#include <teaql/core/row.hpp>
#include <teaql/runtime/resources.hpp>
#include <chrono>

namespace teaql {
class DebugWriter;

namespace detail {
struct QueryStoreAccess;
}

class AggregationCacheBackend {
public:
    virtual ~AggregationCacheBackend() = default;
    virtual Memory memory_resource() const noexcept = 0;
    virtual std::string_view name_space() const noexcept = 0;
    // Reads and writes explicitly copy values into the selected owning domain.
    virtual std::optional<Vector<CompactRow>>
    get(Memory result, std::string_view key, std::uint64_t max_age_millis) const = 0;
    virtual void put(std::string_view key, std::span<const CompactRow>) = 0;
    virtual void invalidate_namespace(std::string_view) = 0;
};

class InMemoryAggregationCache final : public AggregationCacheBackend {
public:
    using allocator_type = Allocator;

    InMemoryAggregationCache(std::allocator_arg_t, Allocator a, std::string_view name_space = {})
        : namespace_(name_space, a.resource()), entries_(a.resource()) {}

    InMemoryAggregationCache(const InMemoryAggregationCache&) = delete;
    InMemoryAggregationCache& operator=(const InMemoryAggregationCache&) = delete;

    Memory memory_resource() const noexcept override {
        return namespace_.get_allocator().resource();
    }

    std::string_view name_space() const noexcept override {
        return namespace_;
    }

    std::optional<Vector<CompactRow>> get(Memory, std::string_view, std::uint64_t) const override;
    void put(std::string_view, std::span<const CompactRow>) override;
    void invalidate_namespace(std::string_view) override;

    void clear() {
        entries_.clear();
    }

private:
    friend struct detail::QueryStoreAccess;
    friend void write_debug(DebugWriter&, const InMemoryAggregationCache&);

    struct Entry {
        using allocator_type = Allocator;
        Vector<CompactRow> rows;
        std::chrono::steady_clock::time_point stored_at;

        // Rust receives an already-owned Vec. Finish the C++ ownership copy
        // before starting its retention age, even for a large result payload.
        Entry(std::allocator_arg_t, Allocator a, std::span<const CompactRow> rows)
            : rows(rows.begin(), rows.end(), a.resource()),
              stored_at(std::chrono::steady_clock::now()) {}
    };

    String namespace_;
    std::pmr::map<String, Entry, std::less<>> entries_;
};
}
