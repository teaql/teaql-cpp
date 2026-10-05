#include <teaql/runtime/id.hpp>
#include <chrono>
#include <thread>

namespace teaql {
Result<std::uint64_t, RuntimeError> AtomicCounterIdGenerator::generate_id(Memory r,
                                                                          std::string_view) const {
    auto previous = counter_.fetch_add(1, std::memory_order_relaxed);
#ifndef NDEBUG
    // Rust's fetch_add wraps before the checked '+ 1' panics in debug builds.
    if (previous == UINT64_MAX) {
        throw std::overflow_error("attempt to add with overflow");
    }
#endif
    return Result<std::uint64_t, RuntimeError>::ok(r, previous + std::uint64_t(1));
}

Result<std::monostate, RuntimeError>
AtomicCounterIdGenerator::do_ensure_floor(Memory r, std::string_view, std::uint64_t floor) const {
    auto observed = counter_.load(std::memory_order_relaxed);
    while (observed < floor &&
           !counter_.compare_exchange_weak(observed, floor, std::memory_order_relaxed)) {
    }
    return Result<std::monostate, RuntimeError>::ok(r, {});
}

detail::IdMillis detail::advance_snowflake(Memory r,
                                           SnowflakeState& state,
                                           std::uint64_t worker,
                                           std::uint64_t datacenter,
                                           FunctionRef<IdMillis()> current,
                                           FunctionRef<IdMillis(std::uint64_t)> wait_next) {
    auto observed = current();
    if (!observed) {
        return observed;
    }
    auto timestamp = observed.value();
    if (timestamp < state.last_timestamp) {
        observed = wait_next(state.last_timestamp);
        if (!observed) {
            return observed;
        }
        timestamp = observed.value();
    }
    if (timestamp == state.last_timestamp) {
        state.sequence = (state.sequence + 1) & 4095;
        if (state.sequence == 0) {
            observed = wait_next(state.last_timestamp);
            if (!observed) {
                return observed;
            }
            timestamp = observed.value();
        }
    } else {
        state.sequence = 0;
    }
    state.last_timestamp = timestamp;
    constexpr std::uint64_t epoch = 1288834974657;
    if (timestamp < epoch) {
        return IdMillis::err(
            r, RuntimeError::id_generation(r, "system clock is before snowflake epoch"));
    }
    return IdMillis::ok(
        r, ((timestamp - epoch) << 22) | (datacenter << 17) | (worker << 12) | state.sequence);
}

SnowflakeIdGenerator::SnowflakeIdGenerator(std::allocator_arg_t,
                                           Allocator a,
                                           std::uint64_t worker,
                                           std::uint64_t datacenter)
    : resource_(a.resource()), worker_(worker), datacenter_(datacenter) {
    if (worker > 31) {
        throw std::invalid_argument("worker id out of range");
    }
    if (datacenter > 31) {
        throw std::invalid_argument("datacenter id out of range");
    }
}

namespace {
detail::IdMillis current_millis(Memory r) {
    auto elapsed = std::chrono::system_clock::now().time_since_epoch();
    if (elapsed < decltype(elapsed)::zero()) {
        return detail::IdMillis::err(
            r, RuntimeError::id_generation(r, "second time provided was later than self"));
    }
    return detail::IdMillis::ok(
        r, std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));
}
}

Result<std::uint64_t, RuntimeError> SnowflakeIdGenerator::generate_id(Memory r,
                                                                      std::string_view) const {
    std::lock_guard lock(mutex_);
    if (poisoned_) {
        return detail::IdMillis::err(r, RuntimeError::id_generation(r, "snowflake state poisoned"));
    }
    try {
        auto current = [&] {
            return current_millis(r);
        };
        auto wait = [&](std::uint64_t last) {
            for (;;) {
                auto now = current();
                if (!now || now.value() > last) {
                    return now;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        };
        return detail::advance_snowflake(r, state_, worker_, datacenter_, current, wait);
    } catch (...) {
        poisoned_ = true;
        throw;
    }
}
}
