#include "../../examples/generated/include/cpp_examples.hpp"
#include "fixture_value.hpp"
#include <iostream>
using namespace cpp_examples;

static Json view(Memory r, const Product& entity) {
    Json::Object result(r);
    Json::Array loaded(r);
    loaded.emplace_back(entity.is_field_loaded("#title"));
    loaded.emplace_back(entity.is_field_loaded("#count"));
    result.emplace(String("loaded", r), Json(r, loaded));
    Json::Object values(r);
    for (const auto& [name, value] : entity.dynamicFieldValues()->values()) {
        values.emplace(String(name, r), value.to_json_value(r));
    }
    result.emplace(String("values", r), Json(r, values));
    result.emplace(String("pending", r), Json(r, entity.hasPendingDynamicMutations()));
    return Json(r, result);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t(7)));
    row.emplace(String("name", r), Value(r, "original"));
    row.emplace(String("version", r), Value(r, std::int64_t(3)));
    auto decoded = Product::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
    auto& entity = user_context.make<Product>(decoded.value());
    const std::array types{DynamicFieldDefinitions::Field{"title", DataType::Text},
                           DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "Product", "v1", types)).value());
    Value::Object values(r);
    for (const auto& [name, payload] : std::get<Json::Object>(field(input, "stored").data)) {
        values.emplace(String(name, r), value(r, payload));
    }
    auto fields = DynamicFieldValues::from_values(r, definitions, values);
    auto base = entity.loaded_state_snapshot().value();
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    auto state = LoadedSnapshot::with_dynamic_fields(pool, base, fields.value()).value();
    if (!entity.install_loaded_dynamic_fields(fields.value(), state)) {
        throw std::runtime_error("invalid carrier fixture");
    }
    auto mode = text(field(input, "mode"));
    if (mode == "missing") {
        entity.load_state = eval::LoadState::fully_loaded(r);
    }
    if (mode == "layout") {
        entity.load_state =
            eval::LoadState::fully_loaded(r).into_indexed(pool, Order::field_layout(r)).value();
    }
    auto before = entity.loaded_state_snapshot();
    auto& copy = user_context.make<Product>(entity);
    std::optional<String> error;
    if (mode == "install_owner") {
        const std::array other_types{DynamicFieldDefinitions::Field{"title", DataType::Text}};
        auto& other = user_context.make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(r, "Other", "v1", other_types)).value());
        Value::Object empty(r);
        auto wrong = DynamicFieldValues::from_values(r, other, empty);
        auto result = entity.install_loaded_dynamic_fields(wrong.value(), base);
        if (!result) {
            error.emplace(result.error().message);
        }
    } else if (mode == "install_layout") {
        auto wrong =
            eval::LoadState::fully_loaded(r).into_indexed(pool, Order::field_layout(r)).value();
        auto result = entity.install_loaded_dynamic_fields(*entity.dynamicFieldValues(),
                                                           *wrong.indexed_snapshot());
        if (!result) {
            error.emplace(result.error().message);
        }
    } else if (text(field(input, "op")) == "delete") {
        auto result = entity.deleteDynamicField(text(field(input, "code")));
        if (!result) {
            error.emplace(result.error().to_string(r));
        }
    } else {
        auto result =
            entity.updateDynamicField(text(field(input, "code")), value(r, field(input, "value")));
        if (!result) {
            error.emplace(result.error().to_string(r));
        }
    }
    auto after = entity.loaded_state_snapshot();
    bool same = (!before && !after) || (before && after && before->identity() == after->identity());
    Json::Object output(r);
    output.emplace(String("error", r), error ? Json(r, *error) : Json(r));
    output.emplace(String("view", r), view(r, entity));
    output.emplace(String("copy", r), view(r, copy));
    output.emplace(String("same_snapshot", r), Json(r, same));
    return Json(r, output);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("invalid carrier fixture");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
