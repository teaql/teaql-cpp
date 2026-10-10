// Direct source registry derive/clone/snapshot comparison using public APIs.
#include <teaql/runtime.hpp>
#include <teaql/core/meta_debug.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static EntityDescriptor descriptor(Memory owner, const Json& input) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), text(field(input, "name")));
    result.table(text(field(input, "table")));
    for (const auto& name : std::get<Json::Array>(field(input, "fields").data)) {
        result.property(text(name), DataType::Text);
    }
    return result;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto& metadata = user.make<InMemoryMetadataStore>();
    auto& registry = user.make<InMemoryEntityRegistry>();
    Json::Object defaults(owner);
    defaults.emplace("metadata", formatted_debug(owner, metadata));
    defaults.emplace("registry", formatted_debug(owner, registry));
    for (const auto& value : std::get<Json::Array>(field(input, "entries").data)) {
        metadata.register_entity(descriptor(owner, value));
    }
    for (const auto& name : std::get<Json::Array>(field(input, "names").data)) {
        registry.register_entity(text(name));
    }
    auto& cloned_metadata = user.make<InMemoryMetadataStore>(metadata);
    auto& cloned_registry = user.make<InMemoryEntityRegistry>(registry);
    const auto* snapshot = metadata.get_entity(text(field(input, "snapshot")));
    auto before = formatted_debug(owner, metadata);
    metadata.with_entity(descriptor(owner, field(input, "replacement")));
    registry.with_entity(text(field(field(input, "replacement"), "name")));
    Json::Array all(owner), contains(owner);
    for (const auto* value : metadata.all_entities(owner)) {
        all.emplace_back(formatted_debug(owner, *value));
    }
    for (const auto& name : std::get<Json::Array>(field(input, "names").data)) {
        contains.emplace_back(registry.contains(text(name)));
    }
    Json::Object output(owner);
    output.emplace("defaults", Json(owner, std::move(defaults)));
    output.emplace("before", std::move(before));
    output.emplace("metadata", formatted_debug(owner, metadata));
    output.emplace("metadata_clone", formatted_debug(owner, cloned_metadata));
    output.emplace("registry", formatted_debug(owner, registry));
    output.emplace("registry_clone", formatted_debug(owner, cloned_registry));
    output.emplace("snapshot", snapshot ? formatted_debug(owner, *snapshot) : Json(owner));
    output.emplace("all", Json(owner, std::move(all)));
    output.emplace("contains", Json(owner, std::move(contains)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid registry diagnostic input");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
