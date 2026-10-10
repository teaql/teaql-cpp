#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>

namespace checker_fixture {
using namespace teaql;

struct Item : EntityDefaults<Item> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Item";
    Value id, version;
    String name;
    std::int64_t score = 0;
    std::optional<FieldSet> loaded, dirty;
    std::optional<MutationValues> boundary;
    bool omit_name = false, null_name = false;

    Item(std::allocator_arg_t, Allocator a)
        : id(a.resource()), version(a.resource()), name(a.resource()) {}

    Item(std::allocator_arg_t, Allocator a, const Item& other)
        : id(a.resource(), other.id), version(a.resource(), other.version),
          name(other.name, a.resource()), score(other.score),
          loaded(detail::clone_in(a.resource(), other.loaded)),
          dirty(detail::clone_in(a.resource(), other.dirty)),
          boundary(detail::clone_in(a.resource(), other.boundary)), omit_name(other.omit_name),
          null_name(other.null_name) {}

    Item(std::allocator_arg_t, Allocator a, Item&& other)
        : Item(detail::move_to(a.resource(), std::move(other))) {}

    Item(const Item&) = delete;
    Item(Item&&) noexcept = default;

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor out(std::allocator_arg, Allocator(r), ENTITY_NAME);
        out.properties.emplace_back("id", DataType::U64);
        out.properties.back().id().not_null();
        out.properties.emplace_back("version", DataType::I64);
        out.properties.back().version().not_null();
        out.properties.emplace_back("name", DataType::Text);
        out.properties.back().not_null();
        out.properties.emplace_back("score", DataType::I64);
        out.properties.back().not_null();
        out.properties.emplace_back("optional", DataType::Text);
        return out;
    }

    static Result<Item, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<Item, EntityError>;
        Item out(std::allocator_arg, Allocator(r));
        for (auto name : {"id", "version"}) {
            if (auto* value = row.get(name)) {
                (name == std::string_view("id") ? out.id : out.version) = Value(r, *value);
            }
        }
        if (auto* value = row.get("name");
            value && !std::holds_alternative<std::monostate>(value->data)) {
            auto text = value->try_text();
            if (!text) {
                return R::err(
                    r, EntityError(std::allocator_arg, Allocator(r), "Item", "name must be Text"));
            }
            out.name = *text;
        }
        if (auto* value = row.get("score");
            value && !std::holds_alternative<std::monostate>(value->data)) {
            auto score = value->try_i64();
            if (!score) {
                return R::err(
                    r, EntityError(std::allocator_arg, Allocator(r), "Item", "score must be I64"));
            }
            out.score = *score;
        }
        return R::ok(r, std::move(out));
    }

    void set_checker_loaded_fields(FieldSet fields) {
        loaded.emplace(std::move(fields));
    }

    void set_checker_dirty_fields(FieldSet fields, const MutationValues& values) {
        dirty.emplace(std::move(fields));
        boundary.emplace(std::allocator_arg, Allocator(memory_resource()), values);
    }

    bool is_field_loaded(std::string_view field) const {
        return !loaded || loaded->contains(field);
    }

    std::optional<FieldSet> dirty_fields() const {
        return detail::clone_in(memory_resource(), dirty);
    }

    MutationValues into_values() && {
        auto r = memory_resource();
        MutationValues out(r);
        out.fields.emplace(String("id", r), std::move(id));
        out.fields.emplace(String("version", r), std::move(version));
        if (!omit_name) {
            out.fields.emplace(String("name", r), null_name ? Value(r) : Value(r, name));
        }
        out.fields.emplace(String("score", r), Value(r, score));
        for (auto key : {"_loaded_fields", "_dirty_fields"}) {
            auto& fields = key == std::string_view("_loaded_fields") ? loaded : dirty;
            if (fields) {
                Value::List list(r);
                for (const auto& field : *fields) {
                    list.emplace_back(field);
                }
                out.fields.emplace(String(key, r), Value(r, list));
            }
        }
        return out;
    }
};

static_assert(Entity<Item>);

inline Json fields_json(Memory r, const std::optional<FieldSet>& fields) {
    if (!fields) {
        return Json(r);
    }
    Json::Array out(r);
    for (const auto& field : *fields) {
        out.emplace_back(field);
    }
    return Json(r, out);
}

struct Fix final : TypedChecker<Item> {
    using allocator_type = Allocator;
    String action;
    mutable Vector<Json> calls;

    Fix(std::allocator_arg_t, Allocator a, std::string_view action)
        : action(action, a.resource()), calls(a.resource()) {}

    Memory memory_resource() const {
        return action.get_allocator().resource();
    }

    void check_and_fix_typed(const UserContext& context,
                             Item& item,
                             CheckObjectStatus status,
                             const ObjectLocation& location,
                             CheckResults& results) const override {
        auto r = memory_resource();
        auto seen = detail::model_json(r);
        detail::model_put(seen, "status", check_object_status_name(status));
        detail::model_put(seen, "loaded", fields_json(r, item.loaded));
        detail::model_put(seen, "dirty", fields_json(r, item.dirty));
        detail::model_put(
            seen,
            "boundary",
            item.boundary
                ? Value(item.boundary->memory_resource(), item.boundary->fields).to_json_value(r)
                : Json(r));
        detail::model_put(seen, "name", item.name);
        detail::model_put(seen, "score", item.score);
        detail::model_put(seen, "language", language_code(context.language()));
        calls.emplace_back(std::move(seen));
        if (action == "fill" || action == "fill_and_report") {
            item.name = "fixed";
        }
        if (action == "increment") {
            ++item.score;
        }
        if (action == "set_id") {
            item.id = Value(item.memory_resource(), std::uint64_t{7});
        }
        if (action == "remove_name") {
            item.omit_name = true;
        }
        if (action == "null_name") {
            item.null_name = true;
        }
        if (action == "dirty") {
            if (!item.dirty) {
                item.dirty.emplace(item.memory_resource());
            }
            item.dirty->emplace("name");
        }
        if (action == "report" || action == "fill_and_report") {
            auto dest = results.get_allocator().resource();
            auto path = detail::clone_in(dest, location);
            path.member("name");
            results.emplace_back(CheckResult::required(dest, std::move(path)));
        }
    }
};

struct Plain final : Checker {
    using allocator_type = Allocator;
    String action, name;
    mutable std::size_t calls = 0;

    Plain(std::allocator_arg_t,
          Allocator a,
          std::string_view action,
          std::string_view name = "Item")
        : action(action, a.resource()), name(name, a.resource()) {}

    Memory memory_resource() const {
        return action.get_allocator().resource();
    }

    std::string_view entity() const override {
        return name;
    }

    void check_and_fix(const UserContext&,
                       EntityValues& values,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        ++calls;
        auto r = values.memory_resource();
        if (action == "fill") {
            values.fields.insert_or_assign(String("name", r), Value(r, "fixed"));
        }
        if (action == "set_id") {
            values.fields.insert_or_assign(String("id", r), Value(r, std::uint64_t{7}));
        }
        if (action == "report" || action == "report_twice") {
            auto dest = results.get_allocator().resource();
            auto path = detail::clone_in(dest, location);
            path.member("name");
            results.emplace_back(CheckResult::required(dest, std::move(path)));
            if (action == "report_twice") {
                results.emplace_back(results.back());
            }
        }
    }
};
}
