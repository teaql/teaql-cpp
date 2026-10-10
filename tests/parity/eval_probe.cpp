#include <teaql/core/eval.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

using namespace teaql::eval;

static Json formatted(Memory owner, const auto& value) {
    Json::Array result(owner);
    const DebugOptions options[] = {
        {}, {true}, {false, DebugNumbers::LowerHex}, {true, DebugNumbers::UpperHex}};
    for (const auto& option : options) {
        String output(owner);
        DebugWriter writer(output, option);
        teaql::eval::write_debug(writer, value);
        result.emplace_back(output);
    }
    return Json(owner, std::move(result));
}

static Json observe(Memory owner, const Json& input) {
    auto output = detail::model_json(owner);
    LoadState default_state(std::allocator_arg, Allocator(owner));
    detail::model_put(output, "default_state", default_state.to_json(owner));
    auto state = LoadState::from_json(owner, field(input, "state"));
    auto other = LoadState::from_json(owner, field(input, "other_state"));
    detail::model_put(output,
                      "state_equal",
                      state && other ? Json(owner, state.value() == other.value()) : Json(owner));
    Json state_output(owner);
    if (state) {
        state_output = detail::model_json(owner);
        detail::model_put(state_output, "json", state.value().to_json(owner));
        Json::Array loaded(owner);
        for (const auto& name : std::get<Json::Array>(field(input, "fields").data)) {
            loaded.emplace_back(state.value().is_loaded(text(name)));
        }
        detail::model_put(state_output, "loaded", std::move(loaded));
        auto clone = detail::clone_in(owner, state.value());
        detail::model_put(state_output, "clone_equal", clone == state.value());
        detail::model_put(state_output,
                          "debug",
                          state.value().kind() == LoadState::Kind::Partial &&
                                  state.value().names().size() > 1
                              ? Json(owner)
                              : formatted(owner, state.value()));
    }
    detail::model_put(output, "state", std::move(state_output));
    auto result =
        EvalResult<String>::from_json(owner, field(input, "result"), [&](const Json& value) {
            return String(detail::model_string(value, "Value"), owner);
        });
    Json result_output(owner);
    if (result) {
        result_output = detail::model_json(owner);
        auto encode = [&](const auto& value) {
            return Json(owner, value);
        };
        detail::model_put(result_output, "json", result.value().to_json(owner, encode));
        int map_calls = 0;
        auto mapped = detail::clone_in(owner, result.value()).map([&](String value) {
            ++map_calls;
            std::uint64_t count = 0;
            for (unsigned char byte : value) {
                if ((byte & 0xc0) != 0x80) {
                    ++count;
                }
            }
            return count;
        });
        detail::model_put(result_output, "mapped", mapped.to_json(owner, encode));
        detail::model_put(result_output, "map_calls", map_calls);
        int chain_calls = 0;
        auto chained = detail::clone_in(owner, result.value())
                           .and_then(text(field(input, "field")), [&](String value) {
                               ++chain_calls;
                               auto next = text(field(input, "next"));
                               if (next == "value") {
                                   return EvalResult<String>::value(owner, std::move(value));
                               }
                               if (next == "null") {
                                   return EvalResult<String>::null(owner);
                               }
                               return EvalResult<String>::not_loaded(
                                   owner, text(field(input, "node")), text(field(input, "path")));
                           });
        detail::model_put(result_output, "chained", chained.to_json(owner, encode));
        detail::model_put(result_output, "chain_calls", chain_calls);
        detail::model_put(result_output, "debug", formatted(owner, result.value()));
        detail::model_put(result_output, "chained_debug", formatted(owner, chained));
    }
    detail::model_put(output, "result", std::move(result_output));
    return output;
}

int main() {
    try {
        std::pmr::set_default_resource(std::pmr::null_memory_resource());
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto owner = user->memory_resource();
            auto input = Json::parse(owner, line);
            if (!input) {
                throw std::invalid_argument("Invalid evaluation probe input");
            }
            std::cout << observe(owner, *input).dump(owner) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
