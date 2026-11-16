#include <teaql/runtime.hpp>

#include <iostream>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace teaql;

std::string_view configuration_data_type(DataType type) {
    switch (type) {
    case DataType::Bool:
        return "Bool";
    case DataType::I64:
        return "I64";
    case DataType::U64:
        return "U64";
    case DataType::F64:
        return "F64";
    case DataType::Decimal:
        return "Decimal";
    case DataType::Text:
        return "Text";
    case DataType::LargeText:
        return "LargeText";
    case DataType::Json:
        return "Json";
    case DataType::Date:
        return "Date";
    case DataType::Timestamp:
        return "Timestamp";
    }
    throw std::runtime_error("Unknown compiled configuration data type");
}

template <class Model>
Json describe_configuration(Memory resource, std::string_view module, std::string_view type) {
    auto descriptor = Model::entity_descriptor(resource);
    (void)Model::field_layout(resource);
    Json::Object row(resource);
    row.emplace("module", Json(resource, module));
    row.emplace("type", Json(resource, type));
    row.emplace("name", Json(resource, descriptor.name));
    row.emplace("table", Json(resource, descriptor.table_name));
    row.emplace("service",
                descriptor.data_service ? Json(resource, *descriptor.data_service)
                                        : Json(resource));
    row.emplace("masks_declared", Json(resource, descriptor.audit_mask_fields_declared));
    row.emplace("indexed_layout", Json(resource, true));
    row.emplace("limit",
                descriptor.audit_value_max_len
                    ? Json(resource, std::to_string(*descriptor.audit_value_max_len))
                    : Json(resource));
    Json::Array masks(resource);
    for (const auto& field : descriptor.audit_mask_fields) {
        masks.emplace_back(Json(resource, field));
    }
    row.emplace("masks", Json(resource, std::move(masks)));
    Json::Array properties(resource);
    for (const auto& property : descriptor.properties) {
        Json::Object item(resource);
        item.emplace("name", Json(resource, property.name));
        item.emplace("data_type", Json(resource, configuration_data_type(property.data_type)));
        item.emplace("nullable", Json(resource, property.nullable));
        // Input column identity follows Rust; physical SQLite spelling retains
        // the separately approved C++ quoted-column compatibility exception.
        item.emplace(
            "column",
            Json(resource,
                 property.json_column_name ? *property.json_column_name : property.column_name));
        item.emplace("id", Json(resource, property.is_id));
        item.emplace("version", Json(resource, property.is_version));
        item.emplace("max_length",
                     property.max_length ? Json(resource, *property.max_length) : Json(resource));
        item.emplace("precision",
                     property.numeric_precision ? Json(resource, *property.numeric_precision)
                                                : Json(resource));
        item.emplace("scale",
                     property.numeric_scale ? Json(resource, *property.numeric_scale)
                                            : Json(resource));
        properties.emplace_back(Json(resource, std::move(item)));
    }
    row.emplace("properties", Json(resource, std::move(properties)));
    Json::Array relations(resource);
    for (const auto& relation : descriptor.relations) {
        Json::Object item(resource);
        item.emplace("name", Json(resource, relation.name));
        item.emplace("target", Json(resource, relation.target_entity));
        item.emplace("local", Json(resource, relation.local_key));
        item.emplace("foreign", Json(resource, relation.foreign_key));
        item.emplace("many", Json(resource, relation.many));
        item.emplace("attach", Json(resource, relation.attach));
        item.emplace("delete_missing", Json(resource, relation.delete_missing));
        relations.emplace_back(Json(resource, std::move(item)));
    }
    row.emplace("relations", Json(resource, std::move(relations)));
    return Json(resource, std::move(row));
}
