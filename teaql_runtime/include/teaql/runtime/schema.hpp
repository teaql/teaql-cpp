#pragma once
#include <teaql/async/operation.hpp>
#include <teaql/runtime/error.hpp>
#include <span>

namespace teaql {
class UserContext;

namespace detail {
class SchemaOperation;
}

class SchemaInvocation final {
public:
    SchemaInvocation(const SchemaInvocation&) = delete;
    SchemaInvocation& operator=(const SchemaInvocation&) = delete;

private:
    friend class detail::SchemaOperation;
    SchemaInvocation() = default;
};

using RuntimeSchemaResult = Result<std::monostate, RuntimeError>;
using GeneratedSchemaBootstrapFuture = AsyncOperation<RuntimeSchemaResult>;
using GeneratedSchemaBootstrap = GeneratedSchemaBootstrapFuture& (*)(const UserContext&);

// Shared support for generated model recipes. Each failed attempt restarts at
// the root; retryable conflicts yield once, with at most five complete attempts.
bool generated_bootstrap_retryable_conflict(const RuntimeError&);
GeneratedSchemaBootstrapFuture&
run_generated_schema_bootstrap(const UserContext&, std::span<const GeneratedSchemaBootstrap>);

namespace runtime {
// Distinct from teaql::SchemaProvider, the data-service metadata lookup SPI.
class SchemaProvider {
public:
    virtual ~SchemaProvider() = default;
    virtual AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext&,
                                                               const SchemaInvocation&) const = 0;
};
}

namespace detail {
class SchemaOperation;
// Crate-internal equivalent: audit/policy implementations inspect the active
// synchronous poll scope; no permission is retained while its future is pending.
bool is_generated_schema_bootstrap(const UserContext&);
}
}
