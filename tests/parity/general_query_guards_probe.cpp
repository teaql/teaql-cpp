#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"
#include "fixture_value.hpp"
#include <teaql/core/request_debug.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

struct NotAnEntity {};

template <class Range>
static String sequence_debug(Memory r, const Range& values) {
    String text(r);
    DebugWriter writer(text, {});
    writer.sequence(values, DefaultDebugElement{});
    return text;
}

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

template <class Marker, class Request>
static Json inspect(UserContext& user_context, Request query, const Json& input) {
    auto r = user_context.memory_resource();
    auto selection = text(field(input, "selection"));
    if (selection == "self") {
        query.selectSelf();
    } else if (selection == "self_fields") {
        query.selectSelfFields();
    } else if (selection == "without_parent") {
        query.selectSelfWithoutParent();
    } else if (selection == "all") {
        query.selectAll();
    } else if (selection == "children") {
        query.selectChildren();
    } else if (selection == "any") {
        query.selectAny();
    }
    Json::Object output(r);
    auto bare = query.coreQuery();
    auto options = query.queryOptions();
    auto consumed = query;
    auto raw = std::move(consumed).intoQuery();
    auto selected = query.selection();
    auto complete = std::move(selected).into_query();
    output.emplace("bare", Json(r, debug_string(r, bare)));
    output.emplace("options", Json(r, debug_string(r, options)));
    output.emplace("relations", Json(r, sequence_debug(r, query.relationSelections())));
    output.emplace("filters", Json(r, sequence_debug(r, query.relationFilters())));
    output.emplace("children", Json(r, sequence_debug(r, query.childEnhancements())));
    output.emplace("raw", Json(r, debug_string(r, raw)));
    output.emplace("complete", Json(r, debug_string(r, complete)));
    output.emplace("selection", Json(r, debug_string(r, query.selection())));
    auto mode = text(field(input, "mode"));
    if (mode == "construct") {
        return Json(r, output);
    }
    if constexpr (Entity<Marker>) {
        auto request = std::move(query)
                           .comment("Inspect general request guards")
                           .purpose("Qualify constructor and selection responsibilities");
        Json::Array rows(r);
        Json error(r);
        if (mode == "list") {
            const auto& result = request.executeForList(user_context).wait();
            if (result) {
                for (const auto& entity : result.value().data) {
                    rows.emplace_back(snapshot(r, entity));
                }
            } else {
                error = Json(r, result.error().to_string(r));
            }
        } else {
            const auto& result = request.executeForRows(user_context).wait();
            if (result) {
                for (const auto& row : result.value().data) {
                    Json::Object values(r);
                    for (const auto& [key, value] : row.iter()) {
                        values.emplace(String(key, r), Json(r, debug_string(r, value)));
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
            log.emplace("debug_sql", Json(r, entry.debug_sql));
            log.emplace("pretty_sql", Json(r, entry.pretty_sql));
            log.emplace("context", Json(r, debug_string(r, entry.log_context)));
            log.emplace("trace", Json(r, trace));
            log.emplace("result_count",
                        entry.result_count ? Json(r, *entry.result_count) : Json(r));
            log.emplace("result_type", entry.result_type ? Json(r, *entry.result_type) : Json(r));
            log.emplace("affected_rows",
                        entry.affected_rows ? Json(r, *entry.affected_rows) : Json(r));
            log.emplace("summary", Json(r, entry.result_summary));
            logs.emplace_back(log);
        }
        output.emplace("logs", Json(r, logs));
        return Json(r, output);
    } else {
        throw std::logic_error("Non-Entity marker cannot execute");
    }
}

template <class Model, template <bool, class> class Request, class Factory>
static Json observe_request(UserContext& user_context, Factory factory, const Json& input) {
    auto observe = [&]<class Marker>() {
        if (text(field(input, "entry")) == "bare") {
            return inspect<Marker>(
                user_context, Request<false, Marker>(user_context, true, false), input);
        }
        return inspect<Marker>(user_context, factory().template returnType<Marker>(), input);
    };
    auto marker = text(field(input, "marker"));
    if (marker == "integer") {
        return observe.template operator()<std::uint64_t>();
    }
    if (marker == "custom") {
        return observe.template operator()<NotAnEntity>();
    }
    return observe.template operator()<Model>();
}

static Json observe(UserContext& user_context, const Json& input) {
    bool inherited = text(field(input, "domain")) == "inherited";
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    for (auto fixture : inherited ? std::vector<std::string_view>{"dynamic_json_inherited_rows.sql"}
                                  : std::vector<std::string_view>{"dynamic_json_schema.sql",
                                                                  "dynamic_json_dataset.sql"}) {
        std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/" +
                           std::string(fixture));
        std::ostringstream sql;
        sql << file.rdbuf();
        if (!file || sqlite3_exec(user_context.get(connection).native_handle(),
                                  sql.str().c_str(),
                                  nullptr,
                                  nullptr,
                                  nullptr) != SQLITE_OK) {
            throw std::runtime_error("Load general query guard fixture");
        }
    }
    auto entry = text(field(input, "entry"));
    auto entity = text(field(input, "entity"));
#define OBSERVE(DOMAIN, MODEL, REQUEST, ORDINARY, MINIMAL, CHILDREN)                               \
    observe_request<DOMAIN::MODEL, DOMAIN::REQUEST>(                                               \
        user_context,                                                                              \
        [&] {                                                                                      \
            auto Q = user_context.queries<DOMAIN::query_factory>();                                \
            return entry == "minimal"         ? Q.MINIMAL()                                        \
                   : entry == "with_children" ? Q.CHILDREN()                                       \
                                              : Q.ORDINARY();                                      \
        },                                                                                         \
        input)
    if (inherited) {
        cpp_inheritance_graph::require_value(
            cpp_inheritance_graph::serviceRuntimeFromConnection(user_context, connection));
        user_context.with_active_root("Platform", 1);
        if (entity == "platform") {
            return OBSERVE(cpp_inheritance_graph,
                           Platform,
                           PlatformRequest,
                           platforms,
                           platformsMinimal,
                           platformsWithChildren);
        }
        if (entity == "specialist") {
            return OBSERVE(cpp_inheritance_graph,
                           Specialist,
                           SpecialistRequest,
                           specialists,
                           specialistsMinimal,
                           specialistsWithChildren);
        }
        if (entity == "school") {
            return OBSERVE(cpp_inheritance_graph,
                           School,
                           SchoolRequest,
                           schools,
                           schoolsMinimal,
                           schoolsWithChildren);
        }
        if (entity == "academy") {
            return OBSERVE(cpp_inheritance_graph,
                           Academy,
                           AcademyRequest,
                           academies,
                           academiesMinimal,
                           academiesWithChildren);
        }
        if (entity == "research_academy") {
            return OBSERVE(cpp_inheritance_graph,
                           ResearchAcademy,
                           ResearchAcademyRequest,
                           researchAcademies,
                           researchAcademiesMinimal,
                           researchAcademiesWithChildren);
        }
        if (entity == "enrollment") {
            return OBSERVE(cpp_inheritance_graph,
                           Enrollment,
                           EnrollmentRequest,
                           enrollments,
                           enrollmentsMinimal,
                           enrollmentsWithChildren);
        }
    } else {
        cpp_dynamic_json::require_value(
            cpp_dynamic_json::serviceRuntimeFromConnection(user_context, connection));
        user_context.with_active_root("Platform", 1);
        if (entity == "platform") {
            return OBSERVE(cpp_dynamic_json,
                           Platform,
                           PlatformRequest,
                           platforms,
                           platformsMinimal,
                           platformsWithChildren);
        }
        if (entity == "person") {
            return OBSERVE(
                cpp_dynamic_json, Person, PersonRequest, people, peopleMinimal, peopleWithChildren);
        }
        if (entity == "order") {
            return OBSERVE(
                cpp_dynamic_json, Order, OrderRequest, orders, ordersMinimal, ordersWithChildren);
        }
        if (entity == "product") {
            return OBSERVE(cpp_dynamic_json,
                           Product,
                           ProductRequest,
                           products,
                           productsMinimal,
                           productsWithChildren);
        }
    }
#undef OBSERVE
    throw std::logic_error("Unknown guard fixture entity");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::logic_error("Guard fixture retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
