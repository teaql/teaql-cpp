#include <teaql/runtime/context.hpp>
#include <teaql/core/list.hpp>
#include <teaql/core/dynamic_fields_debug.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <array>
#include <iostream>

struct RetainedRows {
    std::array<Ref<CompactRow>, 4> rows;
    std::array<Ref<Json>, 4> expected;
    std::size_t count = 0;

    void add(UserContext& user_context, const CompactRow& row) {
        rows.at(count) = user_context.make_global<CompactRow>(row);
        expected.at(count) =
            user_context.make_global<Json>(formatted_debug(user_context.memory_resource(), row));
        ++count;
    }
};

static const DynamicPropertyDefinitions& definitions(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    Vector<DynamicPropertyDefinitions::Field> fields(owner);
    for (const auto& item : std::get<Json::Array>(input.data)) {
        const auto& pair = std::get<Json::Array>(item.data);
        fields.emplace_back(text(pair[0]), DataType(std::get<std::uint64_t>(pair[1].data)));
    }
    auto result = DynamicPropertyDefinitions::from_fields(owner, fields);
    if (!result) {
        throw std::runtime_error("Invalid enhancement schema fixture");
    }
    return user_context.make<DynamicPropertyDefinitions>(std::move(result).value());
}

static CompactRow row(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    Vector<String> names(owner);
    for (const auto& name : std::get<Json::Array>(field(input, "names").data)) {
        names.emplace_back(text(name));
    }
    const auto* layout = &user_context.make<ColumnLayout>(std::move(names));
    if (!std::holds_alternative<std::monostate>(field(input, "schema").data)) {
        layout = &layout->with_dynamic_property_definitions(
            definitions(user_context, field(input, "schema")));
    }
    Vector<Value> values(owner);
    for (const auto& v : std::get<Json::Array>(field(input, "values").data)) {
        values.emplace_back(value(owner, v));
    }
    CompactRow result(std::allocator_arg, Allocator(owner), *layout, std::move(values));
    const auto& dynamic = field(input, "dynamic");
    if (!std::holds_alternative<std::monostate>(dynamic.data)) {
        const std::array<DynamicFieldDefinitions::Field, 1> fields{
            {{"memo", DataType(std::get<std::uint64_t>(field(dynamic, "type").data))}}};
        auto d = DynamicFieldDefinitions::from_fields(
            owner, text(field(dynamic, "owner")), text(field(dynamic, "revision")), fields);
        const auto* schema = &user_context.make<DynamicFieldDefinitions>(std::move(d).value());
        const auto& binding = field(dynamic, "binding");
        if (!std::holds_alternative<std::monostate>(binding.data)) {
            auto bound = schema->with_storage_binding_shared(
                owner,
                text(field(binding, "profile")),
                std::get<std::uint64_t>(field(binding, "executor").data));
            schema = &bound.get();
        }
        Value::Object contents(owner);
        contents.emplace("memo", value(owner, field(dynamic, "value")));
        auto payload = DynamicFieldValues::from_values(owner, *schema, contents);
        result.set_loaded_dynamic_fields(payload.value());
    }
    if (std::get<bool>(field(input, "relation").data)) {
        SmartList<CompactRow> list(std::allocator_arg, Allocator(owner));
        list.loaded();
        result.set_loaded_relation("lines", list);
    }
    return result;
}

static Json constructor(UserContext& user_context, const Json& input, RetainedRows& retained) {
    auto owner = user_context.memory_resource();
    Vector<String> names(owner);
    for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(input, "columns").data); ++i) {
        names.emplace_back("n" + std::to_string(i));
    }
    auto& layout = user_context.make<ColumnLayout>(std::move(names));
    Vector<Value> values(owner);
    for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(input, "values").data); ++i) {
        values.emplace_back(nullptr);
    }
    Json::Object output(owner);
    output.emplace("error", Json(owner));
    output.emplace("row", Json(owner));
    try {
        auto result =
            text(field(input, "entry")) == "new"
                ? CompactRow(
                      std::allocator_arg, Allocator(owner), layout, std::span<const Value>(values))
                : CompactRow(std::allocator_arg, Allocator(owner), layout, std::move(values));
        retained.add(user_context, result);
        output.find("row")->second = Json(owner, debug_string(owner, result));
    } catch (const std::invalid_argument& error) {
        output.find("error")->second = Json(owner, error.what());
    }
    return Json(owner, std::move(output));
}

static Json merge(UserContext& user_context, const Json& input, RetainedRows& retained) {
    auto owner = user_context.memory_resource();
    auto left = row(user_context, field(input, "left"));
    auto right = row(user_context, field(input, "right"));
    CompactRow original(std::allocator_arg, Allocator(owner), left);
    auto* before = left.shared_layout();
    Json::Object output(owner);
    output.emplace("left_before", formatted_debug(owner, left));
    output.emplace("right_before", formatted_debug(owner, right));
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(owner));
    auto result = left.try_extend(right, shapes);
    output.emplace("error", result ? Json(owner) : formatted_debug(owner, result.error()));
    output.emplace("after", formatted_debug(owner, left));
    output.emplace("original", formatted_debug(owner, original));
    output.emplace("right_after", formatted_debug(owner, right));
    output.emplace("left_unchanged", Json(owner, left == original));
    output.emplace("layout_same", Json(owner, before == left.shared_layout()));
    output.emplace("json", compact_row_to_json_value(owner, left));
    auto validation = left.validate_dynamic_properties();
    output.emplace("validation", validation ? Json(owner) : Json(owner, validation.error()));
    retained.add(user_context, left);
    return Json(owner, std::move(output));
}

static Json state(Memory owner, const Vector<CompactRow>& rows) {
    Json::Array values(owner), shared(owner), relations(owner), marks(owner);
    using namespace std::literals::string_view_literals;
    for (const auto& row : rows) {
        values.emplace_back(formatted_debug(owner, row));
        Json::Array identities(owner), availability(owner);
        for (const auto& other : rows) {
            identities.emplace_back(row.shared_layout() == other.shared_layout());
        }
        for (auto name : {""sv, "lines"sv, "雪\0"sv}) {
            availability.emplace_back(row.is_loaded_relation(name));
        }
        shared.emplace_back(std::move(identities));
        relations.emplace_back(row.has_loaded_relations());
        marks.emplace_back(std::move(availability));
    }
    Json::Object output(owner);
    output.emplace("rows", Json(owner, std::move(values)));
    output.emplace("shared", Json(owner, std::move(shared)));
    output.emplace("relations", Json(owner, std::move(relations)));
    output.emplace("marks", Json(owner, std::move(marks)));
    return Json(owner, std::move(output));
}

static Json shapes(UserContext& user_context, const Json& input, RetainedRows& retained) {
    auto owner = user_context.memory_resource();
    Vector<CompactRow> rows(owner);
    for (const auto& item : std::get<Json::Array>(field(input, "rows").data)) {
        rows.emplace_back(row(user_context, item));
    }
    Json::Object output(owner);
    output.emplace("initial", state(owner, rows));
    CompactRow original(std::allocator_arg, Allocator(owner), rows[0]);
    Json::Array stages(owner);
    CompactRow::share_layouts(rows);
    stages.emplace_back(state(owner, rows));
    RelationShapeCache cache(std::allocator_arg, Allocator(owner));
    using namespace std::literals::string_view_literals;

    struct Pass {
        std::array<unsigned, 2> indexes;
        unsigned count;
        std::string_view name;
    };

    const std::array passes{Pass{{0, 1}, 2, "lines"},
                            Pass{{2, 3}, 2, "lines"},
                            Pass{{0, 2}, 2, ""},
                            Pass{{3, 0}, 1, "lines"},
                            Pass{{1, 0}, 1, ""},
                            Pass{{0, 3}, 2, "雪\0"sv}};
    for (const auto& pass : passes) {
        for (unsigned i = 0; i < pass.count; ++i) {
            rows[pass.indexes[i]].mark_relation_loaded(pass.name, cache);
        }
        stages.emplace_back(state(owner, rows));
    }
    CompactRow::attach_dynamic_property_definitions(
        rows, definitions(user_context, field(input, "schema")));
    stages.emplace_back(state(owner, rows));
    CompactRow::attach_dynamic_property_definitions(
        rows, definitions(user_context, field(input, "schema")));
    stages.emplace_back(state(owner, rows));
    CompactRow::share_layouts(rows);
    stages.emplace_back(state(owner, rows));
    String payload(owner);
    for (unsigned i = 0; i < 32; ++i) {
        payload.append("payload\0雪"sv);
    }
    rows[1].insert("extra", Value(owner, std::move(payload)));
    stages.emplace_back(state(owner, rows));
    auto removed = rows[1].remove("lines");
    output.emplace("removed", removed ? formatted_debug(owner, *removed) : Json(owner));
    stages.emplace_back(state(owner, rows));
    auto map = std::move(rows.back()).into_map(owner);
    output.emplace("map", formatted_debug(owner, map));
    rows.back() = CompactRow(std::allocator_arg, Allocator(owner), std::move(map));
    stages.emplace_back(state(owner, rows));
    output.emplace("stages", Json(owner, std::move(stages)));
    output.emplace("original", formatted_debug(owner, original));
    for (const auto& row : rows) {
        retained.add(user_context, row);
    }
    return Json(owner, std::move(output));
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            RetainedRows retained;
            Ref<Json> output;
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("Invalid enhancement input");
                }
                Json result(user_context->memory_resource());
                if (text(field(*input, "kind")) == "constructor") {
                    result = constructor(*user_context, *input, retained);
                } else if (text(field(*input, "kind")) == "merge") {
                    result = merge(*user_context, *input, retained);
                } else if (text(field(*input, "kind")) == "record") {
                    auto model = row(*user_context, field(*input, "row"));
                    retained.add(*user_context, model);
                    auto map = std::move(model).into_map(user_context->memory_resource());
                    result = formatted_debug(user_context->memory_resource(), map);
                } else {
                    result = shapes(*user_context, *input, retained);
                }
                output = user_context->make_global<Json>(result);
            }
            user_context.reset();
            auto next_user_context = global.make_user_context();
            auto owner = next_user_context->memory_resource();
            for (std::size_t i = 0; i < retained.count; ++i) {
                const auto& row =
                    next_user_context->get(next_user_context->rebind_global(retained.rows[i]));
                const auto& expected =
                    next_user_context->get(next_user_context->rebind_global(retained.expected[i]));
                if (formatted_debug(owner, row) != expected) {
                    throw std::runtime_error(
                        "Retained enhancement row changed after actual User deletion");
                }
            }
            std::cout
                << next_user_context->get(next_user_context->rebind_global(output)).dump(owner)
                << '\n';
        }
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
