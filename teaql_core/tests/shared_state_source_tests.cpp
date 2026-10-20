#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static DynamicFieldDefinitions definitions(Memory r) {
    const std::array fields{DynamicFieldDefinitions::Field{"note", DataType::Text},
                            DynamicFieldDefinitions::Field{"extra", DataType::Text},
                            DynamicFieldDefinitions::Field{"number", DataType::I64},
                            DynamicFieldDefinitions::Field{"active", DataType::Bool}};
    return std::move(DynamicFieldDefinitions::from_fields(r, "School", "dynamic-v1", fields))
        .value();
}

static void binding_dictionary() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const auto original_ref = user_context->make_ref<DynamicFieldDefinitions>(definitions(r));
    const auto& original = user_context->get(original_ref);
    const auto* dictionary = &original.fields();
    const auto bound = original.with_storage_binding_shared(r, "PRIVATE-PROFILE-CANARY", 17);
    const auto repeated = bound.get().with_storage_binding_shared(r, "PRIVATE-PROFILE-CANARY", 17);
    check(bound.identity() == repeated.identity(),
          "Equal storage binding replaced metadata identity");
    check(dictionary == &bound.get().fields(),
          "Storage binding copied the immutable field dictionary");
    check(original.same_schema(bound.get()) && original != bound.get(),
          "Storage provenance was conflated with logical schema");
    auto other = original.with_storage_binding_shared(r, "other", 17);
    auto executor = original.with_storage_binding_shared(r, "PRIVATE-PROFILE-CANARY", 18);
    check(bound.get() != other.get() && bound.get() != executor.get(),
          "Profile and executor must independently participate in equality");
    check(debug_string(r, bound.get()).find("PRIVATE-PROFILE-CANARY") == String::npos,
          "Opaque storage binding leaked through diagnostics");
    auto& retained = global.make<DynamicFieldDefinitions>(bound.get());
    check(&retained.fields() != dictionary &&
              retained.fields().get_allocator().resource() == global.memory_resource(),
          "Destination retention did not copy the field dictionary into Global");
    user_context->destroy(original_ref);
    check(bound.get().fields().contains("note"),
          "Destroying one definition destroyed another definition's shared dictionary");
    user_context.reset();
    check(retained.fields().contains("note") &&
              retained.storage_binding_matches("PRIVATE-PROFILE-CANARY", 17),
          "Explicit Global dictionary copy expired with its source UserContext");
}

static void query_selection() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array fields{DynamicFieldSelection::Field{"note", DataType::Text}};
    auto original_ref = user_context->make_ref<SelectQuery>("School");
    auto& original = user_context->get(original_ref);
    {
        auto selected = DynamicFieldSelection::fields(r, fields);
        original.select_dynamic_fields(selected.value());
    }
    auto& clone = user_context->make<SelectQuery>(original);
    check(original.dynamic_field_selection.get() == clone.dynamic_field_selection.get(),
          "Same-Context query clone copied immutable dynamic selection");
    clone.select_dynamic_fields(DynamicFieldSelection::all(r));
    check(original.dynamic_field_selection.get() != clone.dynamic_field_selection.get() &&
              !original.dynamic_field_selection->contains("extra") &&
              clone.dynamic_field_selection->contains("extra"),
          "Replacing one query selection changed another query");
    auto& retained = global.make<SelectQuery>(original);
    auto& retained_in_context = user_context->make<SelectQuery>(original);
    const auto handle_bytes = sizeof(original.dynamic_field_selection);
    check(retained.dynamic_field_selection.get() != original.dynamic_field_selection.get(),
          "Explicit destination query copy retained source-Context selection");
    user_context->destroy(original_ref);
    check(retained_in_context.dynamic_field_selection->contains("note"),
          "Destroying one query destroyed a sibling's shared selection");
    user_context.reset();
    check(retained.dynamic_field_selection->contains("note") &&
              !retained.dynamic_field_selection->contains("extra"),
          "Explicit Global query selection expired with its source");
    std::cout << "selection_handle_bytes=" << handle_bytes << " pointer_bytes=" << sizeof(void*)
              << '\n';
}

static void readonly_metadata() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array fields{DynamicPropertyDefinitions::Field{"_count", DataType::I64},
                            DynamicPropertyDefinitions::Field{"_missing", DataType::I64}};
    auto& schema = user_context->make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, fields)).value());
    Vector<String> names(r);
    names.emplace_back("id");
    names.emplace_back("_count");
    auto& columns =
        user_context->make<ColumnLayout>(std::move(names), std::span<const String>{}, &schema);
    Vector<CompactRow> rows(r);
    Vector<Value> first(r);
    first.emplace_back(std::uint64_t{1});
    first.emplace_back(std::int64_t{0});
    rows.emplace_back(columns, std::move(first));
    Vector<Value> second(r);
    second.emplace_back(std::uint64_t{2});
    second.emplace_back(nullptr);
    rows.emplace_back(columns, std::move(second));
    CompactRow::share_layouts(rows);
    const std::array indexes{FieldLayout::Index{"id", 0},
                             FieldLayout::Index{"version", 1},
                             FieldLayout::Index{"name", 2}};
    const std::array mappings{FieldLayout::Mapping{"id", "id", "id"},
                              FieldLayout::Mapping{"version", "version", "version"},
                              FieldLayout::Mapping{"name", "name", "name"}};
    const std::array<std::string_view, 3> members{"id", "version", "name"};
    auto& layout = user_context->make<FieldLayout>(
        std::move(FieldLayout::from_generated(r, "Probe", "v1", indexes, mappings, {}, members))
            .value());
    auto& pool = user_context->make<SnapshotPool>();
    auto left = rows[0].indexed_load_state(pool, layout);
    auto right = rows[1].indexed_load_state(pool, layout);
    check(left.indexed_snapshot()->identity() == right.indexed_snapshot()->identity() &&
              left.indexed_snapshot()->get().dynamic_property_definitions() == &schema,
          "Compatible row shapes failed to share readonly metadata");
    check(schema.data_type("_missing") == DataType::I64 && !rows[0].contains_key("_missing") &&
              !left.is_loaded("_count") && !layout.index("_count") && layout.field_count() == 3,
          "Readonly type declarations manufactured availability or fixed slots");
    check(rows[0].get("_count")->try_i64() == 0 && rows[1].get("_count")->is_null() &&
              rows[0].validate_dynamic_properties() && rows[1].validate_dynamic_properties(),
          "Shared readonly schema conflated values, NULL or presence");
    rows[0].insert("name", Value(r, "native"));
    rows[0].remove("name");
    auto old = detail::clone_in(r, rows[0]);
    const std::array incompatible{DynamicPropertyDefinitions::Field{"_count", DataType::Text}};
    auto& conflict = user_context->make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, incompatible)).value());
    Vector<String> conflict_names(r);
    conflict_names.emplace_back("id");
    conflict_names.emplace_back("_count");
    auto& conflict_columns = user_context->make<ColumnLayout>(
        std::move(conflict_names), std::span<const String>{}, &conflict);
    Vector<Value> conflict_values(r);
    conflict_values.emplace_back(std::uint64_t{99});
    conflict_values.emplace_back("secret");
    CompactRow incoming(
        std::allocator_arg, Allocator(r), conflict_columns, std::move(conflict_values));
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
    auto merged = rows[0].try_extend(incoming, shapes);
    check(!merged && merged.error().to_string(r).find("secret") == String::npos && rows[0] == old &&
              rows[0].validate_dynamic_properties(),
          "Conflicting readonly merge changed the receiver or leaked a payload");
    const std::array<std::string_view, 3> bad_names{"name", "#count", "_"};
    for (auto name : bad_names) {
        const std::array invalid{DynamicPropertyDefinitions::Field{name, DataType::I64}};
        check(!DynamicPropertyDefinitions::from_fields(r, invalid),
              "Invalid readonly field accepted");
    }
    const std::array duplicates{fields[0], fields[0]};
    check(!DynamicPropertyDefinitions::from_fields(r, duplicates),
          "Duplicate readonly field accepted");
    for (const auto& value : {Value(r),
                              Value(r, TypedNull{DataType::I64}),
                              Value(r, std::int64_t{0}),
                              Value(r, Json(r, std::int64_t{0}))}) {
        check(bool(schema.validate_value("_count", value)),
              "Valid readonly NULL/native/JSON value rejected");
    }
    check(!schema.validate_value("_count", Value(r, TypedNull{DataType::Text})) &&
              !schema.validate_value("_count", Value(r, "secret")),
          "Readonly validation accepted a mismatched value");
}

static void bulk_sidecars() {
    for (auto count : {1u, 100u, 10000u}) {
        ContextLimits limits;
        limits.user.max_bytes = 512 * 1024 * 1024;
        limits.max_live_user_bytes = limits.user.max_bytes;
        GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
        Vector<Value::Object> left_values(r), right_values(r);
        left_values.reserve(count);
        right_values.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            auto& left = left_values.emplace_back();
            left.emplace(String("note", r), Value(r, "private-" + std::to_string(index)));
            left.emplace(String("number", r), Value(r, std::int64_t(index)));
            auto& right = right_values.emplace_back();
            right.emplace(String("extra", r), Value(r));
            right.emplace(String("number", r), Value(r, std::int64_t{0}));
        }
        auto left = DynamicFieldValues::from_batch(r, schema, left_values);
        auto right = DynamicFieldValues::from_batch(r, schema, right_values);
        check(left && right, "Dynamic batch setup failed");
        auto& columns =
            user_context->make<ColumnLayout>(std::initializer_list<std::string_view>{"id", "name"});
        RelationShapeCache relations(std::allocator_arg, Allocator(r));
        DynamicFieldMergeShapes merges(std::allocator_arg, Allocator(r));
        Vector<CompactRow> rows(r);
        rows.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            Vector<Value> values(r);
            values.emplace_back(std::uint64_t(index));
            values.emplace_back("old");
            CompactRow row(std::allocator_arg, Allocator(r), columns, std::move(values));
            row.set_loaded_dynamic_fields(left.value()[index]);
            auto enhancement = detail::clone_in(r, row);
            enhancement.set_loaded_dynamic_fields(right.value()[index]);
            enhancement.insert("name", Value(r, "updated"));
            enhancement.mark_relation_loaded("platform", relations);
            check(bool(row.try_extend(enhancement, merges)),
                  "Compatible dynamic enhancement failed");
            rows.push_back(std::move(row));
        }
        CompactRow::share_layouts(rows);
        std::optional<std::uint64_t> selection;
        for (std::size_t index = 0; index < count; ++index) {
            auto& row = rows[index];
            check(row.shared_layout() == rows.front().shared_layout() &&
                      row.is_loaded_relation("platform") &&
                      row.get("name")->try_text() == "updated",
                  "Bulk enhancement failed to share actual relation geometry");
            auto fields = row.take_loaded_dynamic_fields();
            check(bool(fields), "Merged dynamic sidecar was absent");
            auto& readable = user_context->make<DynamicFieldValues>(std::move(*fields));
            if (!selection) {
                selection = readable.selection_identity();
            }
            check(readable.selection_identity() == *selection &&
                      readable.field("note").value().value()->try_text() ==
                          "private-" + std::to_string(index) &&
                      readable.field("extra").value().state() == DynamicFieldState::Null &&
                      readable.field("number").value().value()->try_i64() == 0 &&
                      readable.field("active").value().state() == DynamicFieldState::NotLoaded &&
                      !row.copy_map(r).contains("#note"),
                  "Bulk dynamic union conflated geometry, values or presentation data");
        }
        std::cout << "bulk_dynamic_rows=" << count << '\n';
    }
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        const auto mode = argc > 1 ? std::string_view(argv[1]) : std::string_view{};
        if (mode.empty() || mode == "dictionary") {
            binding_dictionary();
        }
        if (mode.empty() || mode == "selection") {
            query_selection();
        }
        if (mode.empty()) {
            readonly_metadata();
            bulk_sidecars();
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
