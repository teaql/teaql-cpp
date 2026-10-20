#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

using namespace teaql;

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto flag = [&](std::string_view name) {
        return std::get<bool>(field(input, name).data);
    };
    const std::array fields{DynamicFieldDefinitions::Field{"name", DataType::Text}};
    auto definitions = DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields);
    auto& base = user_context.make<DynamicFieldDefinitions>(std::move(definitions).value());
    const auto& selected = flag("bound") ? user_context.make<DynamicFieldDefinitions>(
                                               base.with_storage_binding(r, "profile", 37))
                                         : base;
    Value::Object values(r);
    auto loaded = std::get<std::uint64_t>(field(input, "loaded").data);
    if (loaded != 0) {
        values.emplace(String("name", r), loaded == 1 ? Value(r) : Value(r, "loaded"));
    }
    auto dynamic = DynamicFieldValues::from_values(r, selected, values);
    EntityRuntimeState state(std::allocator_arg, Allocator(r));
    state.install_loaded_dynamic_fields(dynamic.value());
    EntityKey key(std::allocator_arg, Allocator(r), "Item", Value(r, std::uint64_t{7}));
    if (flag("row")) {
        Record row(r);
        row.emplace(String("id", r), Value(r, std::uint64_t{7}));
        row.emplace(String("version", r), Value(r, std::int64_t{3}));
        state.set_original_compact_row("Item", CompactRow(std::allocator_arg, Allocator(r), row));
    }
    if (flag("active")) {
        state.set_comment("root");
        state.set_entity_comment(key, "local");
        state.mark_as_new(key);
        state.set_original_version(key, 3);
        EntitySnapshot original(r);
        original.fields.emplace(String("name", r), Value(r, "ledger original"));
        state.set_original_snapshot(original);
    }
    std::optional<EntityRuntimeState> copied;
    if (flag("copy")) {
        copied.emplace(std::allocator_arg, Allocator(r), state);
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "before", debug_string(r, state));
    auto action = text(field(input, "action"));
    auto update = [&] {
        if (!state.update_dynamic_field(key, "name", Value(r, "changed"))) {
            throw std::runtime_error("Dynamic diagnostic update failed");
        }
    };
    auto erase = [&] {
        if (!state.delete_dynamic_field(key, "name")) {
            throw std::runtime_error("Dynamic diagnostic delete failed");
        }
    };
    if (action == "set") {
        update();
    } else if (action == "delete") {
        erase();
    } else if (action == "set_delete") {
        update();
        erase();
    } else if (action == "delete_set") {
        erase();
        update();
    } else if (action == "pop" || action == "clear") {
        state.push_change_set();
        update();
        if (action == "pop") {
            state.pop_change_set();
        } else {
            state.clear_current_change_set();
        }
    } else if (action == "commit") {
        update();
        state.clear_committed();
    } else if (action == "mark_delete") {
        update();
        state.mark_as_delete(key);
    } else if (action != "none") {
        throw std::invalid_argument("Unknown dynamic diagnostic action");
    }
    auto changes = state.current_change_set();
    detail::model_put(output, "after", debug_string(r, state));
    detail::model_put(output, "pretty", debug_string(r, state, {.pretty = true}));
    detail::model_put(output, "changes", debug_string(r, changes));
    detail::model_put(output, "pretty_changes", debug_string(r, changes, {.pretty = true}));
    detail::model_put(output, "copy_after", copied ? Json(r, debug_string(r, *copied)) : Json(r));
    return output;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            return 2;
        }
        std::cout << evaluate(*user_context, input.value()).dump(user_context->memory_resource())
                  << '\n';
    }
}
