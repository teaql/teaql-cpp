#include <teaql/runtime/checker.hpp>

namespace teaql {
std::string_view check_object_status_name(CheckObjectStatus status) {
    constexpr std::array<std::string_view, 3> names{"create", "update", "unknown"};
    return names.at(std::size_t(status));
}

CheckObjectStatus check_object_status(const EntityValues& values) {
    auto explicit_status = values.fields.find(check_object_status_field);
    if (explicit_status != values.fields.end()) {
        if (auto* name = std::get_if<String>(&explicit_status->second.data)) {
            if (*name == "create") {
                return CheckObjectStatus::Create;
            }
            if (*name == "update") {
                return CheckObjectStatus::Update;
            }
        }
    }
    return detail::missing_or_untyped_null(values.fields, "id") ? CheckObjectStatus::Create
                                                                : CheckObjectStatus::Update;
}

Value check_object_status_value(Memory r, CheckObjectStatus status) {
    return Value(r, check_object_status_name(status));
}

void mark_entity_status(EntityValues& values, CheckObjectStatus status) {
    auto r = values.memory_resource();
    values.fields.insert_or_assign(String(check_object_status_field, r),
                                   check_object_status_value(r, status));
}

void clear_entity_status(EntityValues& values) {
    auto found = values.fields.find(check_object_status_field);
    if (found != values.fields.end()) {
        values.fields.erase(found);
    }
}

namespace detail {
bool missing_or_untyped_null(const Record& values, std::string_view name) {
    auto found = values.find(name);
    return found == values.end() || std::holds_alternative<std::monostate>(found->second.data);
}

std::optional<FieldSet> take_checker_fields(EntityValues& values, std::string_view name) {
    auto found = values.fields.find(name);
    if (found == values.fields.end()) {
        return {};
    }
    std::optional<FieldSet> out;
    if (auto* list = std::get_if<Value::List>(&found->second.data)) {
        out.emplace(values.memory_resource());
        for (const auto& field : *list) {
            if (auto* text = std::get_if<String>(&field.data)) {
                out->emplace(*text);
            }
        }
    }
    values.fields.erase(found);
    return out;
}
}

void Checker::required(const EntityValues& values,
                       std::string_view field,
                       const ObjectLocation& location,
                       CheckResults& results) const {
    if (detail::missing_or_untyped_null(values.fields, field)) {
        auto r = results.get_allocator().resource();
        auto path = detail::clone_in(r, location);
        path.member(field);
        results.emplace_back(CheckResult::required(r, std::move(path)));
    }
}

namespace {
void string_length(const EntityValues& values,
                   std::string_view field,
                   std::size_t limit,
                   const ObjectLocation& location,
                   CheckResults& results,
                   bool minimum) {
    auto found = values.fields.find(field);
    if (found == values.fields.end()) {
        return;
    }
    auto* text = std::get_if<String>(&found->second.data);
    if (!text) {
        return;
    }
    std::size_t length = 0;
    for (unsigned char byte : *text) {
        if ((byte & 0xc0) != 0x80) {
            ++length;
        }
    }
    if (minimum ? length < limit : length > limit) {
        auto r = results.get_allocator().resource();
        auto path = detail::clone_in(r, location);
        path.member(field);
        results.emplace_back(minimum ? CheckResult::min_str(r, std::move(path), limit, *text)
                                     : CheckResult::max_str(r, std::move(path), limit, *text));
    }
}
}

void Checker::min_string_length(const EntityValues& values,
                                std::string_view field,
                                std::size_t limit,
                                const ObjectLocation& location,
                                CheckResults& results) const {
    string_length(values, field, limit, location, results, true);
}

void Checker::max_string_length(const EntityValues& values,
                                std::string_view field,
                                std::size_t limit,
                                const ObjectLocation& location,
                                CheckResults& results) const {
    string_length(values, field, limit, location, results, false);
}
}
