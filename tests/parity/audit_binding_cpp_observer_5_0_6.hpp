#pragma once

#include <teaql/core/json.hpp>
#include <teaql/core/value.hpp>
#include <teaql/data_service/sql_log.hpp>

#include <array>
#include <string>
#include <utility>

static teaql::Json observe(teaql::Memory resource, teaql::EntityDescriptor descriptor) {
    using namespace teaql;

    Json::Object output(resource);
    output.emplace("entity", Json(resource, descriptor.name));
    output.emplace("table", Json(resource, descriptor.table_name));
    output.emplace("service",
                   descriptor.data_service ? Json(resource, *descriptor.data_service)
                                           : Json(resource));
    output.emplace("declared", Json(resource, descriptor.audit_mask_fields_declared));
    output.emplace("limit",
                   descriptor.audit_value_max_len
                       ? Json(resource, std::to_string(*descriptor.audit_value_max_len))
                       : Json(resource));

    Json::Array masks(resource);
    Json::Array policies(resource);
    Json::Array binding_redactions(resource);
    for (const auto& mask : descriptor.audit_mask_fields) {
        masks.emplace_back(mask);
    }
    for (const auto& property : descriptor.properties) {
        const auto policy = SqlIntentRedactions::field_policy(descriptor, property.name);
        const char* policy_name = policy == SqlParameterLogPolicy::Plain        ? "Plain"
                                  : policy == SqlParameterLogPolicy::Masked     ? "Masked"
                                  : policy == SqlParameterLogPolicy::Credential ? "Credential"
                                                                                : "Unknown";
        Json::Array policy_row(resource);
        policy_row.emplace_back(property.name);
        policy_row.emplace_back(property.column_name);
        policy_row.emplace_back(policy_name);
        policies.emplace_back(std::move(policy_row));

        SqlLogContext context(resource);
        context.generated_sql = true;
        context.parameter_policies.push_back(policy);
        String canary("TEAQL-PROBE-", resource);
        canary += property.name;
        std::array<Value, 1> params{Value(resource, canary)};
        auto redactions = SqlIntentRedactions::from_bindings(resource, context, params, "SELECT ?");
        Vector<String> safe(resource);
        Vector<String> debug(resource);
        redactions.extend_secrets(false, safe);
        redactions.extend_secrets(true, debug);

        Json::Array safe_values(resource);
        Json::Array debug_values(resource);
        for (const auto& value : safe) {
            safe_values.emplace_back(value);
        }
        for (const auto& value : debug) {
            debug_values.emplace_back(value);
        }
        Json::Array binding_row(resource);
        binding_row.emplace_back(property.name);
        binding_row.emplace_back(std::move(safe_values));
        binding_row.emplace_back(std::move(debug_values));
        binding_redactions.emplace_back(std::move(binding_row));
    }
    output.emplace("masks", Json(resource, std::move(masks)));
    output.emplace("policies", Json(resource, std::move(policies)));
    output.emplace("binding_redactions", Json(resource, std::move(binding_redactions)));
    return Json(resource, std::move(output));
}
