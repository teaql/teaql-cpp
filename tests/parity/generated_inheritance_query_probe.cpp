#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>

using namespace cpp_inheritance;

static Json describe(Memory r, const SelectQuery& query) {
    Json::Object out(r);
    Json::Array projection(r), children(r);
    for (const auto& name : query.projection) {
        projection.emplace_back(name);
    }
    for (const auto& child : query.child_enhancements) {
        children.emplace_back(describe(r, child));
    }
    out.emplace(String("entity", r), Json(r, query.entity));
    out.emplace(String("projection", r), Json(r, projection));
    out.emplace(String("filter", r),
                query.predicate ? Json(r, debug_string(r, *query.predicate)) : Json(r));
    out.emplace(String("slice", r), query.slice ? Json(r, debug_string(r, *query.slice)) : Json(r));
    out.emplace(String("hard_limit", r), Json(r, query.hard_limit_value));
    out.emplace(String("children", r), Json(r, children));
    return Json(r, out);
}

template <class Request>
static Json relation_tree(Memory r, const Request& query) {
    Json::Object out(r);
    Json::Array relations(r);
    for (const auto& relation : query.relations) {
        Json::Object value(r);
        value.emplace(String("name", r), Json(r, relation.name));
        value.emplace(String("query", r),
                      relation.query ? relation_tree(r, *relation.query) : Json(r));
        relations.emplace_back(value);
    }
    out.emplace(String("query", r), describe(r, query));
    out.emplace(String("relations", r), Json(r, relations));
    return Json(r, out);
}

template <class Request>
static Json observe(UserContext& user_context, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    if (const auto* items = std::get_if<Json::Array>(&field(input, "types").data)) {
        Vector<Value> types(r);
        for (const auto& item : *items) {
            types.emplace_back(value(r, item));
        }
        request.matchTypes(types);
    }
    if (std::get<bool>(field(input, "group").data)) {
        request.withTypeGroup();
    }
    if (std::get<bool>(field(input, "limit").data)) {
        request.offset(3).limit(5);
    }
    if (std::get<bool>(field(input, "unlimited").data)) {
        request.unlimited();
    }
    if (text(field(input, "enhance")) == "automatic") {
        request.enhanceChildrenIfNeeded();
    } else if (text(field(input, "enhance")) == "manual") {
        auto Q = user_context.queries<query_factory>();
        request.enhanceChild(Q.researchAcademiesMinimal().selectBudget().withRankIs(7));
    }
    if (const auto* shortcut = optional(input, "shortcut")) {
        auto name = text(*shortcut);
        auto repeat = std::get<bool>(field(input, "repeat").data);
        for (int index = 0; index < (repeat ? 2 : 1); ++index) {
            if (name == "self_without_parent") {
                request.selectSelfWithoutParent();
            } else if (name == "all") {
                request.selectAll();
            } else if (name == "children") {
                request.selectChildren();
            } else {
                request.selectAny();
            }
        }
        return relation_tree(r, request.query());
    }
    return describe(r, request.query());
}

static Json dispatch(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto entry = text(field(input, "entry"));
    auto subtype = std::get<bool>(field(input, "subtype").data);
    if (text(field(input, "entity")) == "school") {
        auto request = entry == "minimal"    ? Q.schoolsMinimal()
                       : entry == "children" ? Q.schoolsWithChildren()
                                             : Q.schools();
        if (subtype) {
            request.isAcademy();
        }
        return observe(user_context, std::move(request), input);
    }
    if (text(field(input, "entity")) == "academy") {
        auto request = entry == "minimal"    ? Q.academiesMinimal()
                       : entry == "children" ? Q.academiesWithChildren()
                                             : Q.academies();
        if (subtype) {
            request.isResearchAcademy();
        }
        return observe(user_context, std::move(request), input);
    }
    auto request = entry == "minimal"    ? Q.researchAcademiesMinimal()
                   : entry == "children" ? Q.researchAcademiesWithChildren()
                                         : Q.researchAcademies();
    return observe(user_context, std::move(request), input);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid inherited query fixture");
            }
            std::cout << dispatch(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
