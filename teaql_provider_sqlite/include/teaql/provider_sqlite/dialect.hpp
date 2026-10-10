#pragma once
#include <teaql/sql/dialect.hpp>

namespace teaql {
// SQLite's provider-specific overrides. Generic query and mutation compilation
// remains in SqlDialect; this object owns no connection or ambient state.
class SqliteDialect final : public SqlDialect {
public:
    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory r, std::string_view ident) const override {
        return quote_identifier_if_needed(r, ident, '"');
    }

    String placeholder(Memory r, std::size_t) const override {
        return String("?", r);
    }

    bool prefers_small_parent_relation_probes() const override {
        return true;
    }

    String schema_type_sql(Memory, DataType, const PropertyDescriptor&) const override;
    String
    compile_add_column(Memory, const EntityDescriptor&, const PropertyDescriptor&) const override;
};
}
