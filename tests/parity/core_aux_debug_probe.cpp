#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json error_formats(Memory owner, const DecimalError& error) {
    Json::Array formats(owner);
    formats.emplace_back(error.format(owner));
    formats.emplace_back(error.format(owner, {.precision = 2, .width = 40}));
    formats.emplace_back(error.format(owner,
                                      {.alignment = DecimalAlignment::Center,
                                       .precision = 3,
                                       .width = 18,
                                       .fill = U'\U0001f9ca',
                                       .sign_plus = true}));
    formats.emplace_back(error.format(
        owner,
        {.alignment = DecimalAlignment::Left, .width = 12, .sign_plus = true, .zero_pad = true}));
    for (auto alignment :
         {DecimalAlignment::Left, DecimalAlignment::Right, DecimalAlignment::Center}) {
        formats.emplace_back(error.format(
            owner, {.alignment = alignment, .precision = 3, .width = 12, .fill = U'_'}));
    }
    formats.emplace_back(error.format(owner, {.precision = 0}));
    formats.emplace_back(error.format(owner, {.precision = 1}));
    return Json(owner, std::move(formats));
}

static Json evaluate(Memory owner, const Json& input) {
    using namespace teaql::dynamic_search;
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    auto name = text(field(input, "text"));
    auto number = std::get<std::uint64_t>(field(input, "number").data);
    constexpr BusinessIdErrorCode codes[]{BusinessIdErrorCode::InvalidDefinition,
                                          BusinessIdErrorCode::InvalidFormat,
                                          BusinessIdErrorCode::Immutable,
                                          BusinessIdErrorCode::Exhausted,
                                          BusinessIdErrorCode::Allocation,
                                          BusinessIdErrorCode::KeyNotFound,
                                          BusinessIdErrorCode::Encoding};
    BusinessIdError error(
        std::allocator_arg, Allocator(owner), codes[flags % std::size(codes)], name);
    auto definition =
        flags & 1 ? BusinessIdDefinition::legacy_daily_sequence(owner, name, "prefix", "namespace")
                  : BusinessIdDefinition::daily_sequence(owner, name, "prefix", "namespace");
    definition.split_by_date = flags & 2;
    definition.preserve_digits =
        static_cast<std::uint8_t>(std::get<std::uint64_t>(field(input, "digits").data));
    definition.separator = name;
    BusinessIdScope scope(
        std::allocator_arg, Allocator(owner), name, "Aggregate", "namespace", name);
    auto date = Date::parse(text(field(input, "date")));
    if (!date) {
        throw std::runtime_error("Invalid auxiliary diagnostic date");
    }
    BusinessIdGenerationRequest request(
        std::allocator_arg, Allocator(owner), definition, name, "Aggregate", *date);
    BusinessIdPlan plan(std::allocator_arg,
                        Allocator(owner),
                        scope,
                        name,
                        flags & 4 ? std::optional<std::string_view>(name) : std::nullopt,
                        definition.preserve_digits,
                        name,
                        flags,
                        number);
    BusinessIdAllocation allocation(std::allocator_arg, Allocator(owner), scope, number);
    BusinessIdValue id(std::allocator_arg, Allocator(owner), name);
    SearchModel model(std::allocator_arg, Allocator(owner));
    model.fields.emplace(String(name, owner), String("string", owner));
    model.relations.emplace(String("child", owner), String(name, owner));
    DynamicSearchWarning warning(std::allocator_arg, Allocator(owner), name, "filter", name);
    warning.code = "WarningCode";
    DynamicSearchFilter filter(
        std::allocator_arg, Allocator(owner), name, "$eq", field(input, "value"));
    DynamicSearchOrder order(std::allocator_arg, Allocator(owner), name, flags & 1);
    NormalizedDynamicSearch normalized(std::allocator_arg, Allocator(owner));
    if (flags & 8) {
        normalized.filters.emplace_back(filter);
        normalized.filters.emplace_back(filter);
    }
    normalized.orders.emplace_back(order);
    normalized.warnings.emplace_back(warning);
    SelectQuery query(std::allocator_arg, Allocator(owner), name);
    query.comment(name);
    DynamicSearchResult result(std::allocator_arg, Allocator(owner), query, normalized.warnings);
    DynamicSearchError search_error{"Invalid trusted search setup"};
    const auto& words = std::get<Json::Array>(field(input, "words").data);
    UnpackedDecimal unpacked{
        bool(flags & 1),
        static_cast<std::uint32_t>(std::get<std::uint64_t>(field(input, "scale").data)),
        static_cast<std::uint32_t>(std::get<std::uint64_t>(words[2].data)),
        static_cast<std::uint32_t>(std::get<std::uint64_t>(words[1].data)),
        static_cast<std::uint32_t>(std::get<std::uint64_t>(words[0].data))};
    Json::Object output(owner);
    auto observe = [&](std::string_view key, const auto& value) {
        output.emplace(key, formatted_debug(owner, value));
    };
    Json::Array code_output(owner), rounding_output(owner), error_output(owner), equalities(owner);
    Json::Array error_sources(owner), error_displays(owner), error_clones(owner);
    Json::Array formatted_errors(owner);
    for (auto code : codes) {
        code_output.emplace_back(formatted_debug(owner, code));
    }
    for (unsigned index = 0; index < 12; ++index) {
        rounding_output.emplace_back(formatted_debug(owner, static_cast<RoundingStrategy>(index)));
    }
    for (unsigned index = 0; index < 6; ++index) {
        DecimalError decimal_error(std::allocator_arg,
                                   Allocator(owner),
                                   static_cast<DecimalError::Code>(index),
                                   name,
                                   unpacked.scale);
        error_output.emplace_back(formatted_debug(owner, decimal_error));
        error_sources.emplace_back(decimal_error.source() == nullptr);
        error_displays.emplace_back(decimal_error.to_string(owner));
        error_clones.emplace_back(
            decimal_error == DecimalError(std::allocator_arg, Allocator(owner), decimal_error));
        formatted_errors.emplace_back(error_formats(owner, decimal_error));
    }
    auto clone_equal = [&](const auto& value) {
        using T = std::remove_cvref_t<decltype(value)>;
        equalities.emplace_back(value == T(std::allocator_arg, Allocator(owner), value));
    };
    clone_equal(error);
    clone_equal(definition);
    clone_equal(scope);
    clone_equal(request);
    clone_equal(plan);
    clone_equal(allocation);
    clone_equal(id);
    clone_equal(warning);
    clone_equal(filter);
    clone_equal(order);
    equalities.emplace_back(search_error == search_error);
    equalities.emplace_back(unpacked == unpacked);
    output.emplace("codes", Json(owner, std::move(code_output)));
    output.emplace("rounding", Json(owner, std::move(rounding_output)));
    output.emplace("decimal_errors", Json(owner, std::move(error_output)));
    output.emplace("decimal_error_sources", Json(owner, std::move(error_sources)));
    output.emplace("decimal_error_displays", Json(owner, std::move(error_displays)));
    output.emplace("decimal_error_clones", Json(owner, std::move(error_clones)));
    output.emplace("decimal_error_formats", Json(owner, std::move(formatted_errors)));
    output.emplace("clone_equal", Json(owner, std::move(equalities)));
    output.emplace("warning_json", warning.to_json(owner));
    observe("error", error);
    observe("definition", definition);
    observe("scope", scope);
    observe("request", request);
    observe("plan", plan);
    observe("allocation", allocation);
    observe("id", id);
    observe("model", model);
    observe("warning", warning);
    observe("filter", filter);
    observe("order", order);
    observe("normalized", normalized);
    observe("result", result);
    observe("search_error", search_error);
    observe("unpacked", unpacked);
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
            throw std::runtime_error("Invalid auxiliary diagnostic fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
