#pragma once
#include <teaql/core/value.hpp>

namespace teaql {
// The readonly map emitted by TeaqlEntity is separate from durable #fields.
class GeneratedReadonlyProperties {
public:
    static bool contains(const Record& values, std::string_view key) {
        return key.starts_with('_') && values.contains(key);
    }

    static const Value* get(const Record& values, std::string_view key) {
        if (!key.starts_with('_')) {
            return nullptr;
        }
        const auto found = values.find(key);
        if (found == values.end() || found->second.is_null()) {
            return nullptr;
        }
        if (const auto* json = std::get_if<Json>(&found->second.data);
            json && std::holds_alternative<std::monostate>(json->data)) {
            return nullptr;
        }
        return &found->second;
    }
};
}
