#include <teaql/runtime/detail/relation_installation.hpp>
#include <set>

namespace teaql::detail {
namespace {
using Installed = std::pmr::set<std::pair<String, std::uint64_t>>;

EntityGraphDecodeResult flatten(const UserContext& context,
                                std::string_view entity,
                                Record& record,
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
    for (const auto& relation : relations) {
        if (!ContextEntityAccess::has_entity_graph_decoder(context, relation.target_entity)) {
            continue;
        }
        auto node = record.extract(relation.name);
        if (node.empty()) {
            continue;
        }
        Value value(std::move(node.mapped()));
        const bool null = std::holds_alternative<std::monostate>(value.data) ||
                          std::holds_alternative<TypedNull>(value.data);
        if (!relation.many && null) {
            record.emplace(relation.name, std::move(value));
            continue;
        }
        Vector<Record> children(r);
        if (auto* object = std::get_if<Value::Object>(&value.data)) {
            children.emplace_back(std::move(*object));
        } else if (auto* list = std::get_if<Value::List>(&value.data)) {
            for (auto& item : *list) {
                if (auto* object = std::get_if<Value::Object>(&item.data)) {
                    children.emplace_back(std::move(*object));
                }
            }
        } else if (!null) {
            record.emplace(relation.name, std::move(value));
            continue;
        }
        for (auto& child : children) {
            auto result = flatten(context, relation.target_entity, child, root, graph, installed);
            if (!result) {
                return result;
            }
        }
        if (relation.many || relation.local_key == "id") {
            auto found = record.find("id");
            auto owner = found == record.end() ? std::nullopt : found->second.try_u64();
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
            auto result =
                relation.many
                    ? ContextEntityAccess::decode_entity_graph_compact_list(context,
                                                                            relation.target_entity,
                                                                            std::move(rows),
                                                                            root,
                                                                            graph,
                                                                            entity,
                                                                            *owner,
                                                                            relation.name)
                    : ContextEntityAccess::decode_entity_graph_compact_option(
                          context,
                          relation.target_entity,
                          std::move(rows),
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
            auto found = child.find("id");
            auto id = found == child.end() ? std::nullopt : found->second.try_u64();
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
        auto record = row.into_map(r);
        auto result = flatten(context, entity, record, root, graph, installed);
        if (!result) {
            return R::err(r, std::move(result).error());
        }
        row = CompactRow(std::allocator_arg, Allocator(r), record);
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
