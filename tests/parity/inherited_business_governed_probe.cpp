#include "../generated_inheritance_business_ids/include/cpp_inheritance_business_ids.hpp"
#include "fixture_value.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace cpp_inheritance_business_ids;

static void execute_fixture_sql(sqlite3* database, const std::string& command) {
    if (sqlite3_exec(database, command.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

template <class T>
static void put(Json::Object& object, Memory resource, std::string_view key, T&& value) {
    object.emplace(String(key, resource), Json(resource, std::forward<T>(value)));
}

template <class Model, class Request>
static Json save(UserContext& user_context, sqlite3* database, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    auto& model = request.comment("Create inherited numbered model")
                      .purpose("Qualify inherited Business ID governance")
                      .newEntity(user_context);
    model.updateName(text(field(input, "name")))
        .updateActive(false)
        .updateNote(std::optional<String>{});
    if constexpr (requires { model.updateAmount(*Decimal::parse("12.50")); }) {
        model.updateAmount(*Decimal::parse("12.50"));
    }
    if constexpr (requires { model.updateStatus("open"); }) {
        model.updateStatus("open");
    }
    if constexpr (requires { model.updateReference("reference"); }) {
        model.updateReference("reference");
    }
    const auto& number = field(input, "number");
    if (!std::holds_alternative<std::monostate>(number.data)) {
        auto parsed = DocumentNumber::create(r, text(number));
        if (!parsed) {
            throw std::runtime_error(std::string(parsed.error()));
        }
        model.updateDocumentNumber(parsed.value());
    }
    auto descriptor = Model::entity_descriptor(r);
    auto table = std::string(descriptor.table_name);
    if (text(field(input, "fault")) == "trigger") {
        execute_fixture_sql(database,
                            "CREATE TRIGGER fixture_failure BEFORE INSERT ON " + table +
                                " BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;");
    }
    const auto& result =
        AuditedSaveExt<Model>::save(std::move(model).audit_as("Save inherited numbered model"),
                                    user_context)
            .wait();
    Json saved(r), error(r);
    Json::Array diagnostics(r), fixes(r), rows(r);
    if (result) {
        Json::Object value(r);
        put(value, r, "number", result.value().documentNumber().asStr());
        put(value, r, "tenant", result.value().tenantId());
        put(value, r, "clock_fixed", result.value().createdAt() == result.value().updatedAt());
        put(value, r, "version", result.value().version());
        saved = Json(r, value);
    } else if (result.error().kind() == RuntimeError::Kind::Check) {
        error = Json(r, "Check");
        for (const auto& finding : result.error().check_results()) {
            diagnostics.emplace_back(
                finding.to_wire(r, JsonFieldNamingProfile::SnakeCase).to_json(r));
        }
    } else {
        error = Json(r, result.error().to_string(r));
    }
    for (const auto& fix : user_context.last_fix_evidence()) {
        Json::Object value(r);
        put(value, r, "entity", fix.entity_type);
        put(value, r, "field", fix.model_path);
        put(value, r, "source", fix_evidence_source_name(fix.source));
        put(value, r, "label", fix.source_label);
        fixes.emplace_back(value);
    }
    auto command = "SELECT document_number,tenant,version FROM " + table + " ORDER BY id";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, command.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> cleanup(statement, sqlite3_finalize);
    for (auto status = sqlite3_step(statement); status != SQLITE_DONE;
         status = sqlite3_step(statement)) {
        if (status != SQLITE_ROW) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        Json::Object row(r);
        put(row, r, "number", reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)));
        put(row, r, "tenant", std::int64_t(sqlite3_column_int64(statement, 1)));
        put(row, r, "version", std::int64_t(sqlite3_column_int64(statement, 2)));
        rows.emplace_back(row);
    }
    Json::Object out(r);
    put(out, r, "model", descriptor.name);
    put(out, r, "saved", saved);
    put(out, r, "error", error);
    put(out, r, "diagnostics", diagnostics);
    put(out, r, "row_count", std::uint64_t(rows.size()));
    put(out, r, "rows", rows);
    put(out, r, "fixes", fixes);
    return Json(r, out);
}

static Json observe(UserContext& user_context, sqlite3* database, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto model = text(field(input, "model"));
    if (model == "document") {
        return save<Document>(user_context, database, Q.documents(), input);
    }
    if (model == "intermediate_record") {
        return save<IntermediateRecord>(user_context, database, Q.intermediateRecords(), input);
    }
    if (model == "invoice") {
        return save<Invoice>(user_context, database, Q.invoices(), input);
    }
    if (model == "receipt") {
        return save<Receipt>(user_context, database, Q.receipts(), input);
    }
    if (model == "special_invoice") {
        return save<SpecialInvoice>(user_context, database, Q.specialInvoices(), input);
    }
    throw std::runtime_error("Unknown inherited Business ID model");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto& clock =
                user_context->make<FixedBusinessClock>(*UtcDateTime::from_timestamp(1700000000, 0));
            user_context->set_business_clock(*user_context, clock);
            auto executor = setup(*user_context);
            user_context->disable_sql_log();
            std::array<std::uint8_t, 32> bytes{};
            for (std::size_t index = 0; index < bytes.size(); ++index) {
                bytes[index] = static_cast<std::uint8_t>(index);
            }
            auto key = require_value(BusinessIdEncodingKey::create(r, 1, bytes));
            auto& provider = user_context->make<StaticBusinessIdKeyProvider>(key);
            require_value(user_context->set_business_id_key_provider(*user_context, provider));
            auto* database = user_context->get(executor).transport.connection().native_handle();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("Invalid inherited Business ID input");
            }
            std::cout << observe(*user_context, database, *input).dump(r) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
