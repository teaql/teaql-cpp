#pragma once
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/id_space.hpp>
#include <teaql/memory/function_ref.hpp>
#include <atomic>
#include <charconv>
#include <mutex>

namespace teaql {
class DebugWriter;

class InternalIdGenerator {
public:
    virtual ~InternalIdGenerator() = default;
    virtual Result<std::uint64_t, RuntimeError> generate_id(Memory,
                                                            std::string_view entity) const = 0;

    // A nonvirtual facade lets SQLite retain its inherent MutationExecutorError
    // method with the same name, while calls through this SPI use RuntimeError.
    Result<std::monostate, RuntimeError>
    ensure_floor(Memory r, std::string_view entity, std::uint64_t floor) const {
        return do_ensure_floor(r, entity, floor);
    }

protected:
    virtual Result<std::monostate, RuntimeError>
    do_ensure_floor(Memory r, std::string_view entity, std::uint64_t floor) const {
        String message("ID generator cannot reserve fixed bootstrap ID floor ", r);
        char buffer[32];
        auto end = std::to_chars(buffer, buffer + sizeof(buffer), floor).ptr;
        message.append(buffer, end);
        message += " for ";
        message += entity;
        return Result<std::monostate, RuntimeError>::err(r,
                                                         RuntimeError::id_generation(r, message));
    }
};

class AtomicCounterIdGenerator final : public InternalIdGenerator {
public:
    using allocator_type = Allocator;

    AtomicCounterIdGenerator(std::allocator_arg_t, Allocator a, std::uint64_t start = 1000)
        : resource_(a.resource()), counter_(start) {}

    AtomicCounterIdGenerator(const AtomicCounterIdGenerator&) = delete;

    Memory memory_resource() const noexcept {
        return resource_;
    }

    Result<std::uint64_t, RuntimeError> generate_id(Memory, std::string_view) const override;

protected:
    Result<std::monostate, RuntimeError>
        do_ensure_floor(Memory, std::string_view, std::uint64_t) const override;

private:
    friend void write_debug(DebugWriter&, const AtomicCounterIdGenerator&);
    Memory resource_;
    mutable std::atomic<std::uint64_t> counter_;
};

namespace detail {
// Private algorithm seam: the public generator always uses the system clock.
// Exposing state transitions here allows rollback/exhaustion tests without
// changing the machine clock or adding a runtime clock dependency to the SPI.
struct SnowflakeState {
    std::uint64_t last_timestamp = 0, sequence = 0;
};

using IdMillis = Result<std::uint64_t, RuntimeError>;
IdMillis advance_snowflake(Memory,
                           SnowflakeState&,
                           std::uint64_t worker,
                           std::uint64_t datacenter,
                           FunctionRef<IdMillis()> current,
                           FunctionRef<IdMillis(std::uint64_t)> wait_next);
}

class SnowflakeIdGenerator final : public InternalIdGenerator {
public:
    using allocator_type = Allocator;
    SnowflakeIdGenerator(std::allocator_arg_t,
                         Allocator,
                         std::uint64_t worker = 0,
                         std::uint64_t datacenter = 0);
    SnowflakeIdGenerator(const SnowflakeIdGenerator&) = delete;

    Memory memory_resource() const noexcept {
        return resource_;
    }

    Result<std::uint64_t, RuntimeError> generate_id(Memory, std::string_view) const override;

protected:
    Result<std::monostate, RuntimeError>
    do_ensure_floor(Memory r, std::string_view, std::uint64_t) const override {
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }

private:
    friend void write_debug(DebugWriter&, const SnowflakeIdGenerator&);
    Memory resource_;
    std::uint64_t worker_, datacenter_;
    mutable std::mutex mutex_;
    mutable detail::SnowflakeState state_;
    mutable std::atomic<bool> poisoned_{false};
};
}
