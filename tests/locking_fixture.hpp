#pragma once
#include <teaql/runtime/context.hpp>

namespace lock_fixture {
using namespace teaql;

struct Call {
    using allocator_type = Allocator;
    String key, owner;
    std::uint64_t timeout, expiry;
    bool unlock;

    Call(std::allocator_arg_t,
         Allocator a,
         std::string_view key,
         std::string_view owner,
         std::uint64_t timeout,
         std::uint64_t expiry,
         bool unlock)
        : key(key, a.resource()), owner(owner, a.resource()), timeout(timeout), expiry(expiry),
          unlock(unlock) {}

    Call(std::allocator_arg_t, Allocator a, const Call& other)
        : Call(std::allocator_arg,
               a,
               other.key,
               other.owner,
               other.timeout,
               other.expiry,
               other.unlock) {}

    Call(const Call&) = delete;
    Call(Call&&) noexcept = default;
};

struct Provider final : RemoteLockProvider {
    using allocator_type = Allocator;
    Memory r;
    mutable std::pmr::map<String, String, std::less<>> owners;
    mutable Vector<Call> calls;
    bool pending = false;
    mutable unsigned cancelled = 0;
    mutable WakeSignal* wake = nullptr;

    Provider(std::allocator_arg_t, Allocator a) : r(a.resource()), owners(r), calls(r) {}

    Memory memory_resource() const {
        return r;
    }

    struct Work {
        using allocator_type = Allocator;
        ResourceBorrow<const Provider> provider;
        String key, owner;
        std::uint64_t timeout, expiry;
        bool unlock, noted = false;

        Work(std::allocator_arg_t,
             Allocator a,
             const Provider& p,
             std::string_view key,
             std::string_view owner,
             std::uint64_t timeout,
             std::uint64_t expiry,
             bool unlock)
            : provider(std::allocator_arg, a, p, detail::resource_arena(p.r)),
              key(key, a.resource()), owner(owner, a.resource()), timeout(timeout), expiry(expiry),
              unlock(unlock) {}

        std::optional<bool> poll(WakeSignal& signal) {
            auto& p = provider.get();
            if (!noted) {
                p.calls.emplace_back(key, owner, timeout, expiry, unlock);
                noted = true;
            }
            p.wake = &signal;
            if (p.pending) {
                return {};
            }
            auto found = p.owners.find(key);
            if (unlock) {
                if (found != p.owners.end() && found->second == owner) {
                    p.owners.erase(found);
                    return true;
                }
                return false;
            }
            if (found != p.owners.end()) {
                return false;
            }
            p.owners.emplace(String(key, p.r), String(owner, p.r));
            return true;
        }

        void finish_and_wait() noexcept {
            provider.get().wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            auto& p = provider.get();
            p.wake = nullptr;
            ++p.cancelled;
        }
    };

    AsyncOperation<bool>& try_remote_lock(Memory r,
                                          std::string_view key,
                                          std::string_view owner,
                                          std::uint64_t timeout,
                                          std::uint64_t expiry) const override {
        return make_operation<bool, Work>(r, *this, key, owner, timeout, expiry, false);
    }

    AsyncOperation<bool>&
    unlock_remote(Memory r, std::string_view key, std::string_view owner) const override {
        return make_operation<bool, Work>(r, *this, key, owner, 0, 0, true);
    }
};
}
