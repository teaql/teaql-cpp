#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace teaql;
using namespace cpp_inheritance_graph;

static const Json& field(const Json& value, std::string_view name) {
    return std::get<Json::Object>(value.data).at(String(name, value.memory_resource()));
}

static std::string_view text(const Json& value) {
    return std::get<String>(value.data);
}

static void execute(sqlite3* database, const std::string& sql) {
    if (sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
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

template <Entity T>
static Json original(Memory r, const T& entity) {
    auto values = entity.original_values();
    return values ? record_to_json_value(r, values->fields) : Json(r);
}

static Json database_view(Memory r, sqlite3* database) {
    Json::Object out(r);
    for (std::string_view table : {"platform_data",
                                   "specialist_data",
                                   "school_data",
                                   "academy_data",
                                   "research_academy_data",
                                   "enrollment_data"}) {
        auto inherited =
            table == "school_data" || table == "academy_data" || table == "research_academy_data";
        std::string sql("SELECT id,");
        sql += table == "enrollment_data" ? "note" : "name";
        sql += ",version,";
        sql += inherited ? "rank,advisor," : "0,NULL,";
        sql += table == "platform_data" ? "id" : table == "enrollment_data" ? "0" : "platform";
        sql += ",";
        sql += table == "enrollment_data" ? "school" : "NULL";
        sql += ",";
        sql += table == "specialist_data" ? "active" : "NULL";
        sql += ",";
        sql += table == "academy_data" || table == "research_academy_data" ? "campus_code" : "NULL";
        sql += ",";
        sql += table == "research_academy_data" ? "CAST(budget AS TEXT)" : "NULL";
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
            for (const auto& [name, index] : {std::pair{"id", 0},
                                              {"version", 2},
                                              {"rank", 3},
                                              {"advisor", 4},
                                              {"platform", 5},
                                              {"school", 6},
                                              {"active", 7}}) {
                row.emplace(String(name, r),
                            sqlite3_column_type(statement, index) == SQLITE_NULL
                                ? Json(r)
                                : Json(r, std::int64_t(sqlite3_column_int64(statement, index))));
            }
            for (const auto& [name, index] : {std::pair{"name", 1}, {"campus", 8}, {"budget", 9}}) {
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

template <Entity T, class Request>
static Json run(UserContext& user_context, sqlite3* database, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    auto id = std::get<std::uint64_t>(field(input, "id").data);
    request.selectAdvisorWith(Q.specialists().selectPlatform());
    const auto& object = std::get<Json::Object>(input.data);
    auto shortcut = object.find(String("shortcut", r));
    if (shortcut != object.end()) {
        auto name = text(shortcut->second);
        if (name == "self_without_parent") {
            request.selectSelfWithoutParent();
        } else if (name == "all") {
            request.selectAll();
        } else if (name == "children") {
            request.selectChildren();
        } else {
            request.selectAny();
        }
    }
    const auto& selected = require_value(std::move(request)
                                             .withIdIs(id)
                                             .purpose("Qualify inherited relation graph")
                                             .comment("Inherited optional forward relation")
                                             .executeForOne(user_context)
                                             .wait());
    if (!selected) {
        throw std::runtime_error("Inherited graph fixture row is absent");
    }
    auto& entity = user_context.make<T>(*selected);
    auto root = detail::clone_in(r, entity.root);
    auto* advisor = entity.advisor() ? &user_context.make<Specialist>(*entity.advisor()) : nullptr;
    Json::Object before(r);
    before.emplace(String("root", r), native(r, entity));
    before.emplace(String("original", r), original(r, entity));
    before.emplace(String("advisor", r), advisor ? native(r, *advisor) : Json(r));
    before.emplace(String("advisor_original", r), advisor ? original(r, *advisor) : Json(r));
    before.emplace(String("advisor_loaded", r), Json(r, entity.is_loaded("advisor")));
    auto evaluation = entity.evalAdvisor();
    before.emplace(String("evaluation", r),
                   Json(r,
                        evaluation.is_value()  ? "Value"
                        : evaluation.is_null() ? "Null"
                                               : "NotLoaded"));
    before.emplace(String("nested_platform", r),
                   advisor && advisor->platform() ? native(r, *advisor->platform()) : Json(r));
    if (shortcut != object.end()) {
        Json::Object views(r);
        views.emplace(String("platform", r),
                      entity.platform() ? native(r, *entity.platform()) : Json(r));
        Json reverse(r);
        if constexpr (requires { entity.enrollmentListHandle(); }) {
            auto view = entity.enrollmentListHandle();
            Json::Object summary(r);
            summary.emplace(String("state", r),
                            Json(r,
                                 view.state() == LoadedRelation::Loaded  ? "Loaded"
                                 : view.state() == LoadedRelation::Empty ? "Empty"
                                                                         : "NotLoaded"));
            Json members(r);
            if (view.value()) {
                Json::Array values(r);
                for (const auto& member : view.value()->data) {
                    values.emplace_back(native(r, member));
                }
                members = Json(r, values);
            }
            summary.emplace(String("members", r), std::move(members));
            reverse = Json(r, summary);
        }
        views.emplace(String("reverse", r), std::move(reverse));
        before.emplace(String("shortcut_views", r), Json(r, views));
    }
    auto operation = text(field(input, "operation"));
    if (operation == "delete") {
        entity.markForDeletion();
    }
    if (operation == "update" || operation == "related_update" || operation == "related_before") {
        entity.updateName("updated root");
    }
    if (operation == "clear") {
        entity.updateAdvisorId(std::optional<std::uint64_t>{});
    }
    if ((operation == "related_update" || operation == "related_before") && advisor) {
        if (operation == "related_before") {
            advisor->updateName("updated advisor").updateActive(false);
        }
        advisor->attachRuntimeStateRecursive(root);
        if (operation == "related_update") {
            advisor->updateName("updated advisor").updateActive(false);
        }
    }
    auto fault = text(field(input, "fault"));
    auto table = fault.starts_with("advisor") ? String("specialist_data", r)
                                              : T::entity_descriptor(r).table_name;
    auto fault_id = fault.starts_with("advisor") ? std::uint64_t{2} : id;
    if (fault.ends_with("conflict")) {
        execute(database,
                "UPDATE " + std::string(table) +
                    " SET version=version+1 WHERE id=" + std::to_string(fault_id));
    }
    if (fault.ends_with("trigger")) {
        execute(database,
                "CREATE TRIGGER fixture_failure BEFORE UPDATE ON " + std::string(table) +
                    " BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;");
    }
    auto db_before = database_view(r, database);
    Json saved(r), error(r), kind(r);
    Json::Array diagnostics(r);
    if (operation != "read") {
        auto& copy = user_context.make<T>(entity);
        const auto& result =
            AuditedSaveExt<T>::save(std::move(copy).audit_as("Save inherited relation graph"),
                                    user_context)
                .wait();
        if (result) {
            saved = native(r, result.value());
        } else if (result.error().kind() == RuntimeError::Kind::Check) {
            kind = Json(r, "Check");
            error = Json(r, "Check");
            for (const auto& value : result.error().check_results()) {
                diagnostics.emplace_back(
                    value.to_wire(r, JsonFieldNamingProfile::SnakeCase).to_json(r));
            }
        } else if (result.error().kind() == RuntimeError::Kind::OptimisticLockConflict) {
            kind = Json(r, "OptimisticLockConflict");
            error = Json(r, "OptimisticLockConflict");
        } else {
            kind = Json(r,
                        result.error().kind() == RuntimeError::Kind::Graph         ? "Graph"
                        : result.error().kind() == RuntimeError::Kind::Policy      ? "Policy"
                        : result.error().kind() == RuntimeError::Kind::Transaction ? "Transaction"
                                                                                   : "Other");
            auto message = result.error().to_string(r);
            error = Json(r,
                         message.find("fixture write failed") != String::npos
                             ? "FixtureWriteFailure"
                             : std::string_view(message));
        }
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
    out.emplace(String("before", r), Json(r, before));
    out.emplace(String("saved", r), std::move(saved));
    out.emplace(String("error", r), std::move(error));
    out.emplace(String("kind", r), std::move(kind));
    out.emplace(String("diagnostics", r), Json(r, diagnostics));
    out.emplace(String("db_before", r), std::move(db_before));
    out.emplace(String("db_after", r), database_view(r, database));
    out.emplace(String("pending", r), Json(r, pending));
    out.emplace(String("deleted", r), Json(r, root.is_marked_as_delete(entity.entity_key())));
    return Json(r, out);
}

static Json observe_reverse(UserContext& user_context, sqlite3* database, const Json& input) {
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    auto entry = text(field(input, "entry"));
    auto request = entry == "minimal"    ? Q.schoolsMinimal()
                   : entry == "children" ? Q.schoolsWithChildren()
                                         : Q.schools();
    auto selection = text(field(input, "selection"));
    if (selection == "filtered") {
        request.selectEnrollmentListWith(Q.enrollments().withIdIs(999));
    } else if (selection == "minimal") {
        request.selectEnrollmentListWith(Q.enrollmentsMinimal());
    } else if (selection == "nested") {
        request.selectEnrollmentListWith(
            Q.enrollments().selectSchoolWith(Q.schools().selectAdvisor()));
    } else if (selection != "none") {
        request.selectEnrollmentList();
    }
    auto identity = std::get<std::uint64_t>(field(input, "id").data);
    const auto& selected = require_value(std::move(request)
                                             .withIdIs(identity)
                                             .purpose("Qualify reverse relation ledger")
                                             .comment("Inherited School enrollment view")
                                             .executeForOne(user_context)
                                             .wait());
    if (!selected) {
        throw std::runtime_error("Reverse graph owner is absent");
    }
    auto& entity = user_context.make<School>(*selected);
    auto root = detail::clone_in(r, entity.root);
    auto view = entity.enrollmentListHandle();
    Json children(r);
    if (view.value()) {
        Json::Array values(r);
        for (const auto& child : view.value()->data) {
            Json::Object value(r);
            value.emplace(String("native", r), native(r, child));
            value.emplace(String("original", r), original(r, child));
            value.emplace(String("school", r),
                          child.school() ? native(r, *child.school()) : Json(r));
            values.emplace_back(value);
        }
        children = Json(r, values);
    }
    Json::Object before(r);
    before.emplace(String("root", r), native(r, entity));
    before.emplace(String("original", r), original(r, entity));
    before.emplace(String("state", r),
                   Json(r,
                        view.state() == LoadedRelation::Loaded  ? "Loaded"
                        : view.state() == LoadedRelation::Empty ? "Empty"
                                                                : "NotLoaded"));
    before.emplace(String("evaluation", r),
                   Json(r, entity.evalEnrollmentList().is_value() ? "Value" : "NotLoaded"));
    before.emplace(String("children", r), std::move(children));
    auto operation = text(field(input, "operation"));
    if (operation != "read") {
        entity.updateName("updated owner");
    }
    if (view.value()) {
        auto& copied = user_context.make<SmartList<Enrollment>>(*view.value());
        for (auto& child : copied.data) {
            if (operation == "children_before" || operation == "children_no_attach") {
                child.updateNote("updated member");
            }
            if (operation == "children_delete") {
                child.markForDeletion();
            }
            if (operation == "children_before" || operation == "children_after" ||
                operation == "children_delete") {
                child.attachRuntimeStateRecursive(root);
            }
            if (operation == "children_after") {
                child.updateNote("updated member");
            }
        }
    }
    auto fault = text(field(input, "fault"));
    std::string table = fault.starts_with("root") ? "school_data" : "enrollment_data";
    auto fault_id = fault.starts_with("root") ? identity : std::uint64_t{21};
    if (fault.ends_with("conflict")) {
        execute(database,
                "UPDATE " + table + " SET version=version+1 WHERE id=" + std::to_string(fault_id));
    }
    if (fault.ends_with("trigger")) {
        std::string condition = fault == "second_trigger" ? " WHEN OLD.id=22" : "";
        execute(database,
                "CREATE TRIGGER fixture_failure BEFORE UPDATE ON " + table + condition +
                    " BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;");
    }
    auto db_before = database_view(r, database);
    Json saved(r), error(r), kind(r);
    Json::Array diagnostics(r);
    if (operation != "read") {
        auto& copied = user_context.make<School>(entity);
        const auto& result =
            AuditedSaveExt<School>::save(std::move(copied).audit_as("Save reverse relation ledger"),
                                         user_context)
                .wait();
        if (result) {
            saved = native(r, result.value());
        } else if (result.error().kind() == RuntimeError::Kind::Check) {
            kind = Json(r, "Check");
            error = Json(r, "Check");
            for (const auto& value : result.error().check_results()) {
                diagnostics.emplace_back(
                    value.to_wire(r, JsonFieldNamingProfile::SnakeCase).to_json(r));
            }
        } else if (result.error().kind() == RuntimeError::Kind::OptimisticLockConflict) {
            kind = Json(r, "OptimisticLockConflict");
            error = Json(r, "OptimisticLockConflict");
        } else {
            kind = Json(r,
                        result.error().kind() == RuntimeError::Kind::Graph         ? "Graph"
                        : result.error().kind() == RuntimeError::Kind::Policy      ? "Policy"
                        : result.error().kind() == RuntimeError::Kind::Transaction ? "Transaction"
                                                                                   : "Other");
            auto message = result.error().to_string(r);
            error = Json(r,
                         message.find("fixture write failed") != String::npos
                             ? "FixtureWriteFailure"
                             : std::string_view(message));
        }
    }
    Json::Array pending(r), deleted(r);
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
    for (const auto& key : root.deleted_keys()) {
        String value(key.entity, r);
        value += ':';
        value += std::to_string(*key.id.try_u64());
        deleted.emplace_back(value);
    }
    Json::Object out(r);
    out.emplace(String("before", r), Json(r, before));
    out.emplace(String("saved", r), std::move(saved));
    out.emplace(String("error", r), std::move(error));
    out.emplace(String("kind", r), std::move(kind));
    out.emplace(String("diagnostics", r), Json(r, diagnostics));
    out.emplace(String("db_before", r), std::move(db_before));
    out.emplace(String("db_after", r), database_view(r, database));
    out.emplace(String("pending", r), Json(r, pending));
    out.emplace(String("deleted_keys", r), Json(r, deleted));
    return Json(r, out);
}

static Json observe(UserContext& user_context, sqlite3* database, const Json& input) {
    const auto& object = std::get<Json::Object>(input.data);
    if (auto it = object.find(String("direction", user_context.memory_resource()));
        it != object.end() && text(it->second) == "reverse") {
        execute(database,
                "INSERT INTO enrollment_data(id,school,note,version) VALUES(22,7,'second',3)");
        return observe_reverse(user_context, database, input);
    }
    auto Q = user_context.queries<query_factory>();
    auto entry = text(field(input, "entry"));
    if (text(field(input, "model")) == "school") {
        return run<School>(user_context,
                           database,
                           entry == "minimal"    ? Q.schoolsMinimal()
                           : entry == "children" ? Q.schoolsWithChildren()
                                                 : Q.schools(),
                           input);
    }
    if (text(field(input, "model")) == "academy") {
        return run<Academy>(user_context,
                            database,
                            entry == "minimal"    ? Q.academiesMinimal()
                            : entry == "children" ? Q.academiesWithChildren()
                                                  : Q.academies(),
                            input);
    }
    return run<ResearchAcademy>(user_context,
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
                "PRAGMA foreign_keys=OFF;"
                "UPDATE platform_data SET name='root',version=3 WHERE id=1;"
                "INSERT INTO specialist_data(id,name,platform,active,version) VALUES(2,'advisor',1,1,3);"
                "INSERT INTO school_data(id,name,rank,advisor,platform,version) VALUES(7,'school',1,2,1,3),(8,'nullable',2,NULL,1,3),(9,'missing',3,99,1,3);"
                "INSERT INTO academy_data(id,name,rank,advisor,platform,version,campus_code) VALUES(7,'academy',7,2,1,4,'north'),(8,'nullable',8,NULL,1,4,'south'),(9,'missing',9,99,1,4,'east');"
                "INSERT INTO research_academy_data(id,name,rank,advisor,platform,version,campus_code,budget) VALUES(7,'research',17,2,1,5,'lab','12.50'),(8,'nullable',18,NULL,1,5,'lab','10.50'),(9,'missing',19,99,1,5,'lab','8.50');"
                "INSERT INTO enrollment_data(id,school,note,version) VALUES(21,7,'student',3);"
                "PRAGMA foreign_keys=ON;");
            if (require_value(user_context->require_active_root("Platform")).get().id != 1) {
                throw std::runtime_error(
                    "Generated initialization did not prepare the fixture root");
            }
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid inherited graph fixture");
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
