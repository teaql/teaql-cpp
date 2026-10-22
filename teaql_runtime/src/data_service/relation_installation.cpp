#include <teaql/runtime/detail/data_service/relation_installation.hpp>

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

Vector<CompactRow> take_related(Memory r, Buckets& buckets, const Value& key, bool consume) {
    auto it = buckets.find(graph_identity_key(r, key));
    if (it == buckets.end()) {
        return Vector<CompactRow>(r);
    }
    if (!consume) {
        return Vector<CompactRow>(it->second, r);
    }
    auto rows = std::move(it->second);
    buckets.erase(it);
    return rows;
}

EntityGraphDecodeResult
install_reverse(const UserContext& context,
                const CompactRow& parent,
                const RelationLoadPlan& plan,
                Vector<CompactRow> rows,
                const EntityRuntimeState& root,
                EntityGraphBuilder& graph,
                const EntityGraphBuilder::RelationFacets* facets = nullptr) {
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
        SmartList<CompactRow> list(std::allocator_arg, Allocator(r), std::move(rows));
        if (facets) {
            for (const auto& [name, values] : *facets) {
                list.facets.emplace(name, values);
            }
        }
        return ContextEntityAccess::decode_entity_graph_compact_smart_list(context,
                                                                           plan.target_entity,
                                                                           std::move(list),
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

EntityGraphDecodeResult install_unfetched_forward(const UserContext& context,
                                                  const CompactRow& parent,
                                                  const RelationLoadPlan& plan,
                                                  const EntityRuntimeState& root,
                                                  EntityGraphBuilder& graph) {
    auto r = context.memory_resource();
    const auto* id = parent.get("id");
    auto owner = id ? id->try_u64() : std::nullopt;
    if (!owner) {
        return EntityGraphDecodeResult::err(
            r,
            EntityError(std::allocator_arg,
                        Allocator(r),
                        plan.parent_entity,
                        "forward relation owner is missing its u64 id"));
    }
    Vector<CompactRow> rows(r);
    if (const auto* key = parent.get(plan.local_key); key && !key->is_null()) {
        Record identity(r);
        identity.emplace(String(plan.foreign_key, r), Value(r, *key));
        rows.emplace_back(std::move(identity));
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
                          Vector<CompactRow> children,
                          std::span<const std::optional<Value>> attachment_keys) {
    attach_relation_rows(context, parents, parents, plan, std::move(children), attachment_keys);
}

void attach_relation_rows(const UserContext& context,
                          std::span<CompactRow> parents,
                          std::span<const CompactRow> parent_keys,
                          const RelationLoadPlan& plan,
                          Vector<CompactRow> children,
                          std::span<const std::optional<Value>> attachment_keys) {
    validate_relation_domain(context, parents);
    validate_relation_domain(context, parent_keys);
    if (parents.size() != parent_keys.size()) {
        throw std::invalid_argument("Relation key snapshots must correspond to parent rows");
    }
    auto r = context.memory_resource();
    auto* back = inverse(context, plan);
    if (back && std::ranges::any_of(plan.children, [&](const auto& child) {
            return child.relation_name == back->name;
        })) {
        back = nullptr;
    }
    Buckets grouped(r);
    if (!attachment_keys.empty() && attachment_keys.size() != children.size()) {
        throw std::invalid_argument("Relation attachment keys must match the child batch");
    }
    for (std::size_t i = 0; i < children.size(); ++i) {
        const Value* key = attachment_keys.empty()
                               ? children[i].get(plan.foreign_key)
                               : (attachment_keys[i] ? &*attachment_keys[i] : nullptr);
        if (key) {
            grouped.try_emplace(graph_identity_key(r, *key))
                .first->second.emplace_back(std::move(children[i]));
        }
    }
    for (std::size_t index = 0; index < parents.size(); ++index) {
        auto& parent = parents[index];
        auto* key = parent_keys[index].get(plan.local_key);
        Vector<CompactRow> related(r);
        if (auto it = key ? grouped.find(graph_identity_key(r, *key)) : grouped.end();
            it != grouped.end()) {
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
        if (!plan.many && related.empty() && key && !key->is_null()) {
            Record identity(r);
            identity.emplace(String(plan.foreign_key, r), Value(r, *key));
            related.emplace_back(identity);
        }
        if ((plan.query && !plan.query->facets.empty()) ||
            std::ranges::any_of(related, [](const auto& row) {
                return row.has_loaded_relations();
            })) {
            SmartList<CompactRow> list(std::allocator_arg, Allocator(r));
            if (plan.many) {
                for (const auto& row : related) {
                    list.data.emplace_back(row);
                }
            } else if (!related.empty()) {
                list.data.emplace_back(related.front());
            }
            list.loaded();
            parent.set_loaded_relation(plan.relation_name, list);
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
    return install_flat_relation(context, parents, plan, std::move(children), root, graph, nullptr);
}

EntityGraphDecodeResult install_flat_relation(const UserContext& context,
                                              std::span<CompactRow> parents,
                                              const RelationLoadPlan& plan,
                                              Vector<CompactRow> children,
                                              const EntityRuntimeState& root,
                                              EntityGraphBuilder& graph,
                                              const EntityGraphBuilder::RelationFacets* facets) {
    validate_relation_domain(context, parents, &root, &graph);
    auto r = context.memory_resource();
    auto* back = inverse(context, plan);
    if (back && std::ranges::any_of(plan.children, [&](const auto& child) {
            return child.relation_name == back->name;
        })) {
        back = nullptr;
    }
    auto grouped = buckets(r, plan, std::move(children));
    for (auto& parent : parents) {
        auto* key = parent.get(plan.local_key);
        // The source's ordinary flat path publishes graph edges without adding
        // compact-row availability markers. CompactFlat marks those separately.
        auto related =
            key ? take_related(r, grouped, *key, plan.local_key == "id") : Vector<CompactRow>(r);
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
            auto result =
                install_reverse(context, parent, plan, std::move(related), root, graph, facets);
            if (!result) {
                return result;
            }
        } else if (related.empty()) {
            auto result = install_unfetched_forward(context, parent, plan, root, graph);
            if (!result) {
                return result;
            }
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
                                                      std::span<CompactRow> parents,
                                                      const RelationLoadPlan& plan,
                                                      Vector<CompactRow> children,
                                                      const EntityRuntimeState& root,
                                                      EntityGraphBuilder& graph) {
    return install_compact_flat_relation(
        context, parents, plan, std::move(children), root, graph, nullptr);
}

EntityGraphDecodeResult
install_compact_flat_relation(const UserContext& context,
                              std::span<CompactRow> parents,
                              const RelationLoadPlan& plan,
                              Vector<CompactRow> children,
                              const EntityRuntimeState& root,
                              EntityGraphBuilder& graph,
                              const EntityGraphBuilder::RelationFacets* facets) {
    validate_relation_domain(context, parents, &root, &graph);
    auto r = context.memory_resource();
    RelationShapeCache shapes(std::allocator_arg, Allocator(r));
    if (!plan.many && plan.local_key != "id") {
        std::pmr::set<String> fetched(r);
        for (const auto& child : children) {
            if (const auto* key = child.get(plan.foreign_key)) {
                fetched.emplace(graph_identity_key(r, *key));
            }
        }
        auto result = ContextEntityAccess::decode_entity_graph_compact_batch(
            context, plan.target_entity, std::move(children), root, graph);
        if (!result) {
            return result;
        }
        for (auto& parent : parents) {
            const auto* key = parent.get(plan.local_key);
            const bool matched = key && fetched.contains(graph_identity_key(r, *key));
            if (!matched) {
                auto detail = install_unfetched_forward(context, parent, plan, root, graph);
                if (!detail) {
                    return detail;
                }
            }
            if (matched || (key && key->is_null())) {
                parent.mark_relation_loaded(plan.relation_name, shapes);
            }
        }
        return EntityGraphDecodeResult::ok(r, {});
    }
    auto grouped = buckets(r, plan, std::move(children));
    for (auto& parent : parents) {
        parent.mark_relation_loaded(plan.relation_name, shapes);
        auto* key = parent.get(plan.local_key);
        auto related =
            key ? take_related(r, grouped, *key, plan.local_key == "id") : Vector<CompactRow>(r);
        auto result =
            install_reverse(context, parent, plan, std::move(related), root, graph, facets);
        if (!result) {
            return result;
        }
    }
    return EntityGraphDecodeResult::ok(r, {});
}

EntityGraphDecodeResult install_compact_forward_views(const UserContext& context,
                                                      std::span<const CompactRow> parents,
                                                      const RelationLoadPlan& plan,
                                                      std::span<const CompactRow> children,
                                                      const EntityRuntimeState& root,
                                                      EntityGraphBuilder& graph) {
    validate_relation_domain(context, parents, &root, &graph);
    validate_relation_domain(context, children, &root, &graph);
    auto r = context.memory_resource();
    for (const auto& parent : parents) {
        const auto* id = parent.get("id");
        auto owner = id ? id->try_u64() : std::nullopt;
        if (!owner) {
            return EntityGraphDecodeResult::err(
                r,
                EntityError(std::allocator_arg,
                            Allocator(r),
                            plan.parent_entity,
                            "forward relation owner is missing its u64 id"));
        }
        Vector<CompactRow> rows(r);
        const auto* key = parent.get(plan.local_key);
        for (const auto& child : children) {
            const auto* foreign = child.get(plan.foreign_key);
            if ((!key && !foreign) || (key && foreign && *key == *foreign)) {
                rows.emplace_back(child);
            }
        }
        auto result = ContextEntityAccess::decode_entity_graph_compact_option(context,
                                                                              plan.target_entity,
                                                                              std::move(rows),
                                                                              root,
                                                                              graph,
                                                                              plan.parent_entity,
                                                                              *owner,
                                                                              plan.relation_name);
        if (!result) {
            return result;
        }
    }
    return EntityGraphDecodeResult::ok(r, {});
}
}
