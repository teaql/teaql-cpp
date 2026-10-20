#include <teaql/runtime/context.hpp>
#include <teaql/runtime/generated_schema_provider.hpp>
#include <iostream>

using namespace teaql;

static std::optional<EntityDescriptor> lookup(Memory resource, std::string_view name) {
    if (name != "Record") {
        return std::nullopt;
    }
    EntityDescriptor descriptor(std::allocator_arg, Allocator(resource), "Record");
    descriptor.table_name = "record_data";
    return descriptor;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto provider = user_context->make_ref<GeneratedLocalSchemaProvider>(lookup);
        auto& value = user_context->get(provider);
        auto* first = value.get_entity("Record");
        auto* second = value.get_entity("Record");
        if (!first || !second || first == second || first->table_name != "record_data" ||
            first->memory_resource() != user_context->memory_resource() ||
            value.get_entity("record") || value.get_entity("unknown")) {
            throw std::runtime_error(
                "Generated schema lookup or fresh descriptor ownership failed");
        }
        // Clone a provider and a descriptor into Global explicitly. Neither
        // destination contains references to the source arena or mutable metadata.
        auto retained_provider = user_context->make_global<GeneratedLocalSchemaProvider>(value);
        auto retained_descriptor = user_context->make_global<EntityDescriptor>(*first);
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Schema provider retained User storage");
        }
        auto next_user_context = global.make_user_context();
        bool rejected = false;
        try {
            (void)next_user_context->get(provider);
        } catch (const ReferenceError&) {
            rejected = true;
        }
        if (!rejected) {
            throw std::runtime_error("Generated provider reference survived Context close");
        }
        auto& retained =
            next_user_context->get(next_user_context->rebind_global(retained_provider));
        auto* descriptor = retained.get_entity("Record");
        if (!descriptor || descriptor->memory_resource() != global.memory_resource() ||
            next_user_context->get(next_user_context->rebind_global(retained_descriptor))
                    .table_name != "record_data") {
            throw std::runtime_error(
                "Explicit Global schema copies lost their destination ownership");
        }
        bool null_rejected = false;
        try {
            next_user_context->make<GeneratedLocalSchemaProvider>(nullptr);
        } catch (const std::invalid_argument&) {
            null_rejected = true;
        }
        if (!null_rejected) {
            throw std::runtime_error("Missing generated schema dispatch was accepted");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated schema provider dispatch, freshness and lifetimes passed\n";
    return 0;
}
