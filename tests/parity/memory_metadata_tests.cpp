#include "memory_data_service.hpp"
#include <iostream>
using namespace teaql;
using Service = original_memory::MemoryDataService;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class CustomMetadata final : public MetadataStore {
public:
    using allocator_type = Allocator;
    EntityDescriptor descriptor;
    mutable unsigned lookups = 0;
    bool enabled = true;

    CustomMetadata(std::allocator_arg_t, Allocator a) : descriptor(std::allocator_arg, a, "Item") {}

    Memory memory_resource() const noexcept override {
        return descriptor.memory_resource();
    }

    const EntityDescriptor* entity(std::string_view name) const override {
        ++lookups;
        return enabled && name == descriptor.name ? &descriptor : nullptr;
    }

    Vector<const EntityDescriptor*> all_entities(Memory r) const override {
        Vector<const EntityDescriptor*> result(r);
        if (enabled) {
            result.push_back(&descriptor);
        }
        return result;
    }
};

static void exercise(UserContext& user_context, CustomMetadata& metadata) {
    auto r = user_context.memory_resource();
    const MetadataStore& interface = metadata;
    auto& service = user_context.make<Service>(interface);
    Vector<CompactRow> input(r);
    Record fields(r);
    fields.emplace("id", Value(r, std::uint64_t(1)));
    input.emplace_back(std::move(fields));
    service.seed("Item", input);
    check(metadata.lookups == 0, "seeding must not request metadata");
    SelectQuery query(std::allocator_arg, Allocator(r), "Item");
    auto loaded = service.fetch_all(query);
    check(loaded && loaded.value().size() == 1 && metadata.lookups == 1,
          "custom metadata must serve the query");
    metadata.enabled = false;
    auto denied = service.fetch_all(query);
    check(!denied && denied.error().kind() == Service::Error::Kind::Runtime &&
              metadata.lookups == 2,
          "metadata lookup must remain live, not copied into a concrete store");
    metadata.enabled = true;
    auto restored = service.fetch_all(query);
    check(restored && restored.value().size() == 1 && metadata.lookups == 3,
          "metadata rejection must preserve seeded rows");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    {
        auto user_context = global.make_user_context();
        auto& metadata = user_context->make<CustomMetadata>();
        exercise(*user_context, metadata);
    }
    {
        auto& metadata = global.make<CustomMetadata>();
        auto user_context = global.make_user_context();
        exercise(*user_context, metadata);
    }
    {
        auto source_user_context = global.make_user_context();
        auto next_user_context = global.make_user_context();
        auto& metadata = source_user_context->make<CustomMetadata>();
        bool rejected = false;
        try {
            (void)next_user_context->make<Service>(metadata);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "a foreign UserContext cannot lend metadata");
        rejected = false;
        try {
            (void)global.make<Service>(metadata);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Global service cannot retain UserContext metadata");
    }
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<CustomMetadata>();
        ResourceBorrow<const MetadataStore> borrowed(
            std::allocator_arg, Allocator(r), metadata, detail::resource_arena(r));
        check(&borrowed.get() == &metadata, "interface borrow resolves live object");
        user_context.reset();
        bool closed = false;
        try {
            (void)borrowed.get();
        } catch (const ReferenceError& error) {
            closed = error.kind() == ReferenceError::Kind::Closed;
        }
        check(closed, "interface borrow must reject access after Context destruction");
    }
    std::cout << "Custom metadata dispatch and Context boundaries passed\n";
}
