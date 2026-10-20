#include "../generated_inheritance_business_ids/include/cpp_inheritance_business_ids.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/checker_model_debug.hpp>
#include <teaql/runtime/mutation_policy_debug.hpp>
#include <iostream>
#include <sqlite3.h>

using namespace cpp_inheritance_business_ids;

struct Settings {
    using allocator_type = Allocator;
    String mode;
    mutable Json::Array hooks;

    Settings(std::allocator_arg_t, Allocator a, std::string_view value)
        : mode(value, a.resource()), hooks(a.resource()) {}
};

template <class T>
static void put(Json::Object& object, Memory r, std::string_view name, T&& value) {
    object.emplace(String(name, r), Json(r, std::forward<T>(value)));
}

#define EXTENSION_LOGIC(Model, Callback)                                                           \
    class Model##Logic final : public Model##CheckerLogic {                                        \
    public:                                                                                        \
        using Model##CheckerLogic::Model##CheckerLogic;                                            \
        void Callback(const UserContext& user_context,                                             \
                      Model& entity,                                                               \
                      CheckObjectStatus status,                                                    \
                      const ObjectLocation& location,                                              \
                      CheckResults& results) const override {                                      \
            auto r = user_context.memory_resource();                                               \
            const auto& settings = user_context.require_resource<Settings>().value().get();        \
            Json::Object hook(r);                                                                  \
            put(hook, r, "status", debug_string(r, status));                                       \
            put(hook, r, "before", std::uint64_t(results.size()));                                 \
            put(hook, r, "name", entity.name());                                                   \
            put(hook, r, "number", entity.documentNumber().asStr());                               \
            put(hook, r, "tenant", entity.tenantId());                                             \
            settings.hooks.emplace_back(hook);                                                     \
            requiredText(entity.name(), "name", location, results);                                \
            minStringLength(entity.name(), "name", 2, location, results);                          \
            maxStringLength(entity.name(), "name", 8, location, results);                          \
            if (settings.mode == "deny") {                                                         \
                required(false, "custom_field", location, results);                                \
            }                                                                                      \
            if (settings.mode == "fix") {                                                          \
                entity.updateName("fixed");                                                        \
            }                                                                                      \
        }                                                                                          \
    };
EXTENSION_LOGIC(Document, checkAndFixDocument)
EXTENSION_LOGIC(IntermediateRecord, checkAndFixIntermediateRecord)
EXTENSION_LOGIC(Invoice, checkAndFixInvoice)
EXTENSION_LOGIC(Receipt, checkAndFixReceipt)
EXTENSION_LOGIC(SpecialInvoice, checkAndFixSpecialInvoice)
#undef EXTENSION_LOGIC

class Policy final : public MutationPolicy {
public:
    using allocator_type = Allocator;
    Memory resource;
    mutable Json::Array plans;
    bool allow;

    Policy(std::allocator_arg_t, Allocator a, bool permit)
        : resource(a.resource()), plans(resource), allow(permit) {}

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "inherited-extension", "5.0.6", "fixture");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        Json::Object out(r);
        put(out, r, "request", plan.request_key);
        put(out, r, "root", plan.root_entity_type);
        put(out, r, "reason", plan.audit_reason ? Json(r, *plan.audit_reason) : Json(r));
        Json::Array operations(r);
        for (const auto& operation : plan.operations) {
            Json::Object item(r), values(r);
            for (const auto& [name, value] : operation.changed_values) {
                if (name != "created_at" && name != "updated_at") {
                    values.emplace(String(name, r), value.to_json_value(r));
                }
            }
            put(item, r, "kind", debug_string(r, operation.kind));
            put(item, r, "entity", operation.entity.entity);
            put(item, r, "id", operation.entity.id.to_json_value(r));
            put(item,
                r,
                "version",
                operation.original_version ? Json(r, *operation.original_version) : Json(r));
            put(item, r, "values", Json(r, values));
            Json::Array clocks(r);
            clocks.emplace_back(operation.changed_values.contains("created_at"));
            clocks.emplace_back(operation.changed_values.contains("updated_at"));
            put(item, r, "clock_fields", Json(r, clocks));
            operations.emplace_back(item);
        }
        put(out, r, "operations", Json(r, operations));
        plans.emplace_back(out);
        MutationDecision decision(std::allocator_arg, Allocator(r));
        if (!allow) {
            decision.verdict = MutationVerdict::Deny;
            decision.code.emplace("EXTENSION-DENIED", r);
            decision.message.emplace("business policy denied", r);
            decision.field_paths.emplace_back("name");
        }
        return decision;
    }
};

template <class Model, class Checker>
static void register_default(UserContext& user_context, InMemoryCheckerRegistry& registry) {
    auto& checker = user_context.make<Checker>();
    auto& adapter = user_context.make<TypedEntityChecker<Model, Checker>>(user_context, checker);
    registry.register_checker(user_context, adapter);
}

static void execute(sqlite3* database, const std::string& command) {
    char* message = nullptr;
    if (sqlite3_exec(database, command.c_str(), nullptr, nullptr, &message) != SQLITE_OK) {
        std::string error = message ? message : sqlite3_errmsg(database);
        sqlite3_free(message);
        throw std::runtime_error(error);
    }
}

static Json read_rows(Memory r, sqlite3* database, const std::string& table) {
    auto sql = "SELECT name,document_number,tenant,version FROM " + table + " ORDER BY id";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database, sql.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array rows(r);
    int status;
    while ((status = sqlite3_step(raw)) == SQLITE_ROW) {
        Json::Object row(r);
        for (auto [name, index] : {std::pair{"name", 0}, std::pair{"number", 1}}) {
            put(row,
                r,
                name,
                std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(raw, index)),
                                 sqlite3_column_bytes(raw, index)));
        }
        put(row, r, "tenant", std::int64_t(sqlite3_column_int64(raw, 2)));
        put(row, r, "version", std::int64_t(sqlite3_column_int64(raw, 3)));
        rows.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, rows);
}

template <class Model, class Checker, class Logic, class Request>
static Json run(UserContext& user_context, sqlite3* database, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    Model* model = &request.comment("Prepare an inherited business extension")
                        .purpose("Qualify custom Checker and governed Save")
                        .newEntity(user_context);
    model->updateName("seed").updateActive(false).updateNote(std::optional<String>{});
    if constexpr (requires { model->updateAmount(*Decimal::parse("12.50")); }) {
        model->updateAmount(*Decimal::parse("12.50"));
    }
    if constexpr (requires { model->updateStatus("open"); }) {
        model->updateStatus("open");
    }
    if constexpr (requires { model->updateReference("reference"); }) {
        model->updateReference("reference");
    }
    bool update = text(field(input, "operation")) == "update";
    if (update) {
        const auto& seed =
            AuditedSaveExt<Model>::save(
                std::move(*model).audit_as("Seed inherited business extension"), user_context)
                .wait();
        model = &user_context.make<Model>(require_value(seed));
    }
    auto created_before = model->createdAt();
    String number_before(model->documentNumber().asStr(), r);
    model->updateName(text(field(input, "name")));
    auto& settings = user_context.make<Settings>(text(field(input, "logic")));
    user_context.insert_resource(user_context, settings);
    auto& registry = user_context.make<InMemoryCheckerRegistry>();
    register_default<Tenant, TenantChecker>(user_context, registry);
    register_default<Document, DocumentChecker>(user_context, registry);
    register_default<IntermediateRecord, IntermediateRecordChecker>(user_context, registry);
    register_default<Invoice, InvoiceChecker>(user_context, registry);
    register_default<Receipt, ReceiptChecker>(user_context, registry);
    register_default<SpecialInvoice, SpecialInvoiceChecker>(user_context, registry);
    auto& logic = user_context.make<Logic>();
    auto& checker = user_context.make<Checker>(logic);
    auto& adapter = user_context.make<TypedEntityChecker<Model, Checker>>(user_context, checker);
    registry.register_checker(user_context, adapter);
    user_context.set_checker_registry(user_context, registry);
    auto& policy = user_context.make<Policy>(std::get<bool>(field(input, "allow").data));
    auto& policies = user_context.make<FunctionMutationPolicyRegistry>(
        [r, &policy](Memory owner, std::string_view) -> std::optional<MutationPolicyBorrow> {
            return MutationPolicyBorrow(
                std::allocator_arg, Allocator(owner), policy, detail::resource_arena(r));
        });
    user_context.set_mutation_policy_registry(user_context, policies);
    auto descriptor = Model::entity_descriptor(r);
    auto table = std::string(descriptor.table_name);
    auto fault = text(field(input, "fault"));
    if (fault == "trigger") {
        execute(database,
                "CREATE TRIGGER fixture_failure BEFORE " +
                    std::string(update ? "UPDATE" : "INSERT") + " ON " + table +
                    " BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;");
    } else if (fault == "conflict") {
        execute(database, "UPDATE " + table + " SET version=2");
    }
    const auto& result =
        AuditedSaveExt<Model>::save(
            std::move(*model).audit_as("Apply inherited business extension"), user_context)
            .wait();
    Json saved(r), error(r);
    Json::Array diagnostics(r), fixes(r);
    if (result) {
        Json::Object out(r);
        put(out, r, "name", result.value().name());
        put(out, r, "number", result.value().documentNumber().asStr());
        put(out, r, "tenant", result.value().tenantId());
        put(out, r, "version", result.value().version());
        put(out,
            r,
            "created_retained",
            update ? Json(r, result.value().createdAt() == created_before) : Json(r));
        put(out,
            r,
            "number_retained",
            update ? Json(r, result.value().documentNumber().asStr() == number_before) : Json(r));
        saved = Json(r, out);
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
        Json::Object out(r);
        put(out, r, "entity", fix.entity_type);
        put(out, r, "field", fix.model_path);
        put(out, r, "source", fix_evidence_source_name(fix.source));
        put(out, r, "label", fix.source_label);
        fixes.emplace_back(out);
    }
    Json::Object out(r);
    put(out, r, "model", descriptor.name);
    put(out, r, "saved", saved);
    put(out, r, "error", error);
    put(out, r, "diagnostics", Json(r, diagnostics));
    put(out, r, "rows", read_rows(r, database, table));
    put(out, r, "hooks", Json(r, settings.hooks));
    put(out, r, "plans", Json(r, policy.plans));
    put(out, r, "fixes", Json(r, fixes));
    return Json(r, out);
}

static Json observe(UserContext& user_context, sqlite3* database, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto model = text(field(input, "model"));
    if (model == "document") {
        return run<Document, DocumentChecker, DocumentLogic>(
            user_context, database, Q.documents(), input);
    }
    if (model == "intermediate_record") {
        return run<IntermediateRecord, IntermediateRecordChecker, IntermediateRecordLogic>(
            user_context, database, Q.intermediateRecords(), input);
    }
    if (model == "invoice") {
        return run<Invoice, InvoiceChecker, InvoiceLogic>(
            user_context, database, Q.invoices(), input);
    }
    if (model == "receipt") {
        return run<Receipt, ReceiptChecker, ReceiptLogic>(
            user_context, database, Q.receipts(), input);
    }
    return run<SpecialInvoice, SpecialInvoiceChecker, SpecialInvoiceLogic>(
        user_context, database, Q.specialInvoices(), input);
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
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
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                bytes[i] = static_cast<std::uint8_t>(i);
            }
            auto key = require_value(BusinessIdEncodingKey::create(r, 1, bytes));
            auto& provider = user_context->make<StaticBusinessIdKeyProvider>(key);
            require_value(user_context->set_business_id_key_provider(*user_context, provider));
            auto* database = user_context->get(executor).transport.connection().native_handle();
            Json retained(global.memory_resource());
            {
                auto input = Json::parse(r, line);
                if (!input) {
                    throw std::runtime_error("invalid inherited business extension fixture");
                }
                auto result = observe(*user_context, database, *input);
                retained = detail::clone_in(global.memory_resource(), result);
            }
            user_context.reset();
            std::cout << retained.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
