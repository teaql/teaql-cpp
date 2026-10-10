#pragma once
#include <teaql/async/operation.hpp>

namespace teaql {
// Provider-neutral optional boundary. Providers must release a key only while
// the supplied owner token still owns it. No cloud implementation is included.
class RemoteLockProvider {
public:
    virtual ~RemoteLockProvider() = default;
    virtual AsyncOperation<bool>& try_remote_lock(Memory,
                                                  std::string_view key,
                                                  std::string_view owner_token,
                                                  std::uint64_t timeout_millis,
                                                  std::uint64_t expire_millis) const = 0;
    virtual AsyncOperation<bool>&
    unlock_remote(Memory, std::string_view key, std::string_view owner_token) const = 0;
};

namespace detail {
class LocalLocks;
struct ContextLockAccess;
}
}
