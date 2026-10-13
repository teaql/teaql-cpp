#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/cache.hpp>
#include <iostream>
#include <thread>
using namespace cpp_dynamic_json;

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

static bool flag(const Json& input, std::string_view key) {
    auto* value = optional(input, key);
    return value && std::get<bool>(value->data);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    teaql::test::initialize_dynamic_json_database(user_context);
    auto& cache = user_context.make<RecordingCache>();
    if (!flag(input, "no_backend")) {
        user_context.insert_resource(user_context, static_cast<AggregationCacheBackend&>(cache));
    }
    auto Q = user_context.queries<query_factory>();
    Json::Array rounds(r);
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        const auto& object = std::get<Json::Object>(step.data);
        if (flag(step, "clear")) {
            cache.inner.clear();
        }
        if (auto delay = request::dynamic_json_u64_field(object, "delay")) {
            std::this_thread::sleep_for(std::chrono::milliseconds(*delay));
        }
        if (flag(step, "update")) {
            auto transport = user_context.require_resource<SqliteMutationExecutor>();
            if (sqlite3_exec(require_value(transport).get().connection().native_handle(),
                             "UPDATE order_data SET amount=amount+1 WHERE id=10",
                             nullptr,
                             nullptr,
                             nullptr) != SQLITE_OK) {
                throw std::runtime_error("cannot update cache fixture");
            }
        }
        if (flag(step, "save")) {
            const auto& loaded = require_value(Q.orders()
                                                   .withIdIs(10)
                                                   .comment("load cache mutation")
                                                   .purpose("invalidate aggregation cache")
                                                   .executeForOne(user_context)
                                                   .wait());
            if (!loaded) {
                throw std::runtime_error("cache mutation entity missing");
            }
            auto entity = detail::clone_in(r, *loaded);
            entity.updateAmount(Decimal::from_i64(99));
            require_value(
                AuditedSaveExt<Order>::save(
                    std::move(entity).audit_as("update cached aggregate member"), user_context)
                    .wait());
        }
        auto query = Q.peopleMinimal();
        auto child = Q.orders().unlimited();
        if (auto age = request::dynamic_json_u64_field(object, "child_age")) {
            child.enableAggregationCacheFor(*age);
        }
        if (flag(step, "filter")) {
            child.filter(Expr::gt(r, "amount", std::int64_t(9)));
        }
        if (text(field(input, "kind")) == "sum") {
            query.sumAmountOfOrdersAs("_metric", child);
        } else {
            query.countOrdersWith("_metric", child);
        }
        if (auto age = request::dynamic_json_u64_field(object, "age")) {
            query.enableAggregationCacheFor(*age);
        }
        if (auto age = request::dynamic_json_u64_field(object, "propagate")) {
            query.propagateAggregationCache(*age);
        }
        if (flag(step, "empty")) {
            query.filter(Expr::lt(r, "id", std::int64_t(0)));
        }
        query.orderByIdAsc();
        auto comment = optional(step, "comment") ? text(field(step, "comment")) : "inspect cache";
        auto purpose = optional(step, "purpose") ? text(field(step, "purpose")) : "qualify cache";
        const auto& result =
            std::move(query).comment(comment).purpose(purpose).executeForList(user_context).wait();
        Json::Object out(r);
        Json::Array values(r);
        Json error(r);
        if (result) {
            for (const auto& entity : result.value().data) {
                Json::Array row(r);
                row.emplace_back(entity.id());
                auto found = entity.dynamic.find("_metric");
                row.emplace_back(found == entity.dynamic.end()
                                     ? Json(r)
                                     : Json(r, debug_string(r, found->second)));
                values.emplace_back(row);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
        out.emplace(String("values", r), Json(r, values));
        out.emplace(String("error", r), std::move(error));
        out.emplace(String("events", r), Json(r, cache.events));
        cache.events.clear();
        Json::Array logs(r);
        for (const auto& entry : user_context.sql_logs()) {
            Json::Object log(r);
            Json::Array params(r);
            for (const auto& value : entry.params) {
                params.emplace_back(debug_string(r, value));
            }
            log.emplace(String("sql", r), Json(r, entry.sql));
            log.emplace(String("params", r), Json(r, params));
            log.emplace(String("comment", r), entry.comment ? Json(r, *entry.comment) : Json(r));
            log.emplace(String("purpose", r), entry.purpose ? Json(r, *entry.purpose) : Json(r));
            logs.emplace_back(log);
        }
        out.emplace(String("sql", r), Json(r, logs));
        rounds.emplace_back(out);
    }
    return Json(r, rounds);
}

int main() {
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << observe(*user_context, *input).dump(r) << '\n';
    }
}
