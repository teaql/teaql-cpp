#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/core/query_debug.hpp>
#include <teaql/core/system_time_debug.hpp>
#include <teaql/runtime/cache.hpp>
#include <teaql/runtime/pagination.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const ContinuousPageCursor& value) {
    writer.structure("ContinuousPageCursor", [&](auto& fields) {
        fields.field("cursor_id", [&](auto& out) {
            write_debug(out, value.cursor_id);
        });
        fields.field("query_key", [&](auto& out) {
            write_debug(out, value.query_key);
        });
        fields.field("entity", [&](auto& out) {
            write_debug(out, value.entity);
        });
        fields.field("direction", [&](auto& out) {
            write_debug(out, value.direction);
        });
        fields.field("boundary", [&](auto& out) {
            write_debug(out, value.boundary);
        });
        fields.field("page_size", [&](auto& out) {
            out.integer(value.page_size);
        });
        fields.field("next_offset", [&](auto& out) {
            out.integer(value.next_offset);
        });
        fields.field("expires_at", [&](auto& out) {
            write_debug(out, value.expires_at);
        });
    });
}

inline void write_debug(DebugWriter& writer, const RetainedIdSet& value) {
    writer.structure("RetainedIdSet", [&](auto& fields) {
        fields.field("query_key", [&](auto& out) {
            write_debug(out, value.query_key);
        });
        fields.field("ids", [&](auto& out) {
            out.sequence(value.ids, [](auto& inner, std::uint64_t id) {
                inner.integer(id);
            });
        });
        fields.field("expires_at", [&](auto& out) {
            write_debug(out, value.expires_at);
        });
    });
}

namespace detail {
inline void write_cache_instant(DebugWriter& writer, std::chrono::steady_clock::time_point time) {
    auto total = time.time_since_epoch();
    auto seconds = std::chrono::floor<std::chrono::seconds>(total);
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(total - seconds);
    writer.structure("Instant", [&](auto& fields) {
        fields.field("tv_sec", [&](auto& out) {
            out.integer(seconds.count());
        });
        fields.field("tv_nsec", [&](auto& out) {
            out.integer(nanos.count());
        });
    });
}
}

inline void write_debug(DebugWriter& writer, const InMemoryAggregationCache& value) {
    writer.structure("InMemoryAggregationCache", [&](auto& fields) {
        fields.field("namespace", [&](auto& out) {
            write_debug(out, value.namespace_);
        });
        fields.field("entries", [&](auto& out) {
            // Context storage is owner-thread confined. Preserve the source
            // Mutex diagnostic structure without adding a shared Context owner.
            String text(out.memory_resource());
            DebugWriter nested(text, out.options());
            nested.structure("Mutex", [&](auto& mutex) {
                mutex.field("data", [&](auto& data) {
                    data.sequence(
                        value.entries_,
                        [](auto& inner, const auto& entry) {
                            write_debug(inner, entry.first);
                            inner.raw(": ");
                            inner.structure("AggregationCacheEntry", [&](auto& fields) {
                                fields.field("stored_at", [&](auto& instant) {
                                    detail::write_cache_instant(instant, entry.second.stored_at);
                                });
                                fields.field("rows", [&](auto& rows) {
                                    rows.sequence(entry.second.rows,
                                                  [](auto& row, const auto& value) {
                                                      write_debug(row, value);
                                                  });
                                });
                            });
                        },
                        "{",
                        "}");
                });
                mutex.field("poisoned", [&](auto& flag) {
                    flag.boolean(false);
                });
            });
            if (out.options().pretty) {
                text.insert(text.size() - 1, "    ..\n");
            } else {
                text.insert(text.size() - 2, ", ..");
            }
            out.raw(text);
        });
    });
}
}
