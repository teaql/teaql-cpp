#include <teaql/core/list.hpp>
#include <teaql/core/eval.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static FieldLayout make_layout(Memory r, std::string_view entity = "Item") {
    const std::array indexes{FieldLayout::Index{"id", 0},
                             FieldLayout::Index{"version", 1},
                             FieldLayout::Index{"name", 2}};
    const std::array mappings{FieldLayout::Mapping{"id", "id", "db_id"},
                              FieldLayout::Mapping{"version", "version", "db_version"},
                              FieldLayout::Mapping{"name", "name", "db_name"}};
    const std::array<std::string_view, 3> expected{"id", "version", "name"};
    const std::array<std::string_view, 1> relations{"lines"};
    return std::move(
               FieldLayout::from_generated(r, entity, "v1", indexes, mappings, relations, expected))
        .value();
}

static DynamicFieldDefinitions definitions(Memory r, std::string_view owner = "Item") {
    const std::array fields{DynamicFieldDefinitions::Field{"extension", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    return std::move(DynamicFieldDefinitions::from_fields(r, owner, "v1", fields)).value();
}

static DynamicFieldValues
dynamic_values(Memory r, const DynamicFieldDefinitions& schema, const Value::Object& values) {
    return std::move(DynamicFieldValues::from_values(r, schema, values)).value();
}

static void snapshot_contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& layout = user_context->make<FieldLayout>(make_layout(r));
    auto& pool = user_context->make<SnapshotPool>();
    const std::array<std::string_view, 3> selected{"db_id", "lines", "#stale"};
    auto original = LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
    const std::array props{DynamicPropertyDefinitions::Field{"_total", DataType::I64}};
    auto& properties = user_context->make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, props)).value());
    original = LoadedSnapshot::with_dynamic_property_definitions(pool, original, properties);
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    Value::Object loaded(r);
    loaded.emplace(String("extension", r), Value(r, nullptr));
    auto& values = user_context->make<DynamicFieldValues>(dynamic_values(r, schema, loaded));
    auto changed = LoadedSnapshot::with_dynamic_fields(pool, original, values);
    check(bool(changed), "dynamic availability attaches");
    check(changed.value().get().is_loaded("id") && changed.value().get().is_loaded("lines") &&
              changed.value().get().is_loaded("#extension") &&
              !changed.value().get().is_loaded("#stale"),
          "only persistent selection replaced");
    check(changed.value().get().dynamic_property_definitions() == &properties,
          "readonly schema survives dynamic attach");
    check(original.get().is_loaded("#stale") && !original.get().is_loaded("#extension"),
          "old snapshot remains isolated");
    check(bool(values.assign("extension", Value(r, "value-only"))), "value update valid");
    auto unchanged = LoadedSnapshot::with_dynamic_fields(pool, changed.value(), values);
    check(unchanged.value().identity() == changed.value().identity(),
          "value-only edit preserves availability identity");
    auto cleared = LoadedSnapshot::with_property_schema(pool, changed.value(), nullptr);
    check(!cleared.get().dynamic_property_definitions() && cleared.get().is_loaded("#extension") &&
              cleared.get().is_loaded("lines"),
          "property clearing preserves other availability");
    auto& wrong_schema = user_context->make<DynamicFieldDefinitions>(definitions(r, "Other"));
    auto& wrong_values =
        user_context->make<DynamicFieldValues>(dynamic_values(r, wrong_schema, loaded));
    auto rejected = LoadedSnapshot::with_dynamic_fields(pool, changed.value(), wrong_values);
    check(!rejected && rejected.error() == "dynamic field owner disagrees with fixed entity layout",
          "owner mismatch before mutation");
}

static void row_contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    Value::Object extensions(r);
    extensions.emplace(String("extension", r), Value(r, "loaded"));
    auto& values = user_context->make<DynamicFieldValues>(dynamic_values(r, schema, extensions));
    Record native(r);
    native.emplace(String("id", r), Value(r, 7));
    native.emplace(String("_total", r), Value(r, 0));
    CompactRow row(std::allocator_arg, Allocator(r), native);
    row.set_loaded_dynamic_fields(values);
    check(!row.contains_key("#extension") && row.copy_map(r).size() == 2 &&
              !row.has_loaded_relations(),
          "dynamic read metadata is not a native column or relation list");
    const std::array props{DynamicPropertyDefinitions::Field{"_total", DataType::I64}};
    auto& properties = user_context->make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, props)).value());
    Vector<CompactRow> rows(r);
    rows.push_back(std::move(row));
    rows.emplace_back(rows[0]);
    CompactRow::attach_dynamic_property_definitions(rows, properties);
    check(rows[0].shared_layout() == rows[1].shared_layout(),
          "one schema binding per actual result layout");
    auto& layout = user_context->make<FieldLayout>(make_layout(r));
    auto& pool = user_context->make<SnapshotPool>();
    check(rows[0].indexed_load_state(pool, layout)
                  .indexed_snapshot()
                  ->get()
                  .dynamic_property_definitions() == &properties,
          "row decoder carries readonly schema");
    RelationShapeCache cache(std::allocator_arg, Allocator(r));
    rows[0].mark_relation_loaded("lines", cache);
    check(rows[0].shared_layout()->dynamic_property_definitions() == &properties,
          "relation marking retains readonly schema");
    rows[0].insert("extra", Value(r, false));
    rows[0].remove("extra");
    check(rows[0].shared_layout()->dynamic_property_definitions() == &properties,
          "column edits retain readonly schema");
    auto bound = schema.with_storage_binding(r, "profile", 9);
    auto& wrong_schema = user_context->make<DynamicFieldDefinitions>(std::move(bound));
    auto& wrong_values =
        user_context->make<DynamicFieldValues>(dynamic_values(r, wrong_schema, extensions));
    Record incoming(r);
    incoming.emplace(String("id", r), Value(r, 99));
    CompactRow bad(std::allocator_arg, Allocator(r), incoming);
    bad.set_loaded_dynamic_fields(wrong_values);
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
    const auto before = rows[0].copy_map(r);
    auto rejected = rows[0].try_extend(bad, shapes);
    check(!rejected && rejected.error().code == "DYNAMIC_FIELD_INCOMPATIBLE_VIEW" &&
              rows[0].copy_map(r) == before,
          "incompatible metadata rejects before native overwrite");
    SmartList<CompactRow> list(std::allocator_arg, Allocator(r));
    list.is_loaded = true;
    list.data.emplace_back(incoming);
    rows[0].set_loaded_relation("lines", list);
    check(rows[0].has_loaded_relations() && rows[0].loaded_relation("lines")->data.size() == 1,
          "relation payload lives in separate sidecar");
    auto& retained = global.make<CompactRow>(rows[0]);
    auto borrowed = std::move(rows[0].loaded_dynamic_fields()->field("extension")).value();
    auto taken = rows[0].take_loaded_dynamic_fields();
    check(bool(taken) && !rows[0].loaded_dynamic_fields() && rows[0].has_loaded_relations(),
          "taking dynamic values retains relation lists");
    rows[0].clear_loaded_relations();
    check(!rows[0].has_loaded_relations() && rows[0].is_loaded_relation("lines"),
          "clearing payload does not erase relation shape markers");
    check(retained.loaded_dynamic_fields()->field("extension").value().value()->try_text() ==
              "loaded",
          "Global row retention copies sidecar payload");
    check(borrowed.value()->try_text() == "loaded",
          "Context-managed field object remains bound until Context close");
    rows.clear();
}

struct PresentationFixture : EntityDefaults<PresentationFixture> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Item";
    Record fields;
    const DynamicFieldValues* extensions = nullptr;

    PresentationFixture(std::allocator_arg_t, Allocator a) : fields(a.resource()) {}

    Memory memory_resource() const {
        return fields.get_allocator().resource();
    }

    const DynamicFieldValues* dynamic_field_values() const {
        return extensions;
    }

    MutationValues into_values() && {
        return MutationValues(memory_resource(), std::move(fields));
    }
};

static void default_entity_contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    PresentationFixture model(std::allocator_arg, Allocator(r));
    auto updated = model.update_dynamic_field("extension", Value(r, "no carrier"));
    auto deleted = model.delete_dynamic_field("extension");
    check(!updated && !deleted && updated.error().code == "DYNAMIC_FIELD_CARRIER_MISSING" &&
              deleted.error().field == "extension" && !model.has_pending_dynamic_mutations() &&
              !PresentationFixture::supports_dynamic_field_load(),
          "default entity cannot fabricate a persistent carrier");
    check(!model.dynamic_property("_total") && !model.has_dynamic_property("_total") &&
              !model.dynamic_property_type("_total"),
          "missing readonly properties have no implied presence");
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    Value::Object values(r);
    values.emplace(String("extension", r), Value(r, nullptr));
    auto& extensions = user_context->make<DynamicFieldValues>(dynamic_values(r, schema, values));
    auto& layout = user_context->make<FieldLayout>(make_layout(r));
    auto& pool = user_context->make<SnapshotPool>();
    auto snapshot = LoadedSnapshot::fully_loaded(r, layout).into_shared(pool);
    auto loaded = model.install_loaded_dynamic_fields(extensions, snapshot);
    check(!loaded && loaded.error().entity == "Item",
          "default hydration rejects unsupported carriers");
    model.extensions = &extensions;
    model.fields.emplace(String("id", r), Value(r, 7));
    model.fields.emplace(String("_total", r), Value(r, 0));
    for (std::string_view key :
         {"_comment", "_dirty_fields", "_original_values", "_is_new", "_is_deleted"}) {
        model.fields.emplace(String(key, r), Value(r, "internal"));
    }
    auto json = std::move(model).into_json(r);
    const auto& object = std::get<Json::Object>(json.data);
    check(
        object.size() == 3 && object.contains("_total") && object.contains("#extension") &&
            std::holds_alternative<std::monostate>(object.find("#extension")->second.data),
        "default presentation emits loaded extensions and readonly properties, excluding framework mutation keys");
}

static void retained_row_lifetime() {
    GlobalContext global;
    CompactRow* retained = nullptr;
    std::optional<DynamicFieldValue> expired;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
        Value::Object values(r);
        values.emplace(String("extension", r), Value(r, std::string(8192, 'x')));
        auto& fields = user_context->make<DynamicFieldValues>(dynamic_values(r, schema, values));
        Record native(r);
        native.emplace(String("name", r), Value(r, std::string(4096, 'n')));
        CompactRow row(std::allocator_arg, Allocator(r), native);
        row.set_loaded_dynamic_fields(fields);
        const std::array declarations{DynamicPropertyDefinitions::Field{"_total", DataType::I64}};
        auto& properties = user_context->make<DynamicPropertyDefinitions>(
            std::move(DynamicPropertyDefinitions::from_fields(r, declarations)).value());
        CompactRow::attach_dynamic_property_definitions(std::span(&row, 1), properties);
        SmartList<CompactRow> list(std::allocator_arg, Allocator(r));
        list.is_loaded = true;
        list.data.emplace_back(native);
        row.set_loaded_relation("lines", list);
        retained = &global.make<CompactRow>(row);
        expired.emplace(std::move(row.loaded_dynamic_fields()->field("extension")).value());
    }
    bool rejected = false;
    try {
        (void)expired->value();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "row field borrow rejects after actual User deletion");
    check(
        retained->get("name")->try_text()->size() == 4096 &&
            retained->loaded_dynamic_fields()
                    ->field("extension")
                    .value()
                    .value()
                    ->try_text()
                    ->size() == 8192 &&
            retained->loaded_relation("lines")->data[0].get("name")->try_text()->size() == 4096 &&
            retained->shared_layout()->dynamic_property_definitions()->memory_resource() ==
                global.memory_resource(),
        "Global retention deep-copies native, relation, dynamic and schema data before User destruction");
}

int main() {
    try {
        snapshot_contracts();
        row_contracts();
        default_entity_contracts();
        retained_row_lifetime();
        std::cout << "Dynamic carrier contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
