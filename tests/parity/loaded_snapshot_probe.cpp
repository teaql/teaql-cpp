#include <teaql/core/eval.hpp>
#include <teaql/core/row.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/generated_checker.hpp>
#include "../generated_relations/include/cpp_relations.hpp"
#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include <teaql/core/request_intent.hpp>
#include <teaql/runtime/error_debug.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

static Vector<std::string_view> strings(Memory r, const Json& array) {
    Vector<std::string_view> out(r);
    for (const auto& item : std::get<Json::Array>(array.data)) {
        out.push_back(text(item));
    }
    return out;
}

static Result<DynamicPropertyDefinitions, String> schema(Memory r, const Json& array) {
    Vector<DynamicPropertyDefinitions::Field> fields(r);
    for (const auto& item : std::get<Json::Array>(array.data)) {
        const auto& pair = std::get<Json::Array>(item.data);
        fields.emplace_back(text(pair[0]), DataType(std::get<std::uint64_t>(pair[1].data)));
    }
    return DynamicPropertyDefinitions::from_fields(r, fields);
}

static Json schema_types(Memory r, const DynamicPropertyDefinitions& definitions) {
    Json::Array array(r);
    for (const auto& [name, type] : definitions.types()) {
        Json::Array pair(r);
        pair.emplace_back(name);
        pair.emplace_back(std::uint64_t(type));
        array.emplace_back(std::move(pair));
    }
    return Json(r, std::move(array));
}

static void put(Json::Object& out, std::string_view key, Json input) {
    out.emplace(String(key, out.get_allocator().resource()), std::move(input));
}

static Json schema_probe(Memory r, const Json& input) {
    Json::Object out(r);
    auto left = schema(r, field(input, "fields"));
    if (!left) {
        put(out, "error", Json(r, left.error()));
        return Json(r, std::move(out));
    }
    put(out, "types", schema_types(r, left.value()));
    Json::Array errors(r);
    for (const auto& item : std::get<Json::Array>(field(input, "values").data)) {
        const auto& data = field(item, "input");
        auto typed =
            text(field(data, "kind")) == "typed_null"
                ? Value(r,
                        TypedNull{DataType(std::get<std::uint64_t>(field(data, "datatype").data))})
                : value(r, data);
        auto result = left.value().validate_value(text(field(item, "name")), typed);
        errors.emplace_back(result ? Json(r) : Json(r, result.error()));
    }
    put(out, "validation_errors", Json(r, std::move(errors)));
    auto right = schema(r, field(input, "merge"));
    Json::Object merged(r);
    if (!right) {
        put(merged, "error", Json(r, right.error()));
    } else {
        auto reuse = DynamicPropertyDefinitions::reusable_merge(r, left.value(), right.value());
        if (!reuse) {
            put(merged, "error", Json(r, reuse.error()));
        } else {
            auto copied = DynamicPropertyDefinitions::merge_copy(r, left.value(), right.value());
            put(merged, "left", Json(r, reuse.value() == &left.value()));
            put(merged, "right", Json(r, reuse.value() == &right.value()));
            put(merged, "types", schema_types(r, copied.value()));
        }
    }
    put(out, "merge", Json(r, std::move(merged)));
    return Json(r, std::move(out));
}

static Json row_probe(UserContext& user_context,
                      const Json& input,
                      const FieldLayout& layout,
                      SnapshotPool& pool) {
    auto r = user_context.memory_resource();
    auto selected = strings(r, field(input, "selected"));
    auto& columns =
        user_context.make<ColumnLayout>(Vector<String>(selected.begin(), selected.end(), r));
    Vector<CompactRow> rows(r);
    for (std::int64_t id : {11, 22}) {
        Vector<Value> values(r);
        for (auto name : selected) {
            values.emplace_back(id);
        }
        // Start with independent shapes, as the source constructor does.
        auto& independent = user_context.make<ColumnLayout>(columns);
        rows.emplace_back(independent, std::move(values));
    }
    Record sparse(r);
    sparse.emplace(String("id", r), Value(r, std::int64_t{33}));
    rows.emplace_back(std::move(sparse));
    CompactRow::share_layouts(rows);
    if (auto* marked = optional(input, "relation_loaded"); marked && std::get<bool>(marked->data)) {
        RelationShapeCache shapes(std::allocator_arg, Allocator(r));
        rows[0].mark_relation_loaded("lines", shapes);
        rows[1].mark_relation_loaded("lines", shapes);
    }

    auto first = rows[0].indexed_load_state(pool, layout);
    auto second = rows[1].indexed_load_state(pool, layout);
    auto sparse_state = rows[2].indexed_load_state(pool, layout);
    auto& other_layout = user_context.make<FieldLayout>(layout);
    auto other = rows[0].indexed_load_state(pool, other_layout);
    auto repeated = rows[1].indexed_load_state(pool, other_layout);
    auto lookups = strings(r, field(input, "lookups"));
    auto availability = [&](const eval::LoadState& state) {
        Json::Array values(r);
        for (auto name : lookups) {
            values.emplace_back(state.is_loaded(name));
        }
        return Json(r, std::move(values));
    };
    auto changed = detail::clone_in(r, first);
    changed.mark_unloaded("id", &pool).value();
    Json::Object out(r);
    put(out, "shared_shape", Json(r, rows[0].shared_layout() == rows[1].shared_layout()));
    put(out, "sparse_shape_same", Json(r, rows[0].shared_layout() == rows[2].shared_layout()));
    put(out,
        "shared_state",
        Json(r, first.indexed_snapshot()->identity() == second.indexed_snapshot()->identity()));
    put(out,
        "other_shared",
        Json(r, other.indexed_snapshot()->identity() == repeated.indexed_snapshot()->identity()));
    put(out,
        "other_distinct",
        Json(r, first.indexed_snapshot()->identity() != other.indexed_snapshot()->identity()));
    put(out, "initial", availability(first));
    put(out, "changed", availability(changed));
    put(out, "unchanged", availability(second));
    put(out, "sparse", availability(sparse_state));
    bool isolated = true;
    if (!selected.empty()) {
        rows[0].insert(selected.front(), Value(r, std::int64_t{99}));
        isolated = rows[1].get(selected.front())->try_i64() == 22;
    }
    put(out, "value_isolated", Json(r, isolated));
    return Json(r, std::move(out));
}

static Json snapshot_probe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto count = std::size_t(std::get<std::uint64_t>(field(input, "count").data));
    std::vector<std::string> names;
    names.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        names.push_back(i == 0 ? "id" : i == 1 ? "version" : "field_" + std::to_string(i));
    }
    Vector<FieldLayout::Index> indexes(r);
    Vector<FieldLayout::Mapping> mappings(r);
    Vector<std::string_view> expected(r);
    for (std::size_t i = 0; i < count; ++i) {
        indexes.emplace_back(names[i], i);
        mappings.emplace_back(names[i], names[i], names[i]);
        expected.push_back(names[i]);
    }
    const std::array<std::string_view, 1> relations{"lines"};
    auto created =
        FieldLayout::from_generated(r, "Item", "v1", indexes, mappings, relations, expected);
    auto& layout = user_context.make<FieldLayout>(std::move(created).value());
    auto& pool = user_context.make<SnapshotPool>();
    if (text(field(input, "kind")) == "row") {
        return row_probe(user_context, input, layout, pool);
    }
    auto selected = strings(r, field(input, "selected"));
    auto build = [&] {
        return std::get<bool>(field(input, "full").data)
                   ? LoadedSnapshot::fully_loaded(r, layout).into_shared(pool)
                   : LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
    };
    auto first = build(), duplicate = build(), current = first;
    Json::Array actions(r);
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        const auto& parts = std::get<Json::Array>(action.data);
        auto next = LoadedSnapshot::with_loaded(
            pool, current, text(parts[0]), std::get<bool>(parts[1].data));
        Json::Object result(r);
        if (!next) {
            put(result, "error", Json(r, next.error()));
        } else {
            put(result, "same_previous", Json(r, current.identity() == next.value().identity()));
            put(result, "same_initial", Json(r, first.identity() == next.value().identity()));
            put(result,
                "shared_overflow",
                Json(r,
                     current.get().overflow() &&
                         current.get().overflow() == next.value().get().overflow()));
            put(result,
                "shared_names",
                Json(r,
                     current.get().selected_names() &&
                         current.get().selected_names() == next.value().get().selected_names()));
            current = next.value();
        }
        actions.emplace_back(std::move(result));
    }
    const auto& declared = field(input, "schema");
    if (!std::holds_alternative<std::monostate>(declared.data)) {
        auto definitions = schema(r, declared);
        if (!definitions) {
            Json::Object error(r);
            put(error, "schema_error", Json(r, definitions.error()));
            return Json(r, std::move(error));
        }
        auto& managed =
            user_context.make<DynamicPropertyDefinitions>(std::move(definitions).value());
        current = LoadedSnapshot::with_dynamic_property_definitions(pool, current, managed);
    }
    const auto& state = current.get();
    Json::Object out(r);
    put(out, "same_initial_build", Json(r, first.identity() == duplicate.identity()));
    put(out, "actions", Json(r, std::move(actions)));
    put(out, "bits", Json(r, std::to_string(state.bits())));
    Json::Array overflow(r), selected_names(r), loaded(r);
    if (auto* indexes = state.overflow()) {
        for (auto index : *indexes) {
            overflow.emplace_back(std::uint64_t(index));
        }
    }
    if (auto* selections = state.selected_names()) {
        for (const auto& name : *selections) {
            selected_names.emplace_back(name);
        }
    }
    for (auto field : strings(r, field(input, "lookups"))) {
        loaded.emplace_back(state.is_loaded(field));
    }
    put(out, "overflow", Json(r, std::move(overflow)));
    put(out, "overflow_present", Json(r, state.overflow() != nullptr));
    put(out, "selected_names", Json(r, std::move(selected_names)));
    put(out, "names_present", Json(r, state.selected_names() != nullptr));
    put(out, "loaded", Json(r, std::move(loaded)));
    auto* definitions = state.dynamic_property_definitions();
    put(out, "schema", definitions ? schema_types(r, *definitions) : Json(r));
    try {
        eval::LoadState::indexed(r, current).to_json(r);
        put(out, "serialize_error", Json(r));
    } catch (const std::logic_error& error) {
        put(out, "serialize_error", Json(r, error.what()));
    }
    return Json(r, std::move(out));
}

static Json load_state_probe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto count = std::size_t(std::get<std::uint64_t>(field(input, "count").data));
    std::vector<std::string> names;
    names.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        names.push_back(i == 0 ? "id" : i == 1 ? "version" : "field_" + std::to_string(i));
    }
    Vector<FieldLayout::Index> indexes(r);
    Vector<FieldLayout::Mapping> mappings(r);
    Vector<std::string_view> expected(r);
    for (std::size_t i = 0; i < count; ++i) {
        indexes.emplace_back(names[i], i);
        mappings.emplace_back(names[i], names[i], names[i]);
        expected.push_back(names[i]);
    }
    const std::array<std::string_view, 1> relations{"lines"};
    auto created =
        FieldLayout::from_generated(r, "Item", "v1", indexes, mappings, relations, expected);
    auto& layout = user_context.make<FieldLayout>(std::move(created).value());
    auto& pool = user_context.make<SnapshotPool>();
    auto selected = strings(r, field(input, "selected"));
    auto mode = text(field(input, "mode"));
    auto state = [&] {
        if (mode == "full") {
            return eval::LoadState::fully_loaded(r);
        }
        if (mode == "partial") {
            return eval::LoadState::partial(r, selected);
        }
        if (mode == "partial_compact") {
            return eval::LoadState::partial_compact(r, selected);
        }
        if (mode == "shared_columns") {
            Vector<String> column_names(r);
            for (auto name : selected) {
                column_names.emplace_back(name);
            }
            auto& columns = user_context.make<ColumnLayout>(std::move(column_names));
            return eval::LoadState::shared_columns(r, columns);
        }
        if (mode == "indexed") {
            return eval::LoadState::indexed(
                r, LoadedSnapshot::projection(r, layout, selected).into_shared(pool));
        }
        return eval::LoadState::not_loaded(r);
    }();
    auto initial = detail::clone_in(r, state);
    Json::Array errors(r);
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        const auto& pair = std::get<Json::Array>(action.data);
        auto result = std::get<bool>(pair[1].data) ? state.mark_loaded(text(pair[0]), &pool)
                                                   : state.mark_unloaded(text(pair[0]), &pool);
        errors.emplace_back(result ? Json(r) : Json(r, result.error()));
    }
    Json::Array before(r), original(r);
    auto lookups = strings(r, field(input, "lookups"));
    for (auto name : lookups) {
        before.emplace_back(state.is_loaded(name));
        original.emplace_back(initial.is_loaded(name));
    }
    const FieldLayout* conversion = &layout;
    if (std::get<bool>(field(input, "different_layout").data)) {
        auto other =
            FieldLayout::from_generated(r, "Item", "v1", indexes, mappings, relations, expected);
        conversion = &user_context.make<FieldLayout>(std::move(other).value());
    }
    auto indexed = state.into_indexed(pool, *conversion);
    Json::Object out(r);
    put(out, "before", Json(r, std::move(before)));
    put(out, "original", Json(r, std::move(original)));
    put(out, "actions", Json(r, std::move(errors)));
    if (!indexed) {
        put(out, "conversion_error", Json(r, indexed.error()));
    } else {
        put(out, "conversion_error", Json(r));
        Json::Array after(r);
        for (auto name : lookups) {
            after.emplace_back(indexed.value().is_loaded(name));
        }
        put(out, "after", Json(r, std::move(after)));
        try {
            indexed.value().to_json(r);
            put(out, "serialize_error", Json(r));
        } catch (const std::logic_error& error) {
            put(out, "serialize_error", Json(r, error.what()));
        }
    }
    return Json(r, std::move(out));
}

static Json original_probe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto make_row = [&](const Json& object) {
        Record values(r);
        for (const auto& [name, item] : std::get<Json::Object>(object.data)) {
            values.emplace(String(name, r), value(r, item));
        }
        return CompactRow(std::allocator_arg, Allocator(r), std::move(values));
    };
    auto snapshot = [&](const EntityRuntimeState& state) {
        auto row = state.original_snapshot();
        return row ? record_to_json_value(r, row->fields) : Json(r);
    };
    auto entity = text(field(input, "entity"));
    EntityKey key(std::allocator_arg, Allocator(r), entity, Value(r, std::uint64_t{7}));
    EntityRuntimeState root(std::allocator_arg, Allocator(r));
    root.set_original_compact_row(entity, make_row(field(input, "first")));
    auto before = snapshot(root);
    EntityRuntimeState alias(std::allocator_arg, Allocator(r), root);
    auto materialized = alias.original_snapshot();
    materialized->fields.emplace(String("external", r), Value(r, true));
    auto isolated = snapshot(alias);
    root.set_original_compact_row(entity, make_row(field(input, "second")));
    root.set(key, "payload", value(r, field(input, "dirty")));
    Json::Object out(r);
    auto rebound = root.with_loaded_view_from(alias);
    put(out, "rebound", snapshot(rebound));
    put(out, "rebound_dirty", rebound.get(key, "payload")->to_json_value(r));
    put(out, "before", std::move(before));
    put(out, "isolated", std::move(isolated));
    put(out, "alias", snapshot(alias));
    put(out, "root", snapshot(root));
    auto version = alias.get_original_version(key);
    put(out, "alias_version", version ? Json(r, *version) : Json(r));
    version = root.get_original_version(key);
    put(out, "root_version", version ? Json(r, *version) : Json(r));
    put(out, "alias_dirty", alias.get(key, "payload")->to_json_value(r));
    Json::Array names(r);
    for (const auto& name : alias.changed_field_names(key)) {
        names.emplace_back(name);
    }
    put(out, "alias_names", Json(r, std::move(names)));
    return Json(r, std::move(out));
}

static Json checker_helper_probe(Memory r, const Json& input) {
    CheckResults results(r);
    auto location = ObjectLocation::array_root(r, 2);
    location.member("account_info");
    auto name = text(field(input, "field"));
    auto value = text(field(input, "text"));
    GeneratedCheckerLogic::required(
        std::get<bool>(field(input, "required").data), name, location, results);
    std::optional<std::int64_t> present;
    if (std::get<bool>(field(input, "optional").data)) {
        present = 0;
    }
    GeneratedCheckerLogic::requiredOption(present, name, location, results);
    GeneratedCheckerLogic::requiredText(value, name, location, results);
    GeneratedCheckerLogic::minStringLength(
        value, name, std::get<std::uint64_t>(field(input, "min").data), location, results);
    GeneratedCheckerLogic::maxStringLength(
        value, name, std::get<std::uint64_t>(field(input, "max").data), location, results);
    Json::Array out(r);
    for (auto profile : {JsonFieldNamingProfile::CamelCase,
                         JsonFieldNamingProfile::SnakeCase,
                         JsonFieldNamingProfile::PascalCase}) {
        Json::Array errors(r);
        for (const auto& result : results) {
            errors.push_back(result.to_wire(r, profile).to_json(r));
        }
        out.emplace_back(std::move(errors));
    }
    return Json(r, std::move(out));
}

static Json mutation_envelope_probe(Memory r, const Json& input) {
    auto integer = [](const Json& value) -> std::uint64_t {
        if (auto* number = std::get_if<std::uint64_t>(&value.data)) {
            return *number;
        }
        return static_cast<std::uint64_t>(std::get<std::int64_t>(value.data));
    };
    auto make = [&](auto&& command) {
        if constexpr (!std::same_as<std::remove_cvref_t<decltype(command)>,
                                    MutationRequest::Batch>) {
            for (const auto& node : std::get<Json::Array>(field(input, "trace").data)) {
                std::optional<std::uint64_t> id;
                if (!std::holds_alternative<std::monostate>(field(node, "id").data)) {
                    id = integer(field(node, "id"));
                }
                command.trace_chain.emplace_back(
                    static_cast<TraceKind>(integer(field(node, "kind"))),
                    text(field(node, "entity")),
                    text(field(node, "comment")),
                    id);
            }
        }
        auto* comment = std::get_if<String>(&field(input, "comment").data);
        auto reason = comment ? std::string_view(*comment) : std::string_view{};
        auto* via_command = optional(input, "via_command");
        if (via_command && std::get<bool>(via_command->data)) {
            MutationCommand staged(
                std::allocator_arg, Allocator(r), std::forward<decltype(command)>(command));
            return std::move(staged).request(r, reason);
        }
        return MutationRequest::create(r, std::forward<decltype(command)>(command), reason);
    };
    auto command = text(field(input, "command"));
    auto entity = text(field(input, "entity"));
    auto request = [&]() -> Result<MutationRequest, RequestIntentError> {
        if (command == "insert") {
            InsertCommand mutation(std::allocator_arg, Allocator(r), entity);
            mutation.value("id", value(r, field(input, "id")));
            return make(std::move(mutation));
        }
        if (command == "update") {
            return make(UpdateCommand(
                std::allocator_arg, Allocator(r), entity, value(r, field(input, "id"))));
        }
        if (command == "delete") {
            return make(DeleteCommand(
                std::allocator_arg, Allocator(r), entity, value(r, field(input, "id"))));
        }
        if (command == "recover") {
            return make(RecoverCommand(
                std::allocator_arg, Allocator(r), entity, value(r, field(input, "id")), -1));
        }
        return make(MutationRequest::Batch(r));
    }();
    Json::Object out(r);
    if (!request) {
        put(out, "field", Json(r, request.error().field));
        put(out, "code", Json(r, request.error().code()));
        put(out, "display", Json(r, request.error().to_string(r)));
        return Json(r, std::move(out));
    }
    auto trace = [&](std::span<const TraceNode> chain) {
        Json::Array result(r);
        for (const auto& node : chain) {
            Json::Object value(r);
            put(value, "kind", Json(r, std::int64_t(node.kind)));
            put(value, "entity", Json(r, node.entity_type));
            put(value, "id", node.entity_id ? Json(r, *node.entity_id) : Json(r));
            put(value, "comment", Json(r, node.comment));
            result.emplace_back(std::move(value));
        }
        return Json(r, std::move(result));
    };
    const auto& result = request.value();
    put(out, "comment", Json(r, result.comment()));
    put(out, "trace", trace(result.trace_chain()));
    auto canonical = result.execution_trace_chain();
    put(out, "execution_trace", trace(canonical));
    put(out, "intent_debug", Json(r, debug_string(r, result.intent())));
    put(out, "redactions", Json(r, result.diagnostic_redactions() != nullptr));
    return Json(r, std::move(out));
}

static Json request_envelope_probe(Memory r, const Json& input) {
    auto integer = [](const Json& value) -> std::uint64_t {
        if (auto* number = std::get_if<std::uint64_t>(&value.data)) {
            return *number;
        }
        return static_cast<std::uint64_t>(std::get<std::int64_t>(value.data));
    };
    SelectQuery query(std::allocator_arg, Allocator(r), text(field(input, "entity")));
    auto optional_text = [&](std::string_view name) -> std::optional<std::string_view> {
        if (auto* value = std::get_if<String>(&field(input, name).data)) {
            return *value;
        }
        return std::nullopt;
    };
    if (auto value = optional_text("comment")) {
        query.comment_text.emplace(*value, r);
    }
    if (auto value = optional_text("purpose")) {
        query.purpose_text.emplace(*value, r);
    }
    for (const auto& node : std::get<Json::Array>(field(input, "trace").data)) {
        std::optional<std::uint64_t> id;
        if (!std::holds_alternative<std::monostate>(field(node, "id").data)) {
            id = static_cast<std::uint64_t>(integer(field(node, "id")));
        }
        query.trace_chain.emplace_back(static_cast<TraceKind>(integer(field(node, "kind"))),
                                       text(field(node, "entity")),
                                       text(field(node, "comment")),
                                       id);
    }
    const bool debug = std::get<bool>(field(input, "debug").data);
    const bool metadata = std::get<bool>(field(input, "metadata").data);
    auto request = [&]() -> Result<QueryRequest, RequestIntentError> {
        if (!std::get<bool>(field(input, "explicit").data)) {
            return QueryRequest::from_query(r, query, debug, metadata);
        }
        auto intent = QueryIntent::from_optional(
            r, optional_text("intent_comment"), optional_text("intent_purpose"));
        if (!intent) {
            return Result<QueryRequest, RequestIntentError>::err(r, std::move(intent).error());
        }
        return Result<QueryRequest, RequestIntentError>::ok(
            r,
            QueryRequest(std::allocator_arg, Allocator(r), query, intent.value(), debug, metadata));
    }();
    Json::Object out(r);
    if (!request) {
        put(out, "field", Json(r, request.error().field));
        put(out, "code", Json(r, request.error().code()));
        put(out, "display", Json(r, request.error().to_string(r)));
        return Json(r, std::move(out));
    }
    auto trace = [&](const TraceChain& chain) {
        Json::Array result(r);
        for (const auto& node : chain) {
            Json::Object value(r);
            put(value, "kind", Json(r, std::int64_t(node.kind)));
            put(value, "entity", Json(r, node.entity_type));
            put(value, "id", node.entity_id ? Json(r, *node.entity_id) : Json(r));
            put(value, "comment", Json(r, node.comment));
            result.emplace_back(std::move(value));
        }
        return Json(r, std::move(result));
    };
    const auto& value = request.value();
    put(out, "comment", Json(r, value.comment()));
    put(out, "purpose", Json(r, value.purpose()));
    put(out,
        "query_comment",
        value.query.comment_text ? Json(r, *value.query.comment_text) : Json(r));
    put(out,
        "query_purpose",
        value.query.purpose_text ? Json(r, *value.query.purpose_text) : Json(r));
    put(out, "trace", trace(value.trace_chain));
    put(out, "execution_trace", trace(value.execution_trace_chain()));
    put(out, "debug", Json(r, value.capture_debug_query));
    put(out, "metadata", Json(r, value.capture_execution_metadata));
    return Json(r, std::move(out));
}

static Json request_intent_probe(Memory r, const Json& input) {
    auto optional_text = [&](std::string_view name) -> std::optional<std::string_view> {
        if (auto* value = std::get_if<String>(&field(input, name).data)) {
            return *value;
        }
        return std::nullopt;
    };
    auto error = [&](const RequestIntentError& value) {
        auto runtime = RuntimeError::request_intent(r, value);
        Json::Object out(r);
        put(out, "field", Json(r, value.field));
        put(out, "code", Json(r, value.code()));
        put(out, "display", Json(r, value.to_string(r)));
        put(out, "debug", Json(r, debug_string(r, value)));
        put(out, "runtime_display", Json(r, runtime.to_string(r)));
        put(out, "runtime_debug", Json(r, debug_string(r, runtime)));
        return Json(r, std::move(out));
    };
    Json::Object out(r);
    auto query = QueryIntent::from_optional(r, optional_text("comment"), optional_text("purpose"));
    if (query) {
        Json::Object value(r);
        put(value, "comment", Json(r, query.value().comment()));
        put(value, "purpose", Json(r, query.value().purpose()));
        put(value, "debug", Json(r, debug_string(r, query.value())));
        put(value, "clone_equal", Json(r, detail::clone_in(r, query.value()) == query.value()));
        put(out, "query", Json(r, std::move(value)));
    } else {
        put(out, "query", error(query.error()));
    }
    auto mutation = MutationIntent::from_optional(r, optional_text("comment"));
    if (mutation) {
        Json::Object value(r);
        put(value, "comment", Json(r, mutation.value().comment()));
        put(value, "debug", Json(r, debug_string(r, mutation.value())));
        put(value,
            "clone_equal",
            Json(r, detail::clone_in(r, mutation.value()) == mutation.value()));
        put(out, "mutation", Json(r, std::move(value)));
    } else {
        put(out, "mutation", error(mutation.error()));
    }
    RequestIntentError extra(std::allocator_arg,
                             Allocator(r),
                             std::get<bool>(field(input, "extra_query").data)
                                 ? RequestKind::Query
                                 : RequestKind::Mutation,
                             text(field(input, "extra_field")));
    put(out, "extra", error(extra));
    return Json(r, std::move(out));
}

static Json trace_tokens_probe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Vector<const TraceScopeToken*> tokens(r);
    Json::Array out(r);
    for (const auto& item : std::get<Json::Array>(field(input, "nodes").data)) {
        const TraceScopeToken* parent = nullptr;
        if (auto* index = std::get_if<std::uint64_t>(&field(item, "parent").data)) {
            parent = tokens.at(*index);
        }
        std::optional<std::uint64_t> id;
        if (auto* value = std::get_if<std::uint64_t>(&field(item, "id").data)) {
            id = *value;
        }
        TraceNode track(std::allocator_arg,
                        Allocator(r),
                        TraceKind::AuditReason,
                        text(field(item, "entity")),
                        text(field(item, "comment")),
                        id);
        auto& token = user_context.make<TraceScopeToken>(parent, track, tokens.size());
        tokens.push_back(&token);
        Json::Array chain(r);
        for (const auto& node : token.recover_trace_chain(r)) {
            Json::Object value(r);
            put(value, "entity", Json(r, node.entity_type));
            put(value, "id", node.entity_id ? Json(r, *node.entity_id) : Json(r));
            put(value, "comment", Json(r, node.comment));
            chain.emplace_back(std::move(value));
        }
        out.emplace_back(std::move(chain));
    }
    return Json(r, std::move(out));
}

static Json business_id_probe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& date = user_context.make<BusinessDate>(BusinessDate{*Date::from_ymd(2026, 10, 11)});
    user_context.insert_resource(user_context, date);
    if (auto* root = std::get_if<std::uint64_t>(&field(input, "root").data)) {
        user_context.with_active_root("Tenant", *root);
    }
    if (std::get<bool>(field(input, "service").data)) {
        auto& allocator = user_context.make<InMemoryBusinessIdAllocator>();
        auto& service = user_context.make<BusinessIdService>(user_context, allocator);
        if (std::get<bool>(field(input, "key").data)) {
            std::array<std::uint8_t, 32> bytes{};
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                bytes[i] = static_cast<std::uint8_t>(i);
            }
            auto key = BusinessIdEncodingKey::create(r, 1, bytes);
            auto& provider = user_context.make<StaticBusinessIdKeyProvider>(key.value());
            service.with_key_provider(user_context, provider);
        }
        user_context.set_business_id_service(user_context, service);
    }
    auto evaluate = [&]<class Model, class Number, class Checker>(std::type_identity<Model>,
                                                                  std::type_identity<Number>,
                                                                  std::type_identity<Checker>,
                                                                  std::string_view name,
                                                                  auto getter,
                                                                  auto setter) {
        auto definition = Number::definition(r);
        auto value = Number::create(r, text(field(input, "text")));
        Json::Object factory(r);
        if (value) {
            put(factory, "ok", Json(r, value.value().asStr()));
            put(factory, "debug", Json(r, debug_string(r, value.value())));
        } else {
            put(factory, "error", Json(r, value.error()));
        }
        Record row(r);
        row.emplace(String("id", r), Value(r, std::uint64_t{7}));
        row.emplace(String("name", r), Value(r, "entry"));
        row.emplace(String("tenant_id", r),
                    Value(r, std::get<std::uint64_t>(field(input, "tenant").data)));
        row.emplace(String("version", r), Value(r, std::int64_t{1}));
        row.emplace(String(name, r), Value(r, text(field(input, "before"))));
        auto decoded = Model::from_compact_row(
            r, CompactRow(std::allocator_arg, Allocator(r), std::move(row)));
        auto& model = user_context.make<Model>(decoded.value());
        if (!std::get<bool>(field(input, "loaded").data)) {
            const std::array<std::string_view, 1> fields{"id"};
            model.load_state =
                eval::LoadState::partial(r, fields)
                    .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                                  Model::field_layout(r))
                    .value();
        }
        if (std::get<bool>(field(input, "dirty").data)) {
            auto before = text(field(input, "before"));
            auto number = Number::create(r, before.empty() ? "dirty" : before);
            std::invoke(setter, model, number.value());
        }
        CheckResults results(r);
        auto status = std::get<bool>(field(input, "update").data) ? CheckObjectStatus::Update
                                                                  : CheckObjectStatus::Create;
        auto location = ObjectLocation::array_root(r, 2);
        location.member("account_info");
        auto& checker = user_context.make<Checker>();
        checker.check_and_fix_typed(user_context, model, status, location, results);
        Json::Array diagnostics(r);
        for (auto profile : {JsonFieldNamingProfile::CamelCase,
                             JsonFieldNamingProfile::SnakeCase,
                             JsonFieldNamingProfile::PascalCase}) {
            Json::Array errors(r);
            for (const auto& result : results) {
                errors.emplace_back(result.to_wire(r, profile).to_json(r));
            }
            diagnostics.emplace_back(std::move(errors));
        }
        Json::Object config(r);
        put(config, "field", Json(r, definition.field_name));
        put(config, "profile", Json(r, definition.profile));
        put(config, "prefix", Json(r, definition.prefix));
        put(config, "namespace", Json(r, definition.namespace_name));
        put(config, "split", Json(r, definition.split_by_date));
        put(config, "digits", Json(r, std::uint64_t(definition.preserve_digits)));
        put(config, "separator", Json(r, definition.separator));
        Json::Object out(r);
        put(out, "factory", Json(r, std::move(factory)));
        put(out, "definition", Json(r, std::move(config)));
        auto number = std::invoke(getter, model);
        put(out, "value", Json(r, number.asStr()));
        put(out, "loaded", Json(r, model.is_loaded(name)));
        put(out, "tenant", Json(r, model.tenantId()));
        put(out, "diagnostics", Json(r, std::move(diagnostics)));
        return Json(r, std::move(out));
    };
    using namespace cpp_business_ids;
    if (text(field(input, "entity")) == "receipt") {
        return evaluate(std::type_identity<Receipt>{},
                        std::type_identity<ReceiptNumber>{},
                        std::type_identity<ReceiptChecker>{},
                        "receipt_number",
                        &Receipt::receiptNumber,
                        &Receipt::updateReceiptNumber);
    }
    return evaluate(std::type_identity<Ticket>{},
                    std::type_identity<TicketNumber>{},
                    std::type_identity<TicketChecker>{},
                    "ticket_number",
                    &Ticket::ticketNumber,
                    &Ticket::updateTicketNumber);
}

static Json generated_relations_probe(UserContext& user_context, const Json& input) {
    using cpp_relations::Platform;
    using cpp_relations::School;
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    auto& school = user_context.make<School>();
    school.root = detail::clone_in(r, root);
    school.updateId(1).updatePlatformId(std::get<std::uint64_t>(field(input, "foreign_id").data));
    Vector<std::string_view> selected(r);
    selected.emplace_back("id");
    selected.emplace_back("platform_id");
    if (std::get<bool>(field(input, "selected").data)) {
        selected.emplace_back("platform");
    }
    school.load_state = eval::LoadState::partial(r, selected)
                            .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                                          School::field_layout(r))
                            .value();
    auto& platform = user_context.make<Platform>();
    platform.updateId(9).updateName("platform");
    auto& owner = user_context.make<Platform>();
    owner.root = detail::clone_in(r, root);
    owner.updateId(9);
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    if (std::get<bool>(field(input, "cached").data)) {
        graph.install(9, detail::clone_in(r, platform));
    }
    auto mode = text(field(input, "forward"));
    if (mode == "present") {
        graph.install_relation_option<Platform>(
            "School", 1, "platform", detail::clone_in(r, platform));
    } else if (mode == "empty") {
        graph.install_relation_option<Platform>("School", 1, "platform", std::nullopt);
    } else if (mode == "wrong_type") {
        graph.install_relation_option<std::uint64_t>("School", 1, "platform", std::uint64_t{9});
    }
    if (auto* count = std::get_if<std::uint64_t>(&field(input, "count").data)) {
        SmartList<School> children(std::allocator_arg, Allocator(r));
        children.loaded();
        for (std::uint64_t i = 0; i < *count; ++i) {
            children.emplace_back().updateId(20 + i);
        }
        graph.install_relation_list("Platform", 9, "school_list", std::move(children));
    }
    if (!root.freeze_graph(std::move(graph))) {
        throw std::logic_error("Probe graph was already frozen");
    }
    auto observe = [&](const auto& result) {
        Json::Object out(r);
        put(out,
            "state",
            Json(r,
                 result.is_value()  ? "Value"
                 : result.is_null() ? "Null"
                                    : "NotLoaded"));
        if (result.is_not_loaded()) {
            put(out, "node", Json(r, result.missing().failed_node));
            put(out, "path", Json(r, result.missing().attempted_path));
        } else if (result.is_value()) {
            const auto& value = result.get().get();
            if constexpr (requires { value.data; }) {
                Json::Array ids(r);
                for (const auto& child : value.data) {
                    ids.emplace_back(child.id());
                }
                put(out, "ids", Json(r, std::move(ids)));
            } else {
                put(out, "id", Json(r, value.id()));
            }
        }
        return Json(r, std::move(out));
    };
    Json::Object out(r);
    put(out, "forward", observe(school.evalPlatform()));
    put(out, "list", observe(owner.evalSchoolList()));
    auto handle = owner.schoolListHandle();
    put(out,
        "handle",
        Json(r,
             handle.state() == LoadedRelation::Loaded  ? "Loaded"
             : handle.state() == LoadedRelation::Empty ? "Empty"
                                                       : "NotLoaded"));
    put(out, "loaded", Json(r, handle.is_loaded()));
    put(out, "empty", Json(r, handle.is_empty()));
    return Json(r, std::move(out));
}

static Json comment_probe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    EntityRuntimeState receiving(std::allocator_arg, Allocator(r)),
        source(std::allocator_arg, Allocator(r));
    if (auto* reason = std::get_if<String>(&field(input, "receiving_reason").data)) {
        receiving.set_comment(*reason);
    }
    if (auto* reason = std::get_if<String>(&field(input, "source_reason").data)) {
        source.set_comment(*reason);
    }
    auto apply = [&](const EntityRuntimeState& root, const Json& list) {
        for (const auto& item : std::get<Json::Array>(list.data)) {
            const auto& parts = std::get<Json::Array>(item.data);
            EntityKey key(std::allocator_arg,
                          Allocator(r),
                          text(parts[0]),
                          Value(r, std::get<std::uint64_t>(parts[1].data)));
            root.set_entity_comment(key, text(parts[2]));
        }
    };
    apply(receiving, field(input, "receiving"));
    apply(source, field(input, "source"));
    source.set(EntityKey(std::allocator_arg, Allocator(r), "Item", Value(r, std::uint64_t{7})),
               "quantity",
               Value(r, std::int64_t{0}));
    receiving.adopt_mutations_from(source);
    auto comments = [&] {
        Json::Array out(r);
        for (const auto& item : std::get<Json::Array>(field(input, "lookups").data)) {
            const auto& pair = std::get<Json::Array>(item.data);
            auto value = receiving.get_entity_comment(
                EntityKey(std::allocator_arg,
                          Allocator(r),
                          text(pair[0]),
                          Value(r, std::get<std::uint64_t>(pair[1].data))));
            out.emplace_back(value ? Json(r, *value) : Json(r));
        }
        return Json(r, std::move(out));
    };
    Json::Object out(r);
    auto reason = receiving.get_comment();
    put(out, "reason", reason ? Json(r, *reason) : Json(r));
    put(out, "comments", comments());
    receiving.clear_committed();
    reason = receiving.get_comment();
    put(out, "after_reason", reason ? Json(r, *reason) : Json(r));
    put(out, "after_comments", comments());
    return Json(r, std::move(out));
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("Invalid snapshot probe JSON");
            }
            auto kind = text(field(*input, "kind"));
            auto output = kind == "mutation_envelope"  ? mutation_envelope_probe(r, *input)
                          : kind == "request_envelope" ? request_envelope_probe(r, *input)
                          : kind == "request_intent"   ? request_intent_probe(r, *input)
                          : kind == "trace_tokens"     ? trace_tokens_probe(*user_context, *input)
                          : kind == "business_id"      ? business_id_probe(*user_context, *input)
                          : kind == "generated_relations"
                              ? generated_relations_probe(*user_context, *input)
                          : kind == "schema"          ? schema_probe(r, *input)
                          : kind == "comments"        ? comment_probe(*user_context, *input)
                          : kind == "checker_helpers" ? checker_helper_probe(r, *input)
                          : kind == "original"        ? original_probe(*user_context, *input)
                          : kind == "load_state"      ? load_state_probe(*user_context, *input)
                                                      : snapshot_probe(*user_context, *input);
            std::cout << output.dump(r) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
