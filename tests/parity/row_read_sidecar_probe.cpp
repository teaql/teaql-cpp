#include <teaql/runtime/context.hpp>
#include <teaql/core/list.hpp>
#include <teaql/core/dynamic_fields_debug.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <array>
#include <iostream>

static CompactRow
row(UserContext& user_context, unsigned profile, std::string_view name, bool loaded) {
    auto owner = user_context.memory_resource();
    Vector<String> names(owner);
    names.emplace_back("id");
    auto& layout = user_context.make<ColumnLayout>(std::move(names));
    Vector<Value> values(owner);
    values.emplace_back(std::uint64_t{7});
    CompactRow result(std::allocator_arg, Allocator(owner), layout, std::move(values));
    if (profile == 1 || profile == 2 || profile == 7 || profile == 8) {
        SmartList<CompactRow> list(std::allocator_arg, Allocator(owner));
        list.is_loaded = loaded;
        if (profile == 7) {
            Vector<String> child_names(owner);
            child_names.emplace_back(name);
            auto& child_layout = user_context.make<ColumnLayout>(std::move(child_names));
            String payload(owner);
            for (unsigned i = 0; i < 32; ++i) {
                payload.append(std::string_view("payload\0雪", 11));
            }
            Vector<Value> child_values(owner);
            child_values.emplace_back(std::move(payload));
            list.data.emplace_back(child_layout, std::move(child_values));
        }
        result.set_loaded_relation(name, list);
        if (profile == 1) {
            (void)result.take_loaded_relation(name);
        }
    }
    if (profile == 3 || profile == 4 || profile == 5 || profile == 6 || profile == 8) {
        const std::array<DynamicFieldDefinitions::Field, 1> fields{{{"memo", DataType::Text}}};
        auto definitions = DynamicFieldDefinitions::from_fields(owner, "Row", "revision", fields);
        auto& metadata = user_context.make<DynamicFieldDefinitions>(std::move(definitions).value());
        Value::Object contents(owner);
        if (profile == 5 || profile == 8) {
            contents.emplace("memo", Value(owner, nullptr));
        }
        if (profile == 6) {
            String payload(owner);
            for (unsigned i = 0; i < 32; ++i) {
                payload.append(std::string_view("dynamic\0雪", 11));
            }
            contents.emplace("memo", Value(owner, std::move(payload)));
        }
        auto dynamic = DynamicFieldValues::from_values(owner, metadata, contents);
        result.set_loaded_dynamic_fields(dynamic.value());
        if (profile == 3) {
            (void)result.take_loaded_dynamic_fields();
        }
    }
    return result;
}

static Json observe(UserContext& user_context,
                    const Json& input,
                    Ref<CompactRow>& retained,
                    Ref<Json>& expected) {
    auto owner = user_context.memory_resource();
    auto name = text(field(input, "name"));
    auto loaded = std::get<bool>(field(input, "loaded").data);
    auto left = row(
        user_context, unsigned(std::get<std::uint64_t>(field(input, "left").data)), name, loaded);
    auto right = row(
        user_context, unsigned(std::get<std::uint64_t>(field(input, "right").data)), name, loaded);
    CompactRow initial(std::allocator_arg, Allocator(owner), left);
    auto pristine = row(user_context, 0, name, loaded);
    auto before = formatted_debug(owner, left);
    auto right_before = formatted_debug(owner, right);
    Json error(owner);
    if (std::get<bool>(field(input, "checked").data)) {
        DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(owner));
        auto result = left.try_extend(right, shapes);
        if (!result) {
            error = formatted_debug(owner, result.error());
        }
    } else {
        left.extend(right);
    }
    CompactRow copied(std::allocator_arg, Allocator(owner), left);
    auto merged = formatted_debug(owner, left);
    retained = user_context.make_global<CompactRow>(left);
    expected = user_context.make_global<Json>(merged);
    Json::Object output(owner);
    output.emplace("before", std::move(before));
    output.emplace("right", std::move(right_before));
    output.emplace("error", std::move(error));
    output.emplace("merged", std::move(merged));
    output.emplace("equal_pristine", Json(owner, left == pristine));
    output.emplace("equal_clone", Json(owner, left == copied));
    output.emplace("has_relations", Json(owner, left.has_loaded_relations()));
    auto relation = left.take_loaded_relation(name);
    output.emplace("relation", relation ? formatted_debug(owner, *relation) : Json(owner));
    output.emplace("after_relation", formatted_debug(owner, left));
    auto dynamic = left.take_loaded_dynamic_fields();
    output.emplace("dynamic", dynamic ? formatted_debug(owner, *dynamic) : Json(owner));
    output.emplace("after_dynamic", formatted_debug(owner, left));
    output.emplace("json", compact_row_to_json_value(owner, left));
    left.clear_loaded_relations();
    output.emplace("cleared", formatted_debug(owner, left));
    output.emplace("cleared_equals_pristine", Json(owner, left == pristine));
    output.emplace("initial_unchanged",
                   Json(owner, formatted_debug(owner, initial) == output.find("before")->second));
    output.emplace("right_unchanged",
                   Json(owner, formatted_debug(owner, right) == output.find("right")->second));
    return Json(owner, std::move(output));
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            Ref<CompactRow> retained;
            Ref<Json> expected;
            Ref<Json> output;
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("Invalid row sidecar fixture");
                }
                output = user_context->make_global<Json>(
                    observe(*user_context, *input, retained, expected));
            }
            user_context.reset();
            auto next_user_context = global.make_user_context();
            auto r = next_user_context->memory_resource();
            const auto& row = next_user_context->get(next_user_context->rebind_global(retained));
            const auto& expected_debug =
                next_user_context->get(next_user_context->rebind_global(expected));
            if (formatted_debug(r, row) != expected_debug) {
                throw std::runtime_error(
                    "Retained row metadata changed after actual User deletion");
            }
            std::cout << next_user_context->get(next_user_context->rebind_global(output)).dump(r)
                      << '\n';
        }
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
