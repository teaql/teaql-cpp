#include <teaql/provider_sqlite/dialect.hpp>

namespace teaql {
String SqliteDialect::schema_type_sql(Memory r,DataType type,const PropertyDescriptor& property) const {
    switch(type) {
        case DataType::Json:return String("JSON",r);
        case DataType::Date:return String("DATE",r);
        case DataType::Timestamp:return String("TIMESTAMP",r);
        default:return SqlDialect::schema_type_sql(r,type,property);
    }
}
String SqliteDialect::compile_add_column(Memory r,const EntityDescriptor& entity,const PropertyDescriptor& property) const {
    // SQLite's source override intentionally supplies no generic fallback
    // DEFAULT. Whether existing rows permit adding this column is a schema
    // provider concern, not a compiler rewrite.
    auto definition=column_definition_sql(r,property);
    String sql("ALTER TABLE ",r);sql+=quote_ident(r,entity.table_name);sql+=" ADD COLUMN ";sql+=definition;return sql;
}
}
