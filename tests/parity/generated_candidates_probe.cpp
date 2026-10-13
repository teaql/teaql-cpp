#include "../generated_candidates/include/cpp_candidates.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
#include <sqlite3.h>

using namespace cpp_candidates;

static void put(Json::Object& out, Memory r, std::string_view key, Json value) {
    out.emplace(String(key, r), std::move(value));
}

static Json expression(Memory r, const RuntimeExpression<bool>& value) {
    const auto& result = value.result();
    Json::Object out(r);
    if (result.is_value()) {
        put(out, r, "state", Json(r, "Value"));
        put(out, r, "value", Json(r, result.get()));
    } else if (result.is_null()) {
        put(out, r, "state", Json(r, "Null"));
    } else {
        put(out, r, "state", Json(r, "NotLoaded"));
        put(out, r, "node", Json(r, result.missing().failed_node));
        put(out, r, "path", Json(r, result.missing().attempted_path));
    }
    return Json(r, out);
}

template <class T>
static auto entity_expression_for(const T& entity) {
    if constexpr (std::same_as<T, Person>) {
        return E::person(entity);
    } else {
        return E::record(entity);
    }
}

template <class T>
static Json view(Memory r, const T& entity) {
    auto E = entity_expression_for(entity);
    Json::Array flags(r), expressions(r);
    for (bool value :
         {entity.statusIsDraft(), entity.statusIsInProgress(), entity.statusIsDone()}) {
        flags.emplace_back(value);
    }
    for (const auto& value : {E.statusIsDraft(), E.statusIsInProgress(), E.statusIsDone()}) {
        expressions.emplace_back(expression(r, value));
    }
    Json::Object out(r);
    put(out, r, "id", Json(r, entity.statusId()));
    put(out, r, "loaded", Json(r, entity.is_loaded("status_id")));
    put(out, r, "flags", Json(r, flags));
    put(out, r, "expressions", Json(r, expressions));
    auto changed = entity.changedStatusId();
    put(out, r, "changed", changed ? changed->to_json_value(r) : Json(r));
    return Json(r, out);
}

template <class T>
static Json model(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    teaql::Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t{7}));
    row.emplace(String("version", r), Value(r, std::int64_t{3}));
    row.emplace(String("name", r), Value(r, "original"));
    row.emplace(String("platform_id", r), Value(r, std::uint64_t{1}));
    row.emplace(String("phase", r), Value(r, "Draft"));
    if (std::get<bool>(field(input, "loaded").data)) {
        row.emplace(String("status_id", r),
                    Value(r, std::get<std::uint64_t>(field(input, "id").data)));
    }
    auto decoded = T::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
    auto& entity = user_context.make<T>(require_value(decoded));
    entity.attachRuntimeStateRecursive(user_context.entity_runtime_state());
    auto& copy = user_context.make<T>(entity);
    auto before = view(r, entity);
    for (int index = 0; index < (std::get<bool>(field(input, "repeat").data) ? 2 : 1); ++index) {
        auto candidate = text(field(input, "candidate"));
        if (candidate == "draft") {
            entity.updateStatusToDraft();
        } else if (candidate == "in_progress") {
            entity.updateStatusToInProgress();
        } else if (candidate == "done") {
            entity.updateStatusToDone();
        }
    }
    Json::Object out(r);
    put(out, r, "before", std::move(before));
    put(out, r, "after", view(r, entity));
    put(out, r, "copy", view(r, copy));
    return Json(r, out);
}

template <class Request>
static void filter(Request& request, const Json& input) {
    auto name = text(field(input, "field"));
    auto candidate = text(field(input, "candidate"));
    bool negative = std::get<bool>(field(input, "negative").data);
    if constexpr (requires { request.whoseStatusIsDraft(); }) {
        if (name == "status") {
            if (candidate == "draft") {
                negative ? request.whoseStatusIsNotDraft() : request.whoseStatusIsDraft();
            } else if (candidate == "in_progress") {
                negative ? request.whoseStatusIsNotInProgress() : request.whoseStatusIsInProgress();
            } else {
                negative ? request.whoseStatusIsNotDone() : request.whoseStatusIsDone();
            }
        } else {
            if (candidate == "draft") {
                negative ? request.whosePhaseIsNotDraft() : request.whosePhaseIsDraft();
            } else if (candidate == "in_progress") {
                negative ? request.whosePhaseIsNotInProgress() : request.whosePhaseIsInProgress();
            } else {
                negative ? request.whosePhaseIsNotDone() : request.whosePhaseIsDone();
            }
        }
    } else {
        if (name == "status") {
            if (candidate == "draft") {
                negative ? request.withStatusIsNotDraft() : request.withStatusIsDraft();
            } else if (candidate == "in_progress") {
                negative ? request.withStatusIsNotInProgress() : request.withStatusIsInProgress();
            } else {
                negative ? request.withStatusIsNotDone() : request.withStatusIsDone();
            }
        } else if (name == "tag") {
            if (candidate == "red") {
                negative ? request.withTagIsNotRed() : request.withTagIsRed();
            } else if (candidate == "blue") {
                negative ? request.withTagIsNotBlue() : request.withTagIsBlue();
            } else {
                negative ? request.withTagIsNotNull() : request.withTagIsNull();
            }
        } else if (name == "tag_nullability") {
            candidate == "unknown" ? request.withTagIsUnknown() : request.withTagIsKnown();
        } else {
            if (candidate == "draft") {
                negative ? request.withPhaseIsNotDraft() : request.withPhaseIsDraft();
            } else if (candidate == "in_progress") {
                negative ? request.withPhaseIsNotInProgress() : request.withPhaseIsInProgress();
            } else {
                negative ? request.withPhaseIsNotDone() : request.withPhaseIsDone();
            }
        }
    }
}

static void sql(sqlite3* database, const char* statement) {
    if (sqlite3_exec(database, statement, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json database_view(Memory r, sqlite3* database, std::string_view table) {
    auto statement_text = "SELECT id,status,version FROM " + std::string(table) + " ORDER BY id";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database, statement_text.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array rows(r);
    int status = SQLITE_OK;
    while ((status = sqlite3_step(statement.get())) == SQLITE_ROW) {
        Json::Array row(r);
        for (int index = 0; index < 3; ++index) {
            row.emplace_back(
                static_cast<std::int64_t>(sqlite3_column_int64(statement.get(), index)));
        }
        rows.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, rows);
}

template <class T, class Request>
static Json save(UserContext& user_context, sqlite3* database, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    if constexpr (std::same_as<T, Person>) {
        request.whoseIdIs(7);
    } else {
        request.withIdIs(7);
    }
    const auto& found = require_value(std::move(request)
                                          .comment("Read candidate mutation target")
                                          .purpose("Prepare candidate update")
                                          .executeForOne(user_context)
                                          .wait());
    if (!found) {
        throw std::runtime_error("Candidate save target is absent");
    }
    auto& entity = user_context.make<T>(*found);
    auto root = detail::clone_in(r, entity.root);
    auto key = entity.entity_key();
    auto candidate = text(field(input, "candidate"));
    if (candidate == "draft") {
        entity.updateStatusToDraft();
    } else if (candidate == "in_progress") {
        entity.updateStatusToInProgress();
    } else {
        entity.updateStatusToDone();
    }
    auto table = T::entity_descriptor(r).table_name;
    auto fault = text(field(input, "fault"));
    if (fault == "conflict") {
        auto statement = "UPDATE " + std::string(table) + " SET version=version+1 WHERE id=7";
        sql(database, statement.c_str());
    } else if (fault == "trigger") {
        auto statement = "CREATE TRIGGER fixture_failure BEFORE UPDATE ON " + std::string(table) +
                         " BEGIN SELECT RAISE(ABORT,'candidate write failed'); END;";
        sql(database, statement.c_str());
    }
    auto before = database_view(r, database, table);
    const auto& outcome = AuditedSaveExt<T>::save(
                              std::move(entity).audit_as("Set known model candidate"), user_context)
                              .wait();
    Json saved(r), error(r);
    Json::Array diagnostics(r);
    if (outcome) {
        auto values = std::move(detail::clone_in(r, outcome.value())).into_values();
        Json::Object native(r);
        for (const auto& [name, value] : values.fields) {
            if (!name.starts_with('_')) {
                put(native, r, name, value.to_json_value(r));
            }
        }
        saved = Json(r, native);
    } else {
        error = Json(r, outcome.error().to_string(r));
        if (outcome.error().kind() == RuntimeError::Kind::Check) {
            for (const auto& diagnostic : outcome.error().check_results()) {
                diagnostics.emplace_back(
                    diagnostic.to_wire(r, JsonFieldNamingProfile::SnakeCase).to_json(r));
            }
        }
    }
    Json::Object out(r);
    put(out, r, "saved", std::move(saved));
    put(out, r, "error", std::move(error));
    put(out, r, "diagnostics", Json(r, diagnostics));
    put(out, r, "before", std::move(before));
    put(out, r, "after", database_view(r, database, table));
    put(out, r, "pending", Json(r, bool(root.get(key, "status_id"))));
    return Json(r, out);
}

template <class Request>
static Json query(UserContext& user_context, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    auto sqlite = text(field(input, "op")) == "sqlite";
    for (int index = 0; index < (!sqlite && std::get<bool>(field(input, "repeat").data) ? 2 : 1);
         ++index) {
        filter(request, input);
    }
    Json::Object out(r);
    if (!sqlite) {
        const auto& selection = request.query();
        Json::Array projection(r);
        for (const auto& name : selection.projection) {
            projection.emplace_back(name);
        }
        put(out, r, "projection", Json(r, projection));
        put(out,
            r,
            "filter",
            selection.predicate ? Json(r, debug_string(r, *selection.predicate)) : Json(r));
    } else {
        const auto& result = std::move(request)
                                 .orderByIdAsc()
                                 .comment("Candidate equality and inequality")
                                 .purpose("Read candidate rows")
                                 .executeForList(user_context)
                                 .wait();
        Json::Array ids(r);
        Json error(r);
        if (result) {
            for (const auto& entity : result.value().data) {
                ids.emplace_back(entity.id());
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
        put(out, r, "ids", Json(r, ids));
        put(out, r, "error", std::move(error));
    }
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    bool person = text(field(input, "model")) == "person";
    auto op = text(field(input, "op"));
    if (op == "model") {
        return person ? model<Person>(user_context, input)
                      : model<cpp_candidates::Record>(user_context, input);
    }
    sqlite3* database = nullptr;
    if (op == "sqlite" || op == "save") {
        auto executor = setup(user_context);
        database = user_context.get(executor).transport.connection().native_handle();
        sql(database,
            "INSERT INTO order_status_data(id,name,code,platform,version) VALUES(999,'Unknown fixture','OTHER',1,1);"
            "INSERT INTO record_data(id,name,phase,tag,platform,status,version) VALUES(7,'draft','Draft','red',1,11,3),(8,'next','InProgress','blue',1,22,4),(9,'done','done',NULL,1,2104194,5),(10,'unknown','other',NULL,1,999,6),(11,'literal','other','null',1,11,7);"
            "INSERT INTO person_data(id,name,phase,platform,status,version) VALUES(7,'draft','Draft',1,11,3),(8,'next','InProgress',1,22,4),(9,'done','done',1,2104194,5),(10,'unknown','other',1,999,6);");
    }
    auto Q = user_context.queries<query_factory>();
    bool minimal = text(field(input, "entry")) == "minimal";
    if (op == "save") {
        if (person) {
            return save<Person>(
                user_context, database, minimal ? Q.peopleMinimal() : Q.people(), input);
        }
        return save<cpp_candidates::Record>(
            user_context, database, minimal ? Q.recordsMinimal() : Q.records(), input);
    }
    if (person) {
        return query(user_context, minimal ? Q.peopleMinimal() : Q.people(), input);
    }
    return query(user_context, minimal ? Q.recordsMinimal() : Q.records(), input);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid candidate input");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
