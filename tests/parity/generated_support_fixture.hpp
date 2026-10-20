#pragma once
#include "fixture_query_keys.hpp"
#include "debug_modes.hpp"
#include <teaql/runtime/generated_support.hpp>
#include <teaql/runtime/purposed_query_debug.hpp>
#include <teaql/core/detail/query_debug.hpp>

namespace support_fixture {
static request::QuerySelection selection(Memory, const Json&);

static request::QueryOptions options(Memory r, const Json& input) {
    request::QueryOptions result(std::allocator_arg, Allocator(r));
    if (auto* value = optional(input, "comment")) {
        result.comment.emplace(text(*value), r);
    }
    if (auto* value = optional(input, "raw_sql")) {
        result.raw_sql.emplace(text(*value), r);
    }
    for (const auto& value : items(input, "raw_criteria")) {
        result.raw_sql_search_criteria.emplace_back(text(value));
    }
    for (const auto& value : items(input, "dynamic")) {
        result.dynamic_properties.emplace_back(
            text(field(value, "alias")),
            request::UnsafeRawSqlSegment::trusted(r, text(field(value, "sql"))));
    }
    for (const auto& value : items(input, "raw_projection")) {
        result.raw_projections.emplace_back(
            text(field(value, "alias")),
            request::UnsafeRawSqlSegment::trusted(r, text(field(value, "sql"))));
    }
    for (const auto& value : items(input, "object_groups")) {
        result.object_group_bys.emplace_back(text(field(value, "property")),
                                             text(field(value, "storage")),
                                             selection(r, field(value, "selection")));
    }
    for (const auto& value : items(input, "aggregates")) {
        result.relation_aggregates.emplace_back(text(field(value, "relation")),
                                                text(field(value, "alias")),
                                                selection(r, field(value, "selection")),
                                                std::get<bool>(field(value, "single").data));
    }
    for (const auto& value : items(input, "facets")) {
        result.facets.emplace_back(text(field(value, "name")),
                                   text(field(value, "relation")),
                                   selection(r, field(value, "selection")),
                                   std::get<bool>(field(value, "all").data));
    }
    return result;
}

static request::QuerySelection selection(Memory r, const Json& input) {
    request::QuerySelection result(
        std::allocator_arg, Allocator(r), key_query(r, field(input, "query")));
    auto* settings = optional(input, "options");
    if (settings) {
        result.query_options = options(r, *settings);
    }
    for (const auto& child : items(input, "children")) {
        result.child_enhancements.emplace_back(selection(r, child));
    }
    for (const auto& relation : items(input, "selections")) {
        result.relation_selections.emplace_back(text(field(relation, "name")),
                                                selection(r, field(relation, "selection")));
    }
    for (const auto& relation : items(input, "filters")) {
        result.relation_filters.emplace_back(text(field(relation, "name")),
                                             selection(r, field(relation, "selection")));
    }
    return result;
}

static Json trace(Memory r, std::span<const TraceNode> nodes) {
    Json::Array output(r);
    for (const auto& node : nodes) {
        auto out = detail::model_json(r);
        detail::model_put(out, "kind", unsigned(node.kind));
        detail::model_put(out, "entity", node.entity_type);
        detail::model_put(out, "id", node.entity_id ? Json(r, *node.entity_id) : Json(r));
        detail::model_put(out, "comment", node.comment);
        output.emplace_back(std::move(out));
    }
    return Json(r, std::move(output));
}

static Json aggregates(Memory r, std::span<const RelationAggregate> values) {
    Json::Array output(r);
    for (const auto& value : values) {
        auto out = detail::model_json(r);
        detail::model_put(out, "relation", value.relation_name);
        detail::model_put(out, "alias", value.alias);
        detail::model_put(out, "single", value.single_result);
        detail::model_put(out, "query", detail::debug_query(r, value.query));
        output.emplace_back(std::move(out));
    }
    return Json(r, std::move(output));
}

static Json error(Memory r, const RuntimeError& value) {
    auto result = detail::model_json(r);
    auto kind = value.kind() == RuntimeError::Kind::Graph           ? "Graph"
                : value.kind() == RuntimeError::Kind::Policy        ? "Policy"
                : value.kind() == RuntimeError::Kind::RequestIntent ? "RequestIntent"
                : value.kind() == RuntimeError::Kind::MissingEntity ? "MissingEntity"
                                                                    : "MissingRelation";
    detail::model_put(result, "kind", kind);
    detail::model_put(result, "text", value.to_string(r));
    return result;
}

static Json list(Memory r, const SmartList<CompactRow>& value) {
    auto result = detail::model_json(r);
    Json::Array rows(r);
    for (const auto& row : value.data) {
        rows.emplace_back(compact_row_to_json_value(r, row));
    }
    detail::model_put(result, "rows", rows);
    detail::model_put(result, "loaded", value.is_loaded);
    detail::model_put(result, "total", value.total_count ? Json(r, *value.total_count) : Json(r));
    detail::model_put(result, "aggregations", record_to_json_value(r, value.aggregations));
    detail::model_put(result, "summary", record_to_json_value(r, value.summary));
    auto nested = detail::model_json(r);
    for (const auto& [name, child] : value.facets) {
        detail::model_put(nested, name, list(r, child));
    }
    detail::model_put(result, "facets", nested);
    return result;
}

static Json facets(Memory r, const FacetResults& values) {
    auto result = detail::model_json(r);
    for (const auto& [name, value] : values) {
        detail::model_put(result, name, list(r, value));
    }
    return result;
}

class Runtime final : public TeaqlRuntime {
public:
    using allocator_type = Allocator;
    const UserContext& context;
    mutable Json::Array calls;
    std::optional<std::size_t> pending_at, fail_at;
    bool gate = false;
    mutable unsigned cancelled = 0;

    Runtime(std::allocator_arg_t, Allocator a, const UserContext& context)
        : context(context), calls(a.resource()) {}

    const UserContext& user_context() const override {
        return context;
    }

    struct Work {
        const Runtime& runtime;
        std::string_view entity;
        std::size_t index;

        std::optional<Result<SmartList<CompactRow>, RuntimeError>> poll(WakeSignal&) {
            if (runtime.pending_at == index && !runtime.gate) {
                return {};
            }
            auto r = runtime.context.memory_resource();
            using R = Result<SmartList<CompactRow>, RuntimeError>;
            if (runtime.fail_at == index) {
                return R::err(r, RuntimeError::policy(r, "facet denied"));
            }
            Record record(r);
            record.emplace(String("call", r), Value(r, std::uint64_t(index)));
            Value::List payload(r);
            payload.emplace_back("retained");
            record.emplace(String("payload", r), Value(r, payload));
            SmartList<CompactRow> rows(std::allocator_arg, Allocator(r));
            rows.data.emplace_back(record);
            rows.is_loaded = true;
            rows.total_count = index + 10;
            rows.aggregations.emplace(String("count", r), Value(r, std::uint64_t(index + 1)));
            rows.summary.emplace(String("name", r), Value(r, entity));
            auto& nested = rows.facets.try_emplace(String("nested", r)).first->second;
            nested.is_loaded = true;
            return R::ok(r, std::move(rows));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            ++runtime.cancelled;
        }
    };

    AsyncOperation<Result<SmartList<CompactRow>, RuntimeError>>&
    fetch_facet_smart_list(std::string_view entity,
                           const PurposedSelectQuery& query,
                           std::span<const RelationAggregate> relation_aggregates,
                           std::span<const TraceNode> chain) const override {
        auto r = context.memory_resource();
        auto index = calls.size();
        auto call = detail::model_json(r);
        detail::model_put(call, "entity", entity);
        detail::model_put(call, "query", detail::debug_query(r, query.as_query()));
        detail::model_put(call, "diagnostic", formatted_debug(r, query));
        detail::model_put(call, "aggregates", aggregates(r, relation_aggregates));
        detail::model_put(call, "trace", trace(r, chain));
        calls.emplace_back(std::move(call));
        return make_operation<Result<SmartList<CompactRow>, RuntimeError>, Work>(
            r, Work{*this, entity, index});
    }
};
}
