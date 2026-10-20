#pragma once
#include <teaql/runtime/business_id.hpp>
#include <teaql/runtime/generated_runtime_config.hpp>

namespace teaql {
Result<BusinessIdEncodingKey, GeneratedRuntimeConfigurationError>
    read_generated_business_id_key(Memory);

Result<std::monostate, GeneratedRuntimeConfigurationError>
install_generated_business_id_environment_key(UserContext& user_context);
}
