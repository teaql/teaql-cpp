#include <teaql/runtime/detail/data_service/relation_enhancement.hpp>

namespace teaql::detail {
Value default_aggregate_value(Memory r, bool single) {
    return single ? Value(r, std::uint64_t(0)) : Value(r, Value::List(r));
}

void attach_empty_relation_aggregate(std::span<CompactRow> rows,
                                     std::string_view alias,
                                     bool single) {
    for (auto& row : rows) {
        row.insert(alias, default_aggregate_value(row.memory_resource(), single));
    }
}

void attach_relation_aggregate_rows(std::span<CompactRow> parents,
                                    const RelationLoadPlan& plan,
                                    const RelationAggregate& aggregate,
                                    Vector<CompactRow> rows) {
    attach_relation_aggregate_rows(parents, parents, plan, aggregate, std::move(rows));
}

void attach_relation_aggregate_rows(std::span<CompactRow> parents,
                                    std::span<const CompactRow> keys,
                                    const RelationLoadPlan& plan,
                                    const RelationAggregate& aggregate,
                                    Vector<CompactRow> rows) {
    if (parents.size() != keys.size()) {
        throw std::invalid_argument("Aggregate key snapshots must correspond to parent rows");
    }
    auto r = plan.memory_resource();
    std::pmr::map<String, Vector<CompactRow>, std::less<>> buckets(r);
    for (auto& row : rows) {
        if (auto key = row.remove(plan.foreign_key)) {
            buckets.try_emplace(graph_identity_key(r, *key))
                .first->second.emplace_back(std::move(row));
        }
    }
    for (std::size_t index = 0; index < parents.size(); ++index) {
        auto& parent = parents[index];
        auto value = default_aggregate_value(r, aggregate.single_result);
        if (auto* key = keys[index].get(plan.local_key)) {
            if (auto it = buckets.find(graph_identity_key(r, *key)); it != buckets.end()) {
                if (aggregate.single_result) {
                    const auto& row = it->second.front();
                    value = row.len() == 1 ? clone_in(r, row.values().front())
                                           : Value(r, row.into_map(r));
                } else {
                    Value::List list(r);
                    for (const auto& row : it->second) {
                        list.emplace_back(row.into_map(r));
                    }
                    value = Value(r, list);
                }
            }
        }
        parent.insert(aggregate.alias, value);
    }
}

namespace {
auto index_rows(Memory r, std::span<const CompactRow> rows) {
    std::pmr::map<String, CompactRow, std::less<>> index(r);
    for (const auto& row : rows) {
        if (auto* id = row.get("id")) {
            index.insert_or_assign(graph_identity_key(r, *id), clone_in(r, row));
        }
    }
    return index;
}
}

void attach_object_group_rows(std::span<CompactRow> parents,
                              const ObjectGroupBy& group,
                              std::span<const CompactRow> rows) {
    auto r = group.memory_resource();
    auto index = index_rows(r, rows);
    for (auto& parent : parents) {
        if (auto* key = parent.get(group.storage_field)) {
            auto it = index.find(graph_identity_key(r, *key));
            (void)parent.take_loaded_relation(group.property_name);
            parent.insert(group.property_name,
                          it == index.end() ? Value(r) : Value(r, it->second.into_map(r)));
            if (it != index.end()) {
                Vector<CompactRow> selected(r);
                selected.emplace_back(it->second);
                SmartList<CompactRow> loaded(std::allocator_arg, Allocator(r), std::move(selected));
                parent.set_loaded_relation(group.property_name, loaded);
            }
        }
    }
}

Result<std::monostate, DynamicFieldError>
attach_child_query_rows(std::span<CompactRow> parents, std::span<const CompactRow> rows) {
    auto r = rows.empty() ? (parents.empty() ? nullptr : parents.front().memory_resource())
                          : rows.front().memory_resource();
    if (parents.empty()) {
        return Result<std::monostate, DynamicFieldError>::ok(r, {});
    }
    r = parents.front().memory_resource();
    auto index = index_rows(r, rows);
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
    for (auto& parent : parents) {
        if (auto* key = parent.get("id")) {
            if (auto it = index.find(graph_identity_key(r, *key)); it != index.end()) {
                auto merged = parent.try_extend(it->second, shapes);
                if (!merged) {
                    return merged;
                }
            }
        }
    }
    return Result<std::monostate, DynamicFieldError>::ok(r, {});
}
}
