#include <teaql/runtime/cache.hpp>
#define main composed_general_fixture_main
#include "composed_general_probe.cpp"
#undef main

struct RecordingCache : AggregationCacheBackend {
    using allocator_type = Allocator;
    InMemoryAggregationCache inner;
    mutable Json::Array events;

    RecordingCache(std::allocator_arg_t, Allocator a)
        : inner(std::allocator_arg, a, "fixture"), events(a.resource()) {}

    Memory memory_resource() const noexcept override {
        return inner.memory_resource();
    }

    std::string_view name_space() const noexcept override {
        return inner.name_space();
    }

    std::optional<Vector<CompactRow>>
    get(Memory r, std::string_view key, std::uint64_t age) const override {
        auto rows = inner.get(r, key, age);
        auto owner = memory_resource();
        Json::Object event(owner);
        event.emplace(String("op", owner), Json(owner, "get"));
        event.emplace(String("key", owner), Json(owner, key));
        event.emplace(String("age", owner), Json(owner, age));
        event.emplace(String("hit", owner), Json(owner, bool(rows)));
        events.emplace_back(event);
        return rows;
    }

    void put(std::string_view key, std::span<const CompactRow> rows) override {
        auto r = memory_resource();
        Json::Object event(r);
        event.emplace(String("op", r), Json(r, "put"));
        event.emplace(String("key", r), Json(r, key));
        event.emplace(String("rows", r), Json(r, static_cast<std::uint64_t>(rows.size())));
        events.emplace_back(event);
        inner.put(key, rows);
    }

    void invalidate_namespace(std::string_view name) override {
        auto r = memory_resource();
        Json::Object event(r);
        event.emplace(String("op", r), Json(r, "invalidate"));
        event.emplace(String("namespace", r), Json(r, name));
        events.emplace_back(event);
        inner.invalidate_namespace(name);
    }
};

static Json observe_hot(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
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
    auto& cache = user_context.make<RecordingCache>();
    bool cached = std::get<bool>(field(input, "cache").data);
    if (cached) {
        user_context.insert_resource(user_context, static_cast<AggregationCacheBackend&>(cache));
    }
    Json::Array rounds(r);
    auto Q = user_context.queries<query_factory>();
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        Json round(r, input);
        auto& object = std::get<Json::Object>(round.data);
        object.insert_or_assign(String("policy", r), Json(r, field(step, "policy")));
        object.insert_or_assign(String("logging", r), Json(r, true));
        object.insert_or_assign(String("intent", r), Json(r, "valid"));
        auto& policy = user_context.make<Policy>(text(field(step, "policy")), observation);
        user_context.set_request_policy(user_context, policy);
        if (auto* update = optional(step, "update"); update && std::get<bool>(update->data)) {
            if (sqlite3_exec(
                    connection.native_handle(),
                    "UPDATE school_data SET name='PRIVATE-SCHOOL-CANARY' WHERE id=8;"
                    "UPDATE academy_data SET name='PRIVATE-SCHOOL-CANARY' WHERE id=8;"
                    "UPDATE research_academy_data SET name='PRIVATE-SCHOOL-CANARY' WHERE id=8;",
                    nullptr,
                    nullptr,
                    nullptr) != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
            }
        }
        if (auto* clear = optional(step, "clear"); clear && std::get<bool>(clear->data)) {
            cache.inner.clear();
        }
        if (auto* replace = optional(step, "replace"); replace && std::get<bool>(replace->data)) {
            auto& ids = user_context.make<InMemoryIdSetStore>();
            user_context.set_id_set_store(user_context, ids);
            auto& cursors = user_context.make<InMemoryContinuousPageCursorStore>();
            user_context.set_continuous_page_cursor_store(user_context, cursors);
        }
        cache.events.clear();
        observation.calls.clear();
        observation.selects = 0;
        user_context.clear_sql_logs();
        if (sqlite3_set_authorizer(connection.native_handle(), database, &observation) !=
            SQLITE_OK) {
            throw std::runtime_error("Install hot-query authorizer");
        }
        auto run = [&](auto query, auto facet, auto raw, std::string_view table) {
            query.orderByIdAsc().offsetOnly(number<std::uint64_t>(field(step, "offset"))).limit(1);
            if (cached) {
                query.enableAggregationCacheFor(0).propagateAggregationCache(0);
            }
            return ::apply(
                user_context, connection, observation, std::move(query), round, facet, raw, table);
        };
        auto facet = [&](auto& query) {
            query.facetByAdvisorAsWithOptions(
                "_composed",
                Q.specialistsMinimal().selectName().comment("Inspect facet PRIVATE-SCHOOL-CANARY"),
                true);
        };
        auto raw = [](auto& query) {
            query.selectNameRaw("substr(name,1,7)");
        };
        auto entity = text(field(input, "entity"));
        auto output = [&]() {
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
                    query.facetBySchoolAsWithOptions("_composed",
                                                     Q.schoolsMinimal().selectName().comment(
                                                         "Inspect facet PRIVATE-SCHOOL-CANARY"),
                                                     true);
                },
                [](auto& query) {
                    query.selectNoteRaw("substr(note,1,7)");
                },
                "enrollment_data");
        }();
        auto& result = std::get<Json::Object>(output.data);
        auto plan = user_context.id_set_plan();
        auto count = user_context.id_set_count();
        auto continuous = user_context.continuous_page_plan();
        result.emplace("id_set_plan", plan ? Json(r, *plan) : Json(r));
        result.emplace("id_set_count", count ? Json(r, *count) : Json(r));
        result.emplace("continuous_plan", continuous ? Json(r, *continuous) : Json(r));
        result.emplace("cache_events", Json(r, cache.events));
        rounds.emplace_back(std::move(output));
        sqlite3_set_authorizer(connection.native_handle(), nullptr, nullptr);
    }
    return Json(r, rounds);
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
                    throw std::runtime_error("Invalid hot execution input");
                }
                std::cout
                    << observe_hot(*user_context, *input).dump(user_context->memory_resource())
                    << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Hot execution retained User memory");
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
