#include <teaql/runtime/entity_save.hpp>
#include <teaql/core/detail/debug.hpp>

namespace teaql::detail {
Result<GraphNode, RuntimeError>
graph_node_from_values(const UserContext& context, std::string_view entity, MutationValues values) {
    auto r = context.memory_resource();
    using Output = Result<GraphNode, RuntimeError>;
    auto descriptor = context.require_entity(entity);
    if (!descriptor) {
        return Output::err(r, std::move(descriptor).error());
    }
    GraphNode node(std::allocator_arg, Allocator(r), entity);
    for (const auto& [field, value] : values.fields) {
        if (field == "_comment") {
            if (auto* text = std::get_if<String>(&value.data)) {
                node.set_comment(*text);
            }
            continue;
        }
        if (field == "_dirty_fields") {
            if (auto* list = std::get_if<Value::List>(&value.data)) {
                node.dirty_fields.emplace(r);
                for (const auto& field : *list) {
                    if (auto* text = std::get_if<String>(&field.data)) {
                        node.dirty_fields->emplace(*text);
                    }
                }
            }
            continue;
        }
        if (field == "_original_values") {
            if (auto* record = value.as_object()) {
                node.original_values.emplace(r);
                node.original_values->fields = clone_record(r, *record);
            }
            continue;
        }
        if (field == "_is_new" || field == "_is_deleted") {
            if (auto* flag = std::get_if<bool>(&value.data); flag && *flag) {
                node.operation =
                    field == "_is_new" ? GraphOperation::Create : GraphOperation::Remove;
            }
            continue;
        }
        const auto* relation = descriptor.value().get().relation_by_name(field);
        if (!relation) {
            node.values.fields.emplace(String(field, r), Value(r, value));
            continue;
        }
        auto invalid = [&](std::string_view expected, const Value& value) {
            String message("relation ", r);
            message += entity;
            message += '.';
            message += field;
            message += expected;
            message += debug_value(r, value);
            return Output::err(r, RuntimeError::graph(r, message));
        };
        if (std::holds_alternative<std::monostate>(value.data)) {
            node.relations.try_emplace(String(field, r));
        } else if (auto* record = value.as_object()) {
            MutationValues child_values(r);
            child_values.fields = clone_record(r, *record);
            auto child =
                graph_node_from_values(context, relation->target_entity, std::move(child_values));
            if (!child) {
                return Output::err(r, std::move(child).error());
            }
            node.relations.try_emplace(String(field, r))
                .first->second.emplace_back(std::move(child).value());
        } else if (auto* list = value.as_list()) {
            auto& children = node.relations.try_emplace(String(field, r)).first->second;
            for (const auto& value : *list) {
                auto* record = value.as_object();
                if (!record) {
                    return invalid(" expects object children, got ", value);
                }
                MutationValues child_values(r);
                child_values.fields = clone_record(r, *record);
                auto child = graph_node_from_values(
                    context, relation->target_entity, std::move(child_values));
                if (!child) {
                    return Output::err(r, std::move(child).error());
                }
                children.emplace_back(std::move(child).value());
            }
        } else {
            return invalid(" expects object/list/null, got ", value);
        }
    }
    return Output::ok(r, std::move(node));
}

RuntimeHookResult preflight_graph(const UserContext& context,
                                  GraphNode& node,
                                  const ObjectLocation& location,
                                  const EntityRuntimeState* root) {
    auto r = context.memory_resource();
    auto nr = node.memory_resource();
    if (node.operation != GraphOperation::Remove && node.operation != GraphOperation::Reference) {
        auto before = clone_in(nr, node.values);
        mark_entity_status(node.values,
                           node.operation == GraphOperation::Create ? CheckObjectStatus::Create
                                                                    : CheckObjectStatus::Update);
        if (node.dirty_fields) {
            Value::List fields(nr);
            for (const auto& field : *node.dirty_fields) {
                fields.emplace_back(field);
            }
            node.values.fields.insert_or_assign(String("_dirty_fields", nr), Value(nr, fields));
        }
        auto result = context.check_and_fix_values_at(node.entity, node.values, location);
        clear_entity_status(node.values);
        if (auto field = node.values.fields.find("_dirty_fields");
            field != node.values.fields.end()) {
            node.values.fields.erase(field);
        }
        if (!result) {
            return result;
        }
        if (root) {
            if (const auto* id = node.id()) {
                EntityKey key(std::allocator_arg, Allocator(nr), node.entity, *id);
                for (const auto& [field, value] : node.values.fields) {
                    auto old = before.fields.find(field);
                    if (old == before.fields.end() || old->second != value) {
                        root->set(key, field, value);
                    }
                }
            }
        }
    }
    for (auto& [name, children] : node.relations) {
        for (std::size_t index = 0; index < children.size(); ++index) {
            auto path = clone_in(r, location);
            path.member(name).element(index);
            auto result = preflight_graph(context, children[index], path, root);
            if (!result) {
                return result;
            }
        }
    }
    return RuntimeHookResult::ok(r, {});
}

RuntimeHookResult merge_relation_mutations_into_root(const EntityRuntimeState& root,
                                                     const GraphNode& node) {
    auto r = root.memory_resource();
    for (const auto& [name, children] : node.relations) {
        for (const auto& child : children) {
            const auto* id = child.id();
            if (!id) {
                String message("related mutation ", r);
                message += child.entity;
                message += " is missing its id";
                return RuntimeHookResult::err(r, RuntimeError::graph(r, message));
            }
            EntityKey key(std::allocator_arg, Allocator(r), child.entity, *id);
            if (child.operation == GraphOperation::Create) {
                root.mark_as_new(key);
                for (const auto& [field, value] : child.values.fields) {
                    root.set(key, field, value);
                }
            } else if (child.operation == GraphOperation::Upsert && child.dirty_fields) {
                for (const auto& field : *child.dirty_fields) {
                    if (auto value = child.values.fields.find(field);
                        value != child.values.fields.end()) {
                        root.set(key, field, value->second);
                    }
                }
            } else if (child.operation == GraphOperation::Remove) {
                root.mark_as_delete(key);
            }
            if (child.original_values) {
                if (auto version = child.original_values->fields.find("version");
                    version != child.original_values->fields.end()) {
                    if (auto value = version->second.try_i64()) {
                        root.set_original_version(key, *value);
                    }
                }
            }
            auto result = merge_relation_mutations_into_root(root, child);
            if (!result) {
                return result;
            }
        }
    }
    return RuntimeHookResult::ok(r, {});
}

RuntimeHookResult hydrate_ledger_relations(const UserContext& context,
                                           const EntityRuntimeState& root,
                                           GraphNode& node,
                                           EntityRuntimeState::Keys& visited) {
    auto r = context.memory_resource();
    auto nr = node.memory_resource();
    auto descriptor = context.require_entity(node.entity);
    if (!descriptor) {
        return RuntimeHookResult::err(r, std::move(descriptor).error());
    }
    for (const auto& relation : descriptor.value().get().relations) {
        auto local = node.values.fields.find(relation.local_key);
        if (local == node.values.fields.end()) {
            continue;
        }
        auto local_value = Value(nr, local->second);
        auto& existing = node.relations.try_emplace(String(relation.name, nr)).first->second;
        EntityRuntimeState::Keys existing_keys(nr);
        for (const auto& child : existing) {
            if (const auto* id = child.id()) {
                existing_keys.emplace(child.entity, *id);
            }
        }
        Vector<GraphNode> discovered(nr);
        auto changes = root.current_change_set();
        for (const auto& [key, fields] : changes.changes()) {
            if (key.entity != relation.target_entity || existing_keys.contains(key)) {
                continue;
            }
            auto foreign = fields.fields.find(relation.foreign_key);
            const Value* foreign_value = relation.foreign_key == "id"     ? &key.id
                                         : foreign != fields.fields.end() ? &foreign->second
                                                                          : nullptr;
            if (!foreign_value || *foreign_value != local_value || !visited.emplace(key).second) {
                continue;
            }
            GraphNode child(std::allocator_arg, Allocator(nr), key.entity);
            child.values.fields = clone_record(nr, fields.fields);
            child.values.fields.try_emplace(String("id", nr), key.id);
            child.operation = root.deleted_keys().contains(key) ? GraphOperation::Remove
                              : root.new_keys().contains(key) || !root.get_original_version(key)
                                  ? GraphOperation::Create
                                  : GraphOperation::Upsert;
            auto result = hydrate_ledger_relations(context, root, child, visited);
            if (!result) {
                return result;
            }
            discovered.emplace_back(std::move(child));
        }
        for (auto& child : discovered) {
            existing.emplace_back(std::move(child));
        }
    }
    return RuntimeHookResult::ok(r, {});
}

LedgerObjectLocations ledger_object_locations(Memory r, const GraphNode& node) {
    LedgerObjectLocations locations(r);
    auto visit = [&](auto&& self, const GraphNode& node, const ObjectLocation& path) -> void {
        if (const auto* id = node.id()) {
            locations.try_emplace(EntityKey(std::allocator_arg, Allocator(r), node.entity, *id),
                                  path);
        }
        for (const auto& [name, children] : node.relations) {
            for (std::size_t index = 0; index < children.size(); ++index) {
                auto next = clone_in(r, path);
                next.member(name).element(index);
                self(self, children[index], next);
            }
        }
    };
    visit(visit, node, ObjectLocation::root(r));
    return locations;
}

RuntimeHookResult reject_cancelled_new_root(const EntityRuntimeState& root, const EntityKey& key) {
    auto r = root.memory_resource();
    if (root.new_keys().contains(key) && root.deleted_keys().contains(key)) {
        String message("cancelled new root ", r);
        message += key.debug(r);
        message += ": create-then-delete has no persisted entity to return";
        return RuntimeHookResult::err(r, RuntimeError::graph(r, message));
    }
    return RuntimeHookResult::ok(r, {});
}

bool can_preserve_loaded_relations(const EntityRuntimeState& root,
                                   const EntityKey& key,
                                   const EntityDescriptor& descriptor) {
    if (!root.new_keys().empty() || !root.deleted_keys().empty()) {
        return false;
    }
    auto changes = root.current_change_set();
    for (const auto& [changed, fields] : changes.changes()) {
        if (changed != key) {
            return false;
        }
        for (const auto& [field, value] : fields.fields) {
            for (const auto& relation : descriptor.relations) {
                if (relation.local_key == field) {
                    return false;
                }
            }
        }
    }
    return true;
}
}
