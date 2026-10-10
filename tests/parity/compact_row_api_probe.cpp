#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_value.hpp"
#include <iostream>

static CompactRow row(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    Vector<String> names(r);
    Vector<Value> values(r);
    for (const auto& name : std::get<Json::Array>(field(input, "names").data)) {
        names.emplace_back(text(name));
    }
    for (const auto& v : std::get<Json::Array>(field(input, "values").data)) {
        values.emplace_back(value(r, v));
    }
    if (auto* from_map = optional(input, "from_map"); from_map && std::get<bool>(from_map->data)) {
        Record map(r);
        for (std::size_t index = 0; index < names.size(); ++index) {
            map.insert_or_assign(names[index], std::move(values[index]));
        }
        return CompactRow(std::allocator_arg, Allocator(r), std::move(map));
    }
    auto& layout = user.make<ColumnLayout>(std::move(names));
    return CompactRow(std::allocator_arg, Allocator(r), layout, std::move(values));
}

static Json pairs(Memory r, const CompactRow& row) {
    Json::Array output(r);
    for (auto [name, value] : row.iter()) {
        Json::Array pair(r);
        pair.emplace_back(name);
        pair.emplace_back(detail::debug_value(r, value));
        output.emplace_back(pair);
    }
    return Json(r, std::move(output));
}

static Json layout_json(Memory r, const ColumnLayout& layout) {
    Json::Array result(r);
    for (const auto& name : layout.names()) {
        result.emplace_back(name);
    }
    return Json(r, std::move(result));
}

static Json state(Memory r,
                  const CompactRow& row,
                  const CompactRow& clone,
                  std::span<const ColumnLayout* const> saved,
                  const ColumnLayout* before,
                  const std::optional<Value>& previous) {
    Json::Object result(r);
    result.emplace("pairs", pairs(r, row));
    auto consumed_copy = detail::clone_in(r, row);
    auto map = std::move(consumed_copy).into_map(r);
    Json::Array output(r);
    for (const auto& [name, v] : map) {
        Json::Array pair(r);
        pair.emplace_back(name);
        pair.emplace_back(detail::debug_value(r, v));
        output.emplace_back(pair);
    }
    result.emplace("map", Json(r, std::move(output)));
    result.emplace("length", Json(r, row.len()));
    result.emplace("empty", Json(r, row.is_empty()));
    result.emplace("clone", pairs(r, clone));
    result.emplace("equal_clone", Json(r, row == clone));
    result.emplace("same_clone_layout", Json(r, row.shared_columns() == clone.shared_columns()));
    result.emplace("layout_changed", Json(r, row.shared_columns() != before));
    result.emplace("layout", layout_json(r, *row.shared_columns()));
    Json::Array layouts(r);
    for (auto* layout : saved) {
        layouts.emplace_back(layout_json(r, *layout));
    }
    result.emplace("saved", Json(r, std::move(layouts)));
    result.emplace("previous", previous ? Json(r, detail::debug_value(r, *previous)) : Json(r));
    return Json(r, std::move(result));
}

static Json observe(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    auto current = row(user, input);
    if (auto* kind = optional(input, "kind"); kind && text(*kind) == "json_projection") {
        auto map = current.copy_map(r);
        Json::Object out(r);
        out.emplace("row", compact_row_to_json_value(r, current));
        out.emplace("record", record_to_json_value(r, map));
        out.emplace("rank", Json(r, PARTITION_RANK_PROPERTY));
        Json::Array keys(r), values(r);
        for (const auto& key : current.keys()) {
            keys.emplace_back(key);
        }
        for (const auto& item : current.values()) {
            values.emplace_back(detail::debug_value(r, item));
        }
        out.emplace("keys", Json(r, std::move(keys)));
        out.emplace("values", Json(r, std::move(values)));
        return Json(r, std::move(out));
    }
    auto clone = detail::clone_in(r, current);
    Vector<const ColumnLayout*> saved(r);
    Json::Array result(r);
    result.emplace_back(state(r, current, clone, saved, current.shared_columns(), {}));
    for (const auto& operation : std::get<Json::Array>(field(input, "ops").data)) {
        auto* before = current.shared_columns();
        auto name =
            optional(operation, "name") ? text(field(operation, "name")) : std::string_view{};
        auto op = text(field(operation, "op"));
        std::optional<Value> previous;
        if (op == "save") {
            saved.push_back(current.shared_columns());
        } else if (op == "clone") {
            clone = detail::clone_in(r, current);
        } else if (op == "insert") {
            previous = current.insert(name, value(r, field(operation, "value")));
        } else if (op == "remove") {
            previous = current.remove(name);
        } else if (op == "get" || op == "set") {
            auto* existing = current.get_mut(name);
            if (current.contains_key(name) != (existing != nullptr)) {
                throw std::runtime_error("lookup mismatch");
            }
            if (existing) {
                previous.emplace(r, *existing);
                if (op == "set") {
                    *existing = value(r, field(operation, "value"));
                }
            }
        } else if (op == "extend") {
            current.extend(row(user, operation));
        } else if (op == "self_extend") {
            current.extend(current);
        } else if (op == "consume") {
            auto map = std::move(current).into_map(r);
            current = CompactRow(std::allocator_arg, Allocator(r), std::move(map));
        } else {
            throw std::runtime_error("unknown operation");
        }
        result.emplace_back(state(r, current, clone, saved, before, previous));
    }
    return Json(r, std::move(result));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid fixture");
            }
            std::cout << observe(*user, *input).dump(user->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
