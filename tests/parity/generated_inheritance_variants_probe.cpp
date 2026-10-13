#include "../generated_inheritance_variants/include/cpp_inheritance_variants.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
#include <sqlite3.h>

using namespace cpp_inheritance_variants;

static void sql(sqlite3* database, const std::string& command) {
    if (sqlite3_exec(database, command.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json describe(Memory r, const SelectQuery& query) {
    Json::Object out(r);
    Json::Array projection(r), children(r);
    for (const auto& name : query.projection) {
        projection.emplace_back(name);
    }
    for (const auto& child : query.child_enhancements) {
        children.emplace_back(describe(r, child));
    }
    out.emplace(String("entity", r), Json(r, query.entity));
    out.emplace(String("projection", r), Json(r, projection));
    out.emplace(String("filter", r),
                query.predicate ? Json(r, debug_string(r, *query.predicate)) : Json(r));
    out.emplace(String("slice", r), query.slice ? Json(r, debug_string(r, *query.slice)) : Json(r));
    out.emplace(String("hard_limit", r), Json(r, query.hard_limit_value));
    out.emplace(String("children", r), Json(r, children));
    return Json(r, out);
}

template <Entity T>
static Json native(Memory r, const T& entity) {
    auto values = std::move(detail::clone_in(r, entity)).into_values();
    Json::Object out(r);
    for (const auto& [name, value] : values.fields) {
        if (name.starts_with('_')) {
            continue;
        }
        out.emplace(String(name, r),
                    name == "created_at"   ? Json(r, value.try_timestamp()->millis != 111)
                    : name == "updated_at" ? Json(r, value.try_timestamp()->millis != 222)
                                           : value.to_json_value(r));
    }
    return Json(r, out);
}

static Json database_view(Memory r, sqlite3* database) {
    Json::Object out(r);
    for (std::string_view table : {"document_data",
                                   "intermediate_record_data",
                                   "invoice_data",
                                   "receipt_data",
                                   "special_invoice_data"}) {
        std::string command("SELECT id,name,active,note,tenant,version,created_at,updated_at,");
        command += table == "invoice_data" || table == "special_invoice_data"
                       ? "CAST(amount AS TEXT)"
                       : "NULL";
        command += ",";
        command += table == "receipt_data" ? "status" : "NULL";
        command += ",";
        command += table == "special_invoice_data" ? "reference" : "NULL";
        command += " FROM ";
        command += table;
        command += " ORDER BY id";
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database, command.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
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
            for (const auto& [name, index] : {std::pair{"id", 0}, {"tenant", 4}, {"version", 5}}) {
                row.emplace(String(name, r),
                            Json(r, std::int64_t(sqlite3_column_int64(statement, index))));
            }
            row.emplace(String("active", r), Json(r, sqlite3_column_int(statement, 2) != 0));
            row.emplace(String("created_changed", r),
                        Json(r, sqlite3_column_int64(statement, 6) != 111));
            row.emplace(String("updated_changed", r),
                        Json(r, sqlite3_column_int64(statement, 7) != 222));
            for (const auto& [name, index] : {std::pair{"name", 1},
                                              {"note", 3},
                                              {"amount", 8},
                                              {"status", 9},
                                              {"reference", 10}}) {
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

template <class Model, class Request>
static Json run(UserContext& user_context, sqlite3* database, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    if (std::get<bool>(field(input, "automatic").data)) {
        request.enhanceChildrenIfNeeded();
    }
    if (std::get<bool>(field(input, "manual").data)) {
        request.enhanceChild(Q.receiptsMinimal().selectStatus());
    }
    if (std::get<bool>(field(input, "subtype").data)) {
        const std::array types{Value(r, "Invoice"), Value(r, "SpecialInvoice")};
        request.matchTypes(types);
    }
    if (text(field(input, "mode")) == "construct") {
        return describe(r, request.query());
    }
    const auto& result = request.orderByIdAsc()
                             .purpose("Qualify branched inheritance")
                             .comment("Empty intermediate and inherited fields")
                             .executeForList(user_context)
                             .wait();
    if (!result) {
        Json::Object error(r);
        error.emplace(String("error", r), Json(r, result.error().to_string(r)));
        return Json(r, error);
    }
    const auto& entities = result.value();
    if (text(field(input, "mode")) == "read") {
        Json::Array rows(r);
        for (const auto& entity : entities) {
            rows.emplace_back(native(r, entity));
        }
        Json::Object out(r);
        out.emplace(String("rows", r), Json(r, rows));
        return Json(r, out);
    }
    const Model* selected = nullptr;
    for (const auto& entity : entities) {
        if (entity.id_value().try_u64() == 7) {
            selected = &entity;
        }
    }
    if (!selected) {
        throw std::runtime_error("Inherited variant save row is absent");
    }
    auto& entity = user_context.make<Model>(*selected);
    auto root = detail::clone_in(r, entity.root);
    entity.updateName(text(field(input, "name")))
        .updateActive(false)
        .updateNote(std::optional<String>{});
    auto table = Model::entity_descriptor(r).table_name;
    auto fault = text(field(input, "fault"));
    if (fault == "conflict") {
        sql(database, "UPDATE " + std::string(table) + " SET version=version+1 WHERE id=7");
    }
    if (fault == "trigger") {
        sql(database,
            "CREATE TRIGGER fixture_failure BEFORE UPDATE ON " + std::string(table) +
                " BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;");
    }
    auto before = database_view(r, database);
    const auto& outcome =
        AuditedSaveExt<Model>::save(
            std::move(entity).audit_as("Save inherited optional clock fields"), user_context)
            .wait();
    Json saved(r), error(r), kind(r);
    Json::Array diagnostics(r), pending(r), fixes(r);
    if (outcome) {
        saved = native(r, outcome.value());
    } else if (outcome.error().kind() == RuntimeError::Kind::Check) {
        kind = Json(r, "Check");
        error = Json(r, "Check");
        for (const auto& value : outcome.error().check_results()) {
            diagnostics.emplace_back(
                value.to_wire(r, JsonFieldNamingProfile::SnakeCase).to_json(r));
        }
    } else {
        kind = Json(r,
                    outcome.error().kind() == RuntimeError::Kind::OptimisticLockConflict
                        ? "OptimisticLockConflict"
                    : outcome.error().kind() == RuntimeError::Kind::Graph ? "Graph"
                                                                          : "Other");
        auto message = outcome.error().to_string(r);
        error = Json(r,
                     outcome.error().kind() == RuntimeError::Kind::OptimisticLockConflict
                         ? "OptimisticLockConflict"
                     : message.find("fixture write failed") != String::npos
                         ? "FixtureWriteFailure"
                         : std::string_view(message));
    }
    std::pmr::set<String> names(r);
    auto changes = root.current_change_set();
    for (const auto& [key, fields] : changes.changes()) {
        for (const auto& [name, value] : fields) {
            names.emplace(name);
        }
    }
    for (const auto& name : names) {
        pending.emplace_back(name);
    }
    for (const auto& value : user_context.last_fix_evidence()) {
        fixes.emplace_back(value.model_path);
    }
    Json::Object out(r);
    out.emplace(String("saved", r), std::move(saved));
    out.emplace(String("kind", r), std::move(kind));
    out.emplace(String("error", r), std::move(error));
    out.emplace(String("diagnostics", r), Json(r, diagnostics));
    out.emplace(String("before", r), std::move(before));
    out.emplace(String("after", r), database_view(r, database));
    out.emplace(String("pending", r), Json(r, pending));
    out.emplace(String("fix_fields", r), Json(r, fixes));
    return Json(r, out);
}

static Json observe(UserContext& user_context, sqlite3* database, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto entry = text(field(input, "entry"));
    auto model = text(field(input, "model"));
    if (model == "document") {
        return run<Document>(user_context,
                             database,
                             entry == "minimal"    ? Q.documentsMinimal()
                             : entry == "children" ? Q.documentsWithChildren()
                                                   : Q.documents(),
                             input);
    }
    if (model == "intermediate_record") {
        return run<IntermediateRecord>(user_context,
                                       database,
                                       entry == "minimal"    ? Q.intermediateRecordsMinimal()
                                       : entry == "children" ? Q.intermediateRecordsWithChildren()
                                                             : Q.intermediateRecords(),
                                       input);
    }
    if (model == "invoice") {
        return run<Invoice>(user_context,
                            database,
                            entry == "minimal"    ? Q.invoicesMinimal()
                            : entry == "children" ? Q.invoicesWithChildren()
                                                  : Q.invoices(),
                            input);
    }
    if (model == "receipt") {
        return run<Receipt>(user_context,
                            database,
                            entry == "minimal"    ? Q.receiptsMinimal()
                            : entry == "children" ? Q.receiptsWithChildren()
                                                  : Q.receipts(),
                            input);
    }
    return run<SpecialInvoice>(user_context,
                               database,
                               entry == "minimal"    ? Q.specialInvoicesMinimal()
                               : entry == "children" ? Q.specialInvoicesWithChildren()
                                                     : Q.specialInvoices(),
                               input);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto executor = setup(*user_context);
            auto* database = user_context->get(executor).transport.connection().native_handle();
            sql(database,
                "UPDATE tenant_data SET name='root',version=3 WHERE id=1;"
                "INSERT INTO document_data(id,name,active,note,tenant,created_at,updated_at,version) VALUES(7,'document',1,NULL,1,111,222,3),(8,'deleted document',0,'old',1,111,222,-2),(9,'plain',0,NULL,1,111,222,3);"
                "INSERT INTO intermediate_record_data(id,name,active,note,tenant,created_at,updated_at,version) VALUES(7,'intermediate',0,'mid',1,111,222,4);"
                "INSERT INTO invoice_data(id,name,active,note,tenant,created_at,updated_at,version,amount) VALUES(7,'invoice',1,NULL,1,111,222,5,'12.50'),(10,'orphan',1,NULL,1,111,222,5,'2.50');"
                "INSERT INTO receipt_data(id,name,active,note,tenant,created_at,updated_at,version,status) VALUES(7,'receipt',0,'branch',1,111,222,6,'open');"
                "INSERT INTO special_invoice_data(id,name,active,note,tenant,created_at,updated_at,version,amount,reference) VALUES(7,'special',1,NULL,1,111,222,7,'20.50','ref');");
            if (require_value(user_context->require_active_root("Tenant")).get().id != 1) {
                throw std::runtime_error(
                    "Generated initialization did not prepare the fixture root");
            }
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid inheritance variants fixture");
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
