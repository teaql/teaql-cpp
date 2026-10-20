#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace cpp_dynamic_json;

template <class T, class Error>
static void scalar_result(Json::Object& output, Memory r, const Result<T, Error>& result) {
    if (result) {
        output.insert_or_assign(String("value", r), Json(r, result.value()));
    } else {
        output.insert_or_assign(String("error", r), Json(r, result.error().to_string(r)));
    }
}

template <class T, class Error>
static void
one_result(Json::Object& output, Memory r, const teaql::Result<std::optional<T>, Error>& result) {
    if (!result) {
        output.insert_or_assign(String("error", r), Json(r, result.error().to_string(r)));
    } else if (result.value()) {
        Json::Array row(r);
        row.emplace_back(result.value()->id());
        row.emplace_back(result.value()->version());
        output.insert_or_assign(String("value", r), Json(r, row));
    }
}

template <class T, class Error>
static void
list_result(Json::Object& output, Memory r, const teaql::Result<SmartList<T>, Error>& result) {
    if (!result) {
        output.insert_or_assign(String("error", r), Json(r, result.error().to_string(r)));
        return;
    }
    Json::Array rows(r);
    for (const auto& entity : result.value().data) {
        Json::Array row(r);
        row.emplace_back(entity.id());
        row.emplace_back(entity.version());
        rows.emplace_back(row);
    }
    output.insert_or_assign(String("value", r), Json(r, rows));
    if (result.value().total_count) {
        output.insert_or_assign(String("total_count", r), Json(r, *result.value().total_count));
    }
}

template <class Request>
static Request pagination(Request request, const Json& input) {
    auto* operations = optional(input, "pagination");
    if (!operations) {
        return request;
    }
    for (const auto& operation : std::get<Json::Array>(operations->data)) {
        auto name = text(field(operation, "op"));
        auto number = std::get<std::uint64_t>(field(operation, "number").data);
        auto size = std::get<std::uint64_t>(field(operation, "size").data);
        if (name == "limit") {
            request.limit(size);
        } else if (name == "skip") {
            request.skip(number);
        } else if (name == "offset_only") {
            request.offsetOnly(number);
        } else if (name == "offset") {
            request.offset(number, size);
        } else if (name == "page_offset") {
            request.pageOffset(number, size);
        } else if (name == "offset_size") {
            request.offsetSize(number, size);
        } else if (name == "top") {
            request.top(size);
        } else if (name == "page_number") {
            request.pageNumber(number, size);
        } else if (name == "page_number_default") {
            request.pageNumberDefault(number);
        } else if (name == "page") {
            request.page(number, size);
        } else if (name == "page_default") {
            request.pageDefault(number);
        } else if (name == "unlimited") {
            request.unlimited();
        } else {
            throw std::runtime_error("unknown pagination operation");
        }
    }
    return request;
}

template <class Request>
static Json execute(UserContext& user_context, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    for (const auto& filter : std::get<Json::Array>(field(input, "filters").data)) {
        request.filterWithJson(text(filter));
    }
    if (auto* optimization = optional(input, "optimization")) {
        if (text(*optimization) == "id_set") {
            auto max_ids =
                request::dynamic_json_u64_field(std::get<Json::Object>(input.data), "max_ids")
                    .value();
            request.optimizePaginationWithIdSetConfig("execution_fixture", 60, max_ids);
        } else if (text(*optimization) == "continuous") {
            request.optimizeForContinuousPageFetchWith("execution_fixture", 60);
        }
    }
    auto shape = text(field(input, "shape"));
    auto entity = text(field(input, "entity"));
    auto grouping = entity == "person"    ? "name"
                    : entity == "product" ? "price"
                    : entity == "line"    ? "quantity"
                                          : "priority";
    if (shape == "group_id") {
        request.groupBy("id");
    } else if (shape == "group_field") {
        request.groupBy(grouping);
    } else if (shape == "count") {
        request.countAs("metric");
    } else if (shape == "sum") {
        request.aggregateSum("id", "metric");
    } else if (shape == "group_sum") {
        request.groupBy(grouping).aggregateSum("id", "metric");
    } else if (shape == "empty_group") {
        request.groupBy(grouping).andFilter(Expr::lt(r, "id", std::int64_t{0}));
    } else if (shape == "count_collision") {
        request.aggregateSum("id", request::COUNT_ALIAS);
    } else if (shape == "stat") {
        request.aggregateStddev("id", "metric");
    } else if (shape != "plain") {
        throw std::logic_error("Unknown execution shape");
    }
    const auto* stage = optional(input, "after_purpose");
    if (!stage || !std::get<bool>(stage->data)) {
        request = pagination(std::move(request), input);
    }
    auto executable = std::move(request)
                          .comment("inspect generated execution")
                          .purpose("qualify generated execution");
    if (stage && std::get<bool>(stage->data)) {
        executable = pagination(std::move(executable), input);
    }
    Json::Object output(r);
    for (const auto* key : {"value", "total_count", "error"}) {
        output.emplace(String(key, r), Json(r));
    }
    auto mode = text(field(input, "mode"));
    if (mode == "count") {
        scalar_result(output, r, executable.executeForCount(user_context).wait());
    } else if (mode == "exists") {
        scalar_result(output, r, executable.executeForExists(user_context).wait());
    } else if (mode == "first") {
        one_result(output, r, executable.executeForFirst(user_context).wait());
    } else if (mode == "one") {
        one_result(output, r, executable.executeForOne(user_context).wait());
    } else if (mode == "page") {
        auto offset =
            request::dynamic_json_u64_field(std::get<Json::Object>(input.data), "offset").value();
        auto limit =
            request::dynamic_json_u64_field(std::get<Json::Object>(input.data), "limit").value();
        list_result(output, r, executable.executeForPage(user_context, offset, limit).wait());
    } else {
        list_result(output, r, executable.executeForList(user_context).wait());
    }
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
    output.emplace(String("sql", r), Json(r, logs));
    return Json(r, output);
}

static Json observe(UserContext& user_context, const Json& input) {
    teaql::test::initialize_dynamic_json_database(user_context);
    if (auto* missing = optional(input, "missing_table");
        missing && std::get<bool>(missing->data)) {
        auto entity = text(field(input, "entity"));
        std::string table = entity == "person"    ? "person_data"
                            : entity == "product" ? "product_data"
                            : entity == "line"    ? "line_data"
                                                  : "order_data";
        auto connection = user_context.require_resource<SqliteMutationExecutor>();
        auto sql = "ALTER TABLE " + table + " RENAME TO hidden_" + table;
        if (sqlite3_exec(require_value(connection).get().connection().native_handle(),
                         sql.c_str(),
                         nullptr,
                         nullptr,
                         nullptr) != SQLITE_OK) {
            throw std::runtime_error("cannot prepare missing table fixture");
        }
    }
    auto run = [&](auto request) {
        if (auto* result = optional(input, "return_type"); result && text(*result) == "person") {
            return execute(user_context, std::move(request).template returnType<Person>(), input);
        }
        return execute(user_context, std::move(request), input);
    };
    auto Q = user_context.queries<query_factory>();
    auto entity = text(field(input, "entity"));
    bool minimal = std::get<bool>(field(input, "minimal").data);
    if (entity == "person") {
        return run(minimal ? Q.peopleMinimal() : Q.people());
    }
    if (entity == "product") {
        return run(minimal ? Q.productsMinimal() : Q.products());
    }
    if (entity == "line") {
        return run(minimal ? Q.linesMinimal() : Q.lines());
    }
    return run(minimal ? Q.ordersMinimal() : Q.orders());
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
                    throw std::runtime_error("invalid execution fixture");
                }
                try {
                    std::cout
                        << observe(*user_context, *input).dump(user_context->memory_resource())
                        << '\n';
                } catch (const std::exception& error) {
                    Json::Object failure(user_context->memory_resource());
                    failure.emplace(String("panic", user_context->memory_resource()),
                                    Json(user_context->memory_resource(), error.what()));
                    std::cout << Json(user_context->memory_resource(), failure)
                                     .dump(user_context->memory_resource())
                              << '\n';
                }
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Execution retained User bytes");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        std::pmr::set_default_resource(previous);
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
