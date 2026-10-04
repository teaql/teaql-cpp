#pragma once
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/runtime/context.hpp>
namespace teaql {
class SqliteSchemaProvider final:public runtime::SchemaProvider {
public:
    using allocator_type=Allocator;
    SqliteSchemaProvider(std::allocator_arg_t,Allocator a):resource_(a.resource()){}
    Memory memory_resource() const noexcept{return resource_;}
    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext&,const SchemaInvocation&) const override;
private:Memory resource_;
};
// C++ counterpart of SqliteProviderExt: the executor is an explicit scope
// borrow. DDL remains accessible only through UserContext::ensure_schema.
template<class Owner> UserContext& use_sqlite_provider(UserContext& context,Owner& origin,SqliteMutationExecutor& executor){
    detail::ContextResources::entry(detail::resource_arena(context.memory_resource()),origin.memory_resource(),executor);
    auto& dialect=context.make_resource<SqliteDialect>();context.insert_resource(context,dialect);
    context.insert_resource(origin,executor);
    auto& provider=context.make<SqliteSchemaProvider>();context.set_schema_provider(context,provider);
    return context;
}
}
