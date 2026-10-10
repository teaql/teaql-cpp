#pragma once
#include <teaql/runtime/entity_save.hpp>

namespace save_fixture {
using namespace teaql;

struct Model : EntityDefaults<Model> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Parent";
    MutationValues values;
    std::optional<FieldSet> loaded, dirty;
    std::optional<EntitySnapshot> original;
    std::optional<String> comment;
    bool fresh = false, removed = false;

    Model(std::allocator_arg_t, Allocator a) : values(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, const Model& other)
        : values(std::allocator_arg, a, other.values),
          loaded(detail::clone_in(a.resource(), other.loaded)),
          dirty(detail::clone_in(a.resource(), other.dirty)),
          original(detail::clone_in(a.resource(), other.original)),
          comment(detail::clone_in(a.resource(), other.comment)), fresh(other.fresh),
          removed(other.removed) {}

    Model(const Model&) = delete;
    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor out(std::allocator_arg, Allocator(r), ENTITY_NAME);
        out.property("id", DataType::U64).id();
        out.property("version", DataType::I64).version();
        out.property("name", DataType::Text);
        return out;
    }

    static Result<Model, EntityError> from_compact_row(Memory r, CompactRow row) {
        Model out(std::allocator_arg, Allocator(r));
        out.values.fields = row.into_map(r);
        return Result<Model, EntityError>::ok(r, std::move(out));
    }

    MutationValues into_values() && {
        return std::move(values);
    }

    bool is_field_loaded(std::string_view name) const {
        return !loaded || loaded->contains(name);
    }

    std::optional<FieldSet> dirty_fields() const {
        return detail::clone_in(memory_resource(), dirty);
    }

    std::optional<EntitySnapshot> original_values() const {
        return detail::clone_in(memory_resource(), original);
    }

    std::optional<String> get_comment() const {
        return detail::clone_in(memory_resource(), comment);
    }

    bool is_new() const {
        return fresh;
    }

    bool is_marked_as_delete() const {
        return removed;
    }
};

static_assert(Entity<Model>);

struct Checker final : teaql::Checker {
    using allocator_type = Allocator;
    String name, action;
    mutable Vector<String> calls;

    Checker(std::allocator_arg_t, Allocator a, std::string_view name, std::string_view action)
        : name(name, a.resource()), action(action, a.resource()), calls(a.resource()) {}

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    std::string_view entity() const override {
        return name;
    }

    void check_and_fix(const UserContext&,
                       EntityValues& values,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        calls.emplace_back(location.to_string(memory_resource()));
        auto r = values.memory_resource();
        if (action == "fill") {
            values.fields.insert_or_assign(String("name", r), Value(r, "fixed"));
        }
        if (action == "set_id") {
            values.fields.insert_or_assign(String("id", r), Value(r, std::uint64_t{77}));
        }
        if (action == "erase") {
            values.fields.erase(String("name", r));
        }
        if (action == "error") {
            results.emplace_back(CheckResult::required(
                results.get_allocator().resource(),
                detail::clone_in(results.get_allocator().resource(), location).member("name")));
        }
    }
};
}
