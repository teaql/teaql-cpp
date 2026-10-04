#pragma once
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/provider_sqlite/error.hpp>
#include <array>
struct sqlite3;
namespace teaql {class UserContext;}
namespace teaql::detail {
String sqlite_identifier_lower(Memory,std::string_view);
bool sqlite_schema_utf8_valid(std::string_view) noexcept;
std::string_view strip_sqlite_identifier_quotes(std::string_view);
String sqlite_type_family(Memory,std::string_view);
Vector<std::uint32_t> sqlite_declared_type_args(Memory,std::string_view);
void ensure_sqlite_column_compatibility(Memory,const EntityDescriptor&,const PropertyDescriptor&,std::string_view,bool);
std::array<char,4> sqlite_compatible_soundex(std::optional<std::string_view>);
struct SqliteForeignKey {
    using allocator_type=Allocator;
    std::array<String,3> columns;
    SqliteForeignKey(std::allocator_arg_t,Allocator a,std::string_view column,std::string_view table,std::string_view target):columns{String(column,a.resource()),String(table,a.resource()),String(target,a.resource())}{}
    SqliteForeignKey(std::allocator_arg_t,Allocator a,const SqliteForeignKey& other):SqliteForeignKey(std::allocator_arg,a,other[0],other[1],other[2]){}
    SqliteForeignKey(std::allocator_arg_t,Allocator a,SqliteForeignKey&& other):SqliteForeignKey(detail::move_to(a.resource(),std::move(other))){}
    SqliteForeignKey(const SqliteForeignKey&)=delete;SqliteForeignKey(SqliteForeignKey&&) noexcept=default;
    const String& operator[](std::size_t i) const{return columns[i];}
    Memory memory_resource() const noexcept{return columns[0].get_allocator().resource();}
    auto operator<=>(const SqliteForeignKey&) const=default;
};
Vector<SqliteForeignKey> sqlite_foreign_keys_for(Memory,const EntityDescriptor&,std::span<const EntityDescriptor* const>);
String compile_sqlite_create_table(Memory,const SqliteDialect&,const EntityDescriptor&,std::span<const EntityDescriptor* const>);
using SqliteTableColumns=std::pmr::map<String,std::pair<String,bool>,std::less<>>;
bool sqlite_table_exists(Memory,sqlite3&,std::string_view);
SqliteTableColumns sqlite_table_columns(Memory,sqlite3&,std::string_view);
// Called with the provider's exclusive native connection lease. Source DDL
// audit events occur before commit. Structured failures roll back; an unwinding
// sink exception is propagated to the provider's poisoned-lock handling.
void ensure_sqlite_physical_schema(Memory,sqlite3&,const SqliteDialect&,std::span<const EntityDescriptor* const>,const UserContext&);
}
