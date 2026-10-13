#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

static bool flag(const Json& input, std::string_view key) {
    auto* value = optional(input, key);
    return value && std::get<bool>(value->data);
}

template <class Request>
static Request child_query(Request query, const Json& input) {
    auto r = query.memory_resource();
    std::int64_t cutoff = 0;
    if (const auto* value = optional(input, "cutoff")) {
        if (const auto* number = std::get_if<std::int64_t>(&value->data)) {
            cutoff = *number;
        } else if (const auto* number = std::get_if<std::uint64_t>(&value->data)) {
            cutoff = static_cast<std::int64_t>(*number);
        }
    }
    query.filter(Expr::gt(r, "id", cutoff));
    if (flag(input, "child_deleted")) {
        query.withDeletedRows();
    }
    if (flag(input, "child_slice")) {
        query.orderByIdDesc().limit(1).offset(1, 1);
    }
    if (flag(input, "child_group")) {
        query.groupBy("version").countAs("_child_count");
    }
    if (flag(input, "child_options")) {
        query.comment("child option comment").createPropertyAs("_custom", "42").rawSqlFilter("1=1");
    }
    return query;
}

template <class T>
static Json entity_observation(Memory r, const T& entity, const Json& input) {
    Json::Object out(r);
    out.emplace(String("id", r), Json(r, entity.id()));
    out.emplace(String("version", r), Json(r, entity.version()));
    Json::Array values(r);
    for (const auto& key : std::get<Json::Array>(field(input, "columns").data)) {
        auto name = text(key);
        auto found = entity.dynamic.find(name);
        values.emplace_back(
            found == entity.dynamic.end() ? Json(r) : Json(r, debug_string(r, found->second)));
    }
    out.emplace(String("values", r), Json(r, values));
    Json detail(r);
    if constexpr (requires { entity.customer(); }) {
        if (auto* customer = entity.customer()) {
            Json::Object fields(r);
            fields.emplace(String("id", r), Json(r, customer->id()));
            fields.emplace(String("name", r),
                           customer->is_loaded("name") ? Json(r, customer->name()) : Json(r));
            detail = Json(r, fields);
        }
    }
    out.emplace(String("detail", r), std::move(detail));
    return Json(r, out);
}

template <class Request>
static Json finish(UserContext& user_context, Request query, const Json& input) {
    auto r = user_context.memory_resource();
    if (flag(input, "empty")) {
        query.filter(Expr::lt(r, "id", std::int64_t(0)));
    }
    if (flag(input, "deleted")) {
        query.withDeletedRows();
    }
    if (flag(input, "outer_count")) {
        query.countAs("_outer_count");
    }
    query.orderByIdAsc();
    Json::Object out(r);
    out.emplace(String("query", r), Json(r, debug_string(r, query.query())));
    if (flag(input, "execute")) {
        teaql::test::initialize_dynamic_json_database(user_context);
        auto executable = std::move(query)
                              .comment("inspect relationship analysis")
                              .purpose("qualify relationship analysis");
        auto mode = optional(input, "mode") ? text(field(input, "mode")) : "list";
        Json::Array rows(r);
        Json error(r);
        Json total(r);
        auto list = [&](const auto& result) {
            if (!result) {
                error = Json(r, result.error().to_string(r));
                return;
            }
            if (result.value().total_count) {
                total = Json(r, *result.value().total_count);
            }
            for (const auto& entity : result.value().data) {
                rows.emplace_back(entity_observation(r, entity, input));
            }
        };
        if (mode == "rows") {
            const auto& result = executable.executeForRows(user_context).wait();
            if (!result) {
                error = Json(r, result.error().to_string(r));
            } else {
                for (const auto& row : result.value().data) {
                    Json::Array values(r);
                    for (const auto& key : std::get<Json::Array>(field(input, "columns").data)) {
                        auto* value = row.get(text(key));
                        values.emplace_back(value ? Json(r, debug_string(r, *value)) : Json(r));
                    }
                    rows.emplace_back(values);
                }
            }
        } else if (mode == "one") {
            const auto& result = executable.executeForOne(user_context).wait();
            if (!result) {
                error = Json(r, result.error().to_string(r));
            } else if (result.value()) {
                rows.emplace_back(entity_observation(r, *result.value(), input));
            }
        } else if (mode == "page") {
            list(executable.executeForPage(user_context, 1, 2).wait());
        } else {
            list(executable.executeForList(user_context).wait());
        }
        out.emplace(String("rows", r), Json(r, rows));
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

static Value native_value(Memory r, const Json& input) {
    if (std::holds_alternative<std::monostate>(input.data)) {
        return Value(r);
    }
    if (const auto* value = std::get_if<bool>(&input.data)) {
        return Value(r, *value);
    }
    if (const auto* value = std::get_if<std::int64_t>(&input.data)) {
        return Value(r, *value);
    }
    if (const auto* value = std::get_if<std::uint64_t>(&input.data)) {
        return Value(r, static_cast<std::int64_t>(*value));
    }
    if (const auto* value = std::get_if<double>(&input.data)) {
        return Value(r, *value);
    }
    if (const auto* value = std::get_if<String>(&input.data)) {
        return Value(r, *value);
    }
    if (const auto* value = std::get_if<Json::Array>(&input.data)) {
        Value::List out(r);
        for (const auto& item : *value) {
            out.emplace_back(native_value(r, item));
        }
        return Value(r, out);
    }
    Record out(r);
    for (const auto& [key, item] : std::get<Json::Object>(input.data)) {
        out.emplace(String(key, r), native_value(r, item));
    }
    return Value(r, out);
}

static Json decode_observation(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    CompactRow row(std::allocator_arg, Allocator(r));
    row.insert("id", Value(r, std::int64_t(10)));
    row.insert("version", Value(r, std::int64_t(1)));
    auto relation = text(field(input, "relation"));
    if (const auto* payload = optional(input, "payload")) {
        row.insert(relation, native_value(r, *payload));
    }
    auto& root = user_context.make<EntityRuntimeState>();
    auto decoded = Order::from_compact_row_with_context(r, std::move(row), AnyRef(root));
    Json::Object out(r);
    if (!decoded) {
        out.emplace(String("error", r), Json(r, decoded.error().to_string(r)));
    } else {
        auto& entity = decoded.value();
        auto* customer = entity.customer();
        out.emplace(String("customer", r), customer ? Json(r, customer->id()) : Json(r));
        out.emplace(String("lines", r),
                    Json(r, static_cast<std::uint64_t>(entity.lines().data.size())));
        out.emplace(String("lines_loaded", r), Json(r, entity.linesHandle().is_loaded()));
        out.emplace(String("relation_loaded", r), Json(r, entity.is_loaded(relation)));
        out.emplace(String("error", r), Json(r));
    }
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    if (const auto* kind = optional(input, "kind"); kind && text(*kind) == "decode") {
        return decode_observation(user_context, input);
    }
    auto Q = user_context.queries<query_factory>();
    auto alias = optional(input, "alias") ? text(field(input, "alias")) : "metric";
    if (text(field(input, "entity")) == "person") {
        auto child =
            child_query(flag(input, "child_minimal") ? Q.ordersMinimal() : Q.orders(), input);
        auto query = flag(input, "minimal") ? Q.peopleMinimal() : Q.people();
        for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
            auto name = text(action);
            if (name == "have") {
                query.haveOrders();
            } else if (name == "have_no") {
                query.haveNoOrders();
            } else if (name == "matching") {
                query.withOrdersMatching(child);
            } else if (name == "without") {
                query.withoutOrdersMatching(child);
            } else if (name == "count") {
                query.countOrders();
            } else if (name == "count_as") {
                query.countOrdersAs(alias);
            } else if (name == "count_with") {
                query.countOrdersWith(alias, child);
            } else if (name == "stats") {
                query.statsFromOrders(child);
            } else if (name == "stats_as") {
                query.statsFromOrdersAs(alias, child);
            } else if (name == "group_details") {
                query.groupByOrdersWithDetails(child);
            } else if (name == "sum") {
                query.sumAmountOfOrders();
            } else if (name == "sum_as") {
                query.sumAmountOfOrdersAs(alias, child);
            } else if (name == "avg") {
                query.avgAmountOfOrders();
            } else if (name == "avg_as") {
                query.avgAmountOfOrdersAs(alias, child);
            } else if (name == "min") {
                query.minAmountOfOrders();
            } else if (name == "min_as") {
                query.minAmountOfOrdersAs(alias, child);
            } else if (name == "max") {
                query.maxAmountOfOrders();
            } else if (name == "max_as") {
                query.maxAmountOfOrdersAs(alias, child);
            } else if (name == "standard_deviation") {
                query.standardDeviationAmountOfOrders();
            } else if (name == "standard_deviation_as") {
                query.standardDeviationAmountOfOrdersAs(alias, child);
            } else if (name == "square_root_of_population_standard_deviation") {
                query.squareRootOfPopulationStandardDeviationAmountOfOrders();
            } else if (name == "square_root_of_population_standard_deviation_as") {
                query.squareRootOfPopulationStandardDeviationAmountOfOrdersAs(alias, child);
            } else if (name == "sample_variance") {
                query.sampleVarianceAmountOfOrders();
            } else if (name == "sample_variance_as") {
                query.sampleVarianceAmountOfOrdersAs(alias, child);
            } else if (name == "sample_population_variance") {
                query.samplePopulationVarianceAmountOfOrders();
            } else if (name == "sample_population_variance_as") {
                query.samplePopulationVarianceAmountOfOrdersAs(alias, child);
            } else if (name == "have_profile") {
                query.haveProfile();
            } else if (name == "have_no_profile") {
                query.haveNoProfile();
            } else {
                throw std::runtime_error("unknown person action");
            }
        }
        return finish(user_context, std::move(query), input);
    }
    auto child = child_query(flag(input, "child_minimal") ? Q.linesMinimal() : Q.lines(), input);
    auto customer =
        child_query(flag(input, "child_minimal") ? Q.peopleMinimal() : Q.people(), input);
    auto query = flag(input, "minimal") ? Q.ordersMinimal() : Q.orders();
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto name = text(action);
        if (name == "have") {
            query.haveLines();
        } else if (name == "have_no") {
            query.haveNoLines();
        } else if (name == "matching") {
            query.withLinesMatching(child);
        } else if (name == "without") {
            query.withoutLinesMatching(child);
        } else if (name == "count") {
            query.countLines();
        } else if (name == "count_as") {
            query.countLinesAs(alias);
        } else if (name == "count_with") {
            query.countLinesWith(alias, child);
        } else if (name == "stats") {
            query.statsFromLines(child);
        } else if (name == "stats_as") {
            query.statsFromLinesAs(alias, child);
        } else if (name == "group_details") {
            query.groupByLinesWithDetails(child);
        } else if (name == "sum") {
            query.sumQuantityOfLines();
        } else if (name == "sum_as") {
            query.sumQuantityOfLinesAs(alias, child);
        } else if (name == "avg") {
            query.avgQuantityOfLines();
        } else if (name == "avg_as") {
            query.avgQuantityOfLinesAs(alias, child);
        } else if (name == "min") {
            query.minQuantityOfLines();
        } else if (name == "min_as") {
            query.minQuantityOfLinesAs(alias, child);
        } else if (name == "max") {
            query.maxQuantityOfLines();
        } else if (name == "max_as") {
            query.maxQuantityOfLinesAs(alias, child);
        } else if (name == "standard_deviation") {
            query.standardDeviationQuantityOfLines();
        } else if (name == "standard_deviation_as") {
            query.standardDeviationQuantityOfLinesAs(alias, child);
        } else if (name == "square_root_of_population_standard_deviation") {
            query.squareRootOfPopulationStandardDeviationQuantityOfLines();
        } else if (name == "square_root_of_population_standard_deviation_as") {
            query.squareRootOfPopulationStandardDeviationQuantityOfLinesAs(alias, child);
        } else if (name == "sample_variance") {
            query.sampleVarianceQuantityOfLines();
        } else if (name == "sample_variance_as") {
            query.sampleVarianceQuantityOfLinesAs(alias, child);
        } else if (name == "sample_population_variance") {
            query.samplePopulationVarianceQuantityOfLines();
        } else if (name == "sample_population_variance_as") {
            query.samplePopulationVarianceQuantityOfLinesAs(alias, child);
        } else if (name == "have_customer") {
            query.haveCustomer();
        } else if (name == "have_no_customer") {
            query.haveNoCustomer();
        } else if (name == "matching_customer") {
            query.withCustomerMatching(customer);
        } else if (name == "without_customer") {
            query.withoutCustomerMatching(customer);
        } else if (name == "group_customer") {
            query.groupByCustomerWith(customer);
        } else if (name == "group_customer_details") {
            query.groupByCustomerWithDetails();
        } else if (name == "group_customer_from") {
            query.groupByCustomerWithDetailsFrom(customer);
        } else if (name == "roll_customer") {
            query.rollUpToCustomer();
        } else if (name == "roll_customer_with") {
            query.rollUpToCustomerWith(customer);
        } else {
            throw std::runtime_error("unknown order action");
        }
    }
    return finish(user_context, std::move(query), input);
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
