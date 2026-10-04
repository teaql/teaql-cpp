#pragma once
#include <teaql/core/query.hpp>
namespace teaql::detail {
struct PaginationKeyRoot {std::string_view entity_type;std::uint64_t id;};
SelectQuery canonical_aggregation_query(Memory,const SelectQuery&);
String aggregation_cache_namespace(Memory,std::string_view entity);
String aggregation_cache_key(Memory,std::string_view cache_namespace,std::string_view query_namespace,const SelectQuery&);
String continuous_page_query_key(Memory,const SelectQuery&,std::string_view namespace_name,std::optional<std::string_view> user);
String id_set_query_key(Memory,const SelectQuery&,std::string_view namespace_name,std::optional<std::string_view> user,std::optional<PaginationKeyRoot> root={});
}
