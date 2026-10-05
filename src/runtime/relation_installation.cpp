#include <teaql/runtime/detail/relation_installation.hpp>

namespace teaql::detail {
void validate_relation_domain(const UserContext& context,
                              std::span<const CompactRow> rows,
                              const EntityRuntimeState* root,
                              const EntityGraphBuilder* graph) {
    ContextEntityAccess::ensure_open(context);
    auto r = context.memory_resource();
    if ((root && root->memory_resource() != r) || (graph && graph->memory_resource() != r)) {
        throw std::invalid_argument("Relation graph and root must belong to the query Context");
    }
    for (const auto& row : rows) {
        if (row.memory_resource() != r) {
            throw std::invalid_argument("Mutable relation rows must belong to the query Context");
        }
    }
}

namespace {
const RelationDescriptor* inverse(const UserContext& context, const RelationLoadPlan& plan) {
    if (auto* entity = context.entity(plan.target_entity)) {
        for (const auto& relation : entity->relations) {
            if (relation.target_entity == plan.parent_entity &&
                relation.local_key == plan.foreign_key && relation.foreign_key == plan.local_key) {
                return &relation;
            }
        }
    }
    return nullptr;
}

using Buckets = std::pmr::map<String, Vector<CompactRow>, std::less<>>;

Buckets buckets(Memory r, const RelationLoadPlan& plan, Vector<CompactRow> rows) {
    Buckets out(r);
    for (auto& row : rows) {
        if (auto* key = row.get(plan.foreign_key)) {
            out.try_emplace(graph_identity_key(r, *key)).first->second.emplace_back(std::move(row));
        }
    }
    return out;
}

Vector<CompactRow> take_related(Memory r, Buckets& buckets, const Value& key) {
    auto it = buckets.find(graph_identity_key(r, key));
    if (it == buckets.end()) {
        return Vector<CompactRow>(r);
    }
    auto rows = std::move(it->second);
    buckets.erase(it);
    return rows;
}

EntityGraphDecodeResult install_reverse(const UserContext& context,
                                        const CompactRow& parent,
                                        const RelationLoadPlan& plan,
                                        Vector<CompactRow> rows,
                                        const EntityRuntimeState& root,
                                        EntityGraphBuilder& graph) {
    auto r = context.memory_resource();
    auto* id = parent.get("id");
    auto owner = id ? id->try_u64() : std::nullopt;
    if (!owner) {
        return EntityGraphDecodeResult::err(
            r,
            EntityError(std::allocator_arg,
                        Allocator(r),
                        plan.parent_entity,
                        "loaded reverse relation owner is missing its u64 id"));
    }
    if (plan.many) {
        return ContextEntityAccess::decode_entity_graph_compact_list(context,
                                                                     plan.target_entity,
                                                                     std::move(rows),
                                                                     root,
                                                                     graph,
                                                                     plan.parent_entity,
                                                                     *owner,
                                                                     plan.relation_name);
    }
    return ContextEntityAccess::decode_entity_graph_compact_option(context,
                                                                   plan.target_entity,
                                                                   std::move(rows),
                                                                   root,
                                                                   graph,
                                                                   plan.parent_entity,
                                                                   *owner,
                                                                   plan.relation_name);
}
}

void attach_relation_rows(const UserContext& context,
                          std::span<CompactRow> parents,
                          const RelationLoadPlan& plan,
                          Vector<CompactRow> children) {
    validate_relation_domain(context, parents);
    auto r = context.memory_resource();
    auto* back = inverse(context, plan);
    auto grouped = buckets(r, plan, std::move(children));
    for (auto& parent : parents) {
        auto* key = parent.get(plan.local_key);
        if (!key) {
            continue;
        }
        Vector<CompactRow> related(r);
        if (auto it = grouped.find(graph_identity_key(r, *key)); it != grouped.end()) {
            for (const auto& row : it->second) {
                related.emplace_back(row);
            }
        }
        if (back) {
            auto parent_object = clone_in(r, parent);
            parent_object.remove(plan.relation_name);
            for (auto& row : related) {
                auto parent_value = Value(r, parent_object.into_map(r));
                if (!back->many) {
                    row.insert(back->name, parent_value);
                } else {
                    if (!row.contains_key(back->name)) {
                        row.insert(back->name, Value(r, Value::List(r)));
                    }
                    if (auto* values = std::get_if<Value::List>(&row.get_mut(back->name)->data)) {
                        values->emplace_back(parent_value);
                    }
                }
            }
        }
        if (plan.many) {
            Value::List values(r);
            for (const auto& row : related) {
                values.emplace_back(row.into_map(r));
            }
            parent.insert(plan.relation_name, Value(r, values));
        } else {
            parent.insert(plan.relation_name,
                          related.empty() ? Value(r) : Value(r, related.front().into_map(r)));
        }
    }
}

EntityGraphDecodeResult install_flat_relation(const UserContext& context,
                                              std::span<CompactRow> parents,
                                              const RelationLoadPlan& plan,
                                              Vector<CompactRow> children,
                                              const EntityRuntimeState& root,
                                              EntityGraphBuilder& graph) {
    validate_relation_domain(context, parents, &root, &graph);
    auto r = context.memory_resource();
    auto* back = inverse(context, plan);
    auto grouped = buckets(r, plan, std::move(children));
    for (auto& parent : parents) {
        auto* key = parent.get(plan.local_key);
        if (!key) {
            continue;
        }
        auto related = take_related(r, grouped, *key);
        if (back) {
            for (const auto& child : related) {
                auto* value = child.get("id");
                auto id = value ? value->try_u64() : std::nullopt;
                if (!id) {
                    continue;
                }
                Vector<CompactRow> rows(r);
                rows.emplace_back(parent);
                auto result =
                    back->many
                        ? ContextEntityAccess::decode_entity_graph_compact_list(context,
                                                                                plan.parent_entity,
                                                                                std::move(rows),
                                                                                root,
                                                                                graph,
                                                                                plan.target_entity,
                                                                                *id,
                                                                                back->name)
                        : ContextEntityAccess::decode_entity_graph_compact_option(
                              context,
                              plan.parent_entity,
                              std::move(rows),
                              root,
                              graph,
                              plan.target_entity,
                              *id,
                              back->name);
                if (!result) {
                    return result;
                }
            }
        }
        if (plan.many || plan.local_key == "id") {
            auto result = install_reverse(context, parent, plan, std::move(related), root, graph);
            if (!result) {
                return result;
            }
        } else if (related.empty()) {
            parent.insert(plan.relation_name, Value(r));
        } else {
            for (auto& child : related) {
                auto result = ContextEntityAccess::decode_entity_graph_compact(
                    context, plan.target_entity, std::move(child), root, graph);
                if (!result) {
                    return result;
                }
            }
        }
    }
    return EntityGraphDecodeResult::ok(r, {});
}

EntityGraphDecodeResult install_compact_flat_relation(const UserContext& context,
                                                      std::span<const CompactRow> parents,
                                                      const RelationLoadPlan& plan,
                                                      Vector<CompactRow> children,
                                                      const EntityRuntimeState& root,
                                                      EntityGraphBuilder& graph) {
    validate_relation_domain(context, parents, &root, &graph);
    auto r = context.memory_resource();
    if (!plan.many && plan.local_key != "id") {
        return ContextEntityAccess::decode_entity_graph_compact_batch(
            context, plan.target_entity, std::move(children), root, graph);
    }
    auto grouped = buckets(r, plan, std::move(children));
    for (const auto& parent : parents) {
        auto* key = parent.get(plan.local_key);
        if (!key) {
            continue;
        }
        auto result =
            install_reverse(context, parent, plan, take_related(r, grouped, *key), root, graph);
        if (!result) {
            return result;
        }
    }
    return EntityGraphDecodeResult::ok(r, {});
}
}
