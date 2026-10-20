#include "../generator/parity/hydration_allocation_contracts/cpp/include/cpp_hydration_allocation.hpp"
#include "../support/generated_allocation_counter.hpp"
#include <chrono>
#include <iostream>

using namespace teaql;
using Probe = cpp_hydration_allocation::HydrationProbe;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Plain {
    std::uint64_t id;
    std::int64_t version;
    String name;
    std::optional<String> note;
};

static Plain decode_plain(Memory resource, const CompactRow& row) {
    const auto* id = row.get("id");
    const auto* version = row.get("version");
    const auto* name = row.get("name");
    const auto* note = row.get("note");
    check(id && std::holds_alternative<std::uint64_t>(id->data), "bad ID");
    check(version && std::holds_alternative<std::int64_t>(version->data), "bad version");
    check(name && std::holds_alternative<String>(name->data), "bad name");
    std::optional<String> decoded_note;
    if (note && !std::holds_alternative<std::monostate>(note->data)) {
        check(std::holds_alternative<String>(note->data), "bad note");
        decoded_note.emplace(std::get<String>(note->data), resource);
    }
    return {std::get<std::uint64_t>(id->data),
            std::get<std::int64_t>(version->data),
            String(std::get<String>(name->data), resource),
            std::move(decoded_note)};
}

static void plain_helper_contracts() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto resource = user_context->memory_resource();
    auto row = [&](Value id, Value version, Value name, std::optional<Value> note) {
        Record fields(resource);
        fields.emplace(String("id", resource), std::move(id));
        fields.emplace(String("version", resource), std::move(version));
        fields.emplace(String("name", resource), std::move(name));
        if (note) {
            fields.emplace(String("note", resource), std::move(*note));
        }
        return CompactRow(std::allocator_arg, Allocator(resource), std::move(fields));
    };
    for (unsigned variant = 0; variant < 4; ++variant) {
        std::optional<Value> note;
        if (variant == 1) {
            note.emplace(resource, nullptr);
        } else if (variant == 2) {
            note.emplace(resource, "");
        } else if (variant == 3) {
            note.emplace(resource, "A long note that does not fit in small-string storage");
        }
        auto input = row(Value(resource, std::uint64_t{7}),
                         Value(resource, std::int64_t{3}),
                         Value(resource, "sample name"),
                         std::move(note));
        auto decoded = decode_plain(resource, input);
        check(decoded.id == 7 && decoded.version == 3 && decoded.name == "sample name",
              "plain values");
        check(decoded.note.has_value() == (variant >= 2), "NULL and empty Text differ");
        if (decoded.note) {
            check(*decoded.note == *input.get("note")->try_text() &&
                      decoded.note->get_allocator().resource() == resource,
                  "note copied to destination");
        }
    }
    unsigned rejected = 0;
    for (unsigned variant = 0; variant < 4; ++variant) {
        auto input =
            row(variant == 0 ? Value(resource, std::int64_t{7}) : Value(resource, std::uint64_t{7}),
                variant == 1 ? Value(resource, std::uint64_t{3}) : Value(resource, std::int64_t{3}),
                variant == 2 ? Value(resource, std::int64_t{1}) : Value(resource, "sample name"),
                variant == 3 ? std::optional<Value>(std::in_place, resource, std::int64_t{1})
                             : std::nullopt);
        try {
            (void)decode_plain(resource, input);
        } catch (const std::runtime_error&) {
            ++rejected;
        }
    }
    check(rejected == 4, "exact source variant rejection");
    std::cout << "PLAIN_HYDRATION_HELPER=4 accepted/4 rejected\n";
}

static Vector<CompactRow> rows(Memory r, const ColumnLayout& layout, std::size_t count, bool full) {
    Vector<CompactRow> result(r);
    result.reserve(count);
    for (std::size_t id = 1; id <= count; ++id) {
        Vector<Value> values(r);
        values.emplace_back(static_cast<std::uint64_t>(id));
        values.emplace_back(std::int64_t{1});
        values.emplace_back("sample name");
        if (full) {
            values.emplace_back(nullptr);
        }
        result.emplace_back(layout, std::move(values));
    }
    return result;
}

template <class Action>
static auto measured(std::string_view path, bool full, std::size_t size, Action action) {
    counts = {};
    auto start = std::chrono::steady_clock::now();
    measuring = true;
    auto output = [&] {
        try {
            return action();
        } catch (...) {
            measuring = false;
            throw;
        }
    }();
    measuring = false;
    const auto observed = counts;
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    std::cout << path << ',' << (full ? "full" : "sparse") << ',' << size << ','
              << observed.requests << ',' << observed.bytes << ',' << observed.heap_calls << ','
              << observed.heap_bytes << ',' << elapsed << std::endl;
    if (path != "plain_hydration") {
        const auto ceiling = path == "context_hydration" ? size * 2 + 1
                             : path == "identity_batch"  ? size * 3 + 32
                                                         : size * 3 + 1;
        check(observed.requests <= ceiling && observed.heap_calls <= ceiling,
              "both arena requests and heap calls satisfy the unchanged source ceiling");
    }
    return output;
}

static void sample(bool full) {
    ContextLimits limits;
    limits.user.max_bytes = 512ull * 1024 * 1024;
    limits.max_live_user_bytes = limits.user.max_bytes;
    GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& layout = full ? user_context->make<ColumnLayout>(std::initializer_list<std::string_view>{
                              "id", "version", "name", "note"})
                        : user_context->make<ColumnLayout>(
                              std::initializer_list<std::string_view>{"id", "version", "name"});
    auto warm_inputs = rows(r, layout, 1, full);
    auto warmed = Probe::from_compact_row(r, std::move(warm_inputs.front()));
    check(bool(warmed), "warm typed hydration");
    auto snapshot = warmed.value().loaded_state_snapshot();
    check(bool(snapshot), "warm indexed snapshot");
    const auto& generated_layout = Probe::field_layout(r);
    check(&snapshot->get().layout() == &generated_layout &&
              generated_layout.shared_entity_name().data() ==
                  Probe::field_layout(r).shared_entity_name().data(),
          "warmed snapshot retains the shared generated layout and entity-name storage");
    auto& root = user_context->make<EntityRuntimeState>();
    for (std::size_t size : {1, 100, 10000}) {
        auto inspect = [&](const auto& entities) {
            check(entities.size() == size, "complete output batch");
            for (std::size_t index = 0; index < size; ++index) {
                const auto& entity = entities[index];
                check(entity.id() == index + 1 && entity.version() == 1 &&
                          entity.name() == "sample name" && !entity.note(),
                      "hydrated scalar values");
                check(entity.is_field_loaded("note") == full &&
                          entity.loaded_state_snapshot()->identity() == snapshot->identity(),
                      "shared snapshot and exact note availability");
                auto dirty = entity.dirty_fields();
                check(!dirty || dirty->empty(), "hydration is clean");
            }
        };
        for (bool contextual : {false, true}) {
            auto inputs = rows(r, layout, size, full);
            auto entities =
                measured(contextual ? "context_hydration" : "typed_hydration", full, size, [&] {
                    Vector<Probe> result(r);
                    result.reserve(size);
                    for (auto& row : inputs) {
                        auto decoded = contextual ? Probe::from_compact_row_with_context(
                                                        r, std::move(row), AnyRef(root))
                                                  : Probe::from_compact_row(r, std::move(row));
                        check(bool(decoded), "typed decode succeeds");
                        result.push_back(std::move(decoded).value());
                    }
                    return result;
                });
            inspect(entities);
            if (contextual && size > 1) {
                entities[0].set_comment("first row only");
                check(entities[0].get_comment() == "first row only" && !entities[1].get_comment(),
                      "comments remain private");
            }
        }
        auto inputs = rows(r, layout, size, full);
        auto& graph_root = user_context->make<EntityRuntimeState>();
        auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
        registry.register_entity<Probe>();
        auto builder = measured("identity_batch", full, size, [&] {
            EntityGraphBuilder result(std::allocator_arg, Allocator(r));
            check(bool(registry.decode_compact_batch(
                      "HydrationProbe", std::move(inputs), graph_root, result)),
                  "graph batch decode succeeds");
            return result;
        });
        check(bool(graph_root.freeze_graph(std::move(builder))), "freeze after measurement");
        for (auto id : {std::size_t{1}, size}) {
            const auto* entity = graph_root.resolve_entity<Probe>(id);
            check(entity && entity->loaded_state_snapshot()->identity() == snapshot->identity(),
                  "graph snapshots share warmed identity");
        }

        inputs = rows(r, layout, size, full);
        auto plain = measured("plain_hydration", full, size, [&] {
            Vector<Plain> result(r);
            result.reserve(size);
            for (const auto& row : inputs) {
                result.push_back(decode_plain(r, row));
            }
            return result;
        });
        for (std::size_t index = 0; index < size; ++index) {
            check(plain[index].id == index + 1 && plain[index].version == 1 &&
                      plain[index].name == "sample name" && !plain[index].note,
                  "plain values");
        }
    }
}

static void move_contract() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& layout = user_context->make<ColumnLayout>(
        std::initializer_list<std::string_view>{"id", "version", "name", "note"});
    const std::string_view payload =
        "A long hydration name that cannot fit in small string storage";
    Probe* retained = nullptr;
    {
        Vector<Value> values(r);
        values.emplace_back(std::uint64_t{7});
        values.emplace_back(std::int64_t{3});
        values.emplace_back(payload);
        values.emplace_back(nullptr);
        CompactRow input(std::allocator_arg, Allocator(r), layout, std::move(values));
        const auto* original_buffer = input.values().data();
        auto decoded = Probe::from_compact_row(r, std::move(input));
        check(bool(decoded), "move fixture hydration");
        check(detail::EntityRuntimeAccess::original_row(decoded.value().root)->values().data() ==
                  original_buffer,
              "generated decoder transfers the native original buffer");
        auto& source = decoded.value();
        source.set_comment("retained comment");
        auto snapshot = source.loaded_state_snapshot()->identity();
        counts = {};
        measuring = true;
        auto moved = std::make_obj_using_allocator<Probe>(Allocator(r), std::move(source));
        measuring = false;
        check(counts.requests == 0 && counts.heap_calls == 0,
              "same Context move allocates nothing");
        check(moved.name() == payload && moved.id() == 7 && moved.version() == 3 &&
                  moved.loaded_state_snapshot()->identity() == snapshot &&
                  moved.get_comment() == "retained comment",
              "same Context move retains complete state");
        retained = &global.make<Probe>(std::move(moved));
        check(retained->memory_resource() == global.memory_resource(),
              "cross Context destination owns copy");
        check(moved.name() == payload, "cross Context move uses non-destructive deep copy");
    }
    user_context.reset();
    check(retained->name() == payload && retained->id() == 7 && retained->version() == 3 &&
              !retained->note() && retained->is_field_loaded("note") &&
              retained->get_comment() == "retained comment" &&
              retained->original_values().has_value(),
          "Global retained state survives actual source Context deletion");
    std::cout << "HYDRATION_MOVE_CONTRACT=passed;ORIGINAL_ROW_TRANSFER=passed\n";
}

int main() {
    std::cout
        << "case,projection,rows,arena_requests,arena_bytes,heap_calls,heap_bytes,elapsed_ns\n";
    {
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
              "allocation positive controls");
    }
    plain_helper_contracts();
    move_contract();
    sample(false);
    sample(true);
    std::cout
        << "HYDRATION_SAMPLES=24;POSITIVE_CONTROLS=passed;BUDGET_CEILINGS=passed;SOURCE_ACCEPTED=false\n";
}
