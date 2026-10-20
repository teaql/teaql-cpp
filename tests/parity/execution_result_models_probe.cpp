#define main composed_general_fixture_main
#include "composed_general_probe.cpp"
#undef main

template <class Request, class Facet, class Raw>
static Json run_target(UserContext& user_context,
                       SqliteConnection& connection,
                       Observation& observation,
                       const Json& input,
                       Request query,
                       Facet facet,
                       Raw raw,
                       std::string_view table) {
    auto run = [&](auto request) {
        auto shape = text(field(input, "shape"));
        if (shape == "readonly_null") {
            request.createPropertyAs("_computed", "NULL");
        } else if (shape == "scalar_null") {
            raw(request);
        }
        return ::apply(
            user_context, connection, observation, std::move(request), input, facet, raw, table);
    };
    auto target = text(field(input, "target"));
    if (target == "school") {
        return run(query.template returnType<School>());
    }
    if (target == "academy") {
        return run(query.template returnType<Academy>());
    }
    if (target == "research_academy") {
        return run(query.template returnType<ResearchAcademy>());
    }
    return run(query.template returnType<Enrollment>());
}

static Json observe_models(UserContext& user_context, const Json& input) {
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
    if (sqlite3_set_authorizer(connection.native_handle(), database, &observation) != SQLITE_OK) {
        throw std::runtime_error("Install adapted-result authorizer");
    }
    auto Q = user_context.queries<query_factory>();
    auto facet = [&](auto& query) {
        query.facetByAdvisorAsWithOptions(
            "_composed",
            Q.specialistsMinimal().selectName().comment("Inspect facet PRIVATE-SCHOOL-CANARY"),
            true);
    };
    auto raw = [&](auto& query) {
        query.selectNameRaw(text(field(input, "shape")) == "scalar_null" ? "NULL"
                                                                         : "substr(name,1,7)");
    };
    auto entity = text(field(input, "entity"));
    bool minimal = std::get<bool>(field(input, "minimal").data);
    auto run = [&](auto query, auto facet_request, auto raw_projection, std::string_view table) {
        return run_target(user_context,
                          connection,
                          observation,
                          input,
                          std::move(query),
                          facet_request,
                          raw_projection,
                          table);
    };
    auto result = [&]() {
        if (entity == "school") {
            return run(minimal ? Q.schoolsMinimal() : Q.schools(), facet, raw, "school_data");
        }
        if (entity == "academy") {
            return run(minimal ? Q.academiesMinimal() : Q.academies(), facet, raw, "academy_data");
        }
        if (entity == "research_academy") {
            return run(minimal ? Q.researchAcademiesMinimal() : Q.researchAcademies(),
                       facet,
                       raw,
                       "research_academy_data");
        }
        return run(
            minimal ? Q.enrollmentsMinimal() : Q.enrollments(),
            [&](auto& query) {
                query.facetBySchoolAsWithOptions(
                    "_composed",
                    Q.schoolsMinimal().selectName().comment("Inspect facet PRIVATE-SCHOOL-CANARY"),
                    true);
            },
            [&](auto& query) {
                query.selectNoteRaw(
                    text(field(input, "shape")) == "scalar_null" ? "NULL" : "substr(note,1,7)");
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
                    throw std::runtime_error("Invalid execution result-model input");
                }
                std::cout
                    << observe_models(*user_context, *input).dump(user_context->memory_resource())
                    << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Adapted result retained User memory");
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
