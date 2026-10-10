#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>

namespace teaql {
namespace detail {
Result<GraphNode, RuntimeError>
graph_node_from_values(const UserContext&, std::string_view, MutationValues);
RuntimeHookResult preflight_graph(const UserContext&,
                                  GraphNode&,
                                  const ObjectLocation&,
                                  const EntityRuntimeState* root = nullptr);
RuntimeHookResult merge_relation_mutations_into_root(const EntityRuntimeState&, const GraphNode&);
RuntimeHookResult hydrate_ledger_relations(const UserContext&,
                                           const EntityRuntimeState&,
                                           GraphNode&,
                                           EntityRuntimeState::Keys& visited);
using LedgerObjectLocations = std::pmr::map<EntityKey, ObjectLocation>;
using LedgerChanges = std::pmr::map<EntityKey, EntityValues>;
using LedgerGeneratedIds = std::pmr::map<EntityKey, Value>;
Result<Vector<EntityKey>, RuntimeError>
order_new_ledger_keys(const UserContext&, std::span<const EntityKey>, const LedgerChanges&);
LedgerObjectLocations ledger_object_locations(Memory, const GraphNode&);
RuntimeHookResult reject_cancelled_new_root(const EntityRuntimeState&, const EntityKey&);
bool can_preserve_loaded_relations(const EntityRuntimeState&,
                                   const EntityKey&,
                                   const EntityDescriptor&);
}

// Public extraction only. Saving will use independent planning/governance/SPIs.
template <Entity T>
Result<GraphNode, RuntimeError> graph_node_from_entity(const UserContext& context, T entity) {
    auto r = context.memory_resource();
    auto descriptor = T::entity_descriptor(r);
    Value::List loaded(r);
    for (const auto& property : descriptor.properties) {
        if (entity.is_field_loaded(property.name)) {
            loaded.emplace_back(property.name);
        }
    }
    auto dirty = detail::clone_in(r, entity.dirty_fields());
    auto original = detail::clone_in(r, entity.original_values());
    bool fresh = entity.is_new(), removed = entity.is_marked_as_delete();
    auto comment = detail::clone_in(r, entity.get_comment());
    auto node =
        detail::graph_node_from_values(context, descriptor.name, std::move(entity).into_values());
    if (!node) {
        return node;
    }
    auto& out = node.value();
    out.values.fields.insert_or_assign(String("_loaded_fields", r), Value(r, loaded));
    out.dirty_fields = std::move(dirty);
    out.original_values = std::move(original);
    if (fresh) {
        out.operation = GraphOperation::Create;
    }
    if (removed) {
        out.operation = GraphOperation::Remove;
        out.relations.clear();
    }
    if (comment) {
        out.set_comment(*comment);
    }
    return node;
}
}
