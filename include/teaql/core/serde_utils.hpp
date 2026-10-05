#pragma once
#include <teaql/core/json.hpp>
#include <teaql/core/result.hpp>
#include <teaql/core/detail/text.hpp>

namespace teaql::serde_utils {
enum class Error { ExpectedString, ExpectedOptionalString };

namespace trimmed_string {
inline Json serialize(Memory destination, std::string_view value) {
    detail::trim(value);
    return Json(destination, value);
}

inline Result<String, Error> deserialize(Memory destination, const Json& value) {
    using Outcome = Result<String, Error>;
    if (const auto* text = std::get_if<String>(&value.data)) {
        std::string_view view(*text);
        detail::trim(view);
        return Outcome::ok(destination, String(view, destination));
    }
    return Outcome::err(destination, Error::ExpectedString);
}
}

namespace trimmed_opt_string {
inline Json serialize(Memory destination, std::optional<std::string_view> value) {
    return value ? trimmed_string::serialize(destination, *value) : Json(destination);
}

inline Result<std::optional<String>, Error> deserialize(Memory destination, const Json& value) {
    using Outcome = Result<std::optional<String>, Error>;
    if (std::holds_alternative<std::monostate>(value.data))
        return Outcome::ok(destination, std::optional<String>{});
    if (const auto* text = std::get_if<String>(&value.data)) {
        std::string_view view(*text);
        detail::trim(view);
        return Outcome::ok(destination, std::optional<String>(String(view, destination)));
    }
    return Outcome::err(destination, Error::ExpectedOptionalString);
}
}
}
