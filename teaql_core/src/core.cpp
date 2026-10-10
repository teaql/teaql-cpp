#include <teaql/core/meta.hpp>
#include <teaql/core/query.hpp>
#include <algorithm>

namespace teaql {
std::optional<std::int64_t> Value::try_i64() const {
    if (auto v = std::get_if<Decimal>(&data)) {
        return v->to_i64();
    }
    if (auto v = std::get_if<std::int64_t>(&data)) {
        return *v;
    }
    if (auto v = std::get_if<std::uint64_t>(&data); v && *v <= std::uint64_t(INT64_MAX)) {
        return std::int64_t(*v);
    }
    return {};
}

std::optional<std::uint64_t> Value::try_u64() const {
    if (auto v = std::get_if<Decimal>(&data)) {
        return v->to_u64();
    }
    if (auto v = std::get_if<std::uint64_t>(&data)) {
        return *v;
    }
    if (auto v = std::get_if<std::int64_t>(&data); v && *v >= 0) {
        return std::uint64_t(*v);
    }
    return {};
}

String default_table_name(Memory r, std::string_view name) {
    String out(r);
    for (std::size_t i = 0; i < name.size(); ++i) {
        auto c = name[i];
        if (c >= 'A' && c <= 'Z') {
            if (i) {
                out += '_';
            }
            c = char(c - 'A' + 'a');
        }
        out += c;
    }
    out += "_data";
    return out;
}

Record clone_record(Memory r, const Record& source) {
    Record result(r);
    for (const auto& [key, value] : source) {
        result.emplace(String(key, r), Value(r, value));
    }
    return result;
}

const PropertyDescriptor* EntityDescriptor::find_property(std::string_view field) const noexcept {
    for (const auto& p : properties) {
        if (p.name == field) {
            return &p;
        }
    }
    return nullptr;
}

const RelationDescriptor* EntityDescriptor::find_relation(std::string_view name) const noexcept {
    for (const auto& relation : relations) {
        if (relation.name == name) {
            return &relation;
        }
    }
    return nullptr;
}

const PropertyDescriptor& EntityDescriptor::require_property(std::string_view field) const {
    if (const auto* p = find_property(field)) {
        return *p;
    }
    throw std::invalid_argument("UnknownField");
}

const RelationDescriptor& EntityDescriptor::require_relation(std::string_view name) const {
    if (const auto* relation = find_relation(name)) {
        return *relation;
    }
    throw std::invalid_argument("UnknownRelation");
}

const PropertyDescriptor* EntityDescriptor::id_property() const noexcept {
    for (const auto& p : properties) {
        if (p.is_id) {
            return &p;
        }
    }
    return nullptr;
}

const PropertyDescriptor* EntityDescriptor::version_property() const noexcept {
    for (const auto& p : properties) {
        if (p.is_version) {
            return &p;
        }
    }
    return nullptr;
}

const PropertyDescriptor& EntityDescriptor::require_id_property() const {
    if (const auto* p = id_property()) {
        return *p;
    }
    throw std::invalid_argument("MissingIdProperty");
}

const PropertyDescriptor& EntityDescriptor::require_version_property() const {
    if (const auto* p = version_property()) {
        return *p;
    }
    throw std::invalid_argument("MissingVersionProperty");
}
}
