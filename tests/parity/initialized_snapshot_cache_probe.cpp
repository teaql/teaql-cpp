#include <teaql/core/debug.hpp>
#include <teaql/core/eval.hpp>
#include <teaql/core/row.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

static const FieldLayout&
make_layout(GlobalContext& global, std::size_t count, std::string_view entity) {
    auto r = global.memory_resource();
    Vector<String> names(r), aliases(r);
    for (std::size_t i = 0; i < count; ++i) {
        names.emplace_back(i == 0 ? "id" : i == 1 ? "version" : "field_" + std::to_string(i));
        aliases.emplace_back("sql_" + std::to_string(i));
    }
    Vector<FieldLayout::Index> indexes(r);
    Vector<FieldLayout::Mapping> mappings(r);
    Vector<std::string_view> expected(r);
    for (std::size_t i = 0; i < count; ++i) {
        indexes.emplace_back(names[i], i);
        mappings.emplace_back(names[i], names[i], aliases[i]);
        expected.push_back(names[i]);
    }
    std::array<std::string_view, 1> relations{"lines"};
    return global.make<FieldLayout>(
        std::move(
            FieldLayout::from_generated(r, entity, "v1", indexes, mappings, relations, expected))
            .value());
}

static void put(Json::Object& output, std::string_view name, Json value) {
    output.emplace(String(name, output.get_allocator().resource()), std::move(value));
}

static Json geometry(Memory r, const LoadedSnapshot& state, const Json& lookups) {
    Json::Object output(r);
    put(output, "bits", Json(r, std::to_string(state.bits())));
    Json::Array overflow(r), selected(r), loaded(r);
    if (auto* entries = state.overflow()) {
        for (auto item : *entries) {
            overflow.emplace_back(std::uint64_t(item));
        }
    }
    if (auto* entries = state.selected_names()) {
        for (const auto& item : *entries) {
            selected.emplace_back(item);
        }
    }
    for (const auto& name : std::get<Json::Array>(lookups.data)) {
        loaded.emplace_back(state.is_loaded(text(name)));
    }
    put(output, "overflow", Json(r, std::move(overflow)));
    put(output, "selected", Json(r, std::move(selected)));
    put(output, "loaded", Json(r, std::move(loaded)));
    auto* schema = state.dynamic_property_definitions();
    put(output, "schema", schema ? Json(r, debug_string(r, *schema)) : Json(r));
    return Json(r, std::move(output));
}

static const DynamicPropertyDefinitions& definitions(UserContext& user_context, unsigned profile) {
    auto r = user_context.memory_resource();
    Vector<DynamicPropertyDefinitions::Field> fields(r);
    if (profile == 2) {
        fields.emplace_back("_rank", DataType::U64);
        fields.emplace_back("_label", DataType::Text);
    } else if (profile == 3) {
        fields.emplace_back("_rank", DataType::Bool);
    }
    return user_context.make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, fields)).value());
}

static Json layout_case(GlobalContext& global, UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto count = std::size_t(std::get<std::uint64_t>(field(input, "count").data));
    Vector<String> names(r);
    for (const auto& name : std::get<Json::Array>(field(input, "selected").data)) {
        names.emplace_back(text(name));
    }
    auto profile = unsigned(std::get<std::uint64_t>(field(input, "schema").data));
    const DynamicPropertyDefinitions* schema =
        profile ? &definitions(user_context, profile) : nullptr;
    auto& columns =
        user_context.make<ColumnLayout>(std::move(names), std::span<const String>{}, schema);
    auto& first = user_context.make<CompactRow>(
        columns, Vector<Value>(columns.names().size(), Value(r, std::int64_t{11}), r));
    auto& second = user_context.make<CompactRow>(first);
    if (std::get<bool>(field(input, "relations").data)) {
        RelationShapeCache shapes(std::allocator_arg, Allocator(r));
        first.mark_relation_loaded("lines", shapes);
        second.mark_relation_loaded("lines", shapes);
    }
    const auto& a = make_layout(global, count, "Item");
    const auto& b = make_layout(
        global, count, std::get<bool>(field(input, "other_entity").data) ? "Other" : "Item");
    auto& pool = user_context.make<SnapshotPool>();
    auto primary = *first.indexed_load_state(pool, a).indexed_snapshot();
    auto primary_again = *second.indexed_load_state(pool, a).indexed_snapshot();
    auto secondary = *first.indexed_load_state(pool, b).indexed_snapshot();
    auto secondary_again = *second.indexed_load_state(pool, b).indexed_snapshot();
    auto changed =
        LoadedSnapshot::with_loaded(pool, primary, "id", !primary.get().is_loaded("id")).value();
    auto restored =
        LoadedSnapshot::with_loaded(pool, changed, "id", primary.get().is_loaded("id")).value();
    auto before = debug_string(r, *first.shared_layout());
    const auto* old_columns = first.shared_layout();
    const auto& rebound_columns =
        old_columns->with_dynamic_property_definitions(definitions(user_context, 3));
    auto& rebound = user_context.make<CompactRow>(
        rebound_columns, Vector<Value>(old_columns->names().size(), Value(r, std::int64_t{33}), r));
    auto rebound_state = *rebound.indexed_load_state(pool, a).indexed_snapshot();
    bool isolated = true;
    if (!columns.names().empty()) {
        first.insert(columns.names().front(), Value(r, std::int64_t{99}));
        isolated = second.get(columns.names().front())->try_i64() == 11;
    }
    Json::Object common(r), diagnostics(r), output(r);
    put(common, "shared_columns", Json(r, first.shared_layout() == second.shared_layout()));
    put(common, "primary_hit", Json(r, primary.identity() == primary_again.identity()));
    put(common, "secondary_hit", Json(r, secondary.identity() == secondary_again.identity()));
    put(common, "distinct_layout_state", Json(r, primary.identity() != secondary.identity()));
    for (const auto& [name, state] :
         std::array<std::pair<std::string_view, LoadedSnapshot::Borrow>, 5>{
             {{"primary", primary},
              {"secondary", secondary},
              {"changed", changed},
              {"unchanged", primary_again},
              {"rebound", rebound_state}}}) {
        put(common, name, geometry(r, state.get(), field(input, "lookups")));
    }
    put(common, "restored_hit", Json(r, primary.identity() == restored.identity()));
    put(common,
        "equal_schema_same",
        schema ? Json(r, old_columns == &old_columns->with_dynamic_property_definitions(*schema))
               : Json(r));
    put(common, "schema_rebound", Json(r, old_columns != &rebound_columns));
    put(common, "value_isolated", Json(r, isolated));
    put(diagnostics, "initialized_layout", Json(r, before));
    put(diagnostics, "after_value_change", Json(r, debug_string(r, first)));
    put(output, "common", Json(r, std::move(common)));
    put(output, "diagnostics", Json(r, std::move(diagnostics)));
    return Json(r, std::move(output));
}

static void destroy_snapshot(UserContext& user_context, const LoadedSnapshot::Borrow& state) {
    auto& arena = *dynamic_cast<Arena*>(user_context.memory_resource());
    arena.destroy(std::uint32_t(state.identity()), typeid(LoadedSnapshot), state.identity());
}

static bool alive(const LoadedSnapshot::Borrow& state) {
    try {
        (void)state.get();
        return true;
    } catch (const ReferenceError& error) {
        if (error.kind() != ReferenceError::Kind::Dead &&
            error.kind() != ReferenceError::Kind::Closed) {
            throw;
        }
        return false;
    }
}

static Json saturation_case(GlobalContext& global, UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto count = std::size_t(std::get<std::uint64_t>(field(input, "shapes").data));
    const auto& layout = make_layout(global, 2, "Item");
    auto& pool = user_context.make<SnapshotPool>();
    Vector<LoadedSnapshot::Borrow> states(r);
    for (std::size_t i = 0; i < count; ++i) {
        String name("#shape_" + std::to_string(i), r);
        std::array<std::string_view, 1> selected{name};
        states.push_back(LoadedSnapshot::projection(r, layout, selected).into_shared(pool));
    }
    auto build = [&](std::string_view name) {
        std::array<std::string_view, 1> selected{name};
        return LoadedSnapshot::projection(r, layout, selected).into_shared(pool);
    };
    Json::Array hits(r);
    hits.emplace_back(states.front().identity() == build("#shape_0").identity());
    hits.emplace_back(states.back().identity() ==
                      build("#shape_" + std::to_string(count - 1)).identity());
    auto before = debug_string(r, layout);
    auto hint_count_before = pool.hint_count(layout);
    if (std::get<bool>(field(input, "release").data)) {
        for (std::size_t i = 0; i < count / 2; ++i) {
            destroy_snapshot(user_context, states[i]);
        }
    }
    auto new_state = build("#new"), repeated = build("#new");
    Json::Object common(r), diagnostics(r), output(r);
    put(common, "hits", Json(r, std::move(hits)));
    put(common, "released", Json(r, !alive(states.front())));
    put(common, "new_hit", Json(r, new_state.identity() == repeated.identity()));
    put(common, "hint_count_before", Json(r, std::uint64_t(hint_count_before)));
    put(common, "hint_count_after", Json(r, std::uint64_t(pool.hint_count(layout))));
    put(common,
        "surviving_last",
        Json(r, states.back().get().is_loaded("#shape_" + std::to_string(count - 1))));
    put(common, "new_loaded", Json(r, new_state.get().is_loaded("#new")));
    put(diagnostics, "before", Json(r, before));
    put(diagnostics, "after", Json(r, debug_string(r, layout)));
    put(output, "common", Json(r, std::move(common)));
    put(output, "diagnostics", Json(r, std::move(diagnostics)));
    return Json(r, std::move(output));
}

static Json observe(GlobalContext& global, std::string_view line) {
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    Json retained(global.memory_resource());
    std::optional<LoadedSnapshot::Borrow> expired;
    {
        auto r = user_context.memory_resource();
        auto input = Json::parse(r, line).value();
        Json output(r);
        auto kind = text(field(input, "kind"));
        if (kind == "layout") {
            output = layout_case(global, user_context, input);
        } else if (kind == "saturation") {
            output = saturation_case(global, user_context, input);
        } else {
            const auto& a = make_layout(global, 2, "Item");
            const auto& b = make_layout(global, 2, "Other");
            auto& pool = user_context.make<SnapshotPool>();
            std::optional<LoadedSnapshot::Borrow> primary, secondary;
            std::array<bool, 2> within_row;
            {
                auto& columns =
                    user_context.make<ColumnLayout>(std::initializer_list<std::string_view>{"id"});
                Vector<Value> values(r);
                values.emplace_back(std::int64_t{1});
                CompactRow row(std::allocator_arg, Allocator(r), columns, std::move(values));
                primary = *row.indexed_load_state(pool, a).indexed_snapshot();
                secondary = *row.indexed_load_state(pool, b).indexed_snapshot();
                within_row = {alive(*primary), alive(*secondary)};
            }
            expired = *secondary;
            Json::Object ownership(r), result(r);
            put(ownership, "model", Json(r, "context_arena"));
            for (auto name : {"within_row", "after_row"}) {
                Json::Array values(r);
                values.emplace_back(name == std::string_view("within_row") ? within_row[0]
                                                                           : alive(*primary));
                values.emplace_back(name == std::string_view("within_row") ? within_row[1]
                                                                           : alive(*secondary));
                put(ownership, name, Json(r, std::move(values)));
            }
            put(result, "ownership", Json(r, std::move(ownership)));
            output = Json(r, std::move(result));
        }
        retained = Json(std::allocator_arg, Allocator(global.memory_resource()), output);
    }
    user_context_owner.reset();
    if (expired && alive(*expired)) {
        throw std::runtime_error("Snapshot borrow survived actual UserContext deletion");
    }
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Cache observations retained UserContext blocks");
    }
    return retained;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto output = observe(global, line);
            std::cout << output.dump(output.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
