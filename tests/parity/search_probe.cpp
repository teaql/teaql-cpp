#include <teaql/core/dynamic_search.hpp>
#include <teaql/core/dynamic_search_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
using namespace teaql::dynamic_search;

const Json& get(const Json& value, std::string_view key) {
    return std::get<Json::Object>(value.data).find(key)->second;
}

std::string_view text(const Json& value) {
    return std::get<String>(value.data);
}

Json evaluate(Memory r, const Json& input) {
    if (auto it = std::get<Json::Object>(input.data).find("field_contract");
        it != std::get<Json::Object>(input.data).end()) {
        auto model = text(it->second);
        auto changed = text(get(input, "field"));
        auto observed = [&](const auto& before, const auto& after) {
            Json::Object result(r);
            result.emplace("equal", Json(r, before == after));
            result.emplace("before", Json(r, debug_string(r, before)));
            result.emplace("after", Json(r, debug_string(r, after)));
            return Json(r, std::move(result));
        };
        if (model == "warning") {
            DynamicSearchWarning before(
                std::allocator_arg, Allocator(r), "Entity", "FILTER", "field");
            before.code = "CODE";
            auto after = detail::clone_in(r, before);
            if (changed == "code") {
                after.code = "OTHER";
            } else if (changed == "entity") {
                after.entity = "Changed";
            } else if (changed == "clause") {
                after.clause = "ORDER_BY";
            } else if (changed == "field_path") {
                after.field_path = "changed";
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown warning field");
            }
            auto result = observed(before, after);
            auto& object = std::get<Json::Object>(result.data);
            object.emplace("before_json", before.to_json(r));
            object.emplace("after_json", after.to_json(r));
            return result;
        }
        if (model == "filter") {
            DynamicSearchFilter before(
                std::allocator_arg, Allocator(r), "field", "$eq", Json(r, "value"));
            auto after = detail::clone_in(r, before);
            if (changed == "field_path") {
                after.field_path = "changed";
            } else if (changed == "operator") {
                after.operator_name = "$ne";
            } else if (changed == "value") {
                after.value = Json(r, false);
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown filter field");
            }
            return observed(before, after);
        }
        if (model == "order") {
            DynamicSearchOrder before(std::allocator_arg, Allocator(r), "field", false);
            auto after = detail::clone_in(r, before);
            if (changed == "field_path") {
                after.field_path = "changed";
            } else if (changed == "descending") {
                after.descending = true;
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown order field");
            }
            return observed(before, after);
        }
        if (model == "error") {
            DynamicSearchError before{"error"}, after{changed == "message" ? "changed" : "error"};
            if (changed != "message" && changed != "same") {
                throw std::invalid_argument("Unknown error field");
            }
            return observed(before, after);
        }
        throw std::invalid_argument("Unknown search model");
    }
    if (auto it = std::get<Json::Object>(input.data).find("error_contract");
        it != std::get<Json::Object>(input.data).end()) {
        constexpr std::string_view messages[] = {"Invalid trusted search setup",
                                                 "Invalid value for known search field",
                                                 "binding failure",
                                                 "callback failure"};
        auto index = std::get<std::uint64_t>(it->second.data);
        if (index >= std::size(messages)) {
            throw std::invalid_argument("Invalid search-error contract input");
        }
        DynamicSearchError error{messages[index]};
        auto clone = error;
        Json::Object result(r);
        result.emplace("display", Json(r, error.to_string()));
        result.emplace("debug", Json(r, debug_string(r, error)));
        result.emplace("equal_clone", Json(r, error == clone));
        result.emplace("source_none", Json(r, error.source() == nullptr));
        return Json(r, std::move(result));
    }
    SearchModels models(r);
    for (const auto& [name, model] : std::get<Json::Object>(get(input, "models").data)) {
        auto [entry, inserted] = models.try_emplace(name);
        (void)inserted;
        for (const auto& [field, kind] : std::get<Json::Object>(get(model, "fields").data)) {
            entry->second.fields.emplace(field, text(kind));
        }
        for (const auto& [relation, target] :
             std::get<Json::Object>(get(model, "relations").data)) {
            entry->second.relations.emplace(relation, text(target));
        }
    }
    Json emitted(r);
    emitted.data = Json::Array(r);
    auto capture = [&](const DynamicSearchWarning& warning) {
        std::get<Json::Array>(emitted.data).emplace_back(warning.to_json(r));
    };
    WarningSink sink(capture);
    auto result = normalize_dynamic_search(r,
                                           text(get(input, "source")),
                                           text(get(input, "entity")),
                                           models,
                                           std::get<std::uint64_t>(get(input, "maxClauses").data),
                                           &sink);
    Json output(r);
    Json::Object object(r);
    object.emplace("emitted", std::move(emitted));
    if (!result) {
        object.emplace("error", Json(r, result.error().message));
    } else {
        Json body(r);
        Json::Object fields(r);
        Json filters(r), orders(r), warnings(r);
        filters.data = Json::Array(r);
        orders.data = Json::Array(r);
        warnings.data = Json::Array(r);
        for (const auto& filter : result.value().filters) {
            Json j(r);
            Json::Object o(r);
            o.emplace("fieldPath", Json(r, filter.field_path));
            o.emplace("operator", Json(r, filter.operator_name));
            o.emplace("value", Json(r, filter.value));
            j.data = std::move(o);
            std::get<Json::Array>(filters.data).emplace_back(std::move(j));
        }
        for (const auto& order : result.value().orders) {
            Json j(r);
            Json::Object o(r);
            o.emplace("fieldPath", Json(r, order.field_path));
            o.emplace("descending", Json(r, order.descending));
            j.data = std::move(o);
            std::get<Json::Array>(orders.data).emplace_back(std::move(j));
        }
        for (const auto& warning : result.value().warnings) {
            std::get<Json::Array>(warnings.data).emplace_back(warning.to_json(r));
        }
        fields.emplace("filters", std::move(filters));
        fields.emplace("orders", std::move(orders));
        fields.emplace("warnings", std::move(warnings));
        body.data = std::move(fields);
        object.emplace("result", std::move(body));
    }
    output.data = std::move(object);
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid search fixture");
        }
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
