#include <teaql/runtime/detail/data_service/entity_service.hpp>

namespace teaql::detail {
namespace {
void collect_mapped_fields(Memory r,
                           SqlIntentRedactions& redactions,
                           const EntityDescriptor& descriptor,
                           const Record* first,
                           const Record* second) {
    Vector<Value> values(r);
    SqlLogContext log(r);
    log.generated_sql = true;
    for (auto* row : {first, second}) {
        if (!row) {
            continue;
        }
        for (const auto& [name, value] : *row) {
            if (!descriptor.property_by_name(name)) {
                continue;
            }
            if (auto* id = descriptor.id_property(); id && id->name == name) {
                continue;
            }
            if (auto* version = descriptor.version_property(); version && version->name == name) {
                continue;
            }
            values.emplace_back(value);
            log.parameter_policies.push_back(SqlIntentRedactions::field_policy(descriptor, name));
        }
    }
    auto classified = SqlIntentRedactions::from_bindings(r, log, values, "");
    redactions.extend(classified);
}
}

Result<SqlIntentRedactions, RuntimeError> graph_plan_privacy(const UserContext& context,
                                                             const GraphMutationPlan& plan) {
    auto r = context.memory_resource();
    using R = Result<SqlIntentRedactions, RuntimeError>;
    SqlIntentRedactions redactions(r);
    for (const auto& batch : plan.batches) {
        auto descriptor = context.require_entity(batch.entity);
        if (!descriptor) {
            return R::err(r, std::move(descriptor).error());
        }
        for (const auto& item : batch.items) {
            collect_mapped_fields(r,
                                  redactions,
                                  descriptor.value().get(),
                                  &item.values.fields,
                                  item.old_values ? &item.old_values->fields : nullptr);
        }
    }
    return R::ok(r, std::move(redactions));
}

Result<SqlIntentRedactions, RuntimeError> entity_ledger_privacy(const UserContext& context,
                                                                const EntityRuntimeState& root) {
    auto r = context.memory_resource();
    using R = Result<SqlIntentRedactions, RuntimeError>;
    auto changes = root.current_change_set();
    auto keys = root.deleted_keys();
    for (const auto& [key, values] : changes.changes()) {
        keys.emplace(key);
    }
    if (const auto* dynamic = changes.dynamic_changes()) {
        for (const auto& [key, values] : *dynamic) {
            keys.emplace(key);
        }
    }
    SqlIntentRedactions redactions(r);
    for (const auto& key : keys) {
        auto descriptor = context.require_entity(key.entity);
        if (!descriptor) {
            return R::err(r, std::move(descriptor).error());
        }
        auto original = EntityRuntimeAccess::original_values_for(root, key);
        auto current = changes.changes().find(key);
        collect_mapped_fields(r,
                              redactions,
                              descriptor.value().get(),
                              original ? &original->fields : nullptr,
                              current != changes.changes().end() ? &current->second.fields
                                                                 : nullptr);
        if (const auto* dynamic = changes.dynamic_changes()) {
            auto found = dynamic->find(key);
            if (found != dynamic->end()) {
                Vector<Value> operands(r);
                for (const auto& [code, mutation] : found->second) {
                    if (const auto* value = mutation.value()) {
                        operands.emplace_back(*value);
                    }
                }
                if (const auto* originals = EntityRuntimeAccess::dynamic_originals(changes, key)) {
                    for (const auto& [code, value] : *originals) {
                        if (value) {
                            operands.emplace_back(*value);
                        }
                    }
                }
                SqlLogContext log(r);
                log.generated_sql = true;
                log.parameter_policies.assign(operands.size(), SqlParameterLogPolicy::Unknown);
                redactions.extend(SqlIntentRedactions::from_bindings(r, log, operands, ""));
            }
        }
    }
    return R::ok(r, std::move(redactions));
}
}
