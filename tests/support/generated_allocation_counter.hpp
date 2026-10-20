#pragma once
#include <cstdlib>
#include <new>

// Include only once, after the runtime facade, in a test executable.
// GNU linker wrapping observes ContextArena's virtual Arena allocation entry.
// Test-only counters own no runtime data. Count requests before the arena bump
// allocator as well as system allocations; counting blocks alone hides churn.
struct Counts {
    std::size_t requests = 0;
    std::size_t bytes = 0;
    std::size_t heap_calls = 0;
    std::size_t heap_bytes = 0;
};

static Counts counts;
static bool measuring = false;

static void count_heap(std::size_t bytes) {
    if (measuring) {
        ++counts.heap_calls;
        counts.heap_bytes += bytes;
    }
}

void* operator new(std::size_t bytes) {
    count_heap(bytes);
    if (auto* result = std::malloc(bytes ? bytes : 1)) {
        return result;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t bytes) {
    return ::operator new(bytes);
}

void* operator new(std::size_t bytes, std::align_val_t alignment) {
    count_heap(bytes);
    void* result = nullptr;
    const auto requested = static_cast<std::size_t>(alignment);
    const auto effective = requested < sizeof(void*) ? sizeof(void*) : requested;
    if (posix_memalign(&result, effective, bytes ? bytes : 1) != 0) {
        throw std::bad_alloc();
    }
    return result;
}

void* operator new[](std::size_t bytes, std::align_val_t alignment) {
    return ::operator new(bytes, alignment);
}

void operator delete(void* pointer) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::align_val_t) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::align_val_t) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}

#if __SIZEOF_SIZE_T__ == 8
#define TEAQL_REAL_ARENA_ALLOC __real__ZN5teaql5Arena11do_allocateEmm
#define TEAQL_WRAP_ARENA_ALLOC __wrap__ZN5teaql5Arena11do_allocateEmm
#else
#define TEAQL_REAL_ARENA_ALLOC __real__ZN5teaql5Arena11do_allocateEjj
#define TEAQL_WRAP_ARENA_ALLOC __wrap__ZN5teaql5Arena11do_allocateEjj
#endif
extern "C" void* TEAQL_REAL_ARENA_ALLOC(teaql::Arena*, std::size_t, std::size_t);

extern "C" void*
TEAQL_WRAP_ARENA_ALLOC(teaql::Arena* arena, std::size_t bytes, std::size_t alignment) {
    if (measuring) {
        ++counts.requests;
        counts.bytes += bytes;
    }
    return TEAQL_REAL_ARENA_ALLOC(arena, bytes, alignment);
}

#undef TEAQL_REAL_ARENA_ALLOC
#undef TEAQL_WRAP_ARENA_ALLOC
