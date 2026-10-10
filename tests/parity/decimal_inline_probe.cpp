#include <teaql/core/decimal.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Json evaluate(Memory owner, const Json& input) {
    const auto& words = std::get<Json::Array>(field(input, "words").data);
    std::array<std::uint32_t, 3> coefficient{};
    for (std::size_t index = 0; index < coefficient.size(); ++index) {
        coefficient[index] = static_cast<std::uint32_t>(std::get<std::uint64_t>(words[index].data));
    }
    auto value = *Decimal::from_parts(
        coefficient,
        std::get<bool>(field(input, "negative").data),
        static_cast<unsigned>(std::get<std::uint64_t>(field(input, "scale").data)));
    const auto& rescale = field(input, "rescale");
    if (!std::holds_alternative<std::monostate>(rescale.data)) {
        value.rescale(static_cast<unsigned>(std::get<std::uint64_t>(rescale.data)));
    }
    Json::Object inline_result(owner);
    try {
        auto characters = value.array_string();
        inline_result.emplace("ok", Json(owner, characters.view()));
    } catch (const std::length_error&) {
        inline_result.emplace("panic", Json(owner, true));
    }
    Json::Array raw_words(owner);
    for (auto word : value.coefficient()) {
        raw_words.emplace_back(word);
    }
    Json::Object output(owner);
    output.emplace("words", Json(owner, std::move(raw_words)));
    output.emplace("negative", Json(owner, value.is_sign_negative()));
    output.emplace("scale", Json(owner, value.scale()));
    output.emplace("inline", Json(owner, std::move(inline_result)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid Decimal inline fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
