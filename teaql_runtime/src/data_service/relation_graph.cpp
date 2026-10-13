#include <teaql/runtime/detail/data_service/relation_installation.hpp>
#include <set>

namespace teaql::detail {
namespace {
using Installed = std::pmr::set<std::pair<String, std::uint64_t>>;

EntityGraphDecodeResult flatten(const UserContext& context,
                                std::string_view entity,
                                CompactRow& record,
                                const EntityRuntimeState& root,
                                EntityGraphBuilder& graph,
                                Installed& installed) {
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
        if (!relation.many && loaded && !loaded->facets.empty()) {
            const auto* value = record.get("id");
            auto owner = value ? value->try_u64() : std::nullopt;
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
        if (!relation.many && null) {
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
        for (auto& child : children) {
            auto result = flatten(context, relation.target_entity, child, root, graph, installed);
            if (!result) {
                return result;
            }
        }
        if (!relation.many && relation.local_key != "id" && !children.empty()) {
            if (const auto* key = children.front().get(relation.foreign_key)) {
                record.insert(relation.local_key, *key);
            }
        }
        if (relation.many || relation.local_key == "id") {
            auto* found = record.get("id");
            auto owner = found ? found->try_u64() : std::nullopt;
            if (!owner) {
                return EntityGraphDecodeResult::err(
                    r,
                    EntityError(std::allocator_arg,
                                Allocator(r),
                                entity,
                                "loaded reverse relation owner is missing its u64 id"));
            }
            Vector<CompactRow> rows(r);
            for (const auto& child : children) {
                rows.emplace_back(child);
            }
            SmartList<CompactRow> list(std::allocator_arg, Allocator(r), std::move(rows));
            list.facets.swap(facets);
            auto result = relation.many
                              ? ContextEntityAccess::decode_entity_graph_compact_smart_list(
                                    context,
                                    relation.target_entity,
                                    std::move(list),
                                    root,
                                    graph,
                                    entity,
                                    *owner,
                                    relation.name)
                              : ContextEntityAccess::decode_entity_graph_compact_option(
                                    context,
                                    relation.target_entity,
                                    std::move(list.data),
                                    root,
                                    graph,
                                    entity,
                                    *owner,
                                    relation.name);
            if (!result) {
                return result;
            }
            continue;
        }
        if (children.size() == 1 && children.front().len() == 1 &&
            children.front().contains_key(relation.foreign_key)) {
            const auto* value = record.get("id");
            auto owner = value ? value->try_u64() : std::nullopt;
            if (!owner) {
                return EntityGraphDecodeResult::err(
                    r,
                    EntityError(std::allocator_arg,
                                Allocator(r),
                                entity,
                                "forward relation owner is missing its u64 id"));
            }
            auto result =
                ContextEntityAccess::decode_entity_graph_compact_option(context,
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
            continue;
        }
        for (const auto& child : children) {
            auto* found = child.get("id");
            auto id = found ? found->try_u64() : std::nullopt;
            if (!id) {
                return EntityGraphDecodeResult::err(
                    r,
                    EntityError(std::allocator_arg,
                                Allocator(r),
                                relation.target_entity,
                                "loaded relation is missing its u64 id"));
            }
            if (installed.emplace(String(relation.target_entity, r), *id).second) {
                auto result = ContextEntityAccess::decode_entity_graph_compact(
                    context,
                    relation.target_entity,
                    CompactRow(std::allocator_arg, Allocator(r), child),
                    root,
                    graph);
                if (!result) {
                    return result;
                }
            }
        }
        record.mark_relation_loaded(relation.name, shapes);
    }
    return EntityGraphDecodeResult::ok(r, {});
}
}

Result<EntityRuntimeState, EntityError> attach_flat_relation_graph(const UserContext& context,
                                                                   std::string_view entity,
                                                                   std::span<CompactRow> rows) {
    validate_relation_domain(context, rows);
    auto r = context.memory_resource();
    using R = Result<EntityRuntimeState, EntityError>;
    EntityRuntimeState root(std::allocator_arg, Allocator(r));
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    Installed installed(r);
    for (auto& row : rows) {
        auto result = flatten(context, entity, row, root, graph, installed);
        if (!result) {
            return R::err(r, std::move(result).error());
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
