#pragma once
#include <teaql/runtime/detail/context_entity.hpp>
#include <teaql/runtime/relation.hpp>
namespace teaql::detail {
void validate_relation_domain(const UserContext&,std::span<const CompactRow>,const EntityRuntimeState* =nullptr,const EntityGraphBuilder* =nullptr);
void attach_relation_rows(const UserContext&,std::span<CompactRow>,const RelationLoadPlan&,Vector<CompactRow>);
EntityGraphDecodeResult install_flat_relation(const UserContext&,std::span<CompactRow>,const RelationLoadPlan&,Vector<CompactRow>,const EntityRuntimeState&,EntityGraphBuilder&);
EntityGraphDecodeResult install_compact_flat_relation(const UserContext&,std::span<const CompactRow>,const RelationLoadPlan&,Vector<CompactRow>,const EntityRuntimeState&,EntityGraphBuilder&);
}
