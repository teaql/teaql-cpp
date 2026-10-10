#pragma once
#include <teaql/runtime/business_id.hpp>
#include <teaql/runtime/id.hpp>
#include <teaql/runtime/reference_model_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const SystemBusinessClock&) {
    writer.raw("SystemBusinessClock");
}

inline void write_debug(DebugWriter& writer, const FixedBusinessClock& value) {
    writer.structure("FixedBusinessClock", [&](auto& fields) {
        fields.field("value", [&](auto& out) {
            write_debug(out, value.business_time());
        });
    });
}

inline void write_debug(DebugWriter& writer, BusinessDate value) {
    writer.tuple("BusinessDate", [&](auto& out) {
        write_debug(out, value.value);
    });
}

inline void write_debug(DebugWriter& writer, const DailySequenceBusinessIdProfile&) {
    writer.raw("DailySequenceBusinessIdProfile");
}

namespace detail {
// Like std::sync::Mutex Debug, diagnostics observe a lock without waiting for
// another holder. All scratch/output storage belongs to the caller's Context.
template <class WriteData>
void write_id_mutex(DebugWriter& writer, bool locked, bool poisoned, WriteData write_data) {
    String text(writer.memory_resource());
    DebugWriter nested(text, writer.options());
    nested.structure("Mutex", [&](auto& fields) {
        fields.field("data", [&](auto& out) {
            if (locked) {
                out.raw("<locked>");
            } else {
                write_data(out);
            }
        });
        fields.field("poisoned", [&](auto& out) {
            out.boolean(poisoned);
        });
    });
    if (writer.options().pretty) {
        text.insert(text.size() - 1, "    ..\n");
    } else {
        text.insert(text.size() - 2, ", ..");
    }
    writer.raw(text);
}
}

inline void write_debug(DebugWriter& writer, const AtomicCounterIdGenerator& value) {
    writer.structure("AtomicCounterIdGenerator", [&](auto& fields) {
        fields.field("counter", [&](auto& out) {
            out.integer(value.counter_.load(std::memory_order_relaxed));
        });
    });
}

inline void write_debug(DebugWriter& writer, const InMemoryBusinessIdAllocator& value) {
    if (value.owner_ != std::this_thread::get_id()) {
        throw std::logic_error(
            "Context Business ID allocator diagnostics require its owner thread");
    }
    std::unique_lock lock(value.mutex_, std::try_to_lock);
    writer.structure("InMemoryBusinessIdAllocator", [&](auto& fields) {
        fields.field("levels", [&](auto& out) {
            detail::write_id_mutex(out, !lock.owns_lock(), value.poisoned_, [&](auto& data) {
                data.sequence(
                    value.levels_,
                    [](auto& inner, const auto& entry) {
                        write_debug(inner, entry.first);
                        inner.raw(": ");
                        inner.integer(entry.second);
                    },
                    "{",
                    "}");
            });
        });
    });
}

inline void write_debug(DebugWriter& writer, const SnowflakeIdGenerator& value) {
    std::unique_lock lock(value.mutex_, std::try_to_lock);
    writer.structure("SnowflakeIdGenerator", [&](auto& fields) {
        fields.field("epoch_millis", [&](auto& out) {
            out.integer(std::uint64_t(1288834974657));
        });
        fields.field("worker_id", [&](auto& out) {
            out.integer(value.worker_);
        });
        fields.field("datacenter_id", [&](auto& out) {
            out.integer(value.datacenter_);
        });
        fields.field("state", [&](auto& out) {
            detail::write_id_mutex(out,
                                   !lock.owns_lock(),
                                   value.poisoned_.load(std::memory_order_relaxed),
                                   [&](auto& data) {
                                       data.structure("SnowflakeState", [&](auto& state) {
                                           state.field("last_timestamp", [&](auto& inner) {
                                               inner.integer(value.state_.last_timestamp);
                                           });
                                           state.field("sequence", [&](auto& inner) {
                                               inner.integer(value.state_.sequence);
                                           });
                                       });
                                   });
        });
    });
}
}
