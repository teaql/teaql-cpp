#pragma once
#include <teaql/runtime/cache.hpp>
#include <teaql/runtime/detail/context/pagination.hpp>

namespace teaql::detail {
// Match Rust's private-field fixture bridges without changing store algorithms.
struct QueryStoreAccess {
    static void capacity(InMemoryContinuousPageCursorStore& store, std::size_t n) {
        store.max_entries_ = n;
    }

    static void capacity(InMemoryIdSetStore& store, std::size_t entries, std::size_t bytes) {
        store.max_entries_ = entries;
        store.max_bytes_ = bytes;
    }

    static void age(InMemoryAggregationCache& store, std::string_view key, std::int64_t millis) {
        if (auto it = store.entries_.find(key); it != store.entries_.end()) {
            it->second.stored_at =
                std::chrono::steady_clock::now() - std::chrono::milliseconds(millis);
        }
    }

    static auto timestamp(const InMemoryAggregationCache& store, std::string_view key) {
        return store.entries_.find(key)->second.stored_at;
    }

    static Vector<String> keys(Memory r, const InMemoryAggregationCache& store) {
        Vector<String> out(r);
        for (const auto& [key, entry] : store.entries_) {
            (void)entry;
            out.emplace_back(key);
        }
        return out;
    }

    static Vector<String> keys(Memory r, const InMemoryContinuousPageCursorStore& store) {
        Vector<String> out(r);
        for (const auto& [key, entry] : store.cursors_) {
            (void)entry;
            out.emplace_back(key);
        }
        return out;
    }

    static Vector<String> keys(Memory r, const InMemoryIdSetStore& store) {
        Vector<String> out(r);
        for (const auto& [key, entry] : store.sets_) {
            (void)entry;
            out.emplace_back(key);
        }
        return out;
    }

    static std::size_t bytes(const InMemoryIdSetStore& store) {
        return store.retained_bytes();
    }
};
}
