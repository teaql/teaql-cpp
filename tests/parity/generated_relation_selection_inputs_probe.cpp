#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/core/request_debug.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

template <Entity T>
static Json snapshot(Memory r, const T& entity) {
    Json::Object output(r), loaded(r);
    auto descriptor = T::entity_descriptor(r);
    for (const auto& field : descriptor.properties) {
        loaded.emplace(String(field.name, r), Json(r, entity.is_field_loaded(field.name)));
    }
    for (const auto& relation : descriptor.relations) {
        loaded.emplace(String(relation.name, r), Json(r, entity.is_field_loaded(relation.name)));
    }
    T copy(std::allocator_arg, Allocator(r), entity);
    output.emplace("loaded", Json(r, loaded));
    output.emplace("json", std::move(copy).into_json(r));
    return Json(r, output);
}

template <class Request, class Apply>
static Json execute(UserContext& user_context, Request request, Apply apply, const Json& input) {
    auto r = user_context.memory_resource();
    for (const auto& op : std::get<Json::Array>(field(input, "ops").data)) {
        apply(request, text(op));
    }
    Json::Object output(r);
    output.emplace("query", Json(r, debug_string(r, request.query())));
    output.emplace("options", Json(r, debug_string(r, request.queryOptions())));
    auto selections = request.relationSelections();
    String selection_text(r);
    DebugWriter writer(selection_text, {});
    writer.sequence(std::span<const request::RelationSelection>(selections), DefaultDebugElement{});
    output.emplace("selections", Json(r, selection_text));
    auto executable = std::move(request)
                          .comment("inspect projection selection scope")
                          .purpose("qualify complete selection composition");
    Json::Array rows(r);
    Json error(r);
    if (text(field(input, "mode")) == "list") {
        const auto& result = executable.executeForList(user_context).wait();
        if (result) {
            for (const auto& entity : result.value().data) {
                rows.emplace_back(snapshot(r, entity));
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } else {
        const auto& result = executable.executeForRows(user_context).wait();
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Object values(r);
                for (const auto& [key, value] : row.iter()) {
                    values.insert_or_assign(String(key, r), Json(r, debug_string(r, value)));
                }
                rows.emplace_back(values);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    }
    output.emplace("rows", Json(r, rows));
    output.emplace("error", std::move(error));
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
    bool minimal = std::get<bool>(field(input, "minimal").data);
#define APPLY(                                                                                     \
    QUERY, SCALAR, RAW, REMOVE, GROUP, RELATION, WITH, REMOVE_RELATION, CHILD, RELATION_NAME)      \
    execute(                                                                                       \
        user_context,                                                                              \
        QUERY,                                                                                     \
        [&](auto& query, std::string_view op) {                                                    \
            if (op == "self") {                                                                    \
                query.selectSelf();                                                                \
            } else if (op == "self_fields") {                                                      \
                query.selectSelfFields();                                                          \
            } else if (op == "without_parent") {                                                   \
                query.selectSelfWithoutParent();                                                   \
            } else if (op == "all") {                                                              \
                query.selectAll();                                                                 \
            } else if (op == "children") {                                                         \
                query.selectChildren();                                                            \
            } else if (op == "any") {                                                              \
                query.selectAny();                                                                 \
            } else if (op == "scalar") {                                                           \
                query.SCALAR();                                                                    \
            } else if (op == "raw") {                                                              \
                query.RAW("'fixed'");                                                              \
            } else if (op == "remove") {                                                           \
                query.REMOVE();                                                                    \
            } else if (op == "group_alias") {                                                      \
                query.GROUP("_scalarAlias");                                                       \
            } else if (op == "relation") {                                                         \
                query.RELATION();                                                                  \
            } else if (op == "relation_with") {                                                    \
                auto child =                                                                       \
                    CHILD.selectName().comment("child projection intent").rawSqlFilter("1=1");     \
                auto child_mode = text(field(input, "child_mode"));                                \
                if (child_mode == "options") {                                                     \
                    child.createPropertyAs("_childComputed", "id + 7").rawSqlFilter("1=1");        \
                } else if (child_mode == "deleted") {                                              \
                    child.withDeletedRows();                                                       \
                } else if (child_mode == "slice") {                                                \
                    child.orderByIdDesc().offset(1, 2);                                            \
                }                                                                                  \
                auto apply = [&](const auto& selected) {                                           \
                    query.WITH(selected);                                                          \
                };                                                                                 \
                auto conversion = text(field(input, "conversion"));                                \
                if (conversion == "selection") {                                                   \
                    auto selection = child.selection();                                            \
                    apply(selection);                                                              \
                } else if (conversion == "query") {                                                \
                    auto core = std::move(child.selection()).into_query();                         \
                    apply(core);                                                                   \
                } else {                                                                           \
                    apply(child);                                                                  \
                }                                                                                  \
            } else if (op == "remove_relation") {                                                  \
                query.REMOVE_RELATION();                                                           \
            } else if (op == "computed") {                                                         \
                query.createPropertyAs("_computed", "id + 7");                                     \
            } else if (op == "raw_filter") {                                                       \
                query.rawSqlFilter("1=1");                                                         \
            } else if (op == "search") {                                                           \
                query.searchWithText("Alpha");                                                     \
            } else if (op == "deleted") {                                                          \
                query.withDeletedRows();                                                           \
            } else {                                                                               \
                throw std::logic_error("unknown fixture operation");                               \
            }                                                                                      \
        },                                                                                         \
        input)
    if (text(field(input, "entity")) == "person") {
        return APPLY(minimal ? Q.peopleMinimal() : Q.people(),
                     selectName,
                     selectNameRaw,
                     unselectName,
                     groupByNameAs,
                     selectPlatform,
                     selectPlatformWith,
                     unselectPlatform,
                     Q.platforms(),
                     "platform");
    }
    if (text(field(input, "entity")) == "product") {
        return APPLY(minimal ? Q.productsMinimal() : Q.products(),
                     selectSku,
                     selectSkuRaw,
                     unselectSku,
                     groupBySkuAs,
                     selectPlatform,
                     selectPlatformWith,
                     unselectPlatform,
                     Q.platforms(),
                     "platform");
    }
    return APPLY(minimal ? Q.ordersMinimal() : Q.orders(),
                 selectCode,
                 selectCodeRaw,
                 unselectCode,
                 groupByCodeAs,
                 selectCustomer,
                 selectCustomerWith,
                 unselectCustomer,
                 Q.people(),
                 "customer");
#undef APPLY
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
                    throw std::runtime_error("invalid selection fixture");
                }
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("selection retained User bytes");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
