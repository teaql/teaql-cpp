#pragma once
#include <teaql/core/business_id.hpp>
#include <teaql/core/detail/text.hpp>
#include <teaql/core/value.hpp>
#include <teaql/core/debug.hpp>
#include <compare>

namespace teaql {
class GeneratedBusinessIdAccess;

// Each generated definition tag creates a distinct application value type.
template <class Definition>
class GeneratedBusinessId {
public:
    using allocator_type = Allocator;

    GeneratedBusinessId(std::allocator_arg_t, Allocator a, const GeneratedBusinessId& other)
        : value_(other.value_, a.resource()) {}

    GeneratedBusinessId(std::allocator_arg_t, Allocator a, GeneratedBusinessId&& other)
        : value_(std::move(other.value_), a.resource()) {}

    GeneratedBusinessId(const GeneratedBusinessId&) = delete;
    GeneratedBusinessId(GeneratedBusinessId&&) noexcept = default;

    GeneratedBusinessId& operator=(GeneratedBusinessId&& other) {
        value_ = std::move(other.value_);
        return *this;
    }

    static Result<GeneratedBusinessId, String> create(Memory resource, std::string_view value) {
        detail::trim(value);
        if (value.empty()) {
            String error(Definition::field_name, resource);
            error += " must not be blank";
            return Result<GeneratedBusinessId, String>::err(resource, std::move(error));
        }
        return Result<GeneratedBusinessId, String>::ok(
            resource, GeneratedBusinessId(String(value, resource)));
    }

    std::string_view asStr() const noexcept {
        return value_;
    }

    Memory memory_resource() const noexcept {
        return value_.get_allocator().resource();
    }

    static BusinessIdDefinition definition(Memory resource) {
        return Definition::definition(resource);
    }

    Value toValue(Memory destination) const {
        return Value(destination, value_);
    }

    bool operator==(const GeneratedBusinessId& other) const noexcept {
        return asStr() == other.asStr();
    }

    std::strong_ordering operator<=>(const GeneratedBusinessId& other) const noexcept {
        return asStr() <=> other.asStr();
    }

private:
    friend class GeneratedBusinessIdAccess;

    explicit GeneratedBusinessId(String value) : value_(std::move(value)) {}

    String value_;
};

// Framework hydration/getters may represent canonical empty storage. Application
// creation always uses create(); no public default or string conversion exists.
class GeneratedBusinessIdAccess {
public:
    template <class T>
    static T from_canonical(Memory resource, std::string_view value) {
        return T(String(value, resource));
    }

    template <class T>
    static T from_canonical(Memory resource, String&& value) {
        return T(String(std::move(value), resource));
    }
};

template <class Definition>
void write_debug(DebugWriter& writer, const GeneratedBusinessId<Definition>& value) {
    writer.tuple(Definition::type_name, [&](auto& output) {
        output.text(value.asStr());
    });
}
}

namespace std {
template <class Definition>
struct hash<teaql::GeneratedBusinessId<Definition>> {
    size_t operator()(const teaql::GeneratedBusinessId<Definition>& value) const noexcept {
        return hash<string_view>{}(value.asStr());
    }
};
}
