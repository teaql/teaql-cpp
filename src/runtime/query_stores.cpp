#include <teaql/runtime/cache.hpp>
#include <teaql/runtime/pagination.hpp>
#include <charconv>

namespace teaql {
std::optional<Vector<CompactRow>>
InMemoryAggregationCache::get(Memory r, std::string_view key, std::uint64_t age) const {
    auto it = entries_.find(key);
    if (it == entries_.end())
        return {};
    auto elapsed = std::chrono::steady_clock::now() - it->second.stored_at;
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    if (age && elapsed > decltype(elapsed)::zero() &&
        (std::uint64_t(millis.count()) > age ||
         (std::uint64_t(millis.count()) == age && elapsed > millis)))
        return {};
    return Vector<CompactRow>(it->second.rows, r);
}

void InMemoryAggregationCache::put(std::string_view key, std::span<const CompactRow> rows) {
    auto r = memory_resource();
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        Entry entry(std::allocator_arg, Allocator(r), rows);
        it->second.rows.swap(entry.rows);
        it->second.stored_at = entry.stored_at;
    } else
        entries_.emplace(
            std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(rows));
}

void InMemoryAggregationCache::invalidate_namespace(std::string_view name) {
    std::erase_if(entries_, [&](const auto& entry) {
        return entry.first.starts_with(name) &&
               std::string_view(entry.first).substr(name.size()).starts_with("::");
    });
}

namespace {
String cursor_key(Memory r, std::string_view query, std::uint64_t offset) {
    String key(query, r);
    key += ':';
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), offset).ptr;
    key.append(buffer, end);
    return key;
}

template <class Map>
void evict_earliest(Map& map) {
    auto oldest = std::min_element(map.begin(), map.end(), [](const auto& a, const auto& b) {
        return a.second.expires_at < b.second.expires_at;
    });
    if (oldest != map.end())
        map.erase(oldest);
}

std::size_t saturating_add(std::size_t a, std::size_t b) {
    return b > std::numeric_limits<std::size_t>::max() - a ? std::numeric_limits<std::size_t>::max()
                                                           : a + b;
}

std::size_t id_bytes(std::size_t count) {
    return count > std::numeric_limits<std::size_t>::max() / sizeof(std::uint64_t)
               ? std::numeric_limits<std::size_t>::max()
               : count * sizeof(std::uint64_t);
}
}

std::optional<ContinuousPageCursor>
InMemoryContinuousPageCursorStore::get_now(Memory r, std::string_view query, std::uint64_t offset) {
    auto key = cursor_key(r, query, offset);
    auto it = cursors_.find(key);
    if (it == cursors_.end())
        return {};
    if (it->second.expires_at <= SystemTime::now()) {
        cursors_.erase(it);
        return {};
    }
    return detail::clone_in(r, it->second);
}

void InMemoryContinuousPageCursorStore::put_now(const ContinuousPageCursor& cursor) {
    auto r = memory_resource();
    auto key = cursor_key(r, cursor.query_key, cursor.next_offset);
    auto copy = detail::clone_in(r, cursor);
    if (cursors_.size() >= max_entries_)
        evict_earliest(cursors_);
    cursors_.insert_or_assign(std::move(key), std::move(copy));
}

void InMemoryContinuousPageCursorStore::invalidate_now(std::string_view key) {
    std::erase_if(cursors_, [&](const auto& entry) {
        return entry.first.starts_with(key) &&
               std::string_view(entry.first).substr(key.size()).starts_with(':');
    });
}

std::optional<RetainedIdSet> InMemoryIdSetStore::get_now(Memory r, std::string_view key) {
    auto it = sets_.find(key);
    if (it == sets_.end())
        return {};
    if (it->second.expires_at <= SystemTime::now()) {
        sets_.erase(it);
        return {};
    }
    return detail::clone_in(r, it->second);
}

std::size_t InMemoryIdSetStore::retained_bytes() const {
    std::size_t total = 0;
    for (const auto& [key, value] : sets_) {
        (void)key;
        total += id_bytes(value.ids.size());
    }
    return total;
}

PaginationWriteResult InMemoryIdSetStore::put_now(Memory result, const RetainedIdSet& input) {
    auto incoming = id_bytes(input.ids.size());
    if (incoming > max_bytes_)
        return PaginationWriteResult::err(
            result, String("ID set exceeds the process-local store memory ceiling", result));
    std::erase_if(sets_, [](const auto& entry) {
        return entry.second.expires_at <= SystemTime::now();
    });
    while (sets_.size() >= max_entries_ ||
           saturating_add(retained_bytes(), incoming) > max_bytes_) {
        if (sets_.empty())
            break;
        evict_earliest(sets_);
    }
    sets_.insert_or_assign(String(input.query_key, memory_resource()),
                           detail::clone_in(memory_resource(), input));
    return PaginationWriteResult::ok(result, {});
}

void InMemoryIdSetStore::invalidate_now(std::string_view key) {
    auto it = sets_.find(key);
    if (it != sets_.end())
        sets_.erase(it);
}

namespace detail {
// These immediate in-memory operations are still lazy futures. Their state
// snapshots input in the result Context and borrows a store that outlives it.
struct CursorStoreOperation {
    using allocator_type = Allocator;
    ResourceBorrow<InMemoryContinuousPageCursorStore> store;
    String key;
    std::uint64_t offset = 0;
    std::optional<ContinuousPageCursor> cursor;

    CursorStoreOperation(std::allocator_arg_t,
                         Allocator a,
                         InMemoryContinuousPageCursorStore& store,
                         std::string_view key,
                         std::uint64_t offset,
                         const ContinuousPageCursor* cursor)
        : store(std::allocator_arg, a, store, resource_arena(store.memory_resource())),
          key(key, a.resource()), offset(offset) {
        if (cursor)
            this->cursor.emplace(std::allocator_arg, a, *cursor);
    }

    auto read() {
        return PaginationResult<std::optional<ContinuousPageCursor>>::ok(
            key.get_allocator().resource(),
            store.get().get_now(key.get_allocator().resource(), key, offset));
    }

    auto write() {
        if (cursor)
            store.get().put_now(*cursor);
        else
            store.get().invalidate_now(key);
        return PaginationWriteResult::ok(key.get_allocator().resource(), {});
    }
};

struct IdSetStoreOperation {
    using allocator_type = Allocator;
    ResourceBorrow<InMemoryIdSetStore> store;
    String key;
    std::optional<RetainedIdSet> ids;

    IdSetStoreOperation(std::allocator_arg_t,
                        Allocator a,
                        InMemoryIdSetStore& store,
                        std::string_view key,
                        const RetainedIdSet* ids)
        : store(std::allocator_arg, a, store, resource_arena(store.memory_resource())),
          key(key, a.resource()) {
        if (ids)
            this->ids.emplace(std::allocator_arg, a, *ids);
    }

    auto read() {
        return PaginationResult<std::optional<RetainedIdSet>>::ok(
            key.get_allocator().resource(),
            store.get().get_now(key.get_allocator().resource(), key));
    }

    auto write() {
        if (ids)
            return store.get().put_now(key.get_allocator().resource(), *ids);
        store.get().invalidate_now(key);
        return PaginationWriteResult::ok(key.get_allocator().resource(), {});
    }
};

template <class Operation, bool Read>
struct StoreOperation : Operation {
    using Operation::Operation;
    using R = decltype([](Operation& op) {
        if constexpr (Read)
            return op.read();
        else
            return op.write();
    }(std::declval<Operation&>()));

    std::optional<R> poll(WakeSignal&) {
        if constexpr (Read)
            return this->read();
        else
            return this->write();
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};
}

AsyncOperation<PaginationResult<std::optional<ContinuousPageCursor>>>&
InMemoryContinuousPageCursorStore::get(Memory r, std::string_view key, std::uint64_t offset) {
    return make_operation<PaginationResult<std::optional<ContinuousPageCursor>>,
                          detail::StoreOperation<detail::CursorStoreOperation, true>>(
        r, *this, key, offset, nullptr);
}

AsyncOperation<PaginationWriteResult>&
InMemoryContinuousPageCursorStore::put(Memory r, const ContinuousPageCursor& cursor) {
    return make_operation<PaginationWriteResult,
                          detail::StoreOperation<detail::CursorStoreOperation, false>>(
        r, *this, std::string_view{}, 0, &cursor);
}

AsyncOperation<PaginationWriteResult>&
InMemoryContinuousPageCursorStore::invalidate(Memory r, std::string_view key) {
    return make_operation<PaginationWriteResult,
                          detail::StoreOperation<detail::CursorStoreOperation, false>>(
        r, *this, key, 0, nullptr);
}

AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>&
InMemoryIdSetStore::get(Memory r, std::string_view key) {
    return make_operation<PaginationResult<std::optional<RetainedIdSet>>,
                          detail::StoreOperation<detail::IdSetStoreOperation, true>>(
        r, *this, key, nullptr);
}

AsyncOperation<PaginationWriteResult>& InMemoryIdSetStore::put(Memory r, const RetainedIdSet& ids) {
    return make_operation<PaginationWriteResult,
                          detail::StoreOperation<detail::IdSetStoreOperation, false>>(
        r, *this, std::string_view{}, &ids);
}

AsyncOperation<PaginationWriteResult>& InMemoryIdSetStore::invalidate(Memory r,
                                                                      std::string_view key) {
    return make_operation<PaginationWriteResult,
                          detail::StoreOperation<detail::IdSetStoreOperation, false>>(
        r, *this, key, nullptr);
}
}
