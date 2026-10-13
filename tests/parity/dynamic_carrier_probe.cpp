#include <teaql/core/list.hpp>
#include <teaql/core/eval.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>
#include <array>

static void put(Json::Object& out, std::string_view key, Json value) {
    out.emplace(String(key, out.get_allocator().resource()), std::move(value));
}

static Record record(Memory r, const Json& input) {
    Record result(r);
    for (const auto& [name, v] : std::get<Json::Object>(input.data)) {
        result.emplace(String(name, r), value(r, v));
    }
    return result;
}

static FieldLayout layout(Memory r, const Json& input) {
    auto count = std::get<std::uint64_t>(field(input, "count").data);
    Vector<String> names(r), columns(r);
    names.reserve(count);
    columns.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        names.emplace_back(i == 0 ? "id" : i == 1 ? "version" : "field_" + std::to_string(i));
        columns.emplace_back("db_" + std::to_string(i));
    }
    Vector<FieldLayout::Index> indexes(r);
    Vector<FieldLayout::Mapping> mappings(r);
    Vector<std::string_view> expected(r);
    for (std::size_t i = 0; i < count; ++i) {
        indexes.emplace_back(names[i], i);
        mappings.emplace_back(names[i], names[i], columns[i]);
        expected.push_back(names[i]);
    }
    const std::array<std::string_view, 1> relations{"lines"};
    return std::move(
               FieldLayout::from_generated(r, "Item", "v1", indexes, mappings, relations, expected))
        .value();
}

static const DynamicPropertyDefinitions* schema(UserContext& user_context, const Json& input) {
    if (std::holds_alternative<std::monostate>(input.data)) {
        return nullptr;
    }
    auto r = user_context.memory_resource();
    Vector<DynamicPropertyDefinitions::Field> fields(r);
    for (const auto& pair : std::get<Json::Array>(input.data)) {
        const auto& p = std::get<Json::Array>(pair.data);
        fields.emplace_back(text(p[0]), DataType(std::get<std::uint64_t>(p[1].data)));
    }
    return &user_context.make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, fields)).value());
}

static DynamicFieldValues dynamic(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    const std::array types{DynamicFieldDefinitions::Field{"extension", DataType::Text},
                           DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto d =
        std::move(DynamicFieldDefinitions::from_fields(r, text(field(input, "owner")), "v1", types))
            .value();
    const auto* executor = optional(input, "executor");
    auto& definitions = executor
                            ? user_context.make<DynamicFieldDefinitions>(d.with_storage_binding(
                                  r, "profile", std::get<std::uint64_t>(executor->data)))
                            : user_context.make<DynamicFieldDefinitions>(std::move(d));
    return std::move(
               DynamicFieldValues::from_values(r, definitions, record(r, field(input, "values"))))
        .value();
}

static Json state(Memory r, const LoadedSnapshot& s, const Json& lookups) {
    Json::Object result(r);
    put(result, "bits", Json(r, std::to_string(s.bits())));
    Json overflow(r), names(r);
    if (const auto* fields = s.overflow()) {
        Json::Array a(r);
        for (auto i : *fields) {
            a.emplace_back(std::uint64_t(i));
        }
        overflow = Json(r, a);
    }
    if (const auto* selected = s.selected_names()) {
        Json::Array a(r);
        for (const auto& n : *selected) {
            a.emplace_back(n);
        }
        names = Json(r, a);
    }
    put(result, "overflow", std::move(overflow));
    put(result, "names", std::move(names));
    const auto* definitions = s.dynamic_property_definitions();
    auto type = definitions ? definitions->data_type("_total") : std::nullopt;
    put(result, "schema", type ? Json(r, std::uint64_t(*type)) : Json(r));
    Json::Array loaded(r);
    for (const auto& n : std::get<Json::Array>(lookups.data)) {
        loaded.emplace_back(s.is_loaded(text(n)));
    }
    put(result, "loaded", Json(r, loaded));
    return Json(r, result);
}

static Json row_state(Memory r,
                      const CompactRow& row,
                      const Json& input,
                      SnapshotPool& pool,
                      const FieldLayout& layout) {
    Json::Object result(r);
    put(result, "map", compact_row_to_json_value(r, row));
    auto fields = row.loaded_dynamic_fields();
    put(result, "dynamic", fields ? record_to_json_value(r, fields->values()) : Json(r));
    put(result, "relation_marked", Json(r, row.is_loaded_relation("lines")));
    put(result, "relations", Json(r, row.has_loaded_relations()));
    auto list = row.loaded_relation("lines");
    put(result, "relation_length", list ? Json(r, std::uint64_t(list->data.size())) : Json(r));
    auto loaded = row.indexed_load_state(pool, layout);
    put(result, "snapshot", state(r, loaded.indexed_snapshot()->get(), field(input, "lookups")));
    return Json(r, result);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& l = user_context.make<FieldLayout>(layout(r, input));
    auto& pool = user_context.make<SnapshotPool>();
    if (text(field(input, "mode")) == "snapshot") {
        Vector<std::string_view> selected(r);
        for (const auto& n : std::get<Json::Array>(field(input, "selected").data)) {
            selected.push_back(text(n));
        }
        auto s =
            (std::get<bool>(field(input, "full").data) ? LoadedSnapshot::fully_loaded(r, l)
                                                       : LoadedSnapshot::projection(r, l, selected))
                .into_shared(pool);
        if (auto d = schema(user_context, field(input, "schema"))) {
            s = LoadedSnapshot::with_dynamic_property_definitions(pool, s, *d);
        }
        auto initial = state(r, s.get(), field(input, "lookups"));
        auto original = s;
        Json::Array stages(r);
        for (const auto& p : std::get<Json::Array>(field(input, "payloads").data)) {
            auto next = LoadedSnapshot::with_dynamic_fields(pool, s, dynamic(user_context, p));
            Json::Object info(r);
            if (!next) {
                put(info, "error", Json(r, next.error()));
            } else {
                put(info, "same", Json(r, s.identity() == next.value().identity()));
                s = next.value();
                put(info, "state", state(r, s.get(), field(input, "lookups")));
            }
            stages.emplace_back(info);
        }
        Json::Object result(r);
        put(result, "initial", std::move(initial));
        put(result, "original", state(r, original.get(), field(input, "lookups")));
        put(result, "stages", Json(r, stages));
        return Json(r, result);
    }
    Vector<CompactRow> rows(r);
    for (const auto& v : std::get<Json::Array>(field(input, "rows").data)) {
        rows.emplace_back(record(r, field(v, "native")));
        auto& row = rows.back();
        if (auto d = schema(user_context, field(v, "schema"))) {
            CompactRow::attach_dynamic_property_definitions(std::span(&row, 1), *d);
        }
        if (!std::holds_alternative<std::monostate>(field(v, "dynamic").data)) {
            row.set_loaded_dynamic_fields(dynamic(user_context, field(v, "dynamic")));
        }
        if (std::get<bool>(field(v, "relation").data)) {
            RelationShapeCache cache(std::allocator_arg, Allocator(r));
            row.mark_relation_loaded("lines", cache);
            SmartList<CompactRow> list(std::allocator_arg, Allocator(r));
            list.is_loaded = true;
            row.set_loaded_relation("lines", list);
        }
    }
    Json::Array initial(r), outcomes(r);
    for (const auto& row : rows) {
        initial.push_back(row_state(r, row, input, pool, l));
    }
    CompactRow original(std::allocator_arg, Allocator(r), rows[0]);
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto before = rows[0].shared_layout();
        Json outcome(r);
        auto op = text(action);
        if (op == "merge") {
            auto merged = rows[0].try_extend(rows[1], shapes);
            if (!merged) {
                Json::Object e(r);
                put(e, "error", Json(r, merged.error().code));
                put(e, "field", Json(r, merged.error().field));
                outcome = Json(r, e);
            }
        } else if (op == "clear") {
            rows[0].clear_loaded_relations();
        } else if (op == "take") {
            rows[0].take_loaded_dynamic_fields();
        } else if (op == "remove_relation") {
            rows[0].remove("lines");
        }
        Json::Object info(r);
        put(info, "result", std::move(outcome));
        put(info, "same_layout", Json(r, before == rows[0].shared_layout()));
        put(info, "state", row_state(r, rows[0], input, pool, l));
        outcomes.emplace_back(info);
    }
    Json::Object result(r);
    put(result, "initial", Json(r, initial));
    put(result, "original", row_state(r, original, input, pool, l));
    put(result, "outcomes", Json(r, outcomes));
    return Json(r, result);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("Invalid carrier input");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
