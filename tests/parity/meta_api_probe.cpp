#include <teaql/core/meta_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static std::optional<std::uint64_t> integer_option(const Json& input) {
    if (std::holds_alternative<std::monostate>(input.data)) {
        return {};
    }
    return std::get<std::uint64_t>(input.data);
}

static Json equality(Memory owner, const Json& input) {
    auto model = text(field(input, "model"));
    auto changed = text(field(input, "field"));
    auto result = detail::model_json(owner);
    auto record = [&](const auto& before, const auto& after) {
        detail::model_put(result, "equal", before == after);
        detail::model_put(result, "before", debug_string(owner, before));
        detail::model_put(result, "after", debug_string(owner, after));
    };
    if (model == "property") {
        PropertyDescriptor before(owner, "field", DataType::Text);
        auto after = PropertyDescriptor(std::allocator_arg, Allocator(owner), before);
        if (changed == "name") {
            after.name = "changed";
        } else if (changed == "data_type") {
            after.data_type = DataType::LargeText;
        } else if (changed == "nullable") {
            after.nullable = false;
        } else if (changed == "column_name") {
            after.column_name = "";
        } else if (changed == "is_id") {
            after.is_id = true;
        } else if (changed == "is_version") {
            after.is_version = true;
        } else if (changed == "max_length") {
            after.max_length = 0;
        } else if (changed == "numeric_precision") {
            after.numeric_precision = 0;
        } else if (changed == "numeric_scale") {
            after.numeric_scale = 0;
        } else if (changed != "same") {
            throw std::invalid_argument("Unknown property field");
        }
        record(before, after);
    } else if (model == "relation") {
        RelationDescriptor before(std::allocator_arg, Allocator(owner), "relation", "Target");
        auto after = RelationDescriptor(std::allocator_arg, Allocator(owner), before);
        if (changed == "name") {
            after.name = "changed";
        } else if (changed == "target_entity") {
            after.target_entity = "changed";
        } else if (changed == "local_key") {
            after.local_key = "";
        } else if (changed == "foreign_key") {
            after.foreign_key = "";
        } else if (changed == "many") {
            after.many = true;
        } else if (changed == "attach") {
            after.attach = false;
        } else if (changed == "delete_missing") {
            after.delete_missing = false;
        } else if (changed != "same") {
            throw std::invalid_argument("Unknown relation field");
        }
        record(before, after);
    } else if (model == "entity") {
        EntityDescriptor before(std::allocator_arg, Allocator(owner), "Entity");
        before.property("first", DataType::Text);
        before.property("second", DataType::I64);
        before.relation("first", "First");
        before.relation("second", "Second");
        before.mask_fields({"first", "second"});
        auto after = EntityDescriptor(std::allocator_arg, Allocator(owner), before);
        if (changed == "name") {
            after.name = "changed";
        } else if (changed == "table_name") {
            after.table_name = "";
        } else if (changed == "data_service") {
            after.service("");
        } else if (changed == "properties") {
            after.properties.pop_back();
        } else if (changed == "relations") {
            after.relations.pop_back();
        } else if (changed == "audit_mask_fields") {
            after.audit_mask_fields.pop_back();
        } else if (changed == "audit_mask_fields_declared") {
            after.audit_mask_fields_declared = false;
        } else if (changed == "audit_value_max_len") {
            after.audit_value_max_len = 0;
        } else if (changed == "property_value") {
            after.properties[0].column_name = "changed";
        } else if (changed == "relation_value") {
            after.relations[0].attach = false;
        } else if (changed == "property_order" || changed == "relation_order") {
            // Descriptors are move-constructible, with explicit allocator-aware copies.
            auto replacement = EntityDescriptor(std::allocator_arg, Allocator(owner), before);
            if (changed == "property_order") {
                after.properties.clear();
                after.property(replacement.properties[1]).property(replacement.properties[0]);
            } else {
                after.relations.clear();
                after.relation(replacement.relations[1]).relation(replacement.relations[0]);
            }
        } else if (changed == "mask_order") {
            after.mask_fields({"second", "first"});
        } else if (changed != "same") {
            throw std::invalid_argument("Unknown entity field");
        }
        record(before, after);
    } else {
        throw std::invalid_argument("Unknown descriptor model");
    }
    return result;
}

static Json evaluate(Memory owner, const Json& input) {
    if (const auto* kind = optional(input, "kind")) {
        if (text(*kind) == "equality") {
            return equality(owner, input);
        }
        if (text(*kind) == "naming") {
            auto name = text(field(input, "name"));
            auto result = detail::model_json(owner);
            EntityDescriptor entity(std::allocator_arg, Allocator(owner), name);
            detail::model_put(result, "table", default_table_name(owner, name));
            detail::model_put(result, "entity_table", entity.table_name);
            return result;
        }
    }
    auto mode = std::get<std::uint64_t>(field(input, "mode").data);
    PropertyDescriptor property(owner,
                                text(field(input, "property")),
                                DataType(std::get<std::uint64_t>(field(input, "type").data)));
    RelationDescriptor relation(std::allocator_arg,
                                Allocator(owner),
                                text(field(input, "relation")),
                                text(field(input, "target")));
    EntityDescriptor entity(std::allocator_arg, Allocator(owner), text(field(input, "entity")));
    auto defaults = detail::model_json(owner);
    detail::model_put(defaults, "property", formatted_debug(owner, property));
    detail::model_put(defaults, "relation", formatted_debug(owner, relation));
    detail::model_put(defaults, "entity", formatted_debug(owner, entity));
    property.column(text(field(input, "column")));
    if (mode & 1) {
        property.not_null();
    }
    if (mode & 2) {
        property.id();
    }
    if (mode & 4) {
        property.version();
    }
    if (auto n = integer_option(field(input, "length"))) {
        property.with_max_length(std::uint32_t(*n));
    }
    if (auto n = integer_option(field(input, "precision"))) {
        property.with_numeric_precision(std::uint32_t(*n));
    }
    if (auto n = integer_option(field(input, "scale"))) {
        property.with_numeric_scale(std::uint32_t(*n));
    }
    relation.local(text(field(input, "local"))).foreign(text(field(input, "foreign")));
    if (mode & 1) {
        relation.to_many();
    }
    if (mode & 2) {
        relation.detached().attached().detached();
    } else {
        relation.detached().attached();
    }
    if (mode & 4) {
        relation.keep_missing().remove_missing().keep_missing();
    } else {
        relation.keep_missing().remove_missing();
    }
    auto duplicate = PropertyDescriptor(std::allocator_arg, Allocator(owner), property);
    duplicate.column("duplicate-column").id().version();
    auto duplicate_relation = RelationDescriptor(std::allocator_arg, Allocator(owner), relation);
    duplicate_relation.foreign("duplicate-foreign");
    entity.table(text(field(input, "table")))
        .property(property)
        .property(duplicate)
        .relation(relation)
        .relation(duplicate_relation);
    if (const auto* service = std::get_if<String>(&field(input, "service").data)) {
        entity.service(*service);
    }
    if (const auto* masks = std::get_if<Json::Array>(&field(input, "masks").data)) {
        auto names = *masks | std::views::transform([](const Json& value) {
            return text(value);
        });
        entity.mask_fields(names);
    }
    auto maximum = integer_option(field(input, "max"));
    entity.audit_max_length(maximum ? std::optional<std::size_t>(*maximum) : std::nullopt);
    auto clone = EntityDescriptor(std::allocator_arg, Allocator(owner), entity);
    auto changed = EntityDescriptor(std::allocator_arg, Allocator(owner), entity);
    changed.table("changed").mask_fields({}).audit_max_length({});
    auto result = detail::model_json(owner);
    detail::model_put(result, "defaults", defaults);
    detail::model_put(result, "property", formatted_debug(owner, property));
    detail::model_put(result, "relation", formatted_debug(owner, relation));
    detail::model_put(result, "entity", formatted_debug(owner, entity));
    detail::model_put(result, "clone", formatted_debug(owner, clone));
    detail::model_put(result, "changed", formatted_debug(owner, changed));
    detail::model_put(result, "equal", entity == clone);
    detail::model_put(result, "changed_equal", entity == changed);
    auto shown = [&](const auto* value) {
        return value ? Json(owner, debug_string(owner, *value)) : Json(owner);
    };
    detail::model_put(result, "property_lookup", shown(entity.property_by_name(property.name)));
    detail::model_put(result, "relation_lookup", shown(entity.relation_by_name(relation.name)));
    detail::model_put(result, "missing_property", !entity.property_by_name("definitely missing"));
    detail::model_put(result, "missing_relation", !entity.relation_by_name("definitely missing"));
    detail::model_put(result, "id", shown(entity.id_property()));
    detail::model_put(result, "version", shown(entity.version_property()));
    Json::Array writable(owner);
    for (const auto& value : entity.writable_properties()) {
        writable.emplace_back(debug_string(owner, value));
    }
    detail::model_put(result, "writable", writable);
    return result;
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::invalid_argument("Invalid metadata input");
            }
            std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
