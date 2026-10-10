#include <teaql/core/entity.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/entity_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

struct Legacy : BaseEntityDefaults<Legacy> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Legacy";
    BaseEntityData data;

    Legacy(std::allocator_arg_t, Allocator a) : data(std::allocator_arg, a) {}

    Legacy(std::allocator_arg_t, Allocator a, const Legacy& other)
        : data(std::allocator_arg, a, other.data) {}

    Legacy(Legacy&&) noexcept = default;

    Memory memory_resource() const {
        return data.memory_resource();
    }

    const BaseEntityData& base() const {
        return data;
    }

    BaseEntityData& base_mut() {
        return data;
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<Legacy, EntityError> from_compact_row(Memory r, CompactRow row) {
        using Outcome = Result<Legacy, EntityError>;
        auto base = BaseEntityData::from_values_map(r, std::move(row).into_map(r));
        if (!base) {
            return Outcome::err(r, std::move(base).error());
        }
        Legacy result(std::allocator_arg, Allocator(r));
        result.data = std::move(base).value();
        return Outcome::ok(r, std::move(result));
    }

    MutationValues into_values() && {
        return MutationValues(memory_resource(), data.to_values_map(memory_resource()));
    }

    Value id_value() const {
        return Value(memory_resource(), data.id);
    }

    std::int64_t version() const {
        return data.version;
    }
};

static Json record(Memory r, const Record& values) {
    auto result = detail::model_json(r);
    for (const auto& [key, item] : values) {
        detail::model_put(result, key, detail::debug_value(r, item));
    }
    return result;
}

static Json defaults(Memory r, const Legacy& entity) {
    auto result = detail::model_json(r);
    detail::model_put(result, "loaded", entity.is_field_loaded("unknown"));
    detail::model_put(result, "dirty_none", !entity.dirty_fields());
    detail::model_put(result, "snapshot_none", !entity.original_values());
    detail::model_put(result, "new", entity.is_new());
    detail::model_put(result, "delete", entity.is_marked_as_delete());
    auto comment = entity.get_comment();
    detail::model_put(result, "comment", comment ? Json(r, *comment) : Json(r));
    return result;
}

static Json state(Memory r, const Legacy& entity, std::string_view key) {
    auto result = detail::model_json(r);
    detail::model_put(result, "id", std::to_string(entity.id()));
    detail::model_put(result, "version", std::to_string(entity.version_value()));
    detail::model_put(result, "id_value", detail::debug_value(r, entity.id_value()));
    detail::model_put(result, "versioned", std::to_string(entity.version()));
    detail::model_put(result, "dynamic", record(r, entity.data.dynamic_values));
    detail::model_put(result, "values", record(r, entity.data.to_values_map(r)));
    auto item = entity.dynamic(key);
    detail::model_put(result, "value", item ? Json(r, detail::debug_value(r, *item)) : Json(r));
    auto i = entity.dynamic_i64(key);
    auto u = entity.dynamic_u64(key);
    auto d = entity.dynamic_decimal(key);
    auto f = entity.dynamic_f64(key);
    auto t = entity.dynamic_text(key);
    auto b = entity.dynamic_bool(key);
    detail::model_put(result, "i64", i ? Json(r, std::to_string(*i)) : Json(r));
    detail::model_put(result, "u64", u ? Json(r, std::to_string(*u)) : Json(r));
    detail::model_put(result, "decimal", d ? Json(r, d->to_string(r)) : Json(r));
    detail::model_put(result, "f64", f ? Json(r, detail::debug_value(r, Value(r, *f))) : Json(r));
    detail::model_put(result, "text", t ? Json(r, *t) : Json(r));
    detail::model_put(result, "bool", b ? Json(r, *b) : Json(r));
    return result;
}

static Json evaluate(Memory r, const Json& input) {
    if (auto* kind = optional(input, "kind"); kind && text(*kind) == "error_source") {
        EntityError error(std::allocator_arg,
                          Allocator(r),
                          text(field(input, "entity")),
                          text(field(input, "message")));
        Json::Object result(r);
        result.emplace("source_none", Json(r, error.source() == nullptr));
        result.emplace("debug", Json(r, debug_string(r, error)));
        result.emplace("display", Json(r, error.to_string(r)));
        result.emplace("clone_equal", Json(r, error == detail::clone_in(r, error)));
        return Json(r, std::move(result));
    }
    Legacy entity(std::allocator_arg, Allocator(r));
    entity.data.with_id(number<std::uint64_t>(field(input, "id")))
        .with_version(number<std::int64_t>(field(input, "version")));
    auto initial = defaults(r, entity);
    entity.mark_as_new();
    entity.set_comment("ignored");
    FieldSet loaded(r), dirty(r);
    loaded.emplace("id");
    dirty.emplace("payload");
    entity.set_checker_loaded_fields(std::move(loaded));
    MutationValues changes(r);
    entity.set_checker_dirty_fields(std::move(dirty), changes);
    int marker = 0;
    entity.on_loaded(AnyRef(marker));
    auto after = defaults(r, entity);
    Json::Array steps(r);
    for (const auto& op : std::get<Json::Array>(field(input, "ops").data)) {
        auto kind = text(field(op, "kind"));
        auto* key_input = optional(op, "key");
        auto key = key_input ? text(*key_input) : std::string_view{};
        std::optional<Value> old;
        if (kind == "with") {
            entity.data.with_dynamic(key, value(r, field(op, "value")));
        } else if (kind == "put") {
            old = entity.put_dynamic(key, value(r, field(op, "value")));
        } else if (kind == "remove") {
            old = entity.data.remove_dynamic(key);
        } else if (kind == "id") {
            entity.set_id(number<std::uint64_t>(field(op, "value")));
        } else if (kind == "version") {
            entity.set_version(number<std::int64_t>(field(op, "value")));
        } else if (kind != "get") {
            throw std::invalid_argument("Unknown entity operation");
        }
        auto step = detail::model_json(r);
        detail::model_put(step, "old", old ? Json(r, detail::debug_value(r, *old)) : Json(r));
        detail::model_put(step, "state", state(r, entity, key));
        steps.emplace_back(std::move(step));
    }
    auto copy = Legacy(std::allocator_arg, Allocator(r), entity);
    auto projected = std::move(copy).into_json(r);
    auto parsed = Legacy::from_compact_row_with_context(
        r,
        CompactRow(std::allocator_arg, Allocator(r), entity.data.to_values_map(r)),
        AnyRef(entity));
    auto load = detail::model_json(r);
    if (parsed) {
        detail::model_put(load, "state", state(r, parsed.value(), "payload"));
        detail::model_put(load, "defaults", defaults(r, parsed.value()));
    } else {
        detail::model_put(load, "entity", parsed.error().entity);
        detail::model_put(load, "message", parsed.error().message);
    }
    auto result = detail::model_json(r);
    detail::model_put(result, "initial_defaults", initial);
    detail::model_put(result, "after_defaults", after);
    detail::model_put(result, "steps", steps);
    detail::model_put(result, "json", projected);
    detail::model_put(result, "load", load);
    return result;
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::invalid_argument("Invalid entity defaults input");
            }
            std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
