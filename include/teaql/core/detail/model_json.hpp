#pragma once
#include <teaql/core/json.hpp>
#include <teaql/core/model_decode.hpp>
#include <limits>

namespace teaql::detail {
class ModelReader {
public:
    ModelReader(const Json& input, std::size_t count)
        : object_(std::get_if<Json::Object>(&input.data)),
          array_(std::get_if<Json::Array>(&input.data)) {
        if (!object_ && !array_)
            throw ModelDecodeError{ModelDecodeError::Code::ExpectedStruct, {}};
        if (array_ && array_->size() > count)
            throw ModelDecodeError{ModelDecodeError::Code::ExtraElements, {}};
    }

    const Json* field(std::string_view name,
                      std::size_t index,
                      bool optional = false,
                      bool defaulted = false) const {
        if (object_) {
            auto it = object_->find(name);
            if (it != object_->end())
                return &it->second;
            if (optional || defaulted)
                return nullptr;
        } else {
            if (index < array_->size())
                return &(*array_)[index];
            if (defaulted)
                return nullptr;
        }
        throw ModelDecodeError{ModelDecodeError::Code::MissingField, name};
    }

private:
    const Json::Object* object_;
    const Json::Array* array_;
};

inline const String& model_string(const Json& value, std::string_view field) {
    if (auto* s = std::get_if<String>(&value.data))
        return *s;
    throw ModelDecodeError{ModelDecodeError::Code::ExpectedString, field};
}

inline std::optional<String>
model_optional_string(Memory r, const Json* value, std::string_view field) {
    if (!value || std::holds_alternative<std::monostate>(value->data))
        return {};
    return String(model_string(*value, field), r);
}

template <class T>
T model_integer(const Json& value, std::string_view field) {
    if (auto* n = std::get_if<std::int64_t>(&value.data)) {
        if constexpr (std::is_signed_v<T>) {
            if (*n >= std::numeric_limits<T>::min() && *n <= std::numeric_limits<T>::max())
                return T(*n);
        } else if (*n >= 0 && std::uint64_t(*n) <= std::numeric_limits<T>::max())
            return T(*n);
    }
    if (auto* n = std::get_if<std::uint64_t>(&value.data))
        if (*n <= std::uint64_t(std::numeric_limits<T>::max()))
            return T(*n);
    throw ModelDecodeError{ModelDecodeError::Code::ExpectedInteger, field};
}

inline const Json::Array& model_array(const Json& value, std::string_view field) {
    if (auto* a = std::get_if<Json::Array>(&value.data))
        return *a;
    throw ModelDecodeError{ModelDecodeError::Code::ExpectedArray, field};
}

inline const Json::Object& model_object(const Json& value, std::string_view field) {
    if (auto* o = std::get_if<Json::Object>(&value.data))
        return *o;
    throw ModelDecodeError{ModelDecodeError::Code::ExpectedObject, field};
}

template <class T>
T decoded(ModelResult<T> result) {
    if (!result)
        throw result.error();
    return std::move(result).value();
}

template <class T, class Decode>
ModelResult<T> decode_model(Memory r, Decode&& decode) {
    try {
        return ModelResult<T>::ok(r, decode());
    } catch (const ModelDecodeError& error) {
        return ModelResult<T>::err(r, error);
    }
}

inline Json model_json(Memory r) {
    Json result(r);
    result.data.emplace<Json::Object>(r);
    return result;
}

template <class T>
void model_put(Json& object, std::string_view field, T&& value) {
    auto r = object.memory_resource();
    std::get<Json::Object>(object.data).emplace(String(field, r), Json(r, std::forward<T>(value)));
}

inline void
model_optional(Json& object, std::string_view field, const std::optional<String>& value) {
    if (value)
        model_put(object, field, *value);
}
}
