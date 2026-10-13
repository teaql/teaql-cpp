#include <teaql/runtime/relation.hpp>
#include <teaql/runtime/detail/context/entity.hpp>
#include <teaql/core/request.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <algorithm>
#include <charconv>

namespace teaql::detail {
RelationPlansResult build_relation_plans(const UserContext& context,
                                         std::string_view entity,
                                         std::span<const String> loads) {
    auto r = context.memory_resource();
    auto descriptor = context.require_entity(entity);
    if (!descriptor) {
        return RelationPlansResult::err(r, std::move(descriptor).error());
    }
    std::pmr::map<String, Vector<String>, std::less<>> grouped(r);
    for (const auto& load : loads) {
        auto dot = load.find('.');
        auto head = std::string_view(load).substr(0, dot);
        auto [it, inserted] = grouped.try_emplace(String(head, r));
        (void)inserted;
        if (dot != load.npos) {
            it->second.emplace_back(std::string_view(load).substr(dot + 1));
        }
    }
    Vector<RelationLoadPlan> out(r);
    for (const auto& [name, child_loads] : grouped) {
        auto* relation = descriptor.value().get().relation_by_name(name);
        if (!relation) {
            return RelationPlansResult::err(r, RuntimeError::missing_relation(r, entity, name));
        }
        auto children = build_relation_plans(context, relation->target_entity, child_loads);
        if (!children) {
            return RelationPlansResult::err(r, std::move(children).error());
        }
        auto& plan = out.emplace_back(entity, *relation);
        plan.children = std::move(children).value();
    }
    return RelationPlansResult::ok(r, std::move(out));
}

RelationPlansResult build_relation_plans_from_loads(const UserContext& context,
                                                    std::string_view entity,
                                                    std::span<const RelationLoad> loads) {
    auto r = context.memory_resource();
    auto descriptor = context.require_entity(entity);
    if (!descriptor) {
        return RelationPlansResult::err(r, std::move(descriptor).error());
    }
    Vector<RelationLoadPlan> out(r);
    out.reserve(loads.size());
    for (const auto& load : loads) {
        auto* relation = descriptor.value().get().relation_by_name(load.name);
        if (!relation) {
            return RelationPlansResult::err(r,
                                            RuntimeError::missing_relation(r, entity, load.name));
        }
        auto children = build_relation_plans_from_loads(
            context,
            relation->target_entity,
            load.query ? std::span<const RelationLoad>(load.query->relations)
                       : std::span<const RelationLoad>{});
        if (!children) {
            return RelationPlansResult::err(r, std::move(children).error());
        }
        auto& plan = out.emplace_back(entity, *relation);
        if (load.query) {
            plan.query.emplace(std::allocator_arg, Allocator(r), *load.query);
        }
        plan.children = std::move(children).value();
    }
    return RelationPlansResult::ok(r, std::move(out));
}

namespace {
void integer(String& out, auto value) {
    char buffer[32];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    out.append(buffer, result.ptr);
}

String bucket_key(Memory r, const Value& value) {
    String out(r);
    std::visit(
        [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, std::monostate> || std::same_as<T, TypedNull>) {
                out = "null";
            } else if constexpr (std::same_as<T, bool>) {
                out = v ? "b:true" : "b:false";
            } else if constexpr (std::same_as<T, std::int64_t>) {
                out = "i:";
                integer(out, v);
            } else if constexpr (std::same_as<T, std::uint64_t>) {
                out = "u:";
                integer(out, v);
            } else if constexpr (std::same_as<T, double>) {
                out = "f:";
                append_float(out, v, false);
            } else if constexpr (std::same_as<T, String>) {
                out = "t:";
                out += v;
            } else if constexpr (std::same_as<T, Decimal> || std::same_as<T, Date>) {
                out = "d:";
                out += v.to_string(r);
            } else if constexpr (std::same_as<T, Json>) {
                out = "j:";
                out += v.dump(r);
            } else if constexpr (std::same_as<T, Timestamp>) {
                out = "ts:";
                integer(out, v.as_millis());
            } else if constexpr (std::same_as<T, Value::Object>) {
                out = "o";
            } else if constexpr (std::same_as<T, Value::List>) {
                out = "l";
            }
        },
        value.data);
    return out;
}

void ensure_projection(SelectQuery& query, std::string_view field) {
    if (!query.projection.empty() &&
        std::find(query.projection.begin(), query.projection.end(), field) ==
            query.projection.end()) {
        query.projection.emplace_back(field);
    }
}
}

String graph_identity_key(Memory r, const Value& value) {
    if (auto* id = std::get_if<std::int64_t>(&value.data); id && *id >= 0) {
        String out("u:", r);
        integer(out, std::uint64_t(*id));
        return out;
    }
    return bucket_key(r, value);
}

Vector<Value>
unique_relation_values(Memory r, std::span<const CompactRow> rows, std::string_view field) {
    // The source separates numeric nonnegative IDs from all other bucket keys.
    // Equal keys are identities; Rust's unstable sort does not guarantee which
    // interchangeable signed/unsigned representation survives deduplication.
    using Key = std::variant<std::uint64_t, String>;

    struct Entry {
        Key key;
        const Value* value;
        std::size_t index;
    };

    Vector<Entry> sorted(r);
    sorted.reserve(rows.size());
    for (const auto& row : rows) {
        if (auto* value = row.get(field); value && !value->is_null()) {
            if (auto* id = std::get_if<std::uint64_t>(&value->data)) {
                sorted.push_back({Key(*id), value, sorted.size()});
            } else if (auto* id = std::get_if<std::int64_t>(&value->data); id && *id >= 0) {
                sorted.push_back({Key(std::uint64_t(*id)), value, sorted.size()});
            } else {
                sorted.push_back({Key(bucket_key(r, *value)), value, sorted.size()});
            }
        }
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.key != b.key ? a.key < b.key : a.index < b.index;
    });
    Vector<Value> out(r);
    out.reserve(sorted.size());
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        if (i == 0 || sorted[i - 1].key != sorted[i].key) {
            out.emplace_back(*sorted[i].value);
        }
    }
    return out;
}

SelectQuery query_for_relation_plan(const UserContext& context,
                                    const RelationLoadPlan& plan,
                                    std::span<const CompactRow> rows,
                                    bool compact) {
    auto r = context.memory_resource();
    auto ids = unique_relation_values(r, rows, plan.local_key);
    auto query = plan.query ? detail::clone_in(r, *plan.query)
                            : SelectQuery(std::allocator_arg, Allocator(r), plan.target_entity);
    query.entity = plan.target_entity;
    // The hydration planner owns every descendant in both execution modes.
    query.relations.clear();
    query.facets.clear();
    (void)compact;
    ensure_projection(query, plan.foreign_key);
    for (const auto& child : plan.children) {
        ensure_projection(query, child.local_key);
    }
    if (query.slice) {
        if (!query.group_by_fields.empty() || !query.aggregates.empty()) {
            // Group identity determines stable aggregate pagination.
            for (const auto& field : query.group_by_fields) {
                auto found = std::find_if(
                    query.order_by.begin(), query.order_by.end(), [&](const auto& order) {
                        return !order.expr && order.field == field;
                    });
                if (found == query.order_by.end()) {
                    query.order_asc(field);
                }
            }
        } else if (auto* entity = context.entity(plan.target_entity)) {
            if (auto* id = entity->id_property()) {
                auto found = std::find_if(
                    query.order_by.begin(), query.order_by.end(), [&](const auto& order) {
                        return !order.expr && order.field == id->name;
                    });
                if (found == query.order_by.end()) {
                    query.order_asc(id->name);
                }
            }
        }
    }
    if (!ids.empty()) {
        query.and_filter(Expr::in_list(r, plan.foreign_key, ids));
    }
    if (query.slice) {
        query.partition_field.emplace(plan.foreign_key, r);
    }
    return query;
}

void ensure_identity_projection(const UserContext& context, SelectQuery& query) {
    if (query.projection.empty()) {
        return;
    }
    auto* entity = context.entity(query.entity);
    if (!entity) {
        return;
    }
    if (auto* id = entity->id_property()) {
        ensure_projection(query, id->name);
    }
    if (auto* version = entity->version_property()) {
        ensure_projection(query, version->name);
    }
}

Result<SelectQuery, RuntimeError> prepare_select_query(const UserContext& context,
                                                       std::span<const TraceNode> trace,
                                                       const SelectQuery& source) {
    auto r = context.memory_resource();
    using R = Result<SelectQuery, RuntimeError>;
    auto query = detail::clone_in(r, source);
    TraceChain combined(trace.begin(), trace.end(), r);
    for (const auto& node : query.trace_chain) {
        combined.emplace_back(node);
    }
    query.trace_chain.swap(combined);
    if (!is_generated_schema_bootstrap(context)) {
        if (auto behavior = context.entity_data_service_behavior(query.entity)) {
            auto result = behavior->get().before_select(r, context, query);
            if (!result) {
                return R::err(r, std::move(result).error());
            }
        }
    }
    if (auto* policy = ContextEntityAccess::request_policy(context)) {
        auto result = policy->enforce_select(r, context, query);
        if (!result) {
            return R::err(r, std::move(result).error());
        }
    }
    if (!query.relations.empty()) {
        if (auto* entity = context.entity(query.entity)) {
            for (const auto& load : query.relations) {
                if (auto* relation = entity->relation_by_name(load.name)) {
                    if (std::find(query.projection.begin(),
                                  query.projection.end(),
                                  relation->local_key) == query.projection.end()) {
                        query.projection.emplace_back(relation->local_key);
                    }
                }
            }
        }
    }
    return R::ok(r, std::move(query));
}
}
