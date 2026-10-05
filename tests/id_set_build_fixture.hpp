#pragma once
#include <teaql/runtime/detail/context_pagination.hpp>
#include <teaql/runtime/detail/id_set_build_lock.hpp>

namespace teaql::detail {
struct IdSetBuildAccess {
    static bool same(const IdSetBuildLock& a, const IdSetBuildLock& b) {
        return a.entry_ == b.entry_ && &a.registry_.get() == &b.registry_.get();
    }

    static std::size_t registry_size(const IdSetBuildLock& lock) {
        return lock.registry_.get().entries_.size();
    }

    static std::size_t leases(const IdSetBuildLock& lock) {
        return static_cast<IdSetBuildRegistry::Entry*>(lock.entry_)->leases;
    }

    static Memory entry_memory(const IdSetBuildLock& lock) {
        return static_cast<IdSetBuildRegistry::Entry*>(lock.entry_)
            ->slots.get_allocator()
            .resource();
    }
};
}
