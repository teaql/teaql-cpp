#include "../../examples/generated/include/cpp_examples.hpp"
#include "../support/generated_allocation_counter.hpp"
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void json_cannot_install_runtime_state() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    metadata.register_entity(cpp_examples::Product::entity_descriptor(r));
    user_context->set_metadata(*user_context, metadata);
    // Deliberately no executor: presentation decoding has no persistence authority.
    for (std::string_view key : {"_comment",
                                 "_dirty_fields",
                                 "_original_values",
                                 "_is_new",
                                 "_is_deleted",
                                 "__load_state",
                                 "__teaql_runtime_state"}) {
        String text("{\"id\":1,\"version\":7,\"name\":\"legitimate name\",\"", r);
        text += key;
        text += "\":{\"payload\":\"PRIVATE-STATE-CANARY\"}}";
        auto input = Json::parse(r, text);
        check(bool(input), "parse runtime-state injection fixture");
        auto single = user_context->decode_json_entity<cpp_examples::Product>(*input);
        check(!single, "single entity rejects incoming runtime state");
        auto error = single.error().to_string(r);
        check(error.find("incoming runtime state is forbidden") != String::npos &&
                  error.find("PRIVATE-STATE-CANARY") == String::npos,
              "single entity error gives the reason without disclosing injected values");
        String batch_text("[{\"id\":2,\"name\":\"valid first row\"},", r);
        batch_text += text;
        batch_text += "]";
        auto batch = Json::parse(r, batch_text);
        check(bool(batch), "parse batch runtime-state fixture");
        auto multiple = user_context->decode_json_entities<cpp_examples::Product>(*batch);
        check(!multiple, "batch rejects runtime-state injection after a valid row");
        error = multiple.error().to_string(r);
        check(error.find("incoming runtime state is forbidden") != String::npos &&
                  error.find("PRIVATE-STATE-CANARY") == String::npos,
              "batch error gives the reason without disclosing injected values");
    }
    auto input = Json::parse(
        r, R"({"id":1,"version":7,"name":"native","_name":"derived","_count":0,"_null":null})");
    check(bool(input), "parse legitimate readonly properties");
    auto decoded = user_context->decode_json_entity<cpp_examples::Product>(*input);
    check(bool(decoded), "decode legitimate properties without an executor");
    const auto& row = decoded.value();
    check(row.name() == "native" && row.hasDynamicProperty("_name") &&
              row.hasDynamicProperty("_count") && row.hasDynamicProperty("_null") &&
              !row.dynamicProperty("_null") && !row.hasDynamicProperty("_missing") &&
              !row.dynamicProperty("_missing"),
          "readonly properties preserve native value, explicit NULL and absent value");
    auto snapshot = row.loaded_state_snapshot();
    check(bool(snapshot), "generated JSON row has an indexed loaded snapshot");
    for (std::string_view key : {"_name", "_count", "_null", "_missing"}) {
        check(!snapshot->get().layout().index(key) && !snapshot->get().is_loaded(key),
              "readonly properties do not install fixed slots or loaded bits");
    }
    check(!row.dirty_fields() && !row.hasPendingDynamicMutations(),
          "presentation data creates neither scalar nor persistent dynamic mutations");
    std::cout << "JSON_RUNTIME_STATE_CONTRACTS=2;REJECTIONS=14\n";
}

static void scoped_readonly_copy() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    Ref<cpp_examples::Product> retained;
    {
        Record fields(r);
        fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
        fields.emplace(String("name", r), Value(r, "original"));
        fields.emplace(String("version", r), Value(r, std::int64_t(3)));
        fields.emplace(String("_caption", r), Value(r, String(4096, 'x', r)));
        fields.emplace(String("_empty", r), Value(r));
        fields.emplace(String("_zero", r), Value(r, std::int64_t(0)));
        fields.emplace(String("_false", r), Value(r, false));
        auto row = CompactRow(std::allocator_arg, Allocator(r), std::move(fields));
        auto result = cpp_examples::Product::from_compact_row(r, std::move(row));
        check(bool(result), "decode readonly projection into generated model");
        auto reference = user_context->make_ref<cpp_examples::Product>(std::move(result).value());
        auto& entity = user_context->get(reference);
        retained = user_context->make_global<cpp_examples::Product>(entity);
        auto& copy = user_context->get(retained);
        check(entity.hasDynamicProperty("_empty") && !entity.dynamicProperty("_empty"),
              "NULL projection is present but has no readable value");
        check(entity.dynamicProperty("_zero")->try_i64() == 0 &&
                  entity.dynamicProperty("_false")->try_bool() == false,
              "zero and false remain readable");
        check(!entity.hasDynamicProperty("name") && !entity.dynamicProperty("name"),
              "native columns do not become readonly properties");
        check(copy.dynamicProperty("_caption")->memory_resource() == global.memory_resource(),
              "explicit Global copy owns readonly value storage");
        entity.dynamic.insert_or_assign(String("_caption", r), Value(r, "changed"));
        check(copy.dynamicProperty("_caption")->try_text()->size() == 4096,
              "copied readonly map remains independent");
        check(!entity.hasPendingDynamicMutations() && !copy.hasPendingDynamicMutations(),
              "readonly access and retention do not create durable mutations");
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    auto rebound = next_user_context->rebind_global(retained);
    auto& copy = next_user_context->get(rebound);
    check(copy.dynamicProperty("_caption")->try_text()->size() == 4096 &&
              copy.hasDynamicProperty("_empty"),
          "Global copy survives source Context destruction and explicit rebinding");
    next_user_context.reset();
    std::pmr::set_default_resource(previous);
}

template <class F>
static Counts measured(F&& operation) {
    counts = {};
    measuring = true;
    try {
        operation();
    } catch (...) {
        measuring = false;
        throw;
    }
    measuring = false;
    return counts;
}

static void readonly_read_allocation_contracts() {
    using Product = cpp_examples::Product;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const auto positive = measured([&] {
        (void)r->allocate(128);
        (void)global.memory_resource()->allocate(64);
        auto* plain = ::operator new(128);
        ::operator delete(plain);
        auto* aligned = ::operator new(128, std::align_val_t(64));
        ::operator delete(aligned, std::align_val_t(64));
    });
    check(
        positive.requests >= 2 && positive.bytes >= 192 && positive.heap_calls >= 2 &&
            positive.heap_bytes >= 256,
        "instrumentation observes User/Global arena requests and ordinary/aligned heap allocation");
    auto no_allocations = [&](const char* name, unsigned reads, auto&& operation) {
        const auto result = measured(operation);
        check(result.requests == 0 && result.bytes == 0 && result.heap_calls == 0 &&
                  result.heap_bytes == 0,
              "generated readonly access allocates neither arena payloads nor heap memory");
        std::cout << name << ',' << reads << ',' << result.requests << ',' << result.bytes << ','
                  << result.heap_calls << ',' << result.heap_bytes << '\n';
    };
    const std::array fields{DynamicPropertyDefinitions::Field{"_count", DataType::I64},
                            DynamicPropertyDefinitions::Field{"_missing", DataType::I64}};
    auto& definitions = user_context->make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, fields)).value());
    auto& base = user_context->make<ColumnLayout>(
        std::initializer_list<std::string_view>{"id", "version", "name", "_count"});
    const auto& columns = base.with_dynamic_property_definitions(definitions);
    auto make = [&](std::uint64_t id, Value value) {
        Vector<Value> values(r);
        values.emplace_back(id);
        values.emplace_back(std::int64_t{1});
        values.emplace_back("native");
        values.push_back(std::move(value));
        return Product::from_compact_row(
            r, CompactRow(std::allocator_arg, Allocator(r), columns, std::move(values)));
    };
    auto first = make(1, Value(r, std::int64_t{0}));
    auto second = make(2, Value(r, TypedNull{DataType::I64}));
    check(bool(first) && bool(second), "decode readonly zero and typed NULL");
    const auto state = first.value().loaded_state_snapshot().value();
    check(state.identity() == second.value().loaded_state_snapshot()->identity() &&
              state.get().dynamic_property_definitions() == columns.dynamic_property_definitions(),
          "equal readonly availability shares snapshot and definitions");
    check(
        first.value().dynamicProperty("_count")->try_i64() == 0 &&
            !second.value().dynamicProperty("_count") &&
            second.value().hasDynamicProperty("_count") &&
            first.value().dynamic_property_type("_missing") == DataType::I64 &&
            !first.value().dynamicProperty("_missing") &&
            !first.value().hasDynamicProperty("_missing") &&
            !Product::field_layout(r).index("_count") && !state.get().is_loaded("_count") &&
            !first.value().dirty_fields(),
        "readonly schema distinguishes typed NULL and absence without fixed slots or dirty fields");
    for (auto reads : {1u, 100u, 10000u}) {
        no_allocations("readonly_schema", reads, [&] {
            for (unsigned i = 0; i < reads; ++i) {
                check(first.value().dynamic_property_type("_count") == DataType::I64 &&
                          !first.value().dynamicProperty("_missing"),
                      "repeated schema read");
            }
        });
    }
    auto invalid = make(3, Value(r, "secret"));
    check(!invalid, "typed readonly schema rejects wrong value type");
    auto error = invalid.error().to_string(r);
    check(error.find("_count") != String::npos && error.find("secret") == String::npos,
          "schema error names the property without leaking its value");
    Record values(r);
    values.emplace(String("id", r), Value(r, std::uint64_t{1}));
    values.emplace(String("version", r), Value(r, std::int64_t{1}));
    values.emplace(String("name", r), Value(r, "native"));
    values.emplace(String("_name", r), Value(r, "derived"));
    values.emplace(String("_zero", r), Value(r, std::int64_t{0}));
    values.emplace(String("_false", r), Value(r, false));
    values.emplace(String("_empty", r), Value(r, ""));
    values.emplace(String("_null", r), Value(r));
    values.emplace(String("_json_null", r), Value(r, Json(r)));
    auto decoded =
        Product::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), values));
    check(bool(decoded), "decode all readonly property shapes");
    const auto& entity = decoded.value();
    auto snapshot = entity.loaded_state_snapshot().value();
    check(&entity.dynamic.at(String("_name", r)) == entity.dynamicProperty("_name"),
          "readonly value is borrowed from the generated carrier");
    for (std::string_view key : {"_null", "_json_null"}) {
        check(!entity.dynamicProperty(key) && entity.hasDynamicProperty(key),
              "NULL remains present");
    }
    for (std::string_view key : {"name", "#name", "id", "version", "", "_missing"}) {
        check(!entity.dynamicProperty(key) && !entity.hasDynamicProperty(key),
              "non-property or absent key has no value or presence");
    }
    for (auto reads : {1u, 100u, 10000u}) {
        no_allocations("readonly_values", reads, [&] {
            for (unsigned i = 0; i < reads; ++i) {
                check(entity.dynamicProperty("_zero")->try_i64() == 0 &&
                          entity.dynamicProperty("_false")->try_bool() == false &&
                          entity.dynamicProperty("_empty")->try_text() == "" &&
                          !entity.dynamicProperty("_null") && !entity.dynamicProperty("_missing") &&
                          entity.hasDynamicProperty("_null") &&
                          !entity.hasDynamicProperty("_missing"),
                      "repeated value reads preserve zero false empty NULL and missing");
            }
        });
    }
    check(
        snapshot.identity() == entity.loaded_state_snapshot()->identity() &&
            !snapshot.get().is_loaded("_zero") && !snapshot.get().is_loaded("_missing") &&
            !entity.dirty_fields() && !entity.hasPendingDynamicMutations(),
        "repeated reads preserve snapshot and never create fixed availability or mutation intent");
    std::cout
        << "GENERATED_READONLY_ALLOCATION_CASES=2;READ_COUNTS=1,100,10000;POSITIVE_CONTROLS=passed\n";
}

static void generated_entity_serialization(bool sparse) {
    using Product = cpp_examples::Product;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    Record values(r);
    values.emplace(String("id", r), Value(r, std::uint64_t{1}));
    values.emplace(String("version", r), Value(r, std::int64_t{1}));
    values.emplace(String("_name", r), Value(r, "derived"));
    if (!sparse) {
        values.emplace(String("name", r), Value(r, "native"));
        values.emplace(String("_nil", r), Value(r));
    }
    auto decoded =
        Product::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), values));
    check(bool(decoded), "decode generated serialization fixture");
    auto& row = decoded.value();
    if (sparse) {
        check(!row.is_field_loaded("name"), "omitted native field remains unloaded");
        row.set_comment("private mutation reason");
        row.mark_as_new();
        auto mutation = Product(std::allocator_arg, Allocator(r), row).into_values();
        check(mutation.fields.contains("_comment") && mutation.fields.contains("_is_new"),
              "governed mutation carrier retains internal metadata");
    } else {
        const std::array types{DynamicFieldDefinitions::Field{"name", DataType::Text},
                               DynamicFieldDefinitions::Field{"nil", DataType::Text},
                               DynamicFieldDefinitions::Field{"missing", DataType::Text},
                               DynamicFieldDefinitions::Field{"zero", DataType::I64},
                               DynamicFieldDefinitions::Field{"false", DataType::Bool},
                               DynamicFieldDefinitions::Field{"empty", DataType::Text}};
        auto& definitions = user_context->make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(r, "Product", "serialization-v1", types))
                .value());
        Value::Object payload(r);
        payload.emplace(String("name", r), Value(r, "persistent"));
        payload.emplace(String("nil", r), Value(r));
        payload.emplace(String("zero", r), Value(r, std::int64_t{0}));
        payload.emplace(String("false", r), Value(r, false));
        payload.emplace(String("empty", r), Value(r, ""));
        auto fields = DynamicFieldValues::from_values(r, definitions, payload);
        check(bool(fields), "create selected persistent values without a missing field");
        auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
        auto state = LoadedSnapshot::with_dynamic_fields(
            pool, row.loaded_state_snapshot().value(), fields.value());
        check(bool(state) && bool(row.install_loaded_dynamic_fields(fields.value(), state.value())),
              "install persistent values through generated carrier");
    }
    auto json = std::move(row).into_json(r);
    const auto& object = std::get<Json::Object>(json.data);
    check(object.at(String("_name", r)).dump(r) == "\"derived\"",
          "derived name retains its namespace");
    if (sparse) {
        check(!object.contains("name"),
              "unloaded native name is omitted rather than serialized as a default");
        for (std::string_view internal :
             {"_comment", "_is_new", "_is_deleted", "_dirty_fields", "_original_values"}) {
            check(!object.contains(internal), "presentation JSON excludes mutation bookkeeping");
        }
        check(json.dump(r).find("private mutation reason") == String::npos,
              "private audit reason is absent from presentation JSON");
    } else {
        check(object.at(String("name", r)).dump(r) == "\"native\"" &&
                  object.at(String("#name", r)).dump(r) == "\"persistent\"" &&
                  object.at(String("#zero", r)).dump(r) == "0" &&
                  object.at(String("#false", r)).dump(r) == "false" &&
                  object.at(String("#empty", r)).dump(r) == "\"\"" &&
                  object.at(String("#nil", r)).dump(r) == "null" &&
                  object.at(String("_nil", r)).dump(r) == "null",
              "native derived and persistent names preserve zero false empty and NULL");
        check(!object.contains("#missing") && !object.contains("__load_state") &&
                  !object.contains("storage_binding"),
              "unloaded fields and storage state never become presentation data");
    }
}

int main() {
    generated_entity_serialization(false);
    generated_entity_serialization(true);
    std::cout << "GENERATED_ENTITY_SERIALIZATION_CASES=2\n";
    readonly_read_allocation_contracts();
    json_cannot_install_runtime_state();
    scoped_readonly_copy();
    std::cout << "generated readonly property lifecycle contracts passed\n";
}
