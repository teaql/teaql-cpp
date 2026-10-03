#pragma once
#include <teaql/core/meta.hpp>
#include <teaql/core/mutation.hpp>
#include <teaql/core/query.hpp>
#include <teaql/runtime/context.hpp>

namespace teaql {
enum class SqlParameterLogPolicy { Unknown, Plain, Masked, Credential };
struct CompiledQuery {
    using allocator_type = Allocator;
    String sql;
    Vector<Value> params;
    Vector<SqlParameterLogPolicy> parameter_policies;
    CompiledQuery(std::allocator_arg_t, Allocator a) : sql(a.resource()), params(a.resource()), parameter_policies(a.resource()) {}
};
class SqliteDialect {
public:
    static CompiledQuery& create_table(UserContext&, const EntityDescriptor&);
    static CompiledQuery& select(UserContext&, const EntityDescriptor&, const SelectQuery&,
                                 std::optional<Slice> bounded_slice = {});
    static CompiledQuery& mutation(UserContext&, const EntityDescriptor&, MutationKind, const MutationCommand&);
};
}
