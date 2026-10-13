#pragma once
#include <teaql/runtime/context_error.hpp>
#include <teaql/core/value.hpp>
#include <teaql/core/eval.hpp>
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
    ContextArena(std::size_t size,
                 Memory upstream,
                 const Arena* parent,
                 GlobalContext& global,
                 MemoryLimits limits = {})
        : Arena(size, upstream, parent, limits), global_(global),
          layout_owner_(parent
                            ? const_cast<ContextArena*>(dynamic_cast<const ContextArena*>(parent))
                            : this) {
        if (!layout_owner_) {
            throw std::invalid_argument("Runtime metadata requires an explicit application arena");
        }
    }

    ~ContextArena() override {
        close();
    }

    GlobalContext& global() const noexcept {
        return global_;
    }

    static ContextArena& from_memory(Memory resource) {
        auto* arena = dynamic_cast<ContextArena*>(resource);
        if (!arena) {
            throw std::invalid_argument("Generated entities require an explicit runtime Context");
        }
        arena->check_thread();
        if (arena->closed() || arena->closing()) {
            throw std::logic_error("Generated entities require a live runtime Context");
        }
        return *arena;
    }

    // Borrowed only during one synchronous cross-domain graph copy; never TLS.
    EntityRuntimeClone* entity_runtime_clone = nullptr;

    void bind_state(ContextState& state) noexcept {
        state_ = &state;
    }

    ContextState* state() const noexcept {
        return state_;
    }

    const ContextState* context_state() const noexcept override {
        return state_;
    }

    SnapshotPool& loaded_snapshots() {
        check_thread();
        if (closed() || closing()) {
            throw std::logic_error("Loaded-state access requires a live Context");
        }
        if (!snapshots_) {
            auto& pool = make<SnapshotPool>();
            snapshots_.emplace(std::allocator_arg, Allocator(this), pool);
        }
        return const_cast<SnapshotPool&>(snapshots_->get());
    }

    template <class T>
    const FieldLayout& field_layout() {
        check_thread();
        if (closed() || closing()) {
            throw std::logic_error("Type metadata access requires a live Context");
        }
        auto& owner = *layout_owner_;
        owner.check_thread();
        if (!owner.layouts_) {
            owner.layouts_ = &owner.make_payload<Layouts>();
        }
        auto key = std::type_index(typeid(T));
        if (auto found = owner.layouts_->types.find(key); found != owner.layouts_->types.end()) {
            return found->second.get();
        }
        auto created = T::create_field_layout(&owner);
        if (!created) {
            throw std::invalid_argument(std::string(created.error()));
        }
        auto& layout = owner.make<FieldLayout>(std::move(created).value());
        auto [position, inserted] = owner.layouts_->types.emplace(
            key, ArenaBorrow<FieldLayout>(std::allocator_arg, Allocator(&owner), layout));
        return position->second.get();
    }

private:
    GlobalContext& global_;
    ContextState* state_ = nullptr;

    struct Layouts {
        using allocator_type = Allocator;
        std::pmr::map<std::type_index, ArenaBorrow<FieldLayout>> types;

        Layouts(std::allocator_arg_t, Allocator a) : types(a.resource()) {}
    };

    ContextArena* layout_owner_;
    Layouts* layouts_ = nullptr;
    std::optional<ArenaBorrow<SnapshotPool>> snapshots_;

    void on_closing(std::source_location where) noexcept override {
        if (state_) {
            state_->closed_at = where;
            state_->phase = ContextState::Phase::Closing;
        }
    }

    void on_closed() noexcept override {
        if (state_) {
            state_->phase = ContextState::Phase::Closed;
        }
    }
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

// Only scalar state and static source locations are retained, never User data.
class ContextDirectory final {
public:
    using allocator_type = Allocator;

    ContextDirectory(std::allocator_arg_t, Allocator allocator, std::uint32_t limit)
        : arena_(resource_arena(allocator.resource())), pages_(allocator.resource()),
          limit_(limit) {}

    std::pair<std::uint32_t, ContextState&> create() {
        arena_.check_thread();
        if (issued_ == limit_) {
            throw MemoryLimitError(MemoryLimitError::Kind::Contexts);
        }
        if (issued_ / page_size == pages_.size()) {
            auto& page = arena_.make_payload<Page>();
            pages_.push_back(&page);
        }
        auto id = ++issued_;
        return {id, pages_[(id - 1) / page_size]->states[(id - 1) % page_size]};
    }

    const ContextState* find(std::uint32_t id) const noexcept {
        if (id == 0 || id > issued_) {
            return nullptr;
        }
        return &pages_[(id - 1) / page_size]->states[(id - 1) % page_size];
    }

    std::uint32_t issued() const noexcept {
        return issued_;
    }

private:
    static constexpr std::uint32_t page_size = 64;

    struct Page {
        std::array<ContextState, page_size> states{};
    };

    Arena& arena_;
    std::pmr::vector<Page*> pages_;
    std::uint32_t limit_;
    std::uint32_t issued_ = 0;
};
}

// Allocator-aware carrier for a required resource reference. Copying the result
// into another scope cannot silently retain a shorter-lived registered object.
template <class T>
class ResourceBorrow {
public:
    using allocator_type = Allocator;

    ResourceBorrow(
        std::allocator_arg_t, Allocator a, T& object, Arena& owner, std::uint32_t known_slot = 0)
        : resource_(a.resource()), object_(&object), owner_(&owner),
          scope_(&detail::resource_arena(resource_)),
          slot_(known_slot ? known_slot : owner.slot_for_address(&object)) {
        if (!owner.can_lend_to(*scope_)) {
            throw std::invalid_argument("Resource borrow cannot outlive its owning Context");
        }
        if (auto* context = dynamic_cast<detail::ContextArena*>(scope_)) {
            state_ = context->state();
        }
    }

    ResourceBorrow(std::allocator_arg_t, Allocator a, const ResourceBorrow& other)
        : ResourceBorrow(std::allocator_arg, a, other.get(), other.checked_owner(), other.slot_) {}

    ResourceBorrow(std::allocator_arg_t, Allocator a, ResourceBorrow&& other)
        : ResourceBorrow(std::allocator_arg, a, std::as_const(other)) {}

    ResourceBorrow(const ResourceBorrow&) = delete;
    ResourceBorrow(ResourceBorrow&&) noexcept = default;

    T& get(std::source_location access = std::source_location::current()) const {
        // Check Global metadata before accessing a possibly destroyed User arena.
        if (state_ && (state_->phase == detail::ContextState::Phase::Closed ||
                       state_->phase == detail::ContextState::Phase::Constructing)) {
            throw ReferenceError(ReferenceError::Kind::Closed, 0, access, state_->closed_at);
        }
        scope_->check_thread();
        if (scope_->closed()) {
            throw ReferenceError(ReferenceError::Kind::Closed, 0, access);
        }
        // Trusted kernel quiescence can still access objects before destruction.
        if (slot_) {
            owner_->check_live(slot_, 0, access, true);
        }
        return *object_;
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

private:
    Arena& checked_owner() const {
        (void)get();
        return *owner_;
    }

    Memory resource_;
    T* object_;
    Arena* owner_;
    Arena* scope_;
    std::uint32_t slot_;
    detail::ContextState* state_ = nullptr;
};

namespace detail {
struct RegisteredResource {
    void* object;
    Arena* owner;
    const std::type_info* type;
    std::uint32_t slot = 0;
};

class ContextResources {
public:
    using allocator_type = Allocator;

    explicit ContextResources(std::allocator_arg_t, Allocator a)
        : resource(a.resource()), typed(resource), named(std::less<>{}, resource),
          locals(std::less<>{}, resource) {}

    template <class T>
    static RegisteredResource
    entry(Arena& destination, Memory origin, T& object, std::uint32_t known_slot = 0) {
        static_assert(!std::is_const_v<T>,
                      "Register the owning mutable resource; const lookups remain available");
        auto& owner = resource_arena(origin);
        if (owner.closing() || !owner.can_lend_to(destination)) {
            throw std::invalid_argument(
                "Registered resource belongs to a shorter or unrelated Context");
        }
        auto slot = known_slot ? known_slot : owner.slot_for_address(&object);
        if (slot) {
            owner.check_live(slot, 0);
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
        return {&object, &owner, &typeid(T), slot};
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
        if (it == typed.end()) {
            return nullptr;
        }
        validate(it->second);
        return &it->second;
    }

    template <class T>
    const RegisteredResource* find_named(std::string_view name) const {
        auto it = named.find(name);
        if (it == named.end() || *it->second.type != typeid(T)) {
            return nullptr;
        }
        validate(it->second);
        return &it->second;
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
    static void validate(const RegisteredResource& entry) {
        if (entry.slot) {
            entry.owner->check_live(entry.slot, 0, std::source_location::current(), true);
        }
    }

    Memory resource;
    std::pmr::unordered_map<std::type_index, RegisteredResource> typed;
    std::pmr::map<String, RegisteredResource, std::less<>> named;
    std::pmr::map<String, Value, std::less<>> locals;
};
}
}
