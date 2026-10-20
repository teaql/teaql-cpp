#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/core/request_debug.hpp>
#include <iostream>

using namespace cpp_dynamic_json;

static bool flag(const Json& input, std::string_view key) {
    return std::get<bool>(field(input, key).data);
}

template <class Child>
static Child configure_child(Child child, const Json& input) {
    child.filter(Expr::gt(child.memory_resource(), "id", std::int64_t(0)));
    auto mode = text(field(input, "child_mode"));
    if (mode == "deleted") {
        child.withDeletedRows();
    } else if (mode == "slice") {
        child.orderByIdDesc().offset(1, 2);
    } else if (mode == "group") {
        child.groupBy("version").countAs("_childCount");
    } else if (mode == "options") {
        child.comment("child analysis intent")
            .createPropertyAs("_childComputed", "id + 7")
            .rawSqlFilter("1=1");
    } else if (mode == "relations") {
        child.selectAll();
    }
    return child;
}

template <Entity T>
static Json snapshot(Memory r, const T& entity) {
    Json::Object output(r), loaded(r);
    auto descriptor = T::entity_descriptor(r);
    for (const auto& property : descriptor.properties) {
        loaded.emplace(String(property.name, r), Json(r, entity.is_field_loaded(property.name)));
    }
    for (const auto& relation : descriptor.relations) {
        loaded.emplace(String(relation.name, r), Json(r, entity.is_field_loaded(relation.name)));
    }
    T copy(std::allocator_arg, Allocator(r), entity);
    output.emplace("loaded", Json(r, loaded));
    output.emplace("json", std::move(copy).into_json(r));
    return Json(r, output);
}

static Json row_snapshot(Memory r, const CompactRow& row) {
    Json::Object output(r);
    for (const auto& [key, value] : row.iter()) {
        output.emplace(String(key, r), Json(r, debug_string(r, value)));
    }
    return Json(r, output);
}

template <class Query, class Child, class Apply>
static Json
finish(UserContext& user_context, Query query, const Child& child, Apply apply, const Json& input) {
    auto r = user_context.memory_resource();
    auto before = debug_string(r, child.query());
    auto conversion = text(field(input, "conversion"));
    if (conversion == "selection") {
        auto selection = child.selection();
        apply(query, selection);
    } else if (conversion == "query") {
        auto core = std::move(child.selection()).into_query();
        apply(query, core);
    } else {
        apply(query, child.runtimeQuery());
    }
    query.orderAsc("id");
    Json::Object output(r);
    output.emplace("query", Json(r, debug_string(r, query.query())));
    output.emplace("options", Json(r, debug_string(r, query.queryOptions())));
    String filters(r);
    DebugWriter writer(filters, {});
    writer.sequence(query.relationFilters(), DefaultDebugElement{});
    output.emplace("filters", Json(r, filters));
    output.emplace("child_before", Json(r, before));
    output.emplace("child_after", Json(r, debug_string(r, child.query())));
    auto executable = std::move(query)
                          .comment("inspect analysis input conversion")
                          .purpose("qualify relation analysis conversions");
    Json::Array rows(r);
    Json::Object facets(r);
    Json error(r), total(r);
    auto consume = [&](const auto& result) {
        if (!result) {
            error = Json(r, result.error().to_string(r));
            return;
        }
        if (result.value().total_count) {
            total = Json(r, *result.value().total_count);
        }
        for (const auto& entity : result.value().data) {
            rows.emplace_back(snapshot(r, entity));
        }
        for (const auto& [name, list] : result.value().facets) {
            Json::Array values(r);
            for (const auto& row : list.data) {
                values.emplace_back(row_snapshot(r, row));
            }
            facets.emplace(String(name, r), Json(r, values));
        }
    };
    auto mode = text(field(input, "mode"));
    if (mode == "rows") {
        const auto& result = executable.executeForRows(user_context).wait();
        if (!result) {
            error = Json(r, result.error().to_string(r));
        } else {
            for (const auto& row : result.value().data) {
                rows.emplace_back(row_snapshot(r, row));
            }
        }
    } else if (mode == "page") {
        consume(executable.executeForPage(user_context, 0, 2).wait());
    } else {
        consume(executable.executeForList(user_context).wait());
    }
    output.emplace("rows", Json(r, rows));
    output.emplace("facets", Json(r, facets));
    output.emplace("error", std::move(error));
    output.emplace("total", std::move(total));
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        Json::Array params(r), trace(r);
        for (const auto& value : entry.params) {
            params.emplace_back(debug_string(r, value));
        }
        for (const auto& node : entry.trace_path) {
            trace.emplace_back(debug_string(r, node));
        }
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("params", Json(r, params));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        log.emplace("trace", Json(r, trace));
        logs.emplace_back(log);
    }
    output.emplace("logs", Json(r, logs));
    return Json(r, output);
}

static Json observe(UserContext& user_context, const Json& input) {
    teaql::test::initialize_dynamic_json_database(user_context);
    auto Q = user_context.queries<query_factory>();
    auto kind = text(field(input, "kind"));
    bool minimal = flag(input, "minimal"), child_minimal = flag(input, "child_minimal");
    if (kind == "match_customer" || kind == "without_customer" || kind == "rollup_customer") {
        auto parent = (minimal ? Q.ordersMinimal() : Q.orders()).runtimeQuery();
        auto child = configure_child(child_minimal ? Q.peopleMinimal() : Q.people(), input);
        return finish(
            user_context,
            std::move(parent),
            child,
            [&](auto& query, const auto& selected) {
                auto expected = Person::entity_descriptor(query.memory_resource());
                query.filterRelation("customer_id",
                                     "id",
                                     expected,
                                     selected,
                                     kind == "without_customer",
                                     "customer");
                if (kind == "rollup_customer") {
                    query.groupByObject("customer", "customer_id", selected);
                }
            },
            input);
    }
    auto parent = (minimal ? Q.peopleMinimal() : Q.people()).runtimeQuery();
    auto child = configure_child(child_minimal ? Q.ordersMinimal() : Q.orders(), input);
    return finish(
        user_context,
        std::move(parent),
        child,
        [&](auto& query, const auto& selected) {
            if (kind == "match_orders" || kind == "without_orders") {
                auto expected = Order::entity_descriptor(query.memory_resource());
                query.filterRelation(
                    "id", "customer_id", expected, selected, kind == "without_orders", "orders");
                return;
            }
            AggregateFunction function = AggregateFunction::Sum;
            std::string_view alias = "sum_amount";
            if (kind == "avg") {
                function = AggregateFunction::Avg;
                alias = "avg_amount";
            } else if (kind == "min") {
                function = AggregateFunction::Min;
                alias = "min_amount";
            } else if (kind == "max") {
                function = AggregateFunction::Max;
                alias = "max_amount";
            } else if (kind == "stddev") {
                function = AggregateFunction::Stddev;
                alias = "stdDev_amount";
            } else if (kind == "stddev_pop") {
                function = AggregateFunction::StddevPop;
                alias = "stdDevPop_amount";
            } else if (kind == "var_samp") {
                function = AggregateFunction::VarSamp;
                alias = "varSamp_amount";
            } else if (kind == "var_pop") {
                function = AggregateFunction::VarPop;
                alias = "varPop_amount";
            }
            query.aggregateRelationField("orders", "_metric", selected, "amount", alias, function);
        },
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
                    throw std::runtime_error("Invalid relation analysis input");
                }
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Relation analysis retained User bytes");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
