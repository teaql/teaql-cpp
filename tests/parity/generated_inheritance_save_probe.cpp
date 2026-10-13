#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>

using namespace cpp_inheritance;

static void execute(sqlite3* database, std::string_view sql) {
    std::string text(sql);
    if (sqlite3_exec(database, text.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json database_view(Memory r, sqlite3* database) {
    Json::Object out(r);
    for (const auto* table :
         {"platform_data", "school_data", "academy_data", "research_academy_data"}) {
        std::string sql = std::string_view(table) == "platform_data"
                              ? "SELECT id,name,0,id,version,"
                              : "SELECT id,name,rank,platform,version,";
        sql +=
            std::string_view(table) == "school_data" || std::string_view(table) == "platform_data"
                ? "NULL"
                : "campus_code";
        sql += ",";
        sql += std::string_view(table) == "research_academy_data" ? "CAST(budget AS TEXT)" : "NULL";
        sql += " FROM ";
        sql += table;
        sql += " ORDER BY id";
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> cleanup(statement,
                                                                           sqlite3_finalize);
        Json::Array rows(r);
        for (auto status = sqlite3_step(statement); status != SQLITE_DONE;
             status = sqlite3_step(statement)) {
            if (status != SQLITE_ROW) {
                throw std::runtime_error(sqlite3_errmsg(database));
            }
            Json::Object row(r);
            row.emplace(String("id", r), Json(r, std::int64_t(sqlite3_column_int64(statement, 0))));
            row.emplace(String("name", r),
                        Json(r, reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))));
            row.emplace(String("rank", r),
                        Json(r, std::int64_t(sqlite3_column_int64(statement, 2))));
            row.emplace(String("platform", r),
                        Json(r, std::int64_t(sqlite3_column_int64(statement, 3))));
            row.emplace(String("version", r),
                        Json(r, std::int64_t(sqlite3_column_int64(statement, 4))));
            for (const auto& [name, index] : {std::pair{"campus", 5}, std::pair{"budget", 6}}) {
                row.emplace(String(name, r),
                            sqlite3_column_type(statement, index) == SQLITE_NULL
                                ? Json(r)
                                : Json(r,
                                       reinterpret_cast<const char*>(
                                           sqlite3_column_text(statement, index))));
            }
            rows.emplace_back(row);
        }
        out.emplace(String(table, r), Json(r, rows));
    }
    return Json(r, out);
}

template <Entity T>
static Json native(Memory r, const T& entity) {
    auto values = std::move(detail::clone_in(r, entity)).into_values();
    Json::Object out(r);
    for (const auto& [name, value] : values.fields) {
        if (!name.starts_with('_')) {
            out.emplace(String(name, r), value.to_json_value(r));
        }
    }
    return Json(r, out);
}

template <Entity T, class Request>
static Json save(UserContext& user_context, sqlite3* database, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    auto id = std::get<std::uint64_t>(field(input, "id").data);
    auto related = optional(input, "related") && std::get<bool>(field(input, "related").data);
    if (related) {
        request.selectPlatform();
    }
    const auto& selected = require_value(std::move(request)
                                             .template returnType<T>()
                                             .withIdIs(id)
                                             .comment("Inherited save source view")
                                             .purpose("Qualify inherited graph save")
                                             .executeForOne(user_context)
                                             .wait());
    if (!selected) {
        throw std::runtime_error("Inherited save fixture row is absent");
    }
    auto original = selected->original_values();
    Json::Object related_view(r);
    related_view.emplace(String("loaded", r), Json(r, selected->is_loaded("platform")));
    related_view.emplace(String("present", r), Json(r, selected->platform() != nullptr));
    auto& entity = user_context.make<T>(*selected);
    auto deleting = optional(input, "operation") && text(field(input, "operation")) == "delete";
    if (deleting) {
        entity.markForDeletion();
    } else {
        entity.updateName(text(field(input, "name"))).updateRank(23);
    }
    if (related) {
        auto* selected_platform = entity.platform();
        if (selected_platform) {
            auto& platform = user_context.make<Platform>(*selected_platform);
            platform.attachRuntimeStateRecursive(entity.root);
            platform.updateName("updated root");
        }
    }
    if (const auto* own = optional(input, "own_fields");
        !deleting && own && std::get<bool>(own->data)) {
        if constexpr (requires { entity.updateCampusCode("updated campus"); }) {
            entity.updateCampusCode("updated campus");
        }
        if constexpr (requires { entity.updateBudget(*Decimal::parse("-7.50")); }) {
            entity.updateBudget(*Decimal::parse("-7.50"));
        }
    }
    auto before = native(r, entity);
    auto table = T::entity_descriptor(r).table_name;
    std::string sql;
    if (text(field(input, "fault")) == "conflict") {
        sql = "UPDATE ";
        sql += table;
        sql += " SET version=version+1 WHERE id=";
        sql += std::to_string(id);
        execute(database, sql);
    }
    if (text(field(input, "fault")) == "trigger") {
        sql = "CREATE TRIGGER fixture_failure BEFORE UPDATE ON ";
        sql += table;
        sql += " BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;";
        execute(database, sql);
    }
    auto db_before = database_view(r, database);
    auto deleted_before = entity.is_marked_as_delete();
    auto& copied = user_context.make<T>(entity);
    const auto& result =
        AuditedSaveExt<T>::save(std::move(copied).audit_as("Save inherited returned model"),
                                user_context)
            .wait();
    Json error(r), kind(r), saved(r);
    Json::Array diagnostics(r);
    if (result) {
        saved = native(r, result.value());
    } else {
        switch (result.error().kind()) {
        case RuntimeError::Kind::Check:
            kind = Json(r, "Check");
            error = Json(r, "Check");
            for (const auto& value : result.error().check_results()) {
                diagnostics.emplace_back(
                    value.to_wire(r, JsonFieldNamingProfile::SnakeCase).to_json(r));
            }
            break;
        case RuntimeError::Kind::OptimisticLockConflict:
            kind = Json(r, "OptimisticLockConflict");
            error = Json(r, "OptimisticLockConflict");
            break;
        default:
            kind = Json(r,
                        result.error().kind() == RuntimeError::Kind::Graph         ? "Graph"
                        : result.error().kind() == RuntimeError::Kind::Policy      ? "Policy"
                        : result.error().kind() == RuntimeError::Kind::Transaction ? "Transaction"
                                                                                   : "Other");
            auto message = result.error().to_string(r);
            error = Json(r,
                         message.find("fixture write failed") != String::npos
                             ? std::string_view("FixtureWriteFailure")
                             : std::string_view(message));
        }
    }
    Json::Array pending(r);
    FieldSet names(r);
    auto changes = entity.root.current_change_set();
    for (const auto& [key, fields] : changes.changes()) {
        for (const auto& [name, value] : fields) {
            names.emplace(name);
        }
    }
    for (const auto& name : names) {
        pending.emplace_back(name);
    }
    Json::Object out(r);
    out.emplace(String("type", r), Json(r, T::ENTITY_NAME));
    out.emplace(String("original", r),
                original ? record_to_json_value(r, original->fields) : Json(r));
    out.emplace(String("before", r), std::move(before));
    out.emplace(String("related_view", r), Json(r, related_view));
    out.emplace(String("saved", r), std::move(saved));
    out.emplace(String("error", r), std::move(error));
    out.emplace(String("kind", r), std::move(kind));
    out.emplace(String("diagnostics", r), Json(r, diagnostics));
    out.emplace(String("db_before", r), std::move(db_before));
    out.emplace(String("db_after", r), database_view(r, database));
    out.emplace(String("pending", r), Json(r, pending));
    if (deleting) {
        out.emplace(String("deleted_before", r), Json(r, deleted_before));
        out.emplace(String("deleted_after", r), Json(r, entity.is_marked_as_delete()));
    }
    return Json(r, out);
}

template <class Request>
static Json
target(UserContext& user_context, sqlite3* database, Request request, const Json& input) {
    if (text(field(input, "target")) == "school") {
        return save<School>(user_context, database, std::move(request), input);
    }
    if (text(field(input, "target")) == "academy") {
        return save<Academy>(user_context, database, std::move(request), input);
    }
    return save<ResearchAcademy>(user_context, database, std::move(request), input);
}

template <Entity T, class Request>
static Json create(UserContext& user_context,
                   sqlite3* database,
                   Request request,
                   School* secondary,
                   const Json& input) {
    auto r = user_context.memory_resource();
    auto& entity = std::move(request)
                       .purpose("Create inherited model")
                       .comment("Inherited graph creation")
                       .newEntity(user_context);
    entity.updateName(text(field(input, "name"))).updateRank(23);
    if constexpr (requires { entity.updateCampusCode("updated campus"); }) {
        entity.updateCampusCode("updated campus");
    }
    if constexpr (requires { entity.updateBudget(*Decimal::parse("-7.50")); }) {
        entity.updateBudget(*Decimal::parse("-7.50"));
    }
    auto root = detail::clone_in(r, entity.root);
    if (secondary) {
        secondary->attachRuntimeStateRecursive(root);
    }
    auto before = native(r, entity);
    auto fault = text(field(input, "fault"));
    if (fault == "root_trigger" || fault == "related_trigger") {
        std::string sql("CREATE TRIGGER fixture_failure BEFORE ");
        sql += fault == "related_trigger" && text(field(input, "secondary")) == "existing"
                   ? "UPDATE ON "
                   : "INSERT ON ";
        sql += fault == "related_trigger" ? "school_data" : T::entity_descriptor(r).table_name;
        sql += " BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;";
        execute(database, sql);
    }
    auto db_before = database_view(r, database);
    const auto& result =
        AuditedSaveExt<T>::save(std::move(entity).audit_as("Create inherited graph"), user_context)
            .wait();
    Json error(r), kind(r), saved(r);
    Json::Array diagnostics(r);
    if (result) {
        saved = native(r, result.value());
    } else if (result.error().kind() == RuntimeError::Kind::Check) {
        kind = Json(r, "Check");
        error = Json(r, "Check");
        for (const auto& value : result.error().check_results()) {
            diagnostics.emplace_back(
                value.to_wire(r, JsonFieldNamingProfile::SnakeCase).to_json(r));
        }
    } else {
        kind = Json(r, result.error().kind() == RuntimeError::Kind::Graph ? "Graph" : "Other");
        auto message = result.error().to_string(r);
        error =
            Json(r,
                 message.find("fixture write failed") != String::npos ? "FixtureWriteFailure"
                                                                      : std::string_view(message));
    }
    Json::Array pending(r);
    std::pmr::set<String> names(r);
    auto changes = root.current_change_set();
    for (const auto& [key, values] : changes.changes()) {
        for (const auto& [name, value] : values) {
            names.emplace(name);
        }
    }
    for (const auto& name : names) {
        pending.emplace_back(name);
    }
    Json::Object out(r);
    out.emplace(String("type", r), Json(r, T::ENTITY_NAME));
    out.emplace(String("before", r), std::move(before));
    out.emplace(String("saved", r), std::move(saved));
    out.emplace(String("error", r), std::move(error));
    out.emplace(String("kind", r), std::move(kind));
    out.emplace(String("diagnostics", r), Json(r, diagnostics));
    out.emplace(String("db_before", r), std::move(db_before));
    out.emplace(String("db_after", r), database_view(r, database));
    out.emplace(String("pending", r), Json(r, pending));
    return Json(r, out);
}

static Json observe(UserContext& user_context, sqlite3* database, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    if (const auto* operation = optional(input, "operation");
        operation && text(*operation) == "create") {
        auto& generator = user_context.make_resource<AtomicCounterIdGenerator>(1000);
        user_context.set_internal_id_generator(user_context, generator);
        School* secondary = nullptr;
        if (text(field(input, "secondary")) == "existing") {
            const auto& existing = require_value(Q.schools()
                                                     .withIdIs(9)
                                                     .purpose("Read associated row")
                                                     .comment("Inherited graph creation")
                                                     .executeForOne(user_context)
                                                     .wait());
            secondary = &user_context.make<School>(*existing);
            secondary->updateName("updated sibling").updateRank(11);
        } else if (text(field(input, "secondary")) == "new") {
            secondary = &Q.schools()
                             .purpose("Create associated row")
                             .comment("Inherited graph creation")
                             .newEntity(user_context);
            secondary->updateName("new sibling").updateRank(11);
        }
        if (text(field(input, "target")) == "school") {
            return create<School>(user_context, database, Q.schools(), secondary, input);
        }
        if (text(field(input, "target")) == "academy") {
            return create<Academy>(user_context, database, Q.academies(), secondary, input);
        }
        return create<ResearchAcademy>(
            user_context, database, Q.researchAcademies(), secondary, input);
    }
    auto entry = text(field(input, "entry"));
    if (text(field(input, "source")) == "school") {
        return target(user_context,
                      database,
                      entry == "minimal"    ? Q.schoolsMinimal()
                      : entry == "children" ? Q.schoolsWithChildren()
                                            : Q.schools(),
                      input);
    }
    if (text(field(input, "source")) == "academy") {
        return target(user_context,
                      database,
                      entry == "minimal"    ? Q.academiesMinimal()
                      : entry == "children" ? Q.academiesWithChildren()
                                            : Q.academies(),
                      input);
    }
    return target(user_context,
                  database,
                  entry == "minimal"    ? Q.researchAcademiesMinimal()
                  : entry == "children" ? Q.researchAcademiesWithChildren()
                                        : Q.researchAcademies(),
                  input);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto executor = setup(*user_context);
            auto* database = user_context->get(executor).transport.connection().native_handle();
            execute(
                database,
                "UPDATE platform_data SET name='root',version=3 WHERE id=1;"
                "INSERT INTO school_data(id,name,rank,platform,version) VALUES(7,'school',1,1,3),(9,'plain',3,1,3);"
                "INSERT INTO academy_data(id,name,rank,platform,version,campus_code) VALUES(7,'academy',7,1,4,'north');"
                "INSERT INTO research_academy_data(id,name,rank,platform,version,campus_code,budget) VALUES(7,'research',17,1,5,'lab','12.50');");
            if (require_value(user_context->require_active_root("Platform")).get().id != 1) {
                throw std::runtime_error(
                    "Generated initialization did not prepare the fixture root");
            }
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid inherited save fixture");
            }
            std::cout
                << observe(*user_context, database, *input).dump(user_context->memory_resource())
                << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
