#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include "../generated_inheritance_business_ids/include/cpp_inheritance_business_ids.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

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

template <class Factory, class Number, class Manual, class Lookup>
static Json execute(UserContext& user_context,
                    Factory Q,
                    Manual manual,
                    Lookup lookup,
                    const Json& input,
                    bool runtime_composition) {
    auto r = user_context.memory_resource();
    auto number = Number::create(r, text(field(input, "text")));
    if (!number) {
        Json::Object error(r);
        error.emplace("error", Json(r, number.error()));
        return Json(r, error);
    }
    auto reference = manual(Q, number.value());
    auto query = runtime_composition ? manual(Q, number.value()) : lookup(Q, number.value());
    Json::Object output(r);
    output.emplace("value", Json(r, number.value().asStr()));
    output.emplace("manual", Json(r, debug_string(r, reference.query())));
    output.emplace("lookup", Json(r, debug_string(r, query.query())));
    auto operation = text(field(input, "operation"));
    if (operation == "larger") {
        query.limit(10);
    } else if (operation == "offset") {
        query.skip(2).limit(10);
    } else if (operation == "deleted") {
        query.withDeletedRows().limit(10);
    }
    output.emplace("after", Json(r, debug_string(r, query.query())));
    Json::Array rows(r);
    Json scalar(r), error(r);
    auto mode = text(field(input, "mode"));
    if (mode != "construct") {
        auto request = std::move(query)
                           .comment("Lookup a business identifier")
                           .purpose("Qualify typed lookup scope");
        if (mode == "count") {
            const auto& result = request.executeForCount(user_context).wait();
            if (result) {
                scalar = Json(r, result.value());
            } else {
                error = Json(r, result.error().to_string(r));
            }
        } else if (mode == "one") {
            const auto& result = request.executeForOne(user_context).wait();
            if (result) {
                if (result.value()) {
                    rows.emplace_back(snapshot(r, *result.value()));
                }
            } else {
                error = Json(r, result.error().to_string(r));
            }
        } else if (mode == "list") {
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
    }
    output.emplace("rows", Json(r, rows));
    output.emplace("scalar", std::move(scalar));
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
        log.emplace("result_count", entry.result_count ? Json(r, *entry.result_count) : Json(r));
        log.emplace("result_type", entry.result_type ? Json(r, *entry.result_type) : Json(r));
        log.emplace("affected_rows", entry.affected_rows ? Json(r, *entry.affected_rows) : Json(r));
        log.emplace("summary", Json(r, entry.result_summary));
        logs.emplace_back(log);
    }
    output.emplace("logs", Json(r, logs));
    return Json(r, output);
}

static Json observe(UserContext& user_context, const Json& input, bool runtime_composition) {
    auto entity = text(field(input, "entity"));
    if (entity != "document") {
        auto connection = user_context.make_resource_ref<SqliteConnection>();
        std::ifstream file(std::string(TEAQL_SOURCE_DIR) +
                           "/tests/support/business_id_lookup_rows.sql");
        std::ostringstream sql;
        sql << file.rdbuf();
        if (!file || sqlite3_exec(user_context.get(connection).native_handle(),
                                  sql.str().c_str(),
                                  nullptr,
                                  nullptr,
                                  nullptr) != SQLITE_OK) {
            throw std::runtime_error("Load Business-ID lookup fixture");
        }
        cpp_business_ids::require_value(
            cpp_business_ids::serviceRuntimeFromConnection(user_context, connection));
        user_context.with_active_root("Tenant", 1);
    }
#define LOOKUP(DOMAIN, NUMBER, ORDINARY, PREDICATE, FACTORY)                                       \
    execute<DOMAIN::query_factory, DOMAIN::NUMBER>(                                                \
        user_context,                                                                              \
        user_context.queries<DOMAIN::query_factory>(),                                             \
        [](auto& Q, const auto& value) {                                                           \
            return Q.ORDINARY().PREDICATE(value).limit(2);                                         \
        },                                                                                         \
        [](auto& Q, const auto& value) {                                                           \
            if constexpr (requires { Q.FACTORY(value); }) {                                        \
                return Q.FACTORY(value);                                                           \
            } else {                                                                               \
                throw std::logic_error("Missing generated Business-ID lookup factory");            \
                return Q.ORDINARY().PREDICATE(value).limit(2);                                     \
            }                                                                                      \
        },                                                                                         \
        input,                                                                                     \
        runtime_composition)
    if (entity == "receipt") {
        return LOOKUP(
            cpp_business_ids, ReceiptNumber, receipts, withReceiptNumberIs, receiptByReceiptNumber);
    }
    if (entity == "ticket") {
        return LOOKUP(
            cpp_business_ids, TicketNumber, tickets, withTicketNumberIs, ticketByTicketNumber);
    }
    if (entity == "document") {
        return LOOKUP(cpp_inheritance_business_ids,
                      DocumentNumber,
                      documents,
                      withDocumentNumberIs,
                      documentByDocumentNumber);
    }
#undef LOOKUP
    throw std::logic_error("Unknown Business-ID lookup fixture");
}

int main(int argc, char** argv) {
    bool runtime_composition = argc == 2 && std::string_view(argv[1]) == "--runtime-composition";
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input, runtime_composition)
                                 .dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::logic_error("Lookup retained User data");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
