#include <teaql/core/dynamic_search.hpp>
#include <teaql/core/dynamic_search_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

namespace ds = teaql::dynamic_search;
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static Json snapshot(Memory owner,
                     const ds::SearchModel& model,
                     const ds::NormalizedDynamicSearch& normalized,
                     const ds::DynamicSearchResult& result) {
    Json::Object fields(owner);
    fields.emplace("model", Json(owner, debug_string(owner, model)));
    fields.emplace("normalized", Json(owner, debug_string(owner, normalized)));
    fields.emplace("result", Json(owner, debug_string(owner, result)));
    return Json(owner, std::move(fields));
}

static Json observe(GlobalContext& global, const Json& input) {
    const auto owner = global.memory_resource();
    const auto& fields = std::get<Json::Object>(input.data);
    const auto& text = std::get<String>(fields.find("text")->second.data);
    const auto flags = std::get<std::uint64_t>(fields.find("flags")->second.data);
    const auto& value = fields.find("value")->second;
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    auto local = user_context.memory_resource();
    auto& default_model = user_context.make<ds::SearchModel>();
    auto& model = user_context.make<ds::SearchModel>();
    if (flags & 1) {
        model.fields.emplace(text, "string");
        model.relations.emplace("child", text);
    }
    auto& normalized = user_context.make<ds::NormalizedDynamicSearch>();
    if (flags & 1) {
        normalized.filters.emplace_back(text, "$eq", value);
    }
    if (flags & 2) {
        normalized.orders.emplace_back(text, true);
    }
    if (flags != 0) {
        normalized.warnings.emplace_back("School", "FILTER", text);
    }
    auto& query = user_context.make<SelectQuery>("School");
    query.filter(Expr::eq(local, "name", std::string_view(text))).comment("source scope");
    auto& warnings = user_context.make<Vector<ds::DynamicSearchWarning>>();
    warnings.emplace_back("School", "FILTER", text);
    auto& result = user_context.make<ds::DynamicSearchResult>(query, warnings);
    Json::Object output(owner);
    output.emplace("default", Json(owner, debug_string(owner, default_model)));
    output.emplace("source_before", snapshot(owner, model, normalized, result));
    auto& model_copy = global.make<ds::SearchModel>(model);
    auto& normalized_copy = global.make<ds::NormalizedDynamicSearch>(normalized);
    auto& result_copy = global.make<ds::DynamicSearchResult>(result);
    output.emplace("clone_before", snapshot(owner, model_copy, normalized_copy, result_copy));
    model_copy.fields.insert_or_assign(String("mutated", owner), String("number", owner));
    normalized_copy.filters.emplace_back("later", "$ne", Json(owner, "changed"));
    output.emplace("source_after", snapshot(owner, model, normalized, result));
    user_context_owner.reset();
    check(global.live_user_bytes() == 0, "source UserContext was actually reclaimed");
    check(model_copy.fields.get_allocator().resource() == owner &&
              model_copy.relations.get_allocator().resource() == owner &&
              normalized_copy.memory_resource() == owner && result_copy.memory_resource() == owner,
          "explicit retained models use Global ownership");
    for (const auto* map : {&model_copy.fields, &model_copy.relations}) {
        for (const auto& [key, element] : *map) {
            check(key.get_allocator().resource() == owner &&
                      element.get_allocator().resource() == owner,
                  "retained model map strings use Global ownership");
        }
    }
    for (const auto& filter : normalized_copy.filters) {
        check(filter.field_path.get_allocator().resource() == owner &&
                  filter.operator_name.get_allocator().resource() == owner &&
                  filter.value.memory_resource() == owner,
              "retained normalized filters use Global ownership");
    }
    for (const auto& order : normalized_copy.orders) {
        check(order.field_path.get_allocator().resource() == owner,
              "retained normalized orders use Global ownership");
    }
    for (const auto* list : {&normalized_copy.warnings, &result_copy.warnings}) {
        check(list->get_allocator().resource() == owner,
              "retained warning collection uses Global ownership");
        for (const auto& warning : *list) {
            for (const auto* item :
                 {&warning.code, &warning.entity, &warning.clause, &warning.field_path}) {
                check(item->get_allocator().resource() == owner,
                      "retained warning fields use Global ownership");
            }
        }
    }
    normalized_copy.orders.emplace_back("later", false);
    normalized_copy.warnings.emplace_back("School", "ORDER_BY", "later");
    result_copy.query.comment("retained changed");
    result_copy.warnings.emplace_back("School", "ORDER_BY", "later");
    output.emplace("clone_after_source_drop",
                   snapshot(owner, model_copy, normalized_copy, result_copy));
    return Json(owner, std::move(output));
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            check(bool(input), "valid observation input");
            std::cout << observe(global, *input).dump(global.memory_resource()) << '\n';
        }
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
