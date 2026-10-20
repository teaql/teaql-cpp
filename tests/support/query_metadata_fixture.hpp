#pragma once
#include "../parity/fixture_query_keys.hpp"

// Input decoding only; production canonicalization/hash methods are unchanged.
inline void query_metadata(teaql::Memory r, teaql::SelectQuery& query, const teaql::Json& input) {
    using namespace teaql;
    if (auto* purpose = optional(input, "purpose")) {
        if (auto* value = std::get_if<String>(&purpose->data)) {
            query.purpose_text.emplace(*value, r);
        }
    }
    if (auto* schema = optional(input, "readonly_schema")) {
        Vector<DynamicPropertyDefinitions::Field> fields(r);
        for (const auto& value : std::get<Json::Array>(schema->data)) {
            const auto& pair = std::get<Json::Array>(value.data);
            fields.emplace_back(text(pair[0]), static_cast<DataType>(integer(pair[1])));
        }
        auto definitions = DynamicPropertyDefinitions::from_fields(r, fields);
        if (!definitions) {
            throw std::runtime_error(std::string(definitions.error()));
        }
        query.dynamic_property_definitions =
            detail::make_box<DynamicPropertyDefinitions>(r, std::move(definitions).value());
    }
    auto relations = items(input, "relations");
    for (std::size_t i = 0; i < query.relations.size(); ++i) {
        if (query.relations[i].query) {
            query_metadata(r, *query.relations[i].query, field(relations[i], "query"));
        }
    }
    auto groups = items(input, "object_groups");
    for (std::size_t i = 0; i < query.object_group_bys.size(); ++i) {
        query_metadata(r, query.object_group_bys[i].query, field(groups[i], "query"));
    }
    auto children = items(input, "children");
    for (std::size_t i = 0; i < query.child_enhancements.size(); ++i) {
        query_metadata(r, query.child_enhancements[i], children[i]);
    }
}
