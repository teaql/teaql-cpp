#include <teaql/runtime.hpp>
#include <teaql/data_service/sql_log.hpp>
#include <cpp_inheritance_graph.hpp>

#include <iostream>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <utility>

using namespace teaql;
using namespace cpp_inheritance_graph;

static Json describe(Memory resource, EntityDescriptor descriptor) {
    Json::Object output(resource);
    output.emplace("entity", Json(resource, descriptor.name));
    output.emplace("table", Json(resource, descriptor.table_name));
    output.emplace("service",
                   descriptor.data_service ? Json(resource, *descriptor.data_service)
                                           : Json(resource));
    output.emplace("limit",
                   descriptor.audit_value_max_len
                       ? Json(resource, std::to_string(*descriptor.audit_value_max_len))
                       : Json(resource));
    Json::Array masks(resource);
    for (const auto& mask : descriptor.audit_mask_fields) {
        masks.emplace_back(mask);
    }
    output.emplace("masks", Json(resource, std::move(masks)));
    output.emplace("declared", Json(resource, descriptor.audit_mask_fields_declared));
    const auto policy = SqlIntentRedactions::field_policy(descriptor, "name");
    output.emplace("name_sql_policy",
                   Json(resource,
                        policy == SqlParameterLogPolicy::Plain        ? "Plain"
                        : policy == SqlParameterLogPolicy::Masked     ? "Masked"
                        : policy == SqlParameterLogPolicy::Credential ? "Credential"
                                                                      : "Unknown"));
    const auto password_policy = SqlIntentRedactions::field_policy(descriptor, "password");
    output.emplace("password_sql_policy",
                   Json(resource,
                        password_policy == SqlParameterLogPolicy::Plain        ? "Plain"
                        : password_policy == SqlParameterLogPolicy::Masked     ? "Masked"
                        : password_policy == SqlParameterLogPolicy::Credential ? "Credential"
                                                                               : "Unknown"));
    return Json(resource, std::move(output));
}

static Json describe(Memory resource, const SafeAuditField& field) {
    Json::Object output(resource);
    output.emplace("name", Json(resource, field.name));
    output.emplace("value", field.value ? Json(resource, *field.value) : Json(resource));
    output.emplace("masked", Json(resource, field.masked));
    output.emplace("truncated", Json(resource, field.truncated));
    output.emplace("raw_length",
                   field.raw_length ? Json(resource, static_cast<std::uint64_t>(*field.raw_length))
                                    : Json(resource));
    output.emplace("output_length",
                   field.output_length
                       ? Json(resource, static_cast<std::uint64_t>(*field.output_length))
                       : Json(resource));
    output.emplace("mask_reason",
                   field.mask_reason ? Json(resource, *field.mask_reason) : Json(resource));
    output.emplace("truncate_reason",
                   field.truncate_reason ? Json(resource, *field.truncate_reason) : Json(resource));
    return Json(resource, std::move(output));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    {
        auto resource = user_context->memory_resource();
        Json::Array rows(resource);
        auto school = School::entity_descriptor(resource);
        Record values(resource);
        values.emplace("name", Value(resource, "PRIVATE-SCHOOL-CANARY"));
        auto event = RawAuditEvent::created(resource, "School", values);
        auto safe =
            event.build_safe_event(resource, school.audit_mask_fields, school.audit_value_max_len);
        rows.emplace_back(describe(resource, std::move(school)));
        rows.emplace_back(describe(resource, Academy::entity_descriptor(resource)));
        rows.emplace_back(describe(resource, ResearchAcademy::entity_descriptor(resource)));
        Json::Array fields(resource);
        for (const auto& field : safe.fields) {
            fields.emplace_back(describe(resource, field));
        }
        Json::Object output(resource);
        output.emplace("descriptors", Json(resource, std::move(rows)));
        output.emplace("safe_audit", Json(resource, std::move(fields)));
        std::cout << Json(resource, std::move(output)).dump(resource) << '\n';
    }
    user_context.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Generated descriptor retained UserContext memory");
    }
}
