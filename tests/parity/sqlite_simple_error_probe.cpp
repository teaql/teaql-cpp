#include <teaql/provider_sqlite/simple_error.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

using Error = teaql::detail::SqliteSimpleError;

static Error make_error(Memory owner, const Json& input) {
    auto kind = static_cast<Error::Kind>(std::get<std::uint64_t>(field(input, "kind").data));
    switch (kind) {
    case Error::Kind::IntegralValueOutOfRange:
        return Error::integral_range(owner,
                                     number<std::size_t>(field(input, "index")),
                                     number<std::int64_t>(field(input, "value")));
    case Error::Kind::InvalidParameterName:
    case Error::Kind::InvalidColumnName:
        return Error::named(owner, kind, text(field(input, "name")));
    case Error::Kind::StatementChangedRows:
    case Error::Kind::InvalidDatabaseIndex:
        return Error::indexed(owner, kind, number<std::size_t>(field(input, "index")));
    case Error::Kind::InvalidFunctionParameterType:
        return Error::function_type(
            owner,
            number<std::size_t>(field(input, "index")),
            static_cast<SqliteValueType>(std::get<std::uint64_t>(field(input, "type").data)));
    default:
        return Error::unit(owner, kind);
    }
}

static Json evaluate(Memory owner, const Json& input) {
    auto error = make_error(owner, input);
    auto other = make_error(owner, input);
    Json::Object result(owner);
    result.emplace("debug", formatted_debug(owner, error));
    result.emplace("display", Json(owner, error.to_string(owner)));
    result.emplace("self_equal", Json(owner, error == error));
    result.emplace("same_payload_equal", Json(owner, error == other));
    if (auto* changed = optional(input, "changed")) {
        auto different = make_error(owner, *changed);
        result.emplace("changed_equal", Json(owner, error == different));
    } else {
        result.emplace("changed_equal", Json(owner));
    }
    result.emplace("source_none", Json(owner, error.source() == nullptr));
    return Json(owner, std::move(result));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::invalid_argument("Invalid simple error fixture JSON");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
