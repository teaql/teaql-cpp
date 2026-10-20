#include "../../examples/generated/include/cpp_examples.hpp"
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

int main() {
    json_cannot_install_runtime_state();
    scoped_readonly_copy();
    std::cout << "generated readonly property lifecycle contracts passed\n";
}
