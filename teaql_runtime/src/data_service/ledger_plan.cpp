#include <teaql/runtime/entity_save.hpp>

namespace teaql::detail {
Result<Vector<EntityKey>, RuntimeError> order_new_ledger_keys(const UserContext& context,
                                                              std::span<const EntityKey> input,
                                                              const LedgerChanges& changes) {
    auto r = context.memory_resource();
    using R = Result<Vector<EntityKey>, RuntimeError>;
    EntityRuntimeState::Keys unique(r);
    for (const auto& key : input) {
        // Rust's collected BTreeSet retains the last representative of equal
        // canonical keys. Its original Value still affects foreign-key matching.
        unique.erase(key);
        unique.emplace(key);
    }
    Vector<EntityKey> keys(unique.begin(), unique.end(), r);
    Vector<std::size_t> incoming(keys.size(), 0, r);
    Vector<std::pmr::set<std::size_t>> outgoing(r);
    outgoing.resize(keys.size());

    auto add_dependency = [&](std::size_t parent, std::size_t child) {
        if (parent != child && outgoing[parent].insert(child).second) {
            ++incoming[child];
        }
    };
    for (std::size_t source = 0; source < keys.size(); ++source) {
        // Source requires metadata even when the sparse changes omit this key.
        auto descriptor = context.require_entity(keys[source].entity);
        if (!descriptor) {
            return R::err(r, std::move(descriptor).error());
        }
        auto record = changes.find(keys[source]);
        if (record == changes.end()) {
            continue;
        }
        for (const auto& relation : descriptor.value().get().relations) {
            auto local = record->second.fields.find(relation.local_key);
            if (local == record->second.fields.end() || local->second.is_null()) {
                continue;
            }
            for (std::size_t target = 0; target < keys.size(); ++target) {
                if (keys[target].entity != relation.target_entity) {
                    continue;
                }
                const Value* target_value = nullptr;
                auto target_record = changes.find(keys[target]);
                if (target_record != changes.end()) {
                    auto foreign = target_record->second.fields.find(relation.foreign_key);
                    if (foreign != target_record->second.fields.end()) {
                        target_value = &foreign->second;
                    }
                }
                if (!target_value && relation.foreign_key == "id") {
                    target_value = &keys[target].id;
                }
                // Value equality is intentional. EntityKey canonical equality
                // would incorrectly join signed and unsigned foreign-key values.
                if (!target_value || *target_value != local->second) {
                    continue;
                }
                if (relation.many) {
                    add_dependency(source, target);
                } else {
                    add_dependency(target, source);
                }
            }
        }
    }

    std::pmr::set<std::size_t> ready(r);
    for (std::size_t index = 0; index < keys.size(); ++index) {
        if (incoming[index] == 0) {
            ready.insert(index);
        }
    }
    Vector<EntityKey> ordered(r);
    ordered.reserve(keys.size());
    while (!ready.empty()) {
        auto index = *ready.begin();
        ready.erase(ready.begin());
        ordered.emplace_back(keys[index]);
        for (auto dependent : outgoing[index]) {
            if (--incoming[dependent] == 0) {
                ready.insert(dependent);
            }
        }
    }
    if (ordered.size() != keys.size()) {
        FieldSet entities(r);
        for (std::size_t index = 0; index < keys.size(); ++index) {
            if (incoming[index] != 0) {
                entities.emplace(keys[index].entity);
            }
        }
        String message("new entity graph contains a required foreign-key cycle among: ", r);
        bool first = true;
        for (const auto& entity : entities) {
            if (!first) {
                message += ", ";
            }
            first = false;
            message += entity;
        }
        return R::err(r, RuntimeError::graph(r, message));
    }
    return R::ok(r, std::move(ordered));
}
}
