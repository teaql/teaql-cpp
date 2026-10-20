#pragma once
#include <teaql/runtime/detail/context/entity.hpp>
#include <teaql/runtime/relation.hpp>

namespace teaql::detail {
Result<EntityRuntimeState, EntityError>
attach_flat_relation_graph(const UserContext&, std::string_view, std::span<CompactRow>);
EntityGraphDecodeResult attach_flat_relation_graph(const UserContext&,
                                                   std::string_view,
                                                   std::span<CompactRow>,
                                                   const EntityRuntimeState&,
                                                   EntityGraphBuilder&);
void validate_relation_domain(const UserContext&,
                              std::span<const CompactRow>,
                              const EntityRuntimeState* = nullptr,
                              const EntityGraphBuilder* = nullptr);
void attach_relation_rows(const UserContext&,
                          std::span<CompactRow>,
                          const RelationLoadPlan&,
                          Vector<CompactRow>,
                          std::span<const std::optional<Value>> attachment_keys = {});
void attach_relation_rows(const UserContext&,
                          std::span<CompactRow>,
                          std::span<const CompactRow>,
                          const RelationLoadPlan&,
                          Vector<CompactRow>,
                          std::span<const std::optional<Value>> attachment_keys = {});
EntityGraphDecodeResult install_flat_relation(const UserContext&,
                                              std::span<CompactRow>,
                                              const RelationLoadPlan&,
                                              Vector<CompactRow>,
                                              const EntityRuntimeState&,
                                              EntityGraphBuilder&);
EntityGraphDecodeResult install_compact_flat_relation(const UserContext&,
                                                      std::span<CompactRow>,
                                                      const RelationLoadPlan&,
                                                      Vector<CompactRow>,
                                                      const EntityRuntimeState&,
                                                      EntityGraphBuilder&);
EntityGraphDecodeResult install_flat_relation(const UserContext&,
                                              std::span<CompactRow>,
                                              const RelationLoadPlan&,
                                              Vector<CompactRow>,
                                              const EntityRuntimeState&,
                                              EntityGraphBuilder&,
                                              const EntityGraphBuilder::RelationFacets* facets);
EntityGraphDecodeResult
install_compact_flat_relation(const UserContext&,
                              std::span<CompactRow>,
                              const RelationLoadPlan&,
                              Vector<CompactRow>,
                              const EntityRuntimeState&,
                              EntityGraphBuilder&,
                              const EntityGraphBuilder::RelationFacets* facets);
EntityGraphDecodeResult install_compact_forward_views(const UserContext&,
                                                      std::span<const CompactRow>,
                                                      const RelationLoadPlan&,
                                                      std::span<const CompactRow>,
                                                      const EntityRuntimeState&,
                                                      EntityGraphBuilder&);
}
