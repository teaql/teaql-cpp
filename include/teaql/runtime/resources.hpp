#pragma once
#include <teaql/runtime/context_error.hpp>
#include <teaql/core/value.hpp>
#include <teaql/memory/arena.hpp>
#include <map>
#include <typeindex>
#include <unordered_map>

namespace teaql {
class GlobalContext;

namespace detail {
struct EntityRuntimeClone;

// Runtime allocation domains borrow their explicit application. The generic
// memory layer has no logging policy, owner registry, or runtime dependency.
class ContextArena final : public Arena {
public:
    ContextArena(std::size_t size, Memory upstream, const Arena* parent, GlobalContext& global)
        : Arena(size, upstream, parent), global_(global) {}

    ~ContextArena() override {
        close();
    }

    GlobalContext& global() const noexcept {
        return global_;
    }

    // Borrowed only during one synchronous cross-domain graph copy; never TLS.
    EntityRuntimeClone* entity_runtime_clone = nullptr;

private:
    GlobalContext& global_;
};

inline GlobalContext& application_context(Memory r) {
    auto* arena = dynamic_cast<ContextArena*>(r);
    if (!arena || arena->closed()) {
        throw std::logic_error("Runtime logging requires a live application Context");
    }
    return arena->global();
}

template <class T>
constexpr std::string_view resource_type_name() {
#if defined(__clang__)
    std::string_view text = __PRETTY_FUNCTION__;
    auto start = text.find("T = ") + 4;
    return text.substr(start, text.rfind(']') - start);
#elif defined(__GNUC__)
    std::string_view text = __PRETTY_FUNCTION__;
    auto start = text.find("T = ") + 4;
    auto end = text.find(';', start);
    if (end == text.npos) {
        end = text.rfind(']');
    }
    return text.substr(start, end - start);
#else
    return typeid(T).name();
#endif
}

inline Arena& resource_arena(Memory r) {
    auto* arena = dynamic_cast<Arena*>(r);
    if (!arena || arena->closed()) {
        throw std::logic_error("Resource scope is not a live Context");
    }
    return *arena;
}
}

// Allocator-aware carrier for a required resource reference. Copying the result
// into another scope cannot silently retain a shorter-lived registered object.
template <class T>
class ResourceBorrow {
public:
    using allocator_type = Allocator;

    ResourceBorrow(std::allocator_arg_t, Allocator a, T& object, Arena& owner)
        : resource_(a.resource()), object_(&object), owner_(&owner) {
        if (!owner.can_lend_to(detail::resource_arena(resource_))) {
            throw std::invalid_argument("Resource borrow cannot outlive its owning Context");
        }
    }

    ResourceBorrow(std::allocator_arg_t, Allocator a, const ResourceBorrow& other)
        : ResourceBorrow(std::allocator_arg, a, *other.object_, *other.owner_) {}

    ResourceBorrow(std::allocator_arg_t, Allocator a, ResourceBorrow&& other)
        : ResourceBorrow(std::allocator_arg, a, std::as_const(other)) {}

    ResourceBorrow(const ResourceBorrow&) = delete;
    ResourceBorrow(ResourceBorrow&&) noexcept = default;

    T& get() const noexcept {
        return *object_;
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

private:
    Memory resource_;
    T* object_;
    Arena* owner_;
};

namespace detail {
struct RegisteredResource {
    void* object;
    Arena* owner;
    const std::type_info* type;
};

class ContextResources {
public:
    using allocator_type = Allocator;

    explicit ContextResources(std::allocator_arg_t, Allocator a)
        : resource(a.resource()), typed(resource), named(std::less<>{}, resource),
          locals(std::less<>{}, resource) {}

    template <class T>
    static RegisteredResource entry(Arena& destination, Memory origin, T& object) {
        static_assert(!std::is_const_v<T>,
                      "Register the owning mutable resource; const lookups remain available");
        auto& owner = resource_arena(origin);
        if (owner.closing() || !owner.can_lend_to(destination)) {
            throw std::invalid_argument(
                "Registered resource belongs to a shorter or unrelated Context");
        }
        if constexpr (requires { object.memory_resource(); }) {
            if (object.memory_resource() != origin) {
                throw std::invalid_argument(
                    "Registered resource does not belong to the declared Context");
            }
        } else if constexpr (requires { object.get_allocator().resource(); }) {
            if (object.get_allocator().resource() != origin) {
                throw std::invalid_argument(
                    "Registered resource allocator differs from the declared Context");
            }
        }
        return {&object, &owner, &typeid(T)};
    }

    template <class T>
    void insert(Arena& destination, Memory origin, T& object) {
        typed.insert_or_assign(std::type_index(typeid(T)), entry(destination, origin, object));
    }

    template <class T>
    void insert_named(Arena& destination, Memory origin, std::string_view name, T& object) {
        named.insert_or_assign(String(name, resource), entry(destination, origin, object));
    }

    template <class T>
    const RegisteredResource* find() const {
        auto it = typed.find(std::type_index(typeid(T)));
        return it == typed.end() ? nullptr : &it->second;
    }

    template <class T>
    const RegisteredResource* find_named(std::string_view name) const {
        auto it = named.find(name);
        return it == named.end() || *it->second.type != typeid(T) ? nullptr : &it->second;
    }

    void put_local(std::string_view key, const Value& value) {
        auto found = locals.find(key);
        if (found == locals.end()) {
            locals.emplace(String(key, resource), Value(resource, value));
        } else {
            found->second = Value(resource, value);
        }
    }

    const Value* local(std::string_view key) const {
        auto found = locals.find(key);
        return found == locals.end() ? nullptr : &found->second;
    }

    std::optional<Value> remove_local(std::string_view key) {
        auto found = locals.find(key);
        if (found == locals.end()) {
            return {};
        }
        Value value(std::move(found->second));
        locals.erase(found);
        return value;
    }

private:
    Memory resource;
    std::pmr::unordered_map<std::type_index, RegisteredResource> typed;
    std::pmr::map<String, RegisteredResource, std::less<>> named;
    std::pmr::map<String, Value, std::less<>> locals;
};
}
}
