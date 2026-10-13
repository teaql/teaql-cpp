#include <teaql/core/eval.hpp>
#include <teaql/core/row.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& operation, const char* message) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, message);
}

static FieldLayout layout_value(Memory r, std::size_t count = 141) {
    std::vector<std::string> names;
    names.reserve(count);
    names.emplace_back("id");
    names.emplace_back("version");
    for (std::size_t i = 2; i < count; ++i) {
        names.push_back("field_" + std::to_string(i));
    }
    Vector<FieldLayout::Index> indexes(r);
    Vector<FieldLayout::Mapping> mappings(r);
    Vector<std::string_view> expected(r);
    for (std::size_t i = 0; i < count; ++i) {
        indexes.emplace_back(names[i], i);
        mappings.emplace_back(names[i], names[i], names[i]);
        expected.push_back(names[i]);
    }
    const std::array<std::string_view, 1> relations{"lines"};
    auto result =
        FieldLayout::from_generated(r, "Item", "v1", indexes, mappings, relations, expected);
    return std::move(result).value();
}

struct RegisteredItemLayout {
    static Result<FieldLayout, String> create_field_layout(Memory r) {
        return Result<FieldLayout, String>::ok(r, layout_value(r));
    }
};

static void runtime_layout_ownership() {
    GlobalContext global;
    auto next_user_context = global.make_user_context();
    const FieldLayout* metadata = nullptr;
    {
        auto user_context = global.make_user_context();
        auto& arena = detail::ContextArena::from_memory(user_context->memory_resource());
        metadata = &arena.field_layout<RegisteredItemLayout>();
        check(metadata->memory_resource() == global.memory_resource(),
              "type geometry belongs to the explicit Global Context");
        auto& next_arena = detail::ContextArena::from_memory(next_user_context->memory_resource());
        check(&next_arena.field_layout<RegisteredItemLayout>() == metadata,
              "all User Contexts reuse their application's immutable type geometry");
        auto& pool = arena.loaded_snapshots();
        check(pool.memory_resource() == user_context->memory_resource() &&
                  &pool == &arena.loaded_snapshots() && &pool != &next_arena.loaded_snapshots(),
              "snapshot pools are stable and isolated in their result Contexts");
        const std::array<std::string_view, 1> fields{"id"};
        LoadedSnapshot::projection(user_context->memory_resource(), *metadata, fields)
            .into_shared(pool);
    }
    check(metadata->index("field_129") == 129,
          "Global metadata remains valid after actual User deletion");
    auto& arena = detail::ContextArena::from_memory(next_user_context->memory_resource());
    auto snapshot = LoadedSnapshot::fully_loaded(next_user_context->memory_resource(), *metadata)
                        .into_shared(arena.loaded_snapshots());
    auto& global_arena = detail::ContextArena::from_memory(global.memory_resource());
    auto retained = global_arena.loaded_snapshots().copy(snapshot.get());
    check(&retained.get().layout() == metadata,
          "explicit Global retention preserves canonical Global type geometry");
    Arena raw(4096, std::pmr::new_delete_resource());
    rejects(
        [&] {
            detail::ContextArena::from_memory(&raw);
        },
        "generated entities cannot install implicit owners on generic arenas");
}

static void snapshot_geometry_and_copy_on_write() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& layout = user_context->make<FieldLayout>(layout_value(r));
    auto& pool = user_context->make<SnapshotPool>();
    const std::array<std::string_view, 8> selected{
        "id", "field_31", "field_63", "field_64", "field_129", "lines", "#custom", "_readonly"};
    auto first = LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
    auto second = LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
    check(first.identity() == second.identity(),
          "identical immutable geometry reuses one snapshot");
    check(first.get().bits() == ((std::uint64_t{1} << 63) | (std::uint64_t{1} << 31) | 1),
          "generated indexes drive exact fixed bits");
    check(first.get().overflow()->contains(64) && first.get().overflow()->contains(129),
          "wide slots use lazy overflow");
    check(first.get().is_loaded("lines") && first.get().is_loaded("#custom") &&
              !first.get().is_loaded("_readonly"),
          "relation and durable availability stay named");
    auto unchanged = LoadedSnapshot::with_loaded(pool, first, "id", true);
    check(unchanged && unchanged.value().identity() == first.identity(),
          "no-op preserves exact snapshot");
    auto before_inline = user_context->allocation_stats().issued_objects;
    auto inline_change = LoadedSnapshot::with_loaded(pool, first, "version", true);
    check(inline_change && inline_change.value().get().overflow() == first.get().overflow() &&
              inline_change.value().get().selected_names() == first.get().selected_names(),
          "fixed-bit changes share both immutable auxiliary nodes");
    check(user_context->allocation_stats().issued_objects == before_inline + 1,
          "an unseen fixed-bit shape registers only its snapshot, not new auxiliary nodes");
    auto changed = LoadedSnapshot::with_loaded(pool, first, "field_65", true);
    check(changed && changed.value().identity() != first.identity() &&
              changed.value().get().is_loaded("field_65") && !first.get().is_loaded("field_65"),
          "copy-on-write leaves sibling availability unchanged");
    auto undone = LoadedSnapshot::with_loaded(pool, changed.value(), "field_65", false);
    check(undone && undone.value().identity() == first.identity(),
          "prior geometry is interned again");
    auto invalid = LoadedSnapshot::with_loaded(pool, first, "_readonly", true);
    check(!invalid && invalid.error() == "unknown loadable field: Item._readonly",
          "derived fields acquire no slots");
    const std::array<std::string_view, 0> empty{};
    auto narrow = LoadedSnapshot::projection(r, layout, empty).into_shared(pool);
    check(!narrow.get().overflow() && !narrow.get().selected_names(),
          "empty geometry has no auxiliary sets");
    auto full = LoadedSnapshot::fully_loaded(r, layout).into_shared(pool);
    check(full.get().bits() == UINT64_MAX && full.get().overflow()->size() == 77 &&
              !full.get().is_loaded("lines") && !full.get().is_loaded("#custom"),
          "fully loaded includes fixed fields, not relationships or dynamic fields");
    auto indexed = eval::LoadState::partial(r, selected).into_indexed(pool, layout);
    check(indexed && indexed.value().indexed_snapshot()->identity() == first.identity(),
          "legacy state conversion interns");
    auto copied = detail::clone_in(r, indexed.value());
    check(copied.mark_loaded("field_65", &pool) && !indexed.value().is_loaded("field_65"),
          "indexed model state copies are isolated on mutation");
    check(bool(copied.mark_unloaded("field_65", &pool)), "indexed unloaded mutation");
    rejects(
        [&] {
            copied.to_json(r);
        },
        "internal masks are not client-serializable state");
    auto& foreign_layout = user_context->make<FieldLayout>(layout_value(r));
    auto mismatch = indexed.value().into_indexed(pool, foreign_layout);
    check(!mismatch && mismatch.error() == "incompatible loaded-state type or layout revision",
          "equal metadata from another identity cannot silently replace an installed layout");
}

static void destroyed_snapshot_hints_and_shared_nodes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& layout = user_context->make<FieldLayout>(layout_value(r));
    auto& pool = user_context->make<SnapshotPool>();
    const std::array<std::string_view, 2> selected{"field_129", "lines"};
    auto first = LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
    auto changed = LoadedSnapshot::with_loaded(pool, first, "id", true);
    check(changed && changed.value().get().overflow() == first.get().overflow(),
          "snapshot roots borrow independently managed auxiliary nodes");
    auto& arena = *dynamic_cast<Arena*>(r);
    arena.destroy(std::uint32_t(first.identity()), typeid(LoadedSnapshot), first.identity());
    rejects(
        [&] {
            first.get();
        },
        "explicitly destroyed snapshot borrow rejects");
    check(changed.value().get().is_loaded("field_129") && changed.value().get().is_loaded("lines"),
          "destroying an earlier snapshot does not destroy shared auxiliary payloads");
    auto refreshed = LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
    check(refreshed.identity() != first.identity() && refreshed.get().is_loaded("field_129"),
          "dead cache hints are removed and never resurrect destroyed snapshots");
    check(pool.hint_count(layout) == 2, "only live hints remain after miss cleanup");
}

static void bounded_hints_and_lifetimes() {
    GlobalContext global;
    std::optional<LoadedSnapshot::Borrow> surviving;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto layout_ref = user_context->make_ref<FieldLayout>(layout_value(r, 2));
    auto& layout = user_context->get(layout_ref);
    auto& pool = user_context->make<SnapshotPool>();
    for (std::size_t i = 0; i < 520; ++i) {
        auto name = "#selection_" + std::to_string(i);
        const std::array<std::string_view, 1> selected{name};
        auto state = LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
        if (i == 0) {
            surviving = state;
        }
    }
    check(pool.hint_count(layout) == 512 && surviving->get().is_loaded("#selection_0"),
          "cache admission saturates without invalidating held geometry");
    const std::array<std::string_view, 1> repeated{"#selection_0"};
    auto hit = LoadedSnapshot::projection(r, layout, repeated).into_shared(pool);
    check(hit.identity() == surviving->identity(), "live hits remain reusable after saturation");
    auto next_user_context = global.make_user_context();
    auto& next_pool = next_user_context->make<SnapshotPool>();
    rejects(
        [&] {
            LoadedSnapshot::with_loaded(next_pool, *surviving, "id", true);
        },
        "foreign composition requires explicit copy");
    std::string long_name = "#" + std::string(4096, 'n');
    const std::array<std::string_view, 1> long_selection{long_name};
    auto long_state = LoadedSnapshot::projection(r, layout, long_selection).into_shared(pool);
    auto long_copy = next_pool.copy(long_state.get());
    check(long_copy.get().selected_names()->begin()->get_allocator().resource() ==
              next_user_context->memory_resource(),
          "foreign named selections propagate destination string allocators");
    auto retained = next_pool.copy(surviving->get());
    user_context->destroy(layout_ref);
    rejects(
        [&] {
            surviving->get().is_loaded("id");
        },
        "destroyed source metadata is diagnosed");
    user_context.reset();
    rejects(
        [&] {
            surviving->get();
        },
        "actual User deletion is checked before touching arena memory");
    check(long_copy.get().is_loaded(long_name), "long named payload survives source deletion");
    check(retained.get().is_loaded("#selection_0") && retained.get().layout().entity() == "Item",
          "explicit destination copy retains metadata and selection after source deletion");
    surviving.reset();
}

static void row_shape_loading() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& layout = user_context->make<FieldLayout>(layout_value(r));
    auto& second_type = user_context->make<FieldLayout>(layout_value(r));
    auto& pool = user_context->make<SnapshotPool>();
    Vector<CompactRow> rows(r);
    for (std::int64_t id : {11, 22}) {
        Record values(r);
        values.emplace(String("id", r), Value(r, id));
        values.emplace(String("field_129", r), Value(r, id + 1));
        rows.emplace_back(std::move(values));
    }
    Record sparse(r);
    sparse.emplace(String("id", r), Value(r, std::int64_t{33}));
    rows.emplace_back(std::move(sparse));
    check(rows[0].shared_layout() != rows[1].shared_layout(), "independent row construction");
    CompactRow::share_layouts(rows);
    check(rows[0].shared_layout() == rows[1].shared_layout() &&
              rows[0].shared_layout() != rows[2].shared_layout(),
          "batch sharing preserves actual sparse and wide projections");
    auto first = rows[0].indexed_load_state(pool, layout);
    auto second = rows[1].indexed_load_state(pool, layout);
    check(first.indexed_snapshot()->identity() == second.indexed_snapshot()->identity() &&
              first.is_loaded("field_129") && !first.is_loaded("version"),
          "typed rows share availability with wide slots and missing fixed fields");
    auto other = rows[0].indexed_load_state(pool, second_type);
    auto repeated = rows[1].indexed_load_state(pool, second_type);
    check(other.indexed_snapshot()->identity() == repeated.indexed_snapshot()->identity() &&
              other.indexed_snapshot()->identity() != first.indexed_snapshot()->identity(),
          "one shape supports multiple exact model layouts without cache confusion");
    auto changed = first.mark_unloaded("field_129", &pool);
    check(changed && !first.is_loaded("field_129") && second.is_loaded("field_129"),
          "entity view changes do not mutate cached row availability");
    rows[0].insert("id", Value(r, std::int64_t{99}));
    check(rows[1].get("id")->try_i64() == 22, "shared availability never shares row values");
    auto sparse_state = rows[2].indexed_load_state(pool, layout);
    check(!sparse_state.is_loaded("field_129"), "sparse row remains sparse");
    auto original_shape = rows[0].shared_layout();
    rows[0].insert("version", Value(r, std::int64_t{1}));
    auto widened = rows[0].indexed_load_state(pool, layout);
    check(rows[0].shared_layout() != original_shape && widened.is_loaded("version") &&
              !rows[1].indexed_load_state(pool, layout).is_loaded("version"),
          "shape changes detach both columns and availability without changing earlier rows");
    auto next_user_context = global.make_user_context();
    auto next_r = next_user_context->memory_resource();
    auto& next_pool = next_user_context->make<SnapshotPool>();
    rejects(
        [&] {
            rows[0].indexed_load_state(next_pool, layout);
        },
        "foreign result pool requires explicit row copy");
    auto& retained_layout = next_user_context->make<FieldLayout>(layout);
    CompactRow copied(std::allocator_arg, Allocator(next_r), rows[0]);
    auto retained = copied.indexed_load_state(next_pool, retained_layout);
    check(retained.is_loaded("field_129") && copied.get("id")->try_i64() == 99,
          "copied rows own their shape and values in the destination Context");
    auto& independent = [&]() -> CompactRow& {
        auto source_user_context = global.make_user_context();
        auto source_r = source_user_context->memory_resource();
        Record values(source_r);
        values.emplace(String("field_129", source_r), Value(source_r, std::int64_t{17}));
        CompactRow source(std::allocator_arg, Allocator(source_r), std::move(values));
        source.indexed_load_state(source_user_context->make<SnapshotPool>(),
                                  source_user_context->make<FieldLayout>(layout_value(source_r)));
        return next_user_context->make<CompactRow>(source);
    }();
    auto independent_state = independent.indexed_load_state(next_pool, retained_layout);
    check(independent_state.is_loaded("field_129") && independent.get("field_129")->try_i64() == 17,
          "explicit copy survives actual source User deletion without retaining its cache");
}

static void row_relation_metadata() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& layout = user_context->make<FieldLayout>(layout_value(r));
    auto& pool = user_context->make<SnapshotPool>();
    auto& columns = user_context->make<ColumnLayout>(std::initializer_list<std::string_view>{"id"});
    Vector<CompactRow> rows(r);
    for (std::int64_t id : {1, 2, 3}) {
        Vector<Value> values(r);
        values.emplace_back(id);
        rows.emplace_back(columns, std::move(values));
    }
    RelationShapeCache cache(std::allocator_arg, Allocator(r));
    rows[0].mark_relation_loaded("lines", cache);
    rows[1].mark_relation_loaded("lines", cache);
    check(rows[0].shared_layout() == rows[1].shared_layout() && rows[0].len() == 1 &&
              rows[0].is_loaded_relation("lines") && !rows[2].is_loaded_relation("lines"),
          "relation metadata shares shapes without introducing row values or widening other views");
    CompactRow::share_layouts(rows);
    check(rows[0].shared_layout() != rows[2].shared_layout(),
          "shape sharing distinguishes equal columns with different relation availability");
    auto selected = rows[0].indexed_load_state(pool, layout);
    check(selected.is_loaded("lines") &&
              !rows[2].indexed_load_state(pool, layout).is_loaded("lines"),
          "typed projection includes only explicitly hydrated named relations");
    rows[0].insert("version", Value(r, std::int64_t{1}));
    rows[0].remove("version");
    check(rows[0].is_loaded_relation("lines"), "SQL column shape changes retain relation metadata");
    auto next_user_context = global.make_user_context();
    CompactRow copied(std::allocator_arg, Allocator(next_user_context->memory_resource()), rows[0]);
    check(copied.is_loaded_relation("lines") && copied.len() == 1,
          "destination shape copies preserve separate relation metadata");
}

static void readonly_schema() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array fields{DynamicPropertyDefinitions::Field{"_total", DataType::I64}};
    auto schema = DynamicPropertyDefinitions::from_fields(r, fields);
    check(schema && schema.value().validate_value("_total", Value(r, std::int64_t{0})),
          "zero is a valid readonly value");
    check(!schema.value().validate_value("_total", Value(r, false)),
          "schema rejects incompatible values");
    auto& definitions = user_context->make<DynamicPropertyDefinitions>(std::move(schema).value());
    auto& layout = user_context->make<FieldLayout>(layout_value(r, 2));
    auto& pool = user_context->make<SnapshotPool>();
    auto first = LoadedSnapshot::fully_loaded(r, layout).into_shared(pool);
    auto declared = LoadedSnapshot::with_dynamic_property_definitions(pool, first, definitions);
    check(declared.get().dynamic_property_definitions()->data_type("_total") == DataType::I64 &&
              !declared.get().is_loaded("_total") && declared.get().bits() == first.get().bits(),
          "schema declarations do not widen fixed availability or imply property presence");
    const std::array<std::string_view, 2> wide_selection{"field_129", "lines"};
    auto& wide_layout = user_context->make<FieldLayout>(layout_value(r));
    auto wide = LoadedSnapshot::projection(r, wide_layout, wide_selection).into_shared(pool);
    auto wide_schema = LoadedSnapshot::with_dynamic_property_definitions(pool, wide, definitions);
    check(wide_schema.get().overflow() == wide.get().overflow() &&
              wide_schema.get().selected_names() == wide.get().selected_names(),
          "schema-only changes share availability nodes");
    auto repeated = LoadedSnapshot::with_dynamic_property_definitions(pool, declared, definitions);
    check(repeated.identity() == declared.identity(),
          "unchanged readonly schema preserves snapshot identity");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        snapshot_geometry_and_copy_on_write();
        destroyed_snapshot_hints_and_shared_nodes();
        bounded_hints_and_lifetimes();
        readonly_schema();
        row_relation_metadata();
        row_shape_loading();
        runtime_layout_ownership();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "LoadedSnapshot and Indexed LoadState contracts passed\n";
}
