#include <teaql/core/eval.hpp>
#include <teaql/runtime/context.hpp>
#include "../../tests/support/generated_allocation_counter.hpp"
#include <array>
#include <chrono>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void sample(std::size_t hints) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array<FieldLayout::Index, 3> indexes{{{"id", 0}, {"version", 1}, {"name", 2}}};
    const std::array<FieldLayout::Mapping, 3> mappings{
        {{"id", "id", "id"}, {"version", "version", "version"}, {"name", "name", "name"}}};
    const std::array<std::string_view, 3> members{"id", "version", "name"};
    auto definition =
        FieldLayout::from_generated(r, "StateCacheCost", "v1", indexes, mappings, {}, members);
    check(bool(definition), "validate source-shaped layout");
    auto& layout = user_context->make<FieldLayout>(std::move(definition).value());
    auto& pool = user_context->make<SnapshotPool>();
    const std::array<std::string_view, 2> selected{"id", "name"};
    auto target = LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
    Vector<LoadedSnapshot::Borrow> held(r);
    held.reserve(hints - 1);
    for (std::size_t index = 1; index < hints; ++index) {
        String field("#retained_", r);
        field += std::to_string(index);
        const std::array<std::string_view, 2> other{"id", field};
        held.push_back(LoadedSnapshot::projection(r, layout, other).into_shared(pool));
    }
    check(held.size() == hints - 1 && pool.hint_count(layout) == hints,
          "retain the complete source cache population");
    for (unsigned index = 0; index < 100; ++index) {
        auto warm =
            LoadedSnapshot(std::allocator_arg, Allocator(r), target.get()).into_shared(pool);
        check(warm.identity() == target.identity(), "warm the actual interner lookup");
    }
    for (unsigned round = 0; round < 7; ++round) {
        counts = {};
        auto start = std::chrono::steady_clock::now();
        measuring = true;
        auto last = target;
        try {
            for (unsigned index = 0; index < 10000; ++index) {
                last = LoadedSnapshot(std::allocator_arg, Allocator(r), target.get())
                           .into_shared(pool);
            }
        } catch (...) {
            measuring = false;
            throw;
        }
        measuring = false;
        auto measured = counts;
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::steady_clock::now() - start)
                           .count();
        check(last.identity() == target.identity() && last.get().is_loaded("id") &&
                  last.get().is_loaded("name") && !last.get().is_loaded("version"),
              "cache hits preserve exact identity and loaded geometry");
        check(measured.requests == 0 && measured.bytes == 0 && measured.heap_calls == 0 &&
                  measured.heap_bytes == 0,
              "clone-and-intern cache hits allocate neither arena requests nor heap bytes");
        std::cout << "STATE_CACHE_HIT," << hints << ",10000," << round << ',' << measured.requests
                  << ',' << measured.bytes << ',' << measured.heap_calls << ','
                  << measured.heap_bytes << ',' << elapsed << '\n';
    }
    check(held.size() == hints - 1 && pool.hint_count(layout) == hints,
          "measured cache hits do not change the retained shape population");
}

int main() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    counts = {};
    measuring = true;
    (void)user_context->memory_resource()->allocate(128);
    (void)global.memory_resource()->allocate(64);
    auto* heap = ::operator new(128);
    ::operator delete(heap);
    auto* aligned = ::operator new(128, std::align_val_t(64));
    ::operator delete(aligned, std::align_val_t(64));
    measuring = false;
    check(counts.requests >= 2 && counts.bytes >= 192 && counts.heap_calls >= 2 &&
              counts.heap_bytes >= 256,
          "positive controls observe both Context scopes and ordinary/aligned heap allocations");
    for (std::size_t hints : {1, 64, 512}) {
        sample(hints);
    }
    std::cout << "STATE_CACHE_HIT_SAMPLES=21;READS_PER_SAMPLE=10000;POSITIVE_CONTROLS=passed\n";
}
