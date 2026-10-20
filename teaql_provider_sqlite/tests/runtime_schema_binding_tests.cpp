#include <teaql/provider_sqlite/runtime_assembly.hpp>
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
        auto resource = user_context->memory_resource();
        auto& module = user_context->make<RuntimeModule>();
        {
            EntityDescriptor descriptor(std::allocator_arg, Allocator(resource), "Record");
            descriptor.table_name = "runtime_metadata_override";
            module.descriptor(std::move(descriptor));
        }
        auto& schema = user_context->make<GeneratedLocalSchemaProvider>(lookup);
        auto connection = user_context->make_resource_ref<SqliteConnection>(":memory:");
        using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
        auto reference =
            assemble_generated_sqlite_runtime<Executor>(*user_context, module, connection, schema);
        auto& executor = user_context->get(reference);
        {
            auto entities = user_context->all_entities();
            auto* selected = executor.schema_provider.get_entity("Record");
            if (&executor.schema_provider != &schema || !selected ||
                selected->table_name != "record_data" || entities.size() != 1 ||
                entities[0]->table_name != "runtime_metadata_override") {
                throw std::runtime_error("SQL schema lookup was merged with runtime metadata");
            }
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Independent schema binding retained User memory");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Independent generated SQL schema and runtime metadata binding passed\n";
    return 0;
}
