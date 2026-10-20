#include <atomic>
#include <cstddef>
#include <cstdio>
#include <thread>
#include <teaql/runtime.hpp>
#include "../../tests/support/generated_allocation_counter.hpp"

static void wait_for(const std::atomic<int>& phase, int value) {
    while (phase.load(std::memory_order_acquire) < value) {
        std::this_thread::yield();
    }
}

static Counts
concurrent_sample(std::atomic<unsigned>& ready, std::size_t iterations, std::size_t bytes) {
    teaql::GlobalContext global;
    auto user_context = global.make_user_context();
    auto resource = user_context->memory_resource();
    counts = {};
    measuring = true;
    ready.fetch_add(1, std::memory_order_acq_rel);
    while (ready.load(std::memory_order_acquire) != 2) {
        std::this_thread::yield();
    }
    for (std::size_t i = 0; i < iterations; ++i) {
        (void)resource->allocate(bytes, 8);
        auto* allocation = ::operator new(bytes);
        ::operator delete(allocation);
        if (i % 7 == 0) {
            std::this_thread::yield();
        }
    }
    measuring = false;
    return counts;
}

static bool concurrent_contract() {
    for (auto iterations : {std::size_t{17}, std::size_t{1001}}) {
        std::atomic<unsigned> ready{0};
        Counts worker;
        std::thread thread([&] {
            worker = concurrent_sample(ready, iterations + 19, 29);
        });
        const auto parent = concurrent_sample(ready, iterations, 11);
        thread.join();
        if (parent.requests != iterations || parent.bytes != iterations * 11 ||
            worker.requests != iterations + 19 || worker.bytes != (iterations + 19) * 29 ||
            parent.heap_calls < iterations || worker.heap_calls < iterations + 19) {
            return false;
        }
    }
    return true;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    teaql::GlobalContext global;
    auto user_context = global.make_user_context();
    auto user_memory = user_context->memory_resource();
    auto global_memory = global.memory_resource();
    (void)user_memory->allocate(1024, 8);
    (void)global_memory->allocate(1024, 8);
    std::atomic<int> phase{0};
    Counts worker_result;
    bool worker_initially_disabled = false;
    std::thread worker([&] {
        teaql::GlobalContext worker_global;
        auto worker_user_context = worker_global.make_user_context();
        auto worker_user_memory = worker_user_context->memory_resource();
        auto worker_global_memory = worker_global.memory_resource();
        (void)worker_user_memory->allocate(1024, 8);
        (void)worker_global_memory->allocate(1024, 8);
        wait_for(phase, 1);
        worker_initially_disabled = !measuring && counts.heap_calls == 0;
        counts = {};
        measuring = true;
        auto* ordinary = ::operator new(29);
        auto* aligned = ::operator new(67, std::align_val_t{64});
        auto* arena = worker_user_memory->allocate(41, 8);
        auto* shared = worker_global_memory->allocate(59, 8);
        phase.store(2, std::memory_order_release);
        wait_for(phase, 3);
        ::operator delete(ordinary);
        ::operator delete(aligned, std::align_val_t{64});
        worker_user_memory->deallocate(arena, 41, 8);
        worker_global_memory->deallocate(shared, 59, 8);
        measuring = false;
        worker_result = counts;
        phase.store(4, std::memory_order_release);
    });
    counts = {};
    measuring = true;
    auto* ordinary = ::operator new(11);
    phase.store(1, std::memory_order_release);
    wait_for(phase, 2);
    const bool parent_untouched =
        measuring && counts.heap_calls == 1 && counts.heap_bytes == 11 && counts.requests == 0;
    auto* zero_size = ::operator new(0);
    auto* array = ::operator new[](23);
    auto* aligned = ::operator new[](37, std::align_val_t{128});
    auto* arena = user_memory->allocate(19, 8);
    auto* shared = global_memory->allocate(43, 8);
    ::operator delete(ordinary);
    ::operator delete(zero_size);
    ::operator delete[](array);
    ::operator delete[](aligned, std::align_val_t{128});
    user_memory->deallocate(arena, 19, 8);
    global_memory->deallocate(shared, 43, 8);
    measuring = false;
    const Counts parent_result = counts;
    phase.store(3, std::memory_order_release);
    wait_for(phase, 4);
    worker.join();
    auto* disabled = ::operator new(101);
    ::operator delete(disabled);
    bool passed = worker_initially_disabled && parent_untouched && parent_result.heap_calls == 4 &&
                  parent_result.heap_bytes == 71 && parent_result.requests == 2 &&
                  parent_result.bytes == 62 && worker_result.heap_calls == 2 &&
                  worker_result.heap_bytes == 96 && worker_result.requests == 2 &&
                  worker_result.bytes == 100 && counts.heap_calls == parent_result.heap_calls &&
                  counts.heap_bytes == parent_result.heap_bytes;
    passed = passed && concurrent_contract();
    std::puts(passed ? "REAL_CONTEXT_THREAD_ISOLATION_AND_POSITIVE_CONTROLS=passed" : "FAILED");
    return passed ? 0 : 1;
}
