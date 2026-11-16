#include <teaql/runtime.hpp>

template <class Model>
teaql::Json describe_property_identities(teaql::Memory resource) {
    auto descriptor = Model::entity_descriptor(resource);
    teaql::Json::Array properties(resource);
    for (const auto& property : descriptor.properties) {
        teaql::Json::Object item(resource);
        item.emplace("entity", teaql::Json(resource, descriptor.name));
        item.emplace("name", teaql::Json(resource, property.name));
        item.emplace("physical_column", teaql::Json(resource, property.column_name));
        item.emplace("json_column",
                     teaql::Json(resource,
                                 property.json_column_name ? *property.json_column_name
                                                           : property.column_name));
        item.emplace("max_length",
                     property.max_length ? teaql::Json(resource, *property.max_length)
                                         : teaql::Json(resource));
        properties.emplace_back(teaql::Json(resource, std::move(item)));
    }
    return teaql::Json(resource, std::move(properties));
}
