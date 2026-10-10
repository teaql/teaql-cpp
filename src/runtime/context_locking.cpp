#include <teaql/runtime/detail/context_locking.hpp>
#include <charconv>

namespace teaql::detail {
LockInstant LockInstant::now() {
    auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    auto seconds = std::chrono::floor<std::chrono::seconds>(elapsed);
    return {seconds.count(),
            std::uint32_t(
                std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed - seconds).count())};
}

LockInstant LockInstant::add_millis(std::uint64_t millis) const {
    auto increment = millis / 1000;
    auto ns = std::uint64_t(nanos) + (millis % 1000) * 1'000'000;
    increment += ns / 1'000'000'000;
    auto room = std::uint64_t(std::numeric_limits<std::int64_t>::max()) - std::uint64_t(seconds);
    if (increment > room) {
        throw std::overflow_error("local lock deadline overflow");
    }
    return {std::bit_cast<std::int64_t>(std::uint64_t(seconds) + increment),
            std::uint32_t(ns % 1'000'000'000)};
}

std::chrono::nanoseconds LockInstant::wait_from(LockInstant current) const {
    if (*this <= current) {
        return std::chrono::nanoseconds::zero();
    }
    // Bounded wait slices avoid chrono's native time_point overflow for leases
    // hundreds of millions of years long. Every wake rechecks the same deadline.
    auto whole = std::uint64_t(seconds) - std::uint64_t(current.seconds);
    if (whole > 1) {
        return std::chrono::seconds(1);
    }
    auto ns = std::int64_t(whole * 1'000'000'000) + nanos - current.nanos;
    return std::chrono::nanoseconds(std::min<std::int64_t>(ns, 1'000'000'000));
}

bool LocalLocks::acquire(std::string_view key,
                         std::uint64_t owner,
                         std::uint64_t timeout,
                         std::uint64_t expiry) {
    resource_arena(resource_).check_thread();
    auto deadline = LockInstant::now().add_millis(timeout);
    std::unique_lock lock(mutex_);
    for (;;) {
        auto now = LockInstant::now();
        auto current = entries_.find(key);
        if (current == entries_.end() || current->second.owner == owner ||
            (current->second.expiry && now >= *current->second.expiry)) {
            auto expires = expiry ? std::optional(now.add_millis(expiry)) : std::nullopt;
            if (current == entries_.end()) {
                entries_.emplace(String(key, resource_), Entry{owner, expires});
            } else {
                current->second = Entry{owner, expires};
            }
            return true;
        }
        if (!timeout || now >= deadline) {
            return false;
        }
        auto wake = current->second.expiry ? std::min(deadline, *current->second.expiry) : deadline;
        changed_.wait_for(lock, wake.wait_from(now));
    }
}

void LocalLocks::release(std::string_view key, std::uint64_t owner) {
    resource_arena(resource_).check_thread();
    std::lock_guard lock(mutex_);
    auto current = entries_.find(key);
    if (current != entries_.end() && current->second.owner == owner) {
        entries_.erase(current);
        changed_.notify_all();
    }
}

LocalLocks& ContextLockAccess::local(const UserContext& user) {
    user.check_public();
    auto& global = user.global_;
    if (!global.local_locks_) {
        global.local_locks_ = &global.make_resource<LocalLocks>();
    }
    return *global.local_locks_;
}

std::uint64_t ContextLockAccess::owner(const UserContext& user) {
    user.check_public();
    return (std::uint64_t(user.global_.runtime_identity_) << 20) | user.identity_;
}

const String& ContextLockAccess::remote_owner(const UserContext& user) {
    user.check_public();
    if (!user.remote_lock_owner_) {
        auto r = user.memory_resource();
        String token("teaql:", r);
        auto append = [&](auto value) {
            char bytes[32];
            auto end = std::to_chars(bytes, bytes + sizeof(bytes), value).ptr;
            token.append(bytes, end);
        };
#if defined(_WIN32)
        append(::_getpid());
#else
        append(::getpid());
#endif
        token += ':';
        append(owner(user));
        token += ':';
        auto created = user.remote_lock_created_at_;
        if (created.unix_seconds() > 0) {
            append(created.unix_seconds());
            char ns[9];
            auto rest = created.subsec_nanos();
            for (std::size_t i = 9; i != 0; --i) {
                ns[i - 1] = '0' + rest % 10;
                rest /= 10;
            }
            token.append(ns, 9);
        } else {
            append(created.unix_seconds() < 0 ? 0u : created.subsec_nanos());
        }
        user.remote_lock_owner_ =
            &const_cast<ContextArena&>(user.memory_).make_payload<String>(std::move(token));
    }
    return *user.remote_lock_owner_;
}

std::optional<ResourceBorrow<const RemoteLockProvider>>
ContextLockAccess::remote(const UserContext& user) {
    user.check_public();
    auto* entry = user.resources_ ? user.resources_->find<RemoteLockProvider>() : nullptr;
    if (!entry) {
        return {};
    }
    return ResourceBorrow<const RemoteLockProvider>(
        std::allocator_arg,
        Allocator(user.memory_resource()),
        *static_cast<RemoteLockProvider*>(entry->object),
        *entry->owner,
        entry->slot);
}

class RemoteLockOperation {
public:
    using allocator_type = Allocator;

    RemoteLockOperation(std::allocator_arg_t,
                        Allocator a,
                        const UserContext& user,
                        std::string_view key,
                        std::uint64_t timeout,
                        std::uint64_t expiry,
                        bool unlock)
        : user_(std::allocator_arg, a, user, resource_arena(a.resource())), key_(key, a.resource()),
          timeout_(timeout), expiry_(expiry), unlock_(unlock) {}

    std::optional<bool> poll(WakeSignal& wake) {
        auto& user = user_.get();
        if (!child_) {
            auto provider = ContextLockAccess::remote(user);
            if (!provider) {
                return true;
            }
            provider_.emplace(std::move(*provider));
            const auto& owner = ContextLockAccess::remote_owner(user);
            child_ = unlock_ ? &provider_->get().unlock_remote(user.memory_resource(), key_, owner)
                             : &provider_->get().try_remote_lock(
                                   user.memory_resource(), key_, owner, timeout_, expiry_);
            if (child_->memory_resource() != user.memory_resource()) {
                child_ = nullptr;
                throw std::invalid_argument("Remote lock result must belong to its UserContext");
            }
        }
        (void)provider_->get();
        forwarding_.attach(child_->wake_signal(), wake);
        auto* ready = child_->poll();
        if (!ready) {
            return {};
        }
        return *ready;
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
        }
    }

private:
    ResourceBorrow<const UserContext> user_;
    String key_;
    std::uint64_t timeout_, expiry_;
    bool unlock_;
    std::optional<ResourceBorrow<const RemoteLockProvider>> provider_;
    AsyncOperation<bool>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};
}

namespace teaql {
bool UserContext::try_local_lock(std::string_view key,
                                 std::uint64_t timeout,
                                 std::uint64_t expiry) const {
    check_public();
    Arena::Quiescence active;
    active.attach(detail::resource_arena(memory_resource()), nullptr, [](void*) noexcept {
        std::terminate();
    });
    return detail::ContextLockAccess::local(*this).acquire(
        key, detail::ContextLockAccess::owner(*this), timeout, expiry);
}

void UserContext::unlock_local(std::string_view key) const {
    check_public();
    Arena::Quiescence active;
    active.attach(detail::resource_arena(memory_resource()), nullptr, [](void*) noexcept {
        std::terminate();
    });
    if (global_.local_locks_) {
        global_.local_locks_->release(key, detail::ContextLockAccess::owner(*this));
    }
}

AsyncOperation<bool>& UserContext::try_remote_lock(std::string_view key,
                                                   std::uint64_t timeout,
                                                   std::uint64_t expiry) const {
    check_public();
    return make_operation<bool, detail::RemoteLockOperation>(
        memory_resource(), *this, key, timeout, expiry, false);
}

AsyncOperation<bool>& UserContext::unlock_remote(std::string_view key) const {
    check_public();
    return make_operation<bool, detail::RemoteLockOperation>(
        memory_resource(), *this, key, 0, 0, true);
}
}
