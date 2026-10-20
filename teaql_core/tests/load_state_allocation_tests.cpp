#include <teaql/core.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>

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

using namespace teaql;

class CountingArena final : public Arena {
public:
    CountingArena() : Arena(4096, std::pmr::new_delete_resource(), nullptr, limits()) {}

private:
    static MemoryLimits limits() {
        MemoryLimits result;
        result.max_bytes = 1024 * 1024 * 1024;
        return result;
    }

    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (measuring) {
            ++counts.requests;
            counts.bytes += bytes;
        }
        return Arena::do_allocate(bytes, alignment);
    }
};

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class F>
static Counts
measure(std::string_view name, std::size_t width, std::size_t iterations, F&& action) {
    counts = {};
    const auto start = std::chrono::steady_clock::now();
    measuring = true;
    try {
        action();
    } catch (...) {
        measuring = false;
        throw;
    }
    measuring = false;
    const auto result = counts;
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - start);
    std::cout << name << ',' << width << ',' << iterations << ',' << result.requests << ','
              << result.bytes << ',' << result.heap_calls << ',' << result.heap_bytes << ','
              << elapsed.count() << '\n';
    return result;
}

static void no_allocations(Counts value) {
    check(value.requests == 0 && value.bytes == 0 && value.heap_calls == 0 && value.heap_bytes == 0,
          "Availability operation allocated temporary metadata");
}

struct Fixture {
    const FieldLayout& layout;
    CompactRow row;
};

static Fixture fixture(CountingArena& arena, std::size_t width) {
    Vector<String> names(&arena);
    Vector<FieldLayout::Index> indexes(&arena);
    Vector<FieldLayout::Mapping> mappings(&arena);
    Vector<std::string_view> members(&arena);
    names.reserve(width);
    for (std::size_t index = 0; index < width; ++index) {
        names.emplace_back(index == 0   ? "id"
                           : index == 1 ? "version"
                                        : "field_" + std::to_string(index));
    }
    for (std::size_t index = 0; index < width; ++index) {
        const std::string_view name = names[index];
        indexes.emplace_back(name, index);
        mappings.emplace_back(name, name, name);
        members.push_back(name);
    }
    auto result =
        FieldLayout::from_generated(&arena, "Probe", "v1", indexes, mappings, {}, members);
    check(bool(result), "Valid allocation fixture layout rejected");
    auto& layout = arena.make<FieldLayout>(std::move(result).value());
    auto& columns = arena.make<ColumnLayout>(std::move(names));
    Vector<Value> values(&arena);
    values.resize(width);
    return {layout, CompactRow(std::allocator_arg, Allocator(&arena), columns, std::move(values))};
}

static void instrumentation_controls() {
    CountingArena arena;
    const auto measured = measure("positive_control", 128, 1, [&] {
        (void)arena.allocate(128);
        auto* object = ::operator new(128);
        ::operator delete(object);
        auto* aligned = ::operator new(128, std::align_val_t(64));
        ::operator delete(aligned, std::align_val_t(64));
    });
    check(measured.requests > 0 && measured.bytes >= 128 && measured.heap_calls > 0 &&
              measured.heap_bytes >= 128,
          "Allocation counters missed real requests");
}

static void same_shape_enhancement() {
    for (auto width : {3u, 64u, 141u}) {
        for (auto size : {1u, 100u, 10000u}) {
            CountingArena arena;
            auto data = fixture(arena, width);
            Vector<CompactRow> rows(&arena);
            rows.reserve(size * 2);
            for (std::size_t index = 0; index < size * 2; ++index) {
                rows.emplace_back(data.row);
            }
            const auto* columns = data.row.shared_layout();
            const auto measured = measure("same_shape_merge", width, size, [&] {
                for (std::size_t index = 0; index < size; ++index) {
                    rows[index * 2].extend(rows[index * 2 + 1]);
                    check(rows[index * 2].shared_layout() == columns,
                          "Same-shape merge replaced column geometry");
                }
            });
            no_allocations(measured);
        }
    }
}

static void cold_projection() {
    for (auto width : {4u, 64u, 130u}) {
        std::optional<Counts> baseline;
        for (auto size : {1u, 100u, 10000u}) {
            CountingArena arena;
            auto data = fixture(arena, width);
            auto& pool = arena.make<SnapshotPool>();
            Vector<CompactRow> rows(&arena);
            Vector<eval::LoadState> states(&arena);
            rows.reserve(size);
            states.reserve(size);
            for (std::size_t index = 0; index < size; ++index) {
                rows.emplace_back(data.row);
            }
            const auto measured = measure("cold_projection", width, size, [&] {
                for (auto& row : rows) {
                    states.push_back(row.indexed_load_state(pool, data.layout));
                }
            });
            const auto identity = states.front().indexed_snapshot()->identity();
            for (const auto& state : states) {
                check(state.indexed_snapshot()->identity() == identity,
                      "Compatible cold rows did not share one snapshot");
                check(bool(state.indexed_snapshot()->get().overflow()) == (width > 64),
                      "Cold overflow geometry changed");
                for (const auto& name : data.row.keys()) {
                    check(state.is_loaded(name), "Cold snapshot omitted a fixed slot");
                }
            }
            check(measured.requests > 0, "Cold projection was already initialized");
            if (baseline) {
                check(measured.requests == baseline->requests && measured.bytes == baseline->bytes,
                      "Cold metadata allocation grew with row count");
            }
            baseline = measured;
        }
    }
}

static void flat_relation_shape() {
    std::optional<Counts> baseline;
    for (auto size : {1u, 100u, 10000u}) {
        CountingArena arena;
        auto data = fixture(arena, 4);
        Vector<CompactRow> rows(&arena);
        rows.reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            rows.emplace_back(data.row);
        }
        RelationShapeCache cache(std::allocator_arg, Allocator(&arena));
        const auto measured = measure("flat_edge_shape", 4, size, [&] {
            for (auto& row : rows) {
                row.mark_relation_loaded("child_list", cache);
            }
        });
        check(measured.requests <= 10, "Flat relation shape allocation exceeded its bounded cost");
        if (baseline) {
            check(measured.requests == baseline->requests && measured.bytes == baseline->bytes,
                  "Flat relation shape allocation grew with row count");
        }
        baseline = measured;
        for (const auto& row : rows) {
            check(row.shared_layout() == rows.front().shared_layout() && row.len() == 4 &&
                      row.is_loaded_relation("child_list"),
                  "Flat relation marker became a per-row value");
        }
    }
}

static void warm_availability() {
    for (auto width : {4u, 64u, 130u}) {
        CountingArena arena;
        auto data = fixture(arena, width);
        auto& pool = arena.make<SnapshotPool>();
        auto state = data.row.indexed_load_state(pool, data.layout);
        const auto base = *state.indexed_snapshot();
        const std::array fields{DynamicFieldDefinitions::Field{"note", DataType::Text}};
        auto result = DynamicFieldDefinitions::from_fields(&arena, "Probe", "extension-v1", fields);
        auto& definitions = arena.make<DynamicFieldDefinitions>(std::move(result).value());
        Value::Object payload(&arena);
        payload.emplace(String("note", &arena), Value(&arena));
        auto values_result = DynamicFieldValues::from_values(&arena, definitions, payload);
        auto& values = arena.make<DynamicFieldValues>(std::move(values_result).value());
        const auto selected = LoadedSnapshot::with_dynamic_fields(pool, base, values).value();
        for (auto iterations : {1u, 100u, 10000u}) {
            no_allocations(measure("cached_projection", width, iterations, [&] {
                for (std::size_t index = 0; index < iterations; ++index) {
                    auto next = data.row.indexed_load_state(pool, data.layout);
                    check(next.indexed_snapshot()->identity() == base.identity(),
                          "Warm row cache identity changed");
                }
            }));
            no_allocations(measure("loaded_fixed_noop", width, iterations, [&] {
                for (std::size_t index = 0; index < iterations; ++index) {
                    auto next = LoadedSnapshot::with_loaded(pool, base, "id", true);
                    check(next && next.value().identity() == base.identity(),
                          "No-op fixed update replaced state");
                }
            }));
            no_allocations(measure("loaded_dynamic_noop", width, iterations, [&] {
                for (std::size_t index = 0; index < iterations; ++index) {
                    auto next = LoadedSnapshot::with_dynamic_fields(pool, selected, values);
                    check(next && next.value().identity() == selected.identity(),
                          "No-op dynamic update replaced state");
                }
            }));
            measure("reference_dynamic_noop", width, iterations, [&] {
                for (std::size_t index = 0; index < iterations; ++index) {
                    LoadedSnapshot::SelectedNames names(&arena);
                    for (const auto& name : *selected.get().selected_names()) {
                        if (!name.starts_with('#')) {
                            names.emplace(name);
                        }
                    }
                    for (const auto& code : values.selected_codes()) {
                        String name("#", &arena);
                        name += code;
                        names.emplace(name);
                    }
                    check(names == *selected.get().selected_names(),
                          "Reference reconstruction accepted changed geometry");
                }
            });
        }
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        std::cout
            << "case,width,iterations,arena_requests,requested_bytes,heap_calls,heap_bytes,elapsed_ns\n";
        instrumentation_controls();
        same_shape_enhancement();
        cold_projection();
        flat_relation_shape();
        warm_availability();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
