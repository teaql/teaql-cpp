#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <array>
#include <bit>
#include <iostream>

using namespace teaql;

static void install(UserContext& user_context, EntityRuntimeState& state, const Json& bits) {
    const auto* encoded = std::get_if<String>(&bits.data);
    if (!encoded) {
        return;
    }
    std::uint64_t raw{};
    auto parsed = std::from_chars(encoded->data(), encoded->data() + encoded->size(), raw, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != encoded->data() + encoded->size()) {
        throw std::invalid_argument("Invalid IEEE-754 input bits");
    }
    auto r = user_context.memory_resource();
    const std::array fields{DynamicFieldDefinitions::Field{"number", DataType::F64}};
    auto definitions = DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields);
    auto& owned = user_context.make<DynamicFieldDefinitions>(std::move(definitions).value());
    Value::Object values(r);
    values.emplace(String("number", r), Value(r, std::bit_cast<double>(raw)));
    auto loaded = DynamicFieldValues::from_values(r, owned, values);
    state.install_loaded_dynamic_fields(loaded.value());
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    EntityRuntimeState state(std::allocator_arg, Allocator(r));
    install(user_context, state, field(input, "bits"));
    if (std::get<bool>(field(input, "active").data)) {
        state.set_comment("receiving ledger");
    }
    const bool before_clone = state == state;
    EntityRuntimeState copied(std::allocator_arg, Allocator(r), state);
    auto other = EntityRuntimeState::fresh_with_shared_graph(r, state);
    install(user_context, other, field(input, "other_bits"));
    auto rebound = other.with_shared_graph(state);
    auto before = detail::model_json(r);
    detail::model_put(before, "self_before_clone", before_clone);
    detail::model_put(before, "self_after_clone", state == state);
    detail::model_put(before, "clone_self", copied == copied);
    detail::model_put(before, "clone_forward", state == copied);
    detail::model_put(before, "clone_reverse", copied == state);
    detail::model_put(before, "independent_self", other == other);
    detail::model_put(before, "independent_forward", state == other);
    detail::model_put(before, "independent_reverse", other == state);
    detail::model_put(before, "rebound_self", rebound == rebound);
    detail::model_put(before, "rebound_forward", state == rebound);
    auto dynamic_self = [&](const EntityRuntimeState& value) {
        auto* values = value.loaded_dynamic_fields();
        return values ? Json(r, *values == *values) : Json(r);
    };
    detail::model_put(before, "dynamic_self", dynamic_self(state));
    detail::model_put(before, "copied_dynamic_self", dynamic_self(copied));
    state.clear_committed();
    auto output = detail::model_json(r);
    detail::model_put(output, "before_clear", before);
    detail::model_put(output, "self_after_clear", state == state);
    detail::model_put(output, "clone_after_clear", state == copied);
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
