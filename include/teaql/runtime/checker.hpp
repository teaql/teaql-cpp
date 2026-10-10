#pragma once
#include <teaql/runtime/check_result.hpp>
#include <teaql/runtime/graph.hpp>
#include <teaql/core/entity.hpp>

namespace teaql {
inline constexpr std::string_view check_object_status_field = "__teaql_object_status";
enum class CheckObjectStatus { Create, Update, Unknown };
std::string_view check_object_status_name(CheckObjectStatus);
CheckObjectStatus check_object_status(const EntityValues&);
Value check_object_status_value(Memory, CheckObjectStatus);
void mark_entity_status(EntityValues&, CheckObjectStatus);
void clear_entity_status(EntityValues&);

class UserContext;

class Checker {
public:
    virtual ~Checker() = default;
    virtual std::string_view entity() const = 0;
    virtual void check_and_fix(const UserContext&,
                               EntityValues&,
                               const ObjectLocation&,
                               CheckResults&) const = 0;
    virtual void required(const EntityValues&,
                          std::string_view field,
                          const ObjectLocation&,
                          CheckResults&) const;
    virtual void min_string_length(const EntityValues&,
                                   std::string_view field,
                                   std::size_t,
                                   const ObjectLocation&,
                                   CheckResults&) const;
    virtual void max_string_length(const EntityValues&,
                                   std::string_view field,
                                   std::size_t,
                                   const ObjectLocation&,
                                   CheckResults&) const;
};

using CheckerBorrow = ResourceBorrow<const Checker>;

class CheckerRegistry {
public:
    virtual ~CheckerRegistry() = default;
    virtual Memory memory_resource() const noexcept = 0;
    virtual std::optional<CheckerBorrow> checker(Memory, std::string_view entity) const = 0;
};

namespace detail {
Arena& checker_reference_origin(const UserContext&, std::uint64_t id);
Arena& checker_call_arena(const UserContext&);

// Synchronous callbacks cannot join their own stack during Context teardown.
class CheckerCallScope {
public:
    explicit CheckerCallScope(Arena& arena) {
        registration_.attach(arena, this, [](void*) noexcept {
            std::terminate();
        });
    }

private:
    Arena::Quiescence registration_;
};

template <class Owner, class T>
RegisteredResource checker_resource(Arena& destination, Owner& owner, T& object) {
    auto& origin = resource_arena(owner.memory_resource());
    auto slot = origin.slot_for_address(&object);
    if (!slot) {
        throw std::invalid_argument("Checker resources must be managed Context objects");
    }
    return ContextResources::entry(destination, owner.memory_resource(), object, slot);
}

std::optional<FieldSet> take_checker_fields(EntityValues&, std::string_view name);
bool missing_or_untyped_null(const Record&, std::string_view name);
}

class InMemoryCheckerRegistry final : public CheckerRegistry {
public:
    using allocator_type = Allocator;

    InMemoryCheckerRegistry(std::allocator_arg_t, Allocator a) : checkers_(a.resource()) {}

    InMemoryCheckerRegistry(std::allocator_arg_t, Allocator a, const InMemoryCheckerRegistry& other)
        : checkers_(other.checkers_, a.resource()) {}

    InMemoryCheckerRegistry(const InMemoryCheckerRegistry&) = delete;

    Memory memory_resource() const noexcept override {
        return checkers_.get_allocator().resource();
    }

    template <class Owner, class C>
        requires std::derived_from<C, Checker>
    void register_checker(Owner& owner, C& checker) {
        auto r = memory_resource();
        auto registered = detail::checker_resource(detail::resource_arena(r), owner, checker);
        CheckerBorrow borrow(
            std::allocator_arg, Allocator(r), checker, *registered.owner, registered.slot);
        install(std::move(borrow));
    }

    template <class C>
        requires std::derived_from<C, Checker>
    void register_checker(const UserContext& context, Ref<C> reference) {
        auto& origin = detail::checker_reference_origin(context, reference.id());
        auto& checker = *static_cast<C*>(
            origin.resolve(detail::reference_slot(reference.id()), typeid(C), reference.id()));
        auto r = memory_resource();
        auto registered = detail::ContextResources::entry(
            detail::resource_arena(r), &origin, checker, detail::reference_slot(reference.id()));
        CheckerBorrow borrow(std::allocator_arg, Allocator(r), checker, origin, registered.slot);
        install(std::move(borrow));
    }

    template <class Owner, class C>
        requires std::derived_from<C, Checker>
    InMemoryCheckerRegistry& with_checker(Owner& owner, C& checker) & {
        register_checker(owner, checker);
        return *this;
    }

    template <class C>
        requires std::derived_from<C, Checker>
    InMemoryCheckerRegistry& with_checker(const UserContext& context, Ref<C> reference) & {
        register_checker(context, reference);
        return *this;
    }

    std::optional<CheckerBorrow> checker(Memory r, std::string_view entity) const override {
        auto found = checkers_.find(entity);
        if (found == checkers_.end()) {
            return {};
        }
        return detail::clone_in(r, found->second);
    }

private:
    void install(CheckerBorrow borrow) {
        detail::CheckerCallScope active(detail::resource_arena(memory_resource()));
        String name(borrow.get().entity(), memory_resource());
        auto found = checkers_.find(name);
        if (found == checkers_.end()) {
            checkers_.emplace(std::move(name), std::move(borrow));
        } else {
            detail::assign_scoped(found->second, std::move(borrow));
        }
    }

    std::pmr::map<String, CheckerBorrow, std::less<>> checkers_;
};

template <class T>
class TypedChecker {
public:
    virtual ~TypedChecker() = default;
    virtual void check_and_fix_typed(
        const UserContext&, T&, CheckObjectStatus, const ObjectLocation&, CheckResults&) const = 0;
};

template <Entity T, class C>
    requires std::derived_from<C, TypedChecker<T>>
class TypedEntityChecker final : public Checker {
public:
    using allocator_type = Allocator;

    template <class Owner>
    TypedEntityChecker(std::allocator_arg_t, Allocator a, Owner& owner, C& checker)
        : checker_(borrow(a, owner, checker)),
          name_(T::entity_descriptor(a.resource()).name, a.resource()) {}

    TypedEntityChecker(std::allocator_arg_t,
                       Allocator a,
                       const UserContext& context,
                       Ref<C> checker)
        : checker_(borrow(a, context, checker)),
          name_(T::entity_descriptor(a.resource()).name, a.resource()) {}

    TypedEntityChecker(std::allocator_arg_t, Allocator a, const TypedEntityChecker& other)
        : checker_(std::allocator_arg, a, other.checker_), name_(other.name_, a.resource()) {}

    TypedEntityChecker(const TypedEntityChecker&) = delete;

    Memory memory_resource() const noexcept {
        return name_.get_allocator().resource();
    }

    std::string_view entity() const override {
        return name_;
    }

    void check_and_fix(const UserContext& context,
                       EntityValues& values,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        detail::CheckerCallScope active(detail::checker_call_arena(context));
        // Validate before taking values, including when decoding would fail.
        const auto& checker = checker_.get();
        auto r = values.memory_resource();
        auto status = check_object_status(values);
        EntityValues original(std::allocator_arg, Allocator(r));
        original.fields.swap(values.fields);
        auto loaded = detail::take_checker_fields(original, "_loaded_fields");
        auto dirty = detail::take_checker_fields(original, "_dirty_fields");
        auto changes = original.into_mutation_values(r);
        auto decoded =
            T::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), original.fields));
        if (!decoded) {
            values = std::move(original);
            String error("failed to materialize ", results.get_allocator().resource());
            error += name_;
            error += " for checker: ";
            error += decoded.error().to_string(error.get_allocator().resource());
            results.emplace_back(CheckRule::InvalidType, location);
            results.back().with_message(error);
            return;
        }
        auto& entity = decoded.value();
        if (loaded) {
            entity.set_checker_loaded_fields(std::move(*loaded));
        }
        if (dirty) {
            entity.set_checker_dirty_fields(std::move(*dirty), changes);
        }
        auto before = detail::clone_in(r, entity);
        auto before_values = std::move(before).into_values();
        checker.check_and_fix_typed(context, entity, status, location, results);
        auto after = std::move(entity).into_values();
        auto descriptor = T::entity_descriptor(r);
        for (const auto& property : descriptor.properties) {
            if (property.nullable || property.is_id || property.is_version) {
                continue;
            }
            auto found = after.fields.find(property.name);
            auto prior = before_values.fields.find(property.name);
            if (detail::missing_or_untyped_null(original.fields, property.name) &&
                (found == after.fields.end() ||
                 std::holds_alternative<std::monostate>(found->second.data) ||
                 (prior != before_values.fields.end() && prior->second == found->second))) {
                auto path = detail::clone_in(results.get_allocator().resource(), location);
                path.member(property.name);
                results.emplace_back(
                    CheckResult::required(results.get_allocator().resource(), std::move(path)));
            }
        }
        values = std::move(original);
        for (auto& [field, value] : after.fields) {
            auto found = before_values.fields.find(field);
            if (found == before_values.fields.end() || found->second != value) {
                values.fields.insert_or_assign(String(field, r), Value(r, std::move(value)));
            }
        }
    }

private:
    static ResourceBorrow<const C>
    borrow(Allocator a, const UserContext& context, Ref<C> reference) {
        auto& origin = detail::checker_reference_origin(context, reference.id());
        auto& checker = *static_cast<C*>(
            origin.resolve(detail::reference_slot(reference.id()), typeid(C), reference.id()));
        (void)detail::ContextResources::entry(detail::resource_arena(a.resource()),
                                              &origin,
                                              checker,
                                              detail::reference_slot(reference.id()));
        return ResourceBorrow<const C>(
            std::allocator_arg, a, checker, origin, detail::reference_slot(reference.id()));
    }

    template <class Owner>
    static ResourceBorrow<const C> borrow(Allocator a, Owner& owner, C& checker) {
        auto registered =
            detail::checker_resource(detail::resource_arena(a.resource()), owner, checker);
        return ResourceBorrow<const C>(
            std::allocator_arg, a, checker, *registered.owner, registered.slot);
    }

    ResourceBorrow<const C> checker_;
    String name_;
};
}
