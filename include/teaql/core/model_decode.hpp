#pragma once
#include <teaql/core/result.hpp>
#include <string_view>
namespace teaql {
// Field names refer to static schema literals, never to input storage.
struct ModelDecodeError {
    enum class Code { ExpectedStruct, MissingField, ExtraElements, ExpectedString, ExpectedInteger, ExpectedArray, ExpectedObject };
    Code code;
    std::string_view field;
    bool operator==(const ModelDecodeError&) const = default;
};
template<class T> using ModelResult = Result<T, ModelDecodeError>;
}
