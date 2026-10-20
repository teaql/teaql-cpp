#include <teaql/runtime/detail/data_service/relation_installation.hpp>
#include <set>

namespace teaql::detail {
namespace {
using Installed = std::pmr::set<std::pair<String, std::uint64_t>>;
enum class FlatRelationMode { IdentityGraph, ScopedViews };

EntityGraphDecodeResult flatten(const UserContext& context,
                                std::string_view entity,
                                CompactRow& record,
                                const EntityRuntimeState& root,
                                EntityGraphBuilder& graph,
                                Installed& installed,
                                FlatRelationMode mode) {
    auto r = context.memory_resource();
    Vector<RelationDescriptor> relations(r);
    if (auto* descriptor = context.entity(entity)) {
        for (const auto& relation : descriptor->relations) {
            relations.emplace_back(relation);
        }
    }
    RelationShapeCache shapes(std::allocator_arg, Allocator(r));
    for (const auto& relation : relations) {
        if (!ContextEntityAccess::has_entity_graph_decoder(context, relation.target_entity)) {
            continue;
        }
        auto loaded = record.take_loaded_relation(relation.name);
        auto node = record.remove(relation.name);
        if (!node) {
            continue;
        }
        Value value(std::move(*node));
        const bool null = std::holds_alternative<std::monostate>(value.data) ||
                          std::holds_alternative<TypedNull>(value.data);
        if (mode == FlatRelationMode::IdentityGraph && !relation.many && loaded &&
            !loaded->facets.empty()) {
            const auto* key = record.get("id");
            auto owner = key ? key->try_u64() : std::nullopt;
            if (!owner) {
                return EntityGraphDecodeResult::err(
                    r,
                    EntityError(std::allocator_arg,
                                Allocator(r),
                                entity,
                                "Facet owner is missing its u64 id"));
            }
            graph.install_relation_facets(entity, *owner, relation.name, loaded->facets);
        }
        if (!relation.many && null &&
            (mode == FlatRelationMode::IdentityGraph || relation.local_key != "id")) {
            if (mode == FlatRelationMode::ScopedViews && loaded && !loaded->facets.empty()) {
                const auto* key = record.get("id");
                auto owner = key ? key->try_u64() : std::nullopt;
                if (!owner) {
                    return EntityGraphDecodeResult::err(
                        r,
                        EntityError(std::allocator_arg,
                                    Allocator(r),
                                    entity,
                                    "Facet owner is missing its u64 id"));
                }
                graph.install_relation_facets(
                    root.json_view_id(), entity, *owner, relation.name, loaded->facets);
            }
            record.insert(relation.name, value);
            continue;
        }
        EntityGraphBuilder::RelationFacets facets(r);
        Vector<CompactRow> children(r);
        if (loaded) {
            facets.swap(loaded->facets);
            children = std::move(loaded->data);
        } else if (auto* object = std::get_if<Value::Object>(&value.data)) {
            children.emplace_back(std::move(*object));
        } else if (auto* list = std::get_if<Value::List>(&value.data)) {
            for (auto& item : *list) {
                if (auto* object = std::get_if<Value::Object>(&item.data)) {
                    children.emplace_back(std::move(*object));
                }
            }
        } else if (!null) {
            record.insert(relation.name, value);
            continue;
        }
        if (mode == FlatRelationMode::IdentityGraph) {
            for (auto& child : children) {
                auto result =
                    flatten(context, relation.target_entity, child, root, graph, installed, mode);
                if (!result) {
                    return result;
                }
            }
            if (!relation.many && relation.local_key != "id" && !children.empty()) {
                if (auto* key = children.front().get(relation.foreign_key)) {
                    record.insert(relation.local_key, *key);
                }
            }
            const bool reverse = relation.many || relation.local_key == "id";
            const bool identity_only = children.size() == 1 && children.front().len() == 1 &&
                                       children.front().contains_key(relation.foreign_key);
            if (reverse || identity_only) {
                const auto* key = record.get("id");
                auto owner = key ? key->try_u64() : std::nullopt;
                if (!owner) {
                    return EntityGraphDecodeResult::err(
                        r,
                        EntityError(std::allocator_arg,
                                    Allocator(r),
                                    entity,
                                    reverse ? "loaded reverse relation owner is missing its u64 id"
                                            : "forward relation owner is missing its u64 id"));
                }
                if (relation.many) {
                    SmartList<CompactRow> list(
                        std::allocator_arg, Allocator(r), std::move(children));
                    list.facets.swap(facets);
                    auto result = ContextEntityAccess::decode_entity_graph_compact_smart_list(
                        context,
                        relation.target_entity,
                        std::move(list),
                        root,
                        graph,
                        entity,
                        *owner,
                        relation.name);
                    if (!result) {
                        return result;
                    }
                } else {
                    auto result = ContextEntityAccess::decode_entity_graph_compact_option(
                        context,
                        relation.target_entity,
                        std::move(children),
                        root,
                        graph,
                        entity,
                        *owner,
                        relation.name);
                    if (!result) {
                        return result;
                    }
                }
                continue;
            }
            for (auto& child : children) {
                const auto* key = child.get("id");
                auto child_id = key ? key->try_u64() : std::nullopt;
                if (!child_id) {
                    return EntityGraphDecodeResult::err(
                        r,
                        EntityError(std::allocator_arg,
                                    Allocator(r),
                                    relation.target_entity,
                                    "loaded relation is missing its u64 id"));
                }
                if (installed.emplace(String(relation.target_entity, r), *child_id).second) {
                    auto result = ContextEntityAccess::decode_entity_graph_compact(
                        context, relation.target_entity, std::move(child), root, graph);
                    if (!result) {
                        return result;
                    }
                }
            }
            record.mark_relation_loaded(relation.name, shapes);
            continue;
        }
        auto* id = record.get("id");
        auto owner = id ? id->try_u64() : std::nullopt;
        if (!owner) {
            return EntityGraphDecodeResult::err(
                r,
                EntityError(std::allocator_arg,
                            Allocator(r),
                            entity,
                            "loaded relation owner is missing its u64 id"));
        }
        graph.install_relation_facets(root.json_view_id(), entity, *owner, relation.name, facets);
        InMemoryEntityGraphDecoderRegistry::JsonRows views(r);
        for (auto& child : children) {
            auto view = graph.allocate_read_view();
            auto scoped = EntityRuntimeAccess::fresh_with_weak_graph(r, root).with_json_view(view);
            auto result =
                flatten(context, relation.target_entity, child, scoped, graph, installed, mode);
            if (!result) {
                return result;
            }
            // Retain the ordinary identity table for fallback resolution. Each
            // edge also gets its own projection view, so repeated identities do
            // not overwrite another row's selected relations.
            const auto* key = child.get("id");
            auto child_id = key ? key->try_u64() : std::nullopt;
            if (child_id &&
                installed.emplace(String(relation.target_entity, r), *child_id).second) {
                auto decoded = ContextEntityAccess::decode_entity_graph_compact(
                    context,
                    relation.target_entity,
                    CompactRow(std::allocator_arg, Allocator(r), child),
                    scoped,
                    graph);
                if (!decoded) {
                    return decoded;
                }
            }
            views.emplace_back(view, std::move(child));
        }
        if (!relation.many && relation.local_key != "id" && !views.empty()) {
            if (const auto* key = views.front().second.get(relation.foreign_key)) {
                record.insert(relation.local_key, *key);
            }
        }
        auto decoded = ContextEntityAccess::decode_json_edge(
            context,
            relation.target_entity,
            std::move(views),
            root,
            graph,
            {entity, *owner, relation.name, relation.many, &facets});
        if (!decoded) {
            return decoded;
        }
        record.mark_relation_loaded(relation.name, shapes);
    }
    return EntityGraphDecodeResult::ok(r, {});
}
}

EntityGraphDecodeResult attach_flat_relation_graph(const UserContext& context,
                                                   std::string_view entity,
                                                   std::span<CompactRow> rows,
                                                   const EntityRuntimeState& root,
                                                   EntityGraphBuilder& graph) {
    validate_relation_domain(context, rows, &root, &graph);
    auto r = context.memory_resource();
    Installed installed(r);
    graph.reserve_read_view(root.json_view_id());
    for (auto& row : rows) {
        auto result =
            flatten(context, entity, row, root, graph, installed, FlatRelationMode::ScopedViews);
        if (!result) {
            return result;
        }
    }
    return EntityGraphDecodeResult::ok(r, {});
}

Result<EntityRuntimeState, EntityError> attach_flat_relation_graph(const UserContext& context,
                                                                   std::string_view entity,
                                                                   std::span<CompactRow> rows) {
    auto r = context.memory_resource();
    using R = Result<EntityRuntimeState, EntityError>;
    EntityRuntimeState root(std::allocator_arg, Allocator(r));
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    validate_relation_domain(context, rows, &root, &graph);
    Installed installed(r);
    for (auto& row : rows) {
        auto attached =
            flatten(context, entity, row, root, graph, installed, FlatRelationMode::IdentityGraph);
        if (!attached) {
            return R::err(r, std::move(attached).error());
        }
    }
    if (!root.freeze_graph(std::move(graph))) {
        return R::err(
            r,
            EntityError(
                std::allocator_arg, Allocator(r), entity, "identity graph was already frozen"));
    }
    return R::ok(r, std::move(root));
}
}
