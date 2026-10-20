#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include "../generated_relations/include/cpp_relations.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/core/request_debug.hpp>
#include <iostream>

using namespace cpp_dynamic_json;

static bool flag(const Json& input, std::string_view key) {
    auto* value = optional(input, key);
    return value && std::get<bool>(value->data);
}

template <class Request>
static Request child_query(Request query, const Json& input) {
    auto r = query.memory_resource();
    auto cutoff = static_cast<std::int64_t>(std::get<std::uint64_t>(field(input, "cutoff").data));
    query.filter(Expr::gt(r, "id", cutoff));
    if (flag(input, "child_deleted")) {
        query.withDeletedRows();
    }
    if (flag(input, "child_slice")) {
        query.orderByIdDesc().offset(1, 2);
    }
    if (flag(input, "child_group")) {
        query.groupBy("version").countAs("_count");
    }
    if (flag(input, "child_relations")) {
        query.selectAll();
    }
    if (flag(input, "child_options")) {
        query.comment("child comment")
            .createPropertyAs("_ignored", "77")
            .rawSqlFilter("1=0")
            .rawSql("SELECT 999 AS id");
    }
    return query;
}

template <class Parent, class Child>
static Json
finish(UserContext& user_context, Parent parent, const Child& child, const Json& input) {
    auto r = user_context.memory_resource();
    auto before = debug_string(r, child.query());
    parent.filter(Expr::lt(r, "id", std::int64_t(100)));
    if (flag(input, "outer_deleted")) {
        parent.withDeletedRows();
    }
    auto combine = [&](bool alias) {
        auto apply = [&](const auto& selected) {
            if (alias) {
                parent.enhanceChild(selected);
            } else {
                parent.enhanceChild(selected);
            }
        };
        auto* conversion = optional(input, "conversion");
        if (conversion && text(*conversion) == "selection") {
            auto selection = child.selection();
            apply(selection);
        } else if (conversion && text(*conversion) == "query") {
            auto query = std::move(child.selection()).into_query();
            apply(query);
        } else {
            apply(child);
        }
    };
    combine(flag(input, "alias"));
    if (flag(input, "repeat")) {
        combine(false);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "query", debug_string(r, parent.query()));
    String enhancements(r);
    DebugWriter writer(enhancements, {});
    writer.sequence(parent.childEnhancements(), DefaultDebugElement{});
    detail::model_put(out, "enhancements", enhancements);

    detail::model_put(out, "child_before", before);
    detail::model_put(out, "child_after", debug_string(r, child.query()));
    if (flag(input, "execute")) {
        teaql::test::initialize_dynamic_json_database(user_context);
        const auto& result = std::move(parent)
                                 .comment("inspect child enhancement input")
                                 .purpose("qualify child enhancement conversion")
                                 .executeForRows(user_context)
                                 .wait();
        Json::Array rows(r);
        Json error(r);
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array values(r);
                for (auto key : {"id", "version"}) {
                    auto* value = row.get(key);
                    values.emplace_back(value ? Json(r, debug_string(r, *value)) : Json(r));
                }
                rows.emplace_back(values);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
        detail::model_put(out, "rows", rows);
        detail::model_put(out, "error", error);
    }
    return out;
}

template <class Parent>
static Json choose_child(UserContext& user_context, Parent parent, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto child = text(field(input, "child"));
    if (child == "school") {
        auto other_Q = user_context.queries<cpp_relations::query_factory>();
        return finish(
            user_context,
            std::move(parent),
            child_query(flag(input, "child_minimal") ? other_Q.schoolsMinimal() : other_Q.schools(),
                        input),
            input);
    }
    if (child == "person") {
        return finish(
            user_context,
            std::move(parent),
            child_query(flag(input, "child_minimal") ? Q.peopleMinimal() : Q.people(), input),
            input);
    }
    if (child == "order") {
        return finish(
            user_context,
            std::move(parent),
            child_query(flag(input, "child_minimal") ? Q.ordersMinimal() : Q.orders(), input),
            input);
    }
    return finish(
        user_context,
        std::move(parent),
        child_query(flag(input, "child_minimal") ? Q.productsMinimal() : Q.products(), input),
        input);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("invalid enhancement fixture");
                }
                auto Q = user_context->queries<query_factory>();
                auto parent = text(field(*input, "outer"));
                auto result = parent == "person"
                                  ? choose_child(*user_context,
                                                 flag(*input, "outer_minimal") ? Q.peopleMinimal()
                                                                               : Q.people(),
                                                 *input)
                                  : choose_child(*user_context,
                                                 flag(*input, "outer_minimal") ? Q.ordersMinimal()
                                                                               : Q.orders(),
                                                 *input);
                std::cout << result.dump(user_context->memory_resource()) << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("enhancement retained User bytes");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
