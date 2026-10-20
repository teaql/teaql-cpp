#define main composed_general_fixture_main
#include "composed_general_probe.cpp"
#undef main

static Json observe_lookup(UserContext& user_context, const Json& input) {
    auto connection_ref = user_context.make_resource_ref<SqliteConnection>();
    auto& connection = user_context.get(connection_ref);
    auto& observation = user_context.make<Observation>();
    auto& policy = user_context.make<Policy>("allow", observation);
    user_context.set_request_policy(user_context, policy);
    if (sqlite3_set_authorizer(connection.native_handle(), database, &observation) != SQLITE_OK) {
        throw std::runtime_error("Install entry-lookup authorizer");
    }
    auto Q = user_context.queries<query_factory>();
    auto facet = [&](auto& query) {
        query.facetByAdvisorAsWithOptions(
            "_composed",
            Q.specialistsMinimal().selectName().comment("Inspect facet PRIVATE-SCHOOL-CANARY"),
            true);
    };
    auto raw = [](auto& query) {
        query.selectNameRaw("substr(name,1,7)");
    };
    auto run = [&](auto query, auto facet_request, auto raw_projection, std::string_view table) {
        return ::apply(user_context,
                       connection,
                       observation,
                       std::move(query),
                       input,
                       facet_request,
                       raw_projection,
                       table);
    };
    auto entity = text(field(input, "entity"));
    auto result = [&]() {
        if (entity == "school") {
            return run(Q.schools(), facet, raw, "school_data");
        }
        if (entity == "academy") {
            return run(Q.academies(), facet, raw, "academy_data");
        }
        if (entity == "research_academy") {
            return run(Q.researchAcademies(), facet, raw, "research_academy_data");
        }
        return run(
            Q.enrollments(),
            [&](auto& query) {
                query.facetBySchoolAsWithOptions(
                    "_composed",
                    Q.schoolsMinimal().selectName().comment("Inspect facet PRIVATE-SCHOOL-CANARY"),
                    true);
            },
            [](auto& query) {
                query.selectNoteRaw("substr(note,1,7)");
            },
            "enrollment_data");
    }();
    sqlite3_set_authorizer(connection.native_handle(), nullptr, nullptr);
    return result;
}

static void abandon_lookup(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto request = Q.schools()
                       .comment("Inspect unavailable service")
                       .purpose("Verify failure operation cleanup");
    auto abandon = [&](auto& operation) {
        if (operation.status() != AsyncStatus::Pending) {
            throw std::logic_error("Lookup failure must remain an unpolled operation");
        }
        if (std::get<bool>(field(input, "cancel").data)) {
            operation.cancel();
            if (operation.status() != AsyncStatus::Cancelled) {
                throw std::logic_error("Lookup failure operation did not cancel");
            }
        }
    };
    auto mode = text(field(input, "mode"));
    if (mode == "list") {
        abandon(request.executeForList(user_context));
    } else if (mode == "rows") {
        abandon(request.executeForRows(user_context));
    } else if (mode == "first") {
        abandon(request.executeForFirst(user_context));
    } else if (mode == "one") {
        abandon(request.executeForOne(user_context));
    } else if (mode == "count") {
        abandon(request.executeForCount(user_context));
    } else if (mode == "exists") {
        abandon(request.executeForExists(user_context));
    } else if (mode == "page") {
        abandon(request.executeForPage(user_context, 0, 1));
    } else if (mode == "stream") {
        abandon(request.executeForStream(user_context));
    } else {
        throw std::invalid_argument("Unknown lookup operation");
    }
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
                    throw std::runtime_error("Invalid entry-lookup input");
                }
                if (optional(*input, "ownership")) {
                    abandon_lookup(*user_context, *input);
                    std::cout << "true\n";
                } else {
                    std::cout << observe_lookup(*user_context, *input)
                                     .dump(user_context->memory_resource())
                              << '\n';
                }
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Entry lookup retained User memory");
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
