#include <teaql/runtime/context.hpp>
#include <teaql/runtime/generated_scalar_codec.hpp>
#include "fixture_value.hpp"
#include <iostream>

template <class T>
static Json decode_scalar(Memory r, const Value* input, std::string_view entity) {
    auto result = GeneratedScalarCodec::decode<T>(r, input, entity, "value");
    Json::Object output(r);
    output.emplace("ok", Json(r, bool(result)));
    if (!result) {
        output.emplace("entity", Json(r, result.error().entity));
        output.emplace("error", Json(r, result.error().message));
    } else if constexpr (std::integral<T>) {
        output.emplace("value", Json(r, result.value()));
    } else {
        output.emplace("value", result.value() ? Json(r, *result.value()) : Json(r));
    }
    return Json(r, output);
}

template <class T>
static Json observe_scalar(Memory r,
                           const Value* input,
                           bool optional_value,
                           std::string_view entity,
                           std::string_view optional_entity) {
    if (optional_value) {
        return decode_scalar<std::optional<T>>(r, input, optional_entity);
    }
    return decode_scalar<T>(r, input, entity);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    std::optional<Value> scalar;
    if (std::get<bool>(field(input, "present").data)) {
        scalar.emplace(value(r, field(input, "value")));
    }
    const Value* source = scalar ? &*scalar : nullptr;
    bool optional_value = std::get<bool>(field(input, "optional").data);
    auto carrier = text(field(input, "carrier"));
    if (carrier == "i16") {
        return observe_scalar<std::int16_t>(r, source, optional_value, "I16", "OptionalI16");
    }
    if (carrier == "i32") {
        return observe_scalar<std::int32_t>(r, source, optional_value, "I32", "OptionalI32");
    }
    if (carrier == "u16") {
        return observe_scalar<std::uint16_t>(r, source, optional_value, "U16", "OptionalU16");
    }
    if (carrier == "u32") {
        return observe_scalar<std::uint32_t>(r, source, optional_value, "U32", "OptionalU32");
    }
    throw std::runtime_error("unknown narrow integer case");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::runtime_error("narrow scalar decoding retains UserContext blocks");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
