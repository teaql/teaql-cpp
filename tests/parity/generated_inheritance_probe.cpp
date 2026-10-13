#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace cpp_inheritance;

static Json evaluation(Memory r, const eval::EvalResult<String>& value) {
    Json::Object result(r);
    if (value.is_value()) {
        result.emplace(String("state", r), Json(r, "Value"));
        result.emplace(String("value", r), Json(r, value.get()));
    } else if (value.is_null()) {
        result.emplace(String("state", r), Json(r, "Null"));
    } else {
        result.emplace(String("state", r), Json(r, "NotLoaded"));
        result.emplace(String("node", r), Json(r, value.missing().failed_node));
        result.emplace(String("path", r), Json(r, value.missing().attempted_path));
    }
    return Json(r, result);
}

template <class T>
static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto descriptor = T::entity_descriptor(r);
    const auto& layout = T::field_layout(r);
    Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t(7)));
    row.emplace(String("name", r), Value(r, "original"));
    row.emplace(String("rank", r), Value(r, std::int64_t(1)));
    row.emplace(String("platform_id", r), Value(r, std::uint64_t(42)));
    row.emplace(String("version", r), Value(r, std::int64_t(3)));
    if (descriptor.find_property("campus_code")) {
        row.emplace(String("campus_code", r), Value(r, "north"));
    }
    if (descriptor.find_property("budget")) {
        row.emplace(String("budget", r), Value(r, *Decimal::parse("12.50")));
    }
    auto decoded = T::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
    auto& entity = user_context.make<T>(decoded.value());
    if (!std::get<bool>(field(input, "loaded").data)) {
        const std::array names{std::string_view("id"), std::string_view("version")};
        entity.load_state =
            eval::LoadState::partial(r, names)
                .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(), layout)
                .value();
    }
    auto& copy = user_context.make<T>(entity);
    auto before = evaluation(r, entity.evalName());
    if (std::get<bool>(field(input, "update").data)) {
        entity.updateName(text(field(input, "text")));
        entity.updateRank(-17);
    }
    auto encoded = std::move(detail::clone_in(r, entity)).into_values();
    Json::Object native(r);
    for (const auto& [name, value] : encoded.fields) {
        if (!name.starts_with('_')) {
            native.emplace(String(name, r), value.to_json_value(r));
        }
    }
    Json::Array indexes(r), mappings(r), properties(r), masks(r);
    std::pmr::map<std::size_t, const PropertyDescriptor*> ordered(r);
    for (const auto& property : descriptor.properties) {
        ordered.emplace(*layout.index(property.name), &property);
        Json::Object item(r);
        item.emplace(String("name", r), Json(r, property.name));
        item.emplace(String("column", r), Json(r, property.column_name));
        item.emplace(String("nullable", r), Json(r, property.nullable));
        item.emplace(String("id", r), Json(r, property.is_id));
        item.emplace(String("version", r), Json(r, property.is_version));
        properties.emplace_back(item);
    }
    for (const auto& [index, property] : ordered) {
        auto canonical = property->name == "platform_id" ? std::string_view("platform")
                                                         : std::string_view(property->name);
        Json::Array position(r), mapping(r);
        position.emplace_back(canonical);
        position.emplace_back(std::uint64_t(*layout.index(canonical)));
        mapping.emplace_back(canonical);
        mapping.emplace_back(property->name);
        mapping.emplace_back(property->column_name);
        indexes.emplace_back(position);
        mappings.emplace_back(mapping);
    }
    for (const auto& mask : descriptor.audit_mask_fields) {
        masks.emplace_back(mask);
    }
    Json::Object output(r);
    output.emplace(String("revision", r), Json(r, T::FIELD_LAYOUT_REVISION));
    output.emplace(String("indexes", r), Json(r, indexes));
    output.emplace(String("mappings", r), Json(r, mappings));
    output.emplace(String("descriptor", r), Json(r, properties));
    output.emplace(String("table", r), Json(r, descriptor.table_name));
    output.emplace(String("masks", r), Json(r, masks));
    output.emplace(String("mask_declared", r), Json(r, descriptor.audit_mask_fields_declared));
    output.emplace(String("before", r), std::move(before));
    output.emplace(String("after", r), evaluation(r, entity.evalName()));
    output.emplace(String("copy", r), evaluation(r, copy.evalName()));
    output.emplace(String("name", r), Json(r, entity.name()));
    output.emplace(String("rank", r), Json(r, entity.rank()));
    output.emplace(String("platform", r), Json(r, entity.platformId()));
    output.emplace(String("native", r), Json(r, native));
    return Json(r, output);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid inherited model fixture");
            }
            auto name = text(field(*input, "entity"));
            auto result = name == "school"    ? observe<School>(*user_context, *input)
                          : name == "academy" ? observe<Academy>(*user_context, *input)
                                              : observe<ResearchAcademy>(*user_context, *input);
            std::cout << result.dump(user_context->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
