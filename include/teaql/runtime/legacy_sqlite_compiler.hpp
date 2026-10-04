#pragma once
#include <teaql/core/meta.hpp>
#include <teaql/core/mutation.hpp>
#include <teaql/core/query.hpp>
#include <teaql/data_service/sql_log.hpp>
#include <teaql/sql/types.hpp>

namespace teaql {
// Transitional compiler used only by the old SqliteSession regression path.
// The source-aligned provider dialect lives in teaql/provider_sqlite/dialect.hpp.
class LegacySqliteCompiler {
public:
    static CompiledQuery create_table(Memory, const EntityDescriptor&);
    static CompiledQuery select(Memory, const EntityDescriptor&, const SelectQuery&,
                                 std::optional<Slice> bounded_slice = {});
    static CompiledQuery compile_insert(Memory, const EntityDescriptor&, const InsertCommand&);
    static CompiledQuery compile_update(Memory, const EntityDescriptor&, const UpdateCommand&);
    static CompiledQuery compile_delete(Memory, const EntityDescriptor&, const DeleteCommand&);
    static CompiledQuery compile_recover(Memory, const EntityDescriptor&, const RecoverCommand&);
};
}
