#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

static bool flag(const Json& input, std::string_view key) {
    auto* value = optional(input, key);
    return value && std::get<bool>(value->data);
}

struct WithoutPerson : EntityRegistry {
    using allocator_type = Allocator;
    Memory resource;

    WithoutPerson(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    bool contains(std::string_view entity) const override {
        return entity != "Person";
    }
};

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    auto facet = flag(input, "facet_minimal") ? Q.peopleMinimal() : Q.people();
    if (flag(input, "facet_filter")) {
        facet.filter(Expr::gt(r, "id", std::int64_t(2)));
    }
    if (flag(input, "facet_deleted")) {
        facet.withDeletedRows();
    }
    if (flag(input, "facet_slice")) {
        facet.pageOffset(1, 1);
    }
    if (flag(input, "facet_comment")) {
        facet.comment("facet own comment");
    }
    if (flag(input, "facet_raw")) {
        facet.createPropertyAs("_custom", "42");
    }
    if (flag(input, "facet_membership")) {
        facet.withOrdersMatching(Q.orders().filter(Expr::gt(r, "amount", std::int64_t(9))));
    }
    auto aggregate = Q.orders().unlimited();
    if (flag(input, "aggregate_filter")) {
        aggregate.filter(Expr::gt(r, "amount", std::int64_t(9)));
    }
    if (flag(input, "aggregate_options")) {
        aggregate.comment("aggregate own comment").createPropertyAs("_custom", "43");
    }
    if (flag(input, "aggregate_group")) {
        aggregate.groupByPriority().countAs("_group_count");
    }
    if (flag(input, "aggregate_nested")) {
        aggregate.groupByCustomerWithDetailsFrom(Q.peopleMinimal());
    }
    if (flag(input, "with_count")) {
        facet.countOrdersWith("_metric", aggregate);
    }
    if (flag(input, "with_stats")) {
        facet.statsFromOrdersAs("_stats", aggregate);
    }
    if (flag(input, "with_sum")) {
        facet.sumAmountOfOrdersAs("_sum", aggregate);
    }
    if (flag(input, "with_nested_facet")) {
        facet.facetByPlatformAs("_nested", Q.platforms());
    }
    if (flag(input, "facet_order")) {
        facet.orderByIdDesc();
    }
    auto query = flag(input, "minimal") ? Q.ordersMinimal() : Q.orders();
    if (flag(input, "outer_filter")) {
        query.filter(Expr::gt(r, "amount", std::int64_t(9)));
    }
    if (flag(input, "outer_deleted")) {
        query.withDeletedRows();
    }
    if (flag(input, "empty")) {
        query.filter(Expr::lt(r, "id", std::int64_t(0)));
    }
    query.orderByIdAsc();
    auto name = optional(input, "name") ? text(field(input, "name")) : "_customers";
    if (flag(input, "default_options")) {
        query.facetByCustomerAs(name, facet);
    } else {
        query.facetByCustomerAsWithOptions(name, facet, flag(input, "include_all"));
    }
    if (flag(input, "repeat")) {
        query.facetByCustomerAsWithOptions(name, std::move(facet).withDeletedRows(), true);
    }
    if (flag(input, "second")) {
        query.facetBySellerAs("_sellers", Q.companies());
    }
    Json::Object out(r);
    out.emplace(String("query", r), Json(r, debug_string(r, query.query())));
    if (flag(input, "execute")) {
        teaql::test::initialize_dynamic_json_database(user_context);
        if (flag(input, "missing_outer_table") || flag(input, "missing_facet_table")) {
            auto transport = user_context.require_resource<SqliteMutationExecutor>();
            const char* sql = flag(input, "missing_outer_table")
                                  ? "ALTER TABLE order_data RENAME TO hidden_order"
                                  : "ALTER TABLE person_data RENAME TO hidden_person";
            if (sqlite3_exec(require_value(transport).get().connection().native_handle(),
                             sql,
                             nullptr,
                             nullptr,
                             nullptr) != SQLITE_OK) {
                throw std::runtime_error("cannot prepare missing table fixture");
            }
        }
        if (flag(input, "missing_relation")) {
            auto& metadata = user_context.make<InMemoryMetadataStore>();
            auto outer = Order::entity_descriptor(r);
            Vector<RelationDescriptor> relations(r);
            for (const auto& relation : outer.relations) {
                if (relation.name != "customer") {
                    relations.emplace_back(relation);
                }
            }
            outer.relations.swap(relations);
            metadata.register_entity(std::move(outer));
            metadata.register_entity(Person::entity_descriptor(r));
            user_context.set_metadata(user_context, metadata);
        }
        if (flag(input, "missing_facet_registry")) {
            auto& registry = user_context.make<WithoutPerson>();
            user_context.set_entity_registry(user_context, registry);
        }
        auto executable = std::move(query).comment("inspect facets").purpose("qualify facets");
        Json::Array ids(r);
        Json::Object facets(r);
        Json error(r), total(r);
        auto mode = optional(input, "mode") ? text(field(input, "mode")) : "list";
        auto list = [&](const auto& result) {
            if (!result) {
                error = Json(r, result.error().to_string(r));
                return;
            }
            if (result.value().total_count) {
                total = Json(r, *result.value().total_count);
            }
            for (const auto& entity : result.value().data) {
                ids.emplace_back(entity.id());
            }
            for (const auto& [name, facet] : result.value().facets) {
                Json::Array rows(r);
                for (const auto& row : facet.data) {
                    Json::Array values(r);
                    for (auto key : {"id",
                                     "version",
                                     "name",
                                     "platform_id",
                                     "_metric",
                                     "_stats",
                                     "_sum",
                                     "_custom",
                                     "_nested"}) {
                        auto* value = row.get(key);
                        values.emplace_back(value ? Json(r, debug_string(r, *value)) : Json(r));
                    }
                    rows.emplace_back(values);
                }
                facets.emplace(String(name, r), Json(r, rows));
            }
        };
        if (mode == "one") {
            const auto& result = executable.executeForOne(user_context).wait();
            if (!result) {
                error = Json(r, result.error().to_string(r));
            } else if (result.value()) {
                ids.emplace_back(result.value()->id());
            }
        } else if (mode == "rows") {
            const auto& result = executable.executeForRows(user_context).wait();
            if (!result) {
                error = Json(r, result.error().to_string(r));
            } else {
                for (const auto& row : result.value().data) {
                    ids.emplace_back(*row.get("id")->try_u64());
                }
            }
        } else if (mode == "count") {
            const auto& result = executable.executeForCount(user_context).wait();
            if (!result) {
                error = Json(r, result.error().to_string(r));
            } else {
                out.emplace(String("count", r), Json(r, result.value()));
            }
        } else if (mode == "exists") {
            const auto& result = executable.executeForExists(user_context).wait();
            if (!result) {
                error = Json(r, result.error().to_string(r));
            } else {
                out.emplace(String("exists", r), Json(r, result.value()));
            }
        } else if (mode == "page") {
            list(executable.executeForPage(user_context, 1, 1).wait());
        } else {
            list(executable.executeForList(user_context).wait());
        }
        out.emplace(String("ids", r), Json(r, ids));
        out.emplace(String("facets", r), Json(r, facets));
        out.emplace(String("error", r), std::move(error));
        out.emplace(String("total", r), std::move(total));
        Json::Array logs(r);
        for (const auto& entry : user_context.sql_logs()) {
            Json::Object log(r);
            Json::Array params(r);
            for (const auto& parameter : entry.params) {
                params.emplace_back(debug_string(r, parameter));
            }
            log.emplace(String("sql", r), Json(r, entry.sql));
            log.emplace(String("params", r), Json(r, params));
            log.emplace(String("comment", r), entry.comment ? Json(r, *entry.comment) : Json(r));
            log.emplace(String("purpose", r), entry.purpose ? Json(r, *entry.purpose) : Json(r));
            logs.emplace_back(log);
        }
        out.emplace(String("sql", r), Json(r, logs));
    }
    return Json(r, out);
}

int main() {
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        try {
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        } catch (const std::exception& error) {
            Json::Object out(r);
            out.emplace(String("panic", r), Json(r, error.what()));
            std::cout << Json(r, out).dump(r) << '\n';
        }
    }
}
