#include "assist_workflow_examples.hpp"
#include <iostream>
#include <limits>

using namespace teaql;
using namespace cpp_examples;
using namespace cpp_examples::assist_runtime;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main() {
    try {
        GlobalContext global;
        auto user_context_owner = global.make_user_context();
        auto& user_context = *user_context_owner;
        bool missing = false;
        try {
            readiness(user_context);
        } catch (const std::logic_error&) {
            missing = true;
        }
        check(missing, "Readiness fails without the trusted dependency");
        auto sink = configuredRuntime(user_context, ":memory:");
        auto Q = user_context.queries<query_factory>();
        check(!Q.products()
                   .comment("Check passive provider initialization")
                   .purpose("Verify schema remains an explicit action")
                   .executeForCount(user_context)
                   .wait(),
              "Configuration does not create modeled tables");
        initializeSchema(user_context);
        readiness(user_context);
        auto* tenant = user_context.get_named_resource<String>("trusted_tenant");
        check(tenant && *tenant == "system", "Trusted tenant is an application-owned resource");
        const auto& audit_events = user_context.get(sink).events;
        auto schema_event_count = audit_events.size();
        check(schema_event_count != 0, "Schema initialization also delivers safe audit events");
        for (const auto& event : audit_events) {
            check(event.kind == RawAuditEventKind::SchemaCreated ||
                      event.kind == RawAuditEventKind::SchemaVerified ||
                      event.kind == RawAuditEventKind::FieldAdded,
                  "Startup audit events retain their schema kind");
        }
        enableAllSqlEvidence(user_context);
        const auto& saved = assist_workflows::product::create(user_context, [](Product& entity) {
            entity.updateName("support example");
        });
        check(saved.version() == 1 && audit_events.size() == schema_event_count + 1 &&
                  audit_events.back().kind == RawAuditEventKind::Created &&
                  audit_events.back().entity == "Product",
              "The configured safe audit sink receives the committed save");
        const auto& evidence = sqlEvidence(user_context);
        check(!evidence.empty(), "Application SQL evidence is available");
        for (const auto& entry : evidence) {
            check(!entry.operation.empty() && !entry.parameterized_sql.empty(),
                  "Evidence preserves parameterized SQL and operation");
            check(entry.parameterized_sql.get_allocator().resource() ==
                      user_context.memory_resource(),
                  "Evidence text belongs to UserContext");
        }
        enableSelectSqlEvidence(user_context);
        check(user_context.sql_log_options().select && !user_context.sql_log_options().mutation,
              "Select-only logging is independent");
        enableMutationSqlEvidence(user_context);
        check(!user_context.sql_log_options().select && user_context.sql_log_options().mutation,
              "Mutation-only logging is independent");
        disableSelectSqlEvidence(user_context);
        check(user_context.sql_log_options().mutation,
              "Disabling selects preserves mutation logging");
        disableMutationSqlEvidence(user_context);
        check(!user_context.sql_log_options().mutation,
              "Mutation logging can be disabled separately");
        disableSqlEvidence(user_context);
        check(user_context.sql_logs().empty(), "Disable-all clears retained SQL evidence");
        for (auto source : {R"({"nested":[{"TENANT":"bad"}]})",
                            R"({"nested":[{"provider":{}}]})",
                            R"({"nested":[{"requestPolicy":true}]})",
                            R"({"nested":[{"auditSink":false}]})",
                            R"({"nested":[{"hardLimit":100000}]})",
                            R"({"nested":[{"continuousPage":true}]})"}) {
            auto input = Json::parse(user_context.memory_resource(), source);
            check(bool(input), "Governance fixture parses");
            bool rejected = false;
            try {
                rejectGovernanceOverride(*input);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            check(rejected, "Nested governance overrides are rejected");
        }
        auto allowed =
            Json::parse(user_context.memory_resource(), R"({"name":"allowed","items":[1,2]})");
        check(bool(allowed), "Business fixture parses");
        rejectGovernanceOverride(*allowed);
        check(*user_context.get_named_resource<String>("trusted_tenant") == "system",
              "Request data does not replace the trusted tenant");
        SqlLogEntry synthetic(std::allocator_arg, Allocator(user_context.memory_resource()));
        synthetic.sql = "SELECT ?";
        synthetic.params.emplace_back("binding must not be copied");
        synthetic.debug_sql = "rendered binding must not be copied";
        synthetic.elapsed = LogDuration(std::numeric_limits<std::uint64_t>::max(), 999999999);
        SafeSqlEvidence safe(
            std::allocator_arg, Allocator(user_context.memory_resource()), synthetic);
        check(safe.parameter_count == 1 && safe.parameterized_sql == "SELECT ?" &&
                  safe.elapsed == synthetic.elapsed,
              "Safe evidence retains count/full duration without binding or rendered-SQL fields");
        std::cout << "Extracted runtime support examples passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
