#pragma once
#include <teaql/async/operation.hpp>
#include <teaql/runtime/error.hpp>
namespace teaql {
class UserContext;
namespace detail {class SchemaOperation;}
class SchemaInvocation final {
public:
    SchemaInvocation(const SchemaInvocation&)=delete;
    SchemaInvocation& operator=(const SchemaInvocation&)=delete;
private:
    friend class detail::SchemaOperation;
    SchemaInvocation()=default;
};
using RuntimeSchemaResult=Result<std::monostate,RuntimeError>;
using GeneratedSchemaBootstrapFuture=AsyncOperation<RuntimeSchemaResult>;
using GeneratedSchemaBootstrap=GeneratedSchemaBootstrapFuture& (*)(const UserContext&);
namespace runtime {
// Distinct from teaql::SchemaProvider, the data-service metadata lookup SPI.
class SchemaProvider {
public:
    virtual ~SchemaProvider()=default;
    virtual AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext&,const SchemaInvocation&) const=0;
};
}
namespace detail {
class SchemaOperation;
// Crate-internal equivalent: audit/policy implementations inspect the active
// synchronous poll scope; no permission is retained while its future is pending.
bool is_generated_schema_bootstrap(const UserContext&);
}
}
