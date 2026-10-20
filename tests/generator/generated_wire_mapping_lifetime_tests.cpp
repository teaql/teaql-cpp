#include <teaql/runtime/context.hpp>
#include <teaql/runtime/generated_support.hpp>
#include <array>
#include <iostream>

using namespace teaql;

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto owner = global.make_user_context();
        std::optional<Ref<GeneratedWireFieldMappings>> kept, expired;
        {
            auto& user_context = *owner;
            expired = user_context.make_ref<GeneratedWireFieldMappings>();
            auto& mappings = user_context.get(*expired);
            auto r = user_context.memory_resource();
            String entity(240, 'e', r), canonical(240, 'c', r), wire(240, 'w', r);
            auto& fields = mappings.try_emplace(entity).first->second;
            fields.emplace(canonical, wire);
            if (fields.get_allocator().resource() != r ||
                fields.begin()->first.get_allocator().resource() != r ||
                fields.begin()->second.get_allocator().resource() != r) {
                throw std::runtime_error("Nested wire mapping did not inherit its Context");
            }
            kept = user_context.make_global<GeneratedWireFieldMappings>(mappings);
        }
        owner.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Wire mapping retained User blocks");
        }
        auto next_user_context = global.make_user_context();
        try {
            (void)next_user_context->get(*expired);
            throw std::runtime_error("Closed wire mapping reference was accepted");
        } catch (const ReferenceError& error) {
            if (error.kind() != ReferenceError::Kind::Closed) {
                throw;
            }
        }
        auto& copied = next_user_context->get(next_user_context->rebind_global(*kept));
        std::array<char, 240> entity_key, field_key;
        entity_key.fill('e');
        field_key.fill('c');
        auto entity = copied.find(std::string_view(entity_key.data(), entity_key.size()));
        if (entity == copied.end() ||
            entity->second.find(std::string_view(field_key.data(), field_key.size())) ==
                entity->second.end()) {
            throw std::runtime_error("Borrowed wire lookup allocated a default-resource key");
        }
        auto global_r = global.memory_resource();
        if (copied.get_allocator().resource() != global_r ||
            copied.begin()->first.get_allocator().resource() != global_r ||
            copied.begin()->second.get_allocator().resource() != global_r ||
            copied.begin()->second.begin()->first.get_allocator().resource() != global_r ||
            copied.begin()->second.begin()->second.get_allocator().resource() != global_r ||
            copied.begin()->second.begin()->second != String(240, 'w', global_r)) {
            throw std::runtime_error("Explicit destination copy retained User strings/maps");
        }
        std::cout << "Nested wire mapping allocation and explicit destination copy passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
