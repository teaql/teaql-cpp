#pragma once
#include <teaql/async/operation.hpp>
#include <teaql/async/stream.hpp>

namespace teaql::detail {
// Runtime check for Rust's borrow-before-consuming-self rule. Operations live
// in the same Context as their transaction; only pending ones still borrow it.
class PendingOperations {
public:
    explicit PendingOperations(Memory r) : entries_(r) {}

    template <class T>
    AsyncOperation<T>& track(AsyncOperation<T>& operation) {
        try {
            if (operation.memory_resource() != entries_.get_allocator().resource()) {
                throw std::invalid_argument("Transaction operation belongs to a different Context");
            }
            entries_.push_back({&operation,
                                [](const void* p) {
                                    return static_cast<const AsyncOperation<T>*>(p)->status() ==
                                           AsyncStatus::Pending;
                                },
                                [](void* p) noexcept {
                                    static_cast<AsyncOperation<T>*>(p)->cancel();
                                }});
        } catch (...) {
            operation.cancel();
            throw;
        }
        return operation;
    }

    template <class T>
    AsyncStream<T>& track(AsyncStream<T>& stream) {
        try {
            if (stream.memory_resource() != entries_.get_allocator().resource()) {
                throw std::invalid_argument("Transaction stream belongs to a different Context");
            }
            entries_.push_back({&stream,
                                [](const void* p) {
                                    return static_cast<const AsyncStream<T>*>(p)->status() ==
                                           StreamStatus::Open;
                                },
                                [](void* p) noexcept {
                                    static_cast<AsyncStream<T>*>(p)->cancel();
                                }});
        } catch (...) {
            stream.cancel();
            throw;
        }
        return stream;
    }

    bool any() const {
        for (const auto& entry : entries_) {
            if (entry.pending(entry.operation)) {
                return true;
            }
        }
        return false;
    }

    void cancel_all() noexcept {
        for (auto entry = entries_.rbegin(); entry != entries_.rend(); ++entry) {
            if (entry->pending(entry->operation)) {
                entry->cancel(entry->operation);
            }
        }
    }

private:
    struct Entry {
        void* operation;
        bool (*pending)(const void*);
        void (*cancel)(void*) noexcept;
    };

    Vector<Entry> entries_;
};
}
