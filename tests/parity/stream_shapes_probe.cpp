#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>
using namespace cpp_inheritance_graph;

struct Observation {
    using allocator_type = Allocator;
    Json::Array calls;
    std::uint64_t selects = 0;

    Observation(std::allocator_arg_t, Allocator a) : calls(a.resource()) {}
};

struct Policy final : RequestPolicy {
    using allocator_type = Allocator;
    String mode;
    Observation& observation;

    Policy(std::allocator_arg_t, Allocator a, std::string_view mode, Observation& observation)
        : mode(mode, a.resource()), observation(observation) {}

    RuntimeHookResult
    enforce_select(Memory r, const UserContext&, SelectQuery& query) const override {
        auto before = debug_string(r, query);
        if (mode == "restrict") {
            std::uint64_t id = query.entity == "Enrollment"   ? 21
                               : query.entity == "Specialist" ? 2
                               : query.entity == "Platform"   ? 1
                                                              : 7;
            auto predicate = Expr::eq(r, "id", id);
            query.filter(query.predicate ? query.predicate->and_expr(predicate)
                                         : std::move(predicate));
        }
        bool raw = query.raw_sql_text || !query.raw_sql_search_criteria.empty() ||
                   !query.raw_projections.empty() || !query.dynamic_properties.empty();
        Json::Object call(r);
        call.emplace("before", Json(r, before));
        call.emplace("after", Json(r, debug_string(r, query)));
        call.emplace("raw", Json(r, raw));
        observation.calls.emplace_back(call);
        if (mode == "deny" || (mode == "reject_raw" && raw)) {
            return RuntimeHookResult::err(r, RuntimeError::policy(r, "COMPOSED_QUERY_DENIED"));
        }
        return RuntimeHookResult::ok(r, {});
    }
};

static int database(void* data, int action, const char*, const char*, const char*, const char*) {
    if (action == SQLITE_SELECT) {
        ++static_cast<Observation*>(data)->selects;
    }
    return SQLITE_OK;
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
    output.emplace("loaded", Json(r, loaded));
    const auto* computed = entity.dynamic_property("_computed");
    output.emplace("computed", computed ? Json(r, debug_string(r, *computed)) : Json(r));
    T copy(std::allocator_arg, Allocator(r), entity);
    output.emplace("json", std::move(copy).into_json(r));
    return Json(r, output);
}

template <class Request, class Facet, class Raw>
static Json apply(UserContext& user_context,
                  SqliteConnection& connection,
                  Observation& observation,
                  Request query,
                  const Json& input,
                  Facet facet,
                  Raw raw_projection,
                  auto object_group,
                  std::string_view table) {
    auto r = user_context.memory_resource();
    query.filterWithJson(text(field(input, "entity")) == "enrollment"
                             ? R"({"school.name":"PRIVATE-SCHOOL-CANARY"})"
                             : R"({"name":"PRIVATE-SCHOOL-CANARY"})");
    auto shape = text(field(input, "shape"));
    if (shape == "relation") {
        query.selectAll();
    } else if (shape == "child") {
        auto Q = user_context.queries<query_factory>();
        query.enhanceChild(Q.specialistsMinimal());
    } else if (shape == "object_group") {
        object_group(query);
    } else if (shape == "count_metric") {
        query.aggregateCount("_metric");
    } else if (shape == "sum_metric") {
        query.aggregateSum("id", "_metric");
    } else if (shape == "statistic") {
        query.aggregateStddev("id", "_metric");
    } else if (shape == "readonly") {
        query.createPropertyAs("_computed", "id + 7");
    } else if (shape == "raw_projection") {
        raw_projection(query);
    } else if (shape == "raw_filter") {
        query.rawSqlFilter("id >= 7");
    } else if (shape == "raw_outer") {
        String sql("SELECT id,version FROM ", r);
        sql += table;
        sql += " WHERE id=8";
        query.rawSql(sql);
    } else if (shape == "facet") {
        facet(query);
    } else if (shape == "grouped") {
        query.groupBy("id").aggregateCount("_metric");
    }
    auto optimization = text(field(input, "optimization"));
    if (optimization == "id_set") {
        query.optimizePaginationWithIdSetConfig("composed_query", 60, 100);
    } else if (optimization == "continuous") {
        query.optimizeForContinuousPageFetchWith("composed_query", 60);
    }
    auto slice = text(field(input, "slice"));
    if (slice == "zero") {
        query.limit(0);
    } else if (slice == "offset") {
        query.offsetOnly(1).limit(1);
    } else if (slice == "beyond") {
        query.offsetOnly(99).limit(1);
    }
    Json::Object output(r);

    if (shape == "missing_table") {
        String sql("ALTER TABLE ", r);
        sql += table;
        sql += " RENAME TO hidden_";
        sql += table;
        if (sqlite3_exec(connection.native_handle(), sql.c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
        }
    }
    observation.selects = 0;
    auto intent = text(field(input, "intent"));
    Json::Array rows(r);
    Json error(r);
    Json panic(r);
    Json opened(r);
    try {
        auto chunk = text(field(input, "chunk"));
        if (chunk == "default") {
            query.streamDefault();
        } else {
            query.stream(number<std::size_t>(field(input, "chunk")));
        }
        output.emplace("query", Json(r, debug_string(r, query.query())));
        auto request =
            std::move(query)
                .comment(intent == "blank_comment" ? "" : "Inspect composed PRIVATE-SCHOOL-CANARY")
                .purpose(intent == "blank_purpose" ? "" : "Compare scope PRIVATE-SCHOOL-CANARY");
        const auto& result = request.executeForStream(user_context).wait();
        opened = Json(r, static_cast<bool>(result));
        if (result) {
            auto& stream = result.value().get();
            auto stop = number<std::size_t>(field(input, "stop"));
            for (std::size_t index = 0; index < stop; ++index) {
                auto* item = stream.wait_next();
                if (!item) {
                    break;
                }
                if (!*item) {
                    error = Json(r, item->error().to_string(r));
                    break;
                }
                rows.emplace_back(snapshot(r, item->value()));
            }
            stream.cancel();
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } catch (const std::exception& failure) {
        panic = Json(r, failure.what());
    }
    output.emplace("opened", std::move(opened));
    output.emplace("panic", std::move(panic));
    output.emplace("rows", Json(r, rows));
    output.emplace("error", std::move(error));
    output.emplace("policy", Json(r, observation.calls));
    output.emplace("selects", Json(r, observation.selects));
    bool released = true;
    for (auto* statement = sqlite3_next_stmt(connection.native_handle(), nullptr); statement;
         statement = sqlite3_next_stmt(connection.native_handle(), statement)) {
        released = released && !sqlite3_stmt_busy(statement);
    }
    output.emplace("released", Json(r, released));
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        Json::Array params(r);
        for (const auto& p : entry.params) {
            params.emplace_back(debug_string(r, p));
        }
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("params", Json(r, params));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        log.emplace("debug_sql", Json(r, entry.debug_sql));
        log.emplace("pretty_sql", Json(r, entry.pretty_sql));
        log.emplace("context", Json(r, debug_string(r, entry.log_context)));
        Json::Array trace(r);
        for (const auto& node : entry.trace_path) {
            trace.emplace_back(debug_string(r, node));
        }
        log.emplace("trace", Json(r, trace));
        log.emplace("result_count", entry.result_count ? Json(r, *entry.result_count) : Json(r));
        log.emplace("result_type", entry.result_type ? Json(r, *entry.result_type) : Json(r));
        log.emplace("affected_rows", entry.affected_rows ? Json(r, *entry.affected_rows) : Json(r));
        log.emplace("summary", Json(r, entry.result_summary));
        logs.emplace_back(log);
    }
    output.emplace("logs", Json(r, logs));
    if (shape == "missing_table") {
        String sql("ALTER TABLE hidden_", r);
        sql += table;
        sql += " RENAME TO ";
        sql += table;
        if (sqlite3_exec(connection.native_handle(), sql.c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
        }
    }
    observation.calls.clear();
    observation.selects = 0;
    user_context.clear_sql_logs();
    auto Q = user_context.queries<query_factory>();
    const auto& reused = Q.platforms()
                             .comment("Verify stream connection reuse")
                             .purpose("Confirm cleanup permits the next request")
                             .executeForExists(user_context)
                             .wait();
    Json::Object reuse(r);
    reuse.emplace("value", reused ? Json(r, reused.value()) : Json(r));
    reuse.emplace("error", reused ? Json(r) : Json(r, reused.error().to_string(r)));
    reuse.emplace("policy", Json(r, observation.calls));
    reuse.emplace("selects", Json(r, observation.selects));
    output.emplace("reuse", Json(r, reuse));
    return Json(r, output);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto connection_ref = user_context.make_resource_ref<SqliteConnection>();
    auto& connection = user_context.get(connection_ref);
    for (auto name : {"dynamic_json_inherited_rows.sql", "dynamic_json_privacy_values.sql"}) {
        std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/" + name);
        std::ostringstream sql;
        sql << file.rdbuf();
        if (!file ||
            sqlite3_exec(
                connection.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
                SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
        }
    }
    require_value(serviceRuntimeFromConnection(user_context, connection_ref));
    user_context.with_active_root("Platform", 1);
    auto& observation = user_context.make<Observation>();
    auto& policy = user_context.make<Policy>(text(field(input, "policy")), observation);
    user_context.set_request_policy(user_context, policy);
    if (!std::get<bool>(field(input, "logging").data)) {
        user_context.disable_sql_log();
    }
    if (sqlite3_set_authorizer(connection.native_handle(), database, &observation) != SQLITE_OK) {
        throw std::runtime_error("Install query authorizer");
    }
    auto Q = user_context.queries<query_factory>();
    auto entity = text(field(input, "entity"));
    auto result = [&] {
        auto specialist = [&](auto& root) {
            root.facetByAdvisorAsWithOptions(
                "_composed",
                Q.specialistsMinimal().selectName().comment("Inspect facet PRIVATE-SCHOOL-CANARY"),
                true);
        };
        auto raw_name = [](auto& root) {
            root.selectNameRaw("substr(name,1,7)");
        };
        if (entity == "school") {
            return apply(
                user_context,
                connection,
                observation,
                Q.schools(),
                input,
                specialist,
                raw_name,
                [&](auto& root) {
                    root.groupByAdvisorWithDetailsFrom(Q.specialistsMinimal());
                },
                "school_data");
        }
        if (entity == "academy") {
            return apply(
                user_context,
                connection,
                observation,
                Q.academies(),
                input,
                specialist,
                raw_name,
                [&](auto& root) {
                    root.groupByAdvisorWithDetailsFrom(Q.specialistsMinimal());
                },
                "academy_data");
        }
        if (entity == "research_academy") {
            return apply(
                user_context,
                connection,
                observation,
                Q.researchAcademies(),
                input,
                specialist,
                raw_name,
                [&](auto& root) {
                    root.groupByAdvisorWithDetailsFrom(Q.specialistsMinimal());
                },
                "research_academy_data");
        }
        return apply(
            user_context,
            connection,
            observation,
            Q.enrollments(),
            input,
            [&](auto& root) {
                root.facetBySchoolAsWithOptions(
                    "_composed",
                    Q.schoolsMinimal().selectName().comment("Inspect facet PRIVATE-SCHOOL-CANARY"),
                    true);
            },
            [](auto& root) {
                root.selectNoteRaw("substr(note,1,7)");
            },
            [&](auto& root) {
                root.groupBySchoolWithDetailsFrom(Q.schoolsMinimal());
            },
            "enrollment_data");
    }();
    sqlite3_set_authorizer(connection.native_handle(), nullptr, nullptr);
    return result;
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
                    throw std::runtime_error("Invalid stream shape input");
                }
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Stream shape retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
