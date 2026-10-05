#include <teaql/core/dynamic_search.hpp>
#include <array>
#include <cmath>
#include <iostream>

namespace teaql::dynamic_search {
DynamicSearchWarning::DynamicSearchWarning(std::allocator_arg_t,
                                           Allocator a,
                                           std::string_view entity,
                                           std::string_view clause,
                                           std::string_view path)
    : code("DYNAMIC_SEARCH_UNKNOWN_FIELD", a.resource()), entity(entity, a.resource()),
      clause(clause, a.resource()), field_path(path, a.resource()) {}

DynamicSearchWarning::DynamicSearchWarning(std::allocator_arg_t,
                                           Allocator a,
                                           const DynamicSearchWarning& other)
    : DynamicSearchWarning(std::allocator_arg, a, other.entity, other.clause, other.field_path) {
    code = other.code;
}

Json DynamicSearchWarning::to_json(Memory r) const {
    Json json(r);
    Json::Object object(r);
    object.emplace("code", Json(r, code));
    object.emplace("entity", Json(r, entity));
    object.emplace("clause", Json(r, clause));
    object.emplace("fieldPath", Json(r, field_path));
    json.data = std::move(object);
    return json;
}

DynamicSearchFilter::DynamicSearchFilter(std::allocator_arg_t,
                                         Allocator a,
                                         std::string_view path,
                                         std::string_view op,
                                         const Json& value)
    : field_path(path, a.resource()), operator_name(op, a.resource()), value(a.resource(), value) {}

DynamicSearchFilter::DynamicSearchFilter(std::allocator_arg_t,
                                         Allocator a,
                                         const DynamicSearchFilter& other)
    : DynamicSearchFilter(
          std::allocator_arg, a, other.field_path, other.operator_name, other.value) {}

DynamicSearchOrder::DynamicSearchOrder(std::allocator_arg_t,
                                       Allocator a,
                                       std::string_view path,
                                       bool descending)
    : field_path(path, a.resource()), descending(descending) {}

DynamicSearchOrder::DynamicSearchOrder(std::allocator_arg_t,
                                       Allocator a,
                                       const DynamicSearchOrder& other)
    : DynamicSearchOrder(std::allocator_arg, a, other.field_path, other.descending) {}

NormalizedDynamicSearch::NormalizedDynamicSearch(std::allocator_arg_t, Allocator a)
    : filters(a.resource()), orders(a.resource()), warnings(a.resource()) {}

NormalizedDynamicSearch::NormalizedDynamicSearch(std::allocator_arg_t,
                                                 Allocator a,
                                                 const NormalizedDynamicSearch& other)
    : NormalizedDynamicSearch(std::allocator_arg, a) {
    for (const auto& filter : other.filters) {
        filters.emplace_back(filter);
    }
    for (const auto& order : other.orders) {
        orders.emplace_back(order);
    }
    for (const auto& warning : other.warnings) {
        warnings.emplace_back(warning);
    }
}

DynamicSearchResult::DynamicSearchResult(std::allocator_arg_t,
                                         Allocator a,
                                         const SelectQuery& query,
                                         std::span<const DynamicSearchWarning> warnings)
    : query(std::allocator_arg, a, query), warnings(a.resource()) {
    for (const auto& warning : warnings) {
        this->warnings.emplace_back(warning);
    }
}

DynamicSearchResult::DynamicSearchResult(std::allocator_arg_t,
                                         Allocator a,
                                         const DynamicSearchResult& other)
    : DynamicSearchResult(std::allocator_arg, a, other.query, other.warnings) {}

namespace {
[[noreturn]] void fail(std::string_view message) {
    throw DynamicSearchError{message};
}

const Json* get(const Json::Object& object, std::string_view key) {
    auto it = object.find(key);
    return it == object.end() ? nullptr : &it->second;
}

std::optional<std::string_view>
field_type(std::string_view path, std::string_view entity, const SearchModels& models) {
    std::array<std::string_view, 16> parts;
    std::size_t count = 0;
    while (true) {
        if (count == parts.size()) {
            fail("Invalid search field path");
        }
        auto dot = path.find('.');
        auto part = path.substr(0, dot);
        if (part.empty() || part.starts_with('$') || part == "__proto__" || part == "prototype" ||
            part == "constructor") {
            fail("Invalid search field path");
        }
        parts[count++] = part;
        if (dot == path.npos) {
            break;
        }
        path.remove_prefix(dot + 1);
    }
    const auto* model = &models.find(entity)->second;
    for (std::size_t i = 0; i + 1 < count; ++i) {
        auto relation = model->relations.find(parts[i]);
        if (relation == model->relations.end()) {
            return {};
        }
        auto target = models.find(relation->second);
        if (target == models.end()) {
            fail("Invalid trusted search relation metadata");
        }
        model = &target->second;
    }
    auto field = model->fields.find(parts[count - 1]);
    if (field == model->fields.end()) {
        return {};
    }
    return std::string_view(field->second);
}

bool decimal_text(std::string_view text) {
    if (!text.empty() && (text.front() == '+' || text.front() == '-')) {
        text.remove_prefix(1);
    }
    bool dot = false, digit = false;
    for (char c : text) {
        if (c == '.') {
            if (dot || !digit) {
                return false;
            }
            dot = true;
            digit = false;
        } else if (c >= '0' && c <= '9') {
            digit = true;
        } else {
            return false;
        }
    }
    return digit;
}

void validate_scalar(Memory r, const Json& value, std::string_view kind) {
    if (std::holds_alternative<std::monostate>(value.data)) {
        return;
    }
    std::optional<double> number;
    if (auto n = std::get_if<double>(&value.data); n && std::isfinite(*n)) {
        number = *n;
    }
    if (auto n = std::get_if<std::int64_t>(&value.data)) {
        number = double(*n);
    }
    if (auto n = std::get_if<std::uint64_t>(&value.data)) {
        number = double(*n);
    }
    const auto* text = std::get_if<String>(&value.data);
    bool valid = false;
    if (kind == "integer" || kind == "timestamp") {
        valid = number && std::abs(*number) <= 9007199254740991.0 && std::trunc(*number) == *number;
    } else if (kind == "number") {
        valid = bool(number);
    } else if (kind == "string") {
        valid = text;
    } else if (kind == "boolean") {
        valid = std::holds_alternative<bool>(value.data);
    } else if (kind == "decimal") {
        valid = number || (text && decimal_text(*text));
    } else if (kind == "date" && text && text->size() == 10 && !text->starts_with("0000")) {
        if (auto date = Date::parse(*text)) {
            valid = date->to_string(r) == *text;
        }
    }
    if (!valid) {
        fail("Invalid value for known search field");
    }
}

void emit(std::span<const DynamicSearchWarning> warnings, WarningSink* sink) {
    for (const auto& warning : warnings) {
        if (sink) {
            (*sink)(warning);
        } else {
            auto r = warning.entity.get_allocator().resource();
            auto json = warning.to_json(r);
            std::get<Json::Object>(json.data).find("fieldPath")->second = Json(r, "<omitted>");
            std::cerr << json.dump(r) << '\n';
        }
    }
}
}

SearchResult<NormalizedDynamicSearch> normalize_dynamic_search(Memory r,
                                                               std::string_view source,
                                                               std::string_view entity,
                                                               const SearchModels& models,
                                                               std::size_t limit,
                                                               WarningSink* warn) {
    using Outcome = SearchResult<NormalizedDynamicSearch>;
    auto validate = [&]() -> Outcome {
        try {
            if (!limit || !models.contains(entity)) {
                fail("Invalid trusted search setup");
            }
            auto value = Json::parse(r, source);
            if (!value) {
                fail("Dynamic search requires valid JSON");
            }
            auto root = std::get_if<Json::Object>(&value->data);
            if (!root) {
                fail("Expected search object");
            }
            for (const auto& [key, unused] : *root) {
                (void)unused;
                if (key != "filter" && key != "orderBy") {
                    fail("Unsupported dynamic search control");
                }
            }
            const Json::Object empty_filters(r);
            const Json::Array empty_orders(r);
            const auto* filters = &empty_filters;
            const auto* orders = &empty_orders;
            if (auto input = get(*root, "filter")) {
                filters = std::get_if<Json::Object>(&input->data);
                if (!filters) {
                    fail("Invalid search filter");
                }
            }
            if (auto input = get(*root, "orderBy")) {
                orders = std::get_if<Json::Array>(&input->data);
                if (!orders) {
                    fail("Invalid search ordering");
                }
            }
            if (filters->size() > limit || orders->size() > limit - filters->size()) {
                fail("Dynamic search exceeds clause limit");
            }
            NormalizedDynamicSearch result(std::allocator_arg, Allocator(r));
            for (const auto& [path, predicate] : *filters) {
                std::string_view op = "$eq";
                const Json* operand = &predicate;
                if (auto parts = std::get_if<Json::Object>(&predicate.data)) {
                    if (parts->size() != 1) {
                        fail("Malformed dynamic search operator");
                    }
                    op = parts->begin()->first;
                    operand = &parts->begin()->second;
                    if (op != "$eq" && op != "$ne" && op != "$gt" && op != "$gte" && op != "$lt" &&
                        op != "$lte" && op != "$in" && op != "$notIn" && op != "$contains") {
                        fail("Unsupported dynamic search operator");
                    }
                }
                auto items = std::get_if<Json::Array>(&operand->data);
                if ((op == "$in" || op == "$notIn") && (!items || items->size() > 1000)) {
                    fail("Invalid or oversized search value list");
                }
                auto kind = field_type(path, entity, models);
                if (!kind) {
                    result.warnings.emplace_back(entity, "FILTER", path);
                    continue;
                }
                if (op == "$contains" && *kind != "string") {
                    fail("String operator requires a string field");
                }
                if (items) {
                    if (op != "$in" && op != "$notIn") {
                        fail("Unexpected search value list");
                    }
                    for (const auto& item : *items) {
                        validate_scalar(r, item, *kind);
                    }
                } else {
                    validate_scalar(r, *operand, *kind);
                }
                result.filters.emplace_back(path, op, *operand);
            }
            for (const auto& value : *orders) {
                auto order = std::get_if<Json::Object>(&value.data);
                if (!order) {
                    fail("Invalid search ordering");
                }
                auto field = get(*order, "field");
                auto path = field ? std::get_if<String>(&field->data) : nullptr;
                if (!path) {
                    fail("Invalid ordering field");
                }
                auto direction_value = get(*order, "direction");
                auto direction =
                    direction_value ? std::get_if<String>(&direction_value->data) : nullptr;
                if (!direction) {
                    fail("Invalid ordering direction");
                }
                if (order->size() != 2 || (*direction != "asc" && *direction != "desc")) {
                    fail("Invalid dynamic search ordering");
                }
                if (!field_type(*path, entity, models)) {
                    result.warnings.emplace_back(entity, "ORDER_BY", *path);
                } else {
                    result.orders.emplace_back(*path, *direction == "desc");
                }
            }
            return Outcome::ok(r, std::move(result));
        } catch (const DynamicSearchError& error) {
            return Outcome::err(r, error);
        }
    };
    auto result = validate();
    if (result) {
        emit(result.value().warnings, warn);
    }
    return result;
}

SearchResult<DynamicSearchResult> merge_dynamic_search(Memory r,
                                                       const SelectQuery& base,
                                                       std::string_view source,
                                                       const SearchModels& models,
                                                       FilterBinding filter_binding,
                                                       OrderBinding order_binding,
                                                       WarningSink* warn) {
    using Outcome = SearchResult<DynamicSearchResult>;
    auto ignore = [](const DynamicSearchWarning&) {};
    WarningSink silent(ignore);
    auto search = normalize_dynamic_search(r, source, base.entity, models, 100, &silent);
    if (!search) {
        return Outcome::err(r, search.error());
    }
    Vector<Expr> filters(r);
    Vector<OrderBy> orders(r);
    for (const auto& filter : search.value().filters) {
        auto result = filter_binding(filter);
        if (!result) {
            return Outcome::err(r, result.error());
        }
        filters.emplace_back(std::move(result).value());
    }
    for (const auto& order : search.value().orders) {
        auto result = order_binding(order);
        if (!result) {
            return Outcome::err(r, result.error());
        }
        orders.emplace_back(std::move(result).value());
    }
    SelectQuery query(std::allocator_arg, Allocator(r), base);
    for (const auto& filter : filters) {
        query.and_filter(filter);
    }
    for (auto& order : orders) {
        query.order_by.emplace_back(std::move(order));
    }
    emit(search.value().warnings, warn);
    return Outcome::ok(
        r, DynamicSearchResult(std::allocator_arg, Allocator(r), query, search.value().warnings));
}
}
