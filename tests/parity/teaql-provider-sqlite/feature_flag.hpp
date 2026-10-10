#pragma once
#include <teaql/core/entity.hpp>
#include <teaql/core/detail/debug.hpp>

namespace original_provider {
using namespace teaql;

// Handwritten counterpart of the pinned private derive model. This is field
// mapping, never provider decoding or an implementation of the generator.
struct FeatureFlagRow : EntityDefaults<FeatureFlagRow> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "FeatureFlag";
    std::uint64_t id = 0;
    std::int64_t revision = 0;
    bool enabled = false;
    std::optional<bool> optional_enabled;

    FeatureFlagRow(std::allocator_arg_t, Allocator a) : owner_(a.resource()) {}

    FeatureFlagRow(std::allocator_arg_t, Allocator a, const FeatureFlagRow& other)
        : id(other.id), revision(other.revision), enabled(other.enabled),
          optional_enabled(other.optional_enabled), owner_(a.resource()) {}

    FeatureFlagRow(std::allocator_arg_t, Allocator a, FeatureFlagRow&& other)
        : FeatureFlagRow(std::allocator_arg, a, std::as_const(other)) {}

    FeatureFlagRow(const FeatureFlagRow&) = delete;
    FeatureFlagRow(FeatureFlagRow&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return owner_;
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("feature_flags");
        result.property("id", DataType::U64).id().not_null();
        result.property("version", DataType::I64).version().not_null();
        result.property("enabled", DataType::Bool).not_null();
        result.property("optional_enabled", DataType::Bool);
        return result;
    }

    static Result<FeatureFlagRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<FeatureFlagRow, EntityError>;
        try {
            FeatureFlagRow result(std::allocator_arg, Allocator(owner));
            if (auto* id = row.get("id"); id && !std::holds_alternative<std::monostate>(id->data)) {
                auto value = id->try_u64();
                if (!value) {
                    invalid(owner,
                            "id",
                            *id,
                            std::holds_alternative<std::int64_t>(id->data)
                                ? "negative i64 cannot map to u64"
                            : std::holds_alternative<Decimal>(id->data)
                                ? "decimal cannot map exactly to u64"
                                : "");
                }
                result.id = *value;
            }
            if (auto* version = row.get("version");
                version && !std::holds_alternative<std::monostate>(version->data)) {
                if (!std::holds_alternative<std::int64_t>(version->data) &&
                    !std::holds_alternative<std::uint64_t>(version->data)) {
                    invalid(owner, "version", *version);
                }
                auto value = version->try_i64();
                if (!value) {
                    invalid(owner, "version", *version, "u64 out of i64 range");
                }
                result.revision = *value;
            }
            if (auto* enabled = row.get("enabled");
                enabled && !std::holds_alternative<std::monostate>(enabled->data)) {
                result.enabled = boolean(owner, "enabled", *enabled);
            }
            if (auto* enabled = row.get("optional_enabled");
                enabled && !std::holds_alternative<std::monostate>(enabled->data)) {
                result.optional_enabled = boolean(owner, "optional_enabled", *enabled);
            }
            return Output::ok(owner, std::move(result));
        } catch (const EntityError& error) {
            return Output::err(owner, detail::clone_in(owner, error));
        }
    }

    Value id_value() const {
        return Value(owner_, id);
    }

    std::int64_t version() const noexcept {
        return revision;
    }

    MutationValues into_values() && {
        MutationValues result(owner_);
        result.fields.emplace("id", Value(owner_, id));
        result.fields.emplace("version", Value(owner_, revision));
        result.fields.emplace("enabled", Value(owner_, enabled));
        result.fields.emplace("optional_enabled",
                              optional_enabled ? Value(owner_, *optional_enabled) : Value(owner_));
        return result;
    }

private:
    Memory owner_;

    [[noreturn]] static void
    invalid(Memory owner, std::string_view name, const Value& value, std::string_view reason = {}) {
        String message("invalid field ", owner);
        message += name;
        message += ": ";
        message += reason.empty() ? detail::debug_value(owner, value) : String(reason, owner);
        throw EntityError(std::allocator_arg, Allocator(owner), ENTITY_NAME, message);
    }

    static bool boolean(Memory owner, std::string_view name, const Value& value) {
        if (auto actual = value.try_bool()) {
            return *actual;
        }
        if (auto* actual = std::get_if<std::int64_t>(&value.data);
            actual && (*actual == 0 || *actual == 1)) {
            return *actual == 1;
        }
        if (auto* actual = std::get_if<std::uint64_t>(&value.data);
            actual && (*actual == 0 || *actual == 1)) {
            return *actual == 1;
        }
        invalid(owner, name, value);
    }
};
}
