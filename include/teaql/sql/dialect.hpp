#pragma once
#include <teaql/sql/bindings.hpp>
#include <teaql/core/mutation.hpp>

namespace teaql {
String quote_identifier_if_needed(Memory, std::string_view, char quote);
String bounded_sql_identifier(Memory, std::string_view, std::size_t max_bytes);

struct SchemaIndexSpec {
    using allocator_type = Allocator;
    String name, table;
    Vector<String> columns;
    bool unique = false;

    SchemaIndexSpec(std::allocator_arg_t, Allocator a)
        : name(a.resource()), table(a.resource()), columns(a.resource()) {}

    SchemaIndexSpec(std::allocator_arg_t, Allocator a, const SchemaIndexSpec& other)
        : name(other.name, a.resource()), table(other.table, a.resource()),
          columns(other.columns, a.resource()), unique(other.unique) {}

    SchemaIndexSpec(std::allocator_arg_t, Allocator a, SchemaIndexSpec&& other)
        : SchemaIndexSpec(detail::move_to(a.resource(), std::move(other))) {}

    SchemaIndexSpec(const SchemaIndexSpec&) = delete;
    SchemaIndexSpec(SchemaIndexSpec&&) noexcept = default;

    SchemaIndexSpec& operator=(SchemaIndexSpec&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    bool operator==(const SchemaIndexSpec&) const = default;
};

Vector<SchemaIndexSpec>
schema_index_specs(Memory, const EntityDescriptor&, std::optional<std::size_t> max_name_bytes);
String schema_foreign_key_name(Memory,
                               std::string_view source_table,
                               std::string_view source_column,
                               std::string_view referenced_table,
                               std::string_view referenced_column,
                               std::size_t max_name_bytes);
void validate_schema_identifier_lengths(Memory,
                                        std::span<const EntityDescriptor* const>,
                                        std::size_t max_bytes);
bool storage_length_covers(std::uint32_t, std::optional<std::uint32_t>);
bool storage_numeric_covers(std::uint32_t,
                            std::uint32_t,
                            std::optional<std::uint32_t>,
                            std::optional<std::uint32_t>);

// Provider-independent compiler. All returned strings/results name a destination
// allocation domain. Compilation failures throw structured SqlCompileError values.
// No connection, runtime UserContext or ambient binding state is consulted.
class SqlDialect {
public:
    virtual ~SqlDialect() = default;
    virtual DatabaseKind kind() const = 0;
    virtual String quote_ident(Memory, std::string_view) const = 0;
    virtual String placeholder(Memory, std::size_t index) const = 0;

    virtual bool large_in_uses_array_param() const {
        return false;
    }

    virtual bool prefers_small_parent_relation_probes() const {
        return false;
    }

    virtual std::span<const std::string_view> schema_setup_sqls() const {
        return {};
    }

    virtual String schema_type_sql(Memory, DataType, const PropertyDescriptor&) const;
    virtual String column_definition_sql(Memory, const PropertyDescriptor&) const;
    virtual String compile_create_table(Memory, const EntityDescriptor&) const;
    virtual Vector<String> schema_indexes_sqls(Memory, const EntityDescriptor&) const;
    virtual std::string_view fallback_default_value_sql(DataType) const;
    virtual String
    compile_add_column(Memory, const EntityDescriptor&, const PropertyDescriptor&) const;
    virtual CompiledQuery compile_select(Memory, const EntityDescriptor&, const SelectQuery&) const;
    virtual String
    compile_select_sql(Memory, const EntityDescriptor&, const SelectQuery&, SqlBindings&) const;
    virtual CompiledQuery
    compile_insert(Memory, const EntityDescriptor&, const InsertCommand&) const;
    virtual CompiledQuery
    compile_batch_insert(Memory, const EntityDescriptor&, const BatchInsertCommand&) const;
    virtual CompiledQuery
    compile_update(Memory, const EntityDescriptor&, const UpdateCommand&) const;
    virtual CompiledQuery compile_guarded_update(Memory,
                                                 const EntityDescriptor&,
                                                 const UpdateCommand&,
                                                 const Expr&) const;
    virtual CompiledQuery compile_update_with_guard(Memory,
                                                    const EntityDescriptor&,
                                                    const UpdateCommand&,
                                                    const Expr*) const;
    virtual CompiledQuery
    compile_batch_update(Memory, const EntityDescriptor&, const BatchUpdateCommand&) const;
    virtual CompiledQuery
    compile_delete(Memory, const EntityDescriptor&, const DeleteCommand&) const;
    virtual CompiledQuery compile_guarded_delete(Memory,
                                                 const EntityDescriptor&,
                                                 const DeleteCommand&,
                                                 const Expr&) const;
    virtual CompiledQuery compile_delete_with_guard(Memory,
                                                    const EntityDescriptor&,
                                                    const DeleteCommand&,
                                                    const Expr*) const;
    virtual CompiledQuery
    compile_recover(Memory, const EntityDescriptor&, const RecoverCommand&) const;
    virtual CompiledQuery compile_guarded_recover(Memory,
                                                  const EntityDescriptor&,
                                                  const RecoverCommand&,
                                                  const Expr&) const;
    virtual CompiledQuery compile_recover_with_guard(Memory,
                                                     const EntityDescriptor&,
                                                     const RecoverCommand&,
                                                     const Expr*) const;
    virtual String column_sql(Memory, const EntityDescriptor&, std::string_view field) const;
    virtual String
    order_by_sql(Memory, const EntityDescriptor&, const OrderBy&, SqlBindings&) const;
    virtual String
    select_projection(Memory, const EntityDescriptor&, const SelectQuery&, SqlBindings&) const;
    virtual String
    aggregate_projection(Memory, const EntityDescriptor&, const SelectQuery&, SqlBindings&) const;
    virtual String aggregate_call_sql(Memory, AggregateFunction, std::string_view field) const;
    virtual std::string_view aggregate_function_sql(AggregateFunction) const;
    virtual String compile_expr(Memory, const EntityDescriptor&, const Expr&, SqlBindings&) const;
    virtual String compile_function(
        Memory, const EntityDescriptor&, ExprFunction, std::span<const Expr>, SqlBindings&) const;
    virtual String compile_single_arg_function(Memory,
                                               const EntityDescriptor&,
                                               std::string_view function,
                                               std::span<const Expr>,
                                               SqlBindings&) const;
    virtual String compile_gbk_function(Memory,
                                        const EntityDescriptor&,
                                        std::span<const Expr>,
                                        SqlBindings&) const;
    virtual String compile_subquery(Memory,
                                    const EntityDescriptor&,
                                    const Expr& left,
                                    BinaryOp,
                                    const EntityDescriptor& sub_entity,
                                    const SelectQuery&,
                                    SqlBindings&) const;
    virtual String compile_joined(Memory,
                                  const EntityDescriptor&,
                                  std::span<const Expr>,
                                  std::string_view joiner,
                                  SqlBindings&) const;
    virtual String compile_in(Memory,
                              const EntityDescriptor&,
                              const Expr& left,
                              BinaryOp,
                              const Expr& right,
                              SqlBindings&) const;
    virtual String
    compile_projection(Memory, const EntityDescriptor&, const SelectQuery&, SqlBindings&) const;
    virtual String
    resolve_order_field(Memory, const EntityDescriptor&, const OrderBy&, SqlBindings&) const;
    virtual String column_with_alias(Memory, const PropertyDescriptor&) const;
    virtual String resolve_aggregate_field(Memory, const EntityDescriptor&, const Aggregate&) const;
};
}
