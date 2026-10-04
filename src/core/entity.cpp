#include <teaql/core/entity.hpp>

namespace teaql {
Record BaseEntityData::to_values_map(Memory r) const {
    Record values(r); values.emplace("id", Value(r, id)); values.emplace("version", Value(r, version));
    // Dynamic fields deliberately win on collision, just as the Rust map insertion does.
    for (const auto& [key, value] : dynamic_values) values.insert_or_assign(String(key, r), Value(r, value));
    return values;
}
Result<BaseEntityData, EntityError> BaseEntityData::from_values_map(Memory r, const Record& values) {
    using Outcome = Result<BaseEntityData, EntityError>;
    BaseEntityData data(std::allocator_arg, Allocator(r));
    const auto invalid = [&](std::string_view field, const Value& value) {
        String message("invalid ", r); message += field; message += " field: "; message += value.to_json_value(r).dump(r);
        return Outcome::err(r, EntityError(std::allocator_arg, Allocator(r), "BaseEntity", message));
    };
    if (auto it = values.find("id"); it != values.end()) {
        const auto& v = it->second;
        if (auto n = std::get_if<std::uint64_t>(&v.data)) data.id = *n;
        else if (auto n = std::get_if<std::int64_t>(&v.data); n && *n >= 0) data.id = std::uint64_t(*n);
        else if (!std::holds_alternative<std::monostate>(v.data)) return invalid("id", v);
    }
    if (auto it = values.find("version"); it != values.end()) {
        const auto& v = it->second;
        if (auto n = std::get_if<std::int64_t>(&v.data)) data.version = *n;
        else if (!std::holds_alternative<std::monostate>(v.data)) return invalid("version", v);
    }
    for (const auto& [key, value] : values)
        if (key != "id" && key != "version") data.dynamic_values.emplace(key, value);
    return Outcome::ok(r, std::move(data));
}
}
