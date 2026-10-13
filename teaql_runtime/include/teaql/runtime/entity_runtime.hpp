#pragma once
#include <teaql/runtime/entity_key.hpp>
#include <teaql/runtime/id_space.hpp>
#include <teaql/runtime/expression_binding.hpp>
#include <teaql/core/list.hpp>
#include <teaql/core/eval.hpp>
#include <teaql/core/mutation.hpp>
#include <teaql/core/detail/text.hpp>
#include <teaql/memory/arena.hpp>
#include <teaql/memory/function_ref.hpp>
#include <typeindex>
#include <tuple>
#include <unordered_map>

namespace teaql {
class DebugWriter;
class TraceScopeToken;
enum class LoadedRelation { Loaded, Empty, NotLoaded };

template <class T>
class RelationHandle {
public:
    LoadedRelation state() const {
        (void)scope_.get();
        return state_;
    }

    const T* value() const {
        (void)scope_.get();
        return value_;
    }

    bool is_loaded() const {
        (void)scope_.get();
        return state_ != LoadedRelation::NotLoaded;
    }

    bool is_empty() const {
        (void)scope_.get();
        return state_ == LoadedRelation::Empty;
    }

private:
    friend class EntityRuntimeState;

    RelationHandle(Memory resource, LoadedRelation state, const T* value)
        : scope_(resource), state_(state), value_(value) {}

    ExpressionScope scope_;
    LoadedRelation state_;
    const T* value_;
};

namespace detail {
struct EntityRuntimeClone;
void with_entity_runtime_copy(Memory, FunctionRef<void()>);
struct EntityLedgerSlot;
struct EntityGraphSlot;
struct FrozenEntityGraph;

// Preserve graph-reference roles without reference-counted storage ownership.
// Both roles expire at Context close; only an owning view may publish a graph.
struct EntityGraphReference {
    EntityGraphSlot* slot;
    bool is_weak = false;

    EntityGraphReference preserve() const noexcept {
        return *this;
    }

    EntityGraphReference promote() const noexcept {
        return {slot, false};
    }

    EntityGraphReference weak() const noexcept {
        return {slot, true};
    }
};
struct EntityMutationLedger;
struct EntityRuntimeAccess;

template <class T>
struct GraphPayload {
    static constexpr bool copyable =
        std::uses_allocator_v<T, Allocator>
            ? (std::is_constructible_v<T, std::allocator_arg_t, Allocator, const T&> ||
               std::is_constructible_v<T, const T&, Allocator>)
            : (std::is_trivially_destructible_v<T> && std::is_copy_constructible_v<T>);

    static T move(Memory r, T&& value) {
        if constexpr (requires { value.memory_resource(); }) {
            if (r == value.memory_resource()) {
                return std::move(value);
            }
            if constexpr (copyable) {
                return clone_in(r, value);
            } else {
                throw std::invalid_argument(
                    "Non-copyable graph payload belongs to another Context");
            }
        } else {
            return move_in(r, std::move(value));
        }
    }
};

template <class T>
struct GraphPayload<std::optional<T>> {
    static constexpr bool copyable = GraphPayload<T>::copyable;

    static std::optional<T> move(Memory r, std::optional<T>&& value) {
        if (!value) {
            return {};
        }
        return GraphPayload<T>::move(r, std::move(*value));
    }
};

template <class T>
struct GraphPayload<SmartList<T>> {
    static constexpr bool copyable = GraphPayload<T>::copyable;

    static SmartList<T> move(Memory r, SmartList<T>&& value) {
        if (r == value.memory_resource()) {
            return std::move(value);
        }
        if constexpr (copyable) {
            return clone_in(r, value);
        } else {
            throw std::invalid_argument("Non-copyable graph list belongs to another Context");
        }
    }
};

// Type-erased ownership stays inside the graph's Context domain. Replacement
// destroys the old payload immediately; freeing the arena remains Context-owned.
class EntityGraphValue {
public:
    using allocator_type = Allocator;

    template <class T>
    EntityGraphValue(std::allocator_arg_t, Allocator a, std::in_place_type_t<T>, T&& value)
        : resource_(a.resource()), type_(typeid(T)) {
        static_assert(std::is_nothrow_destructible_v<T>);
        object_ = resource_->allocate(sizeof(T), alignof(T));
        try {
            std::construct_at(static_cast<T*>(object_),
                              GraphPayload<T>::move(resource_, std::move(value)));
        } catch (...) {
            resource_->deallocate(object_, sizeof(T), alignof(T));
            throw;
        }
        destroy_ = [](Memory r, void* object) noexcept {
            std::destroy_at(static_cast<T*>(object));
            r->deallocate(object, sizeof(T), alignof(T));
        };
        if constexpr (GraphPayload<T>::copyable) {
            clone_ = [](Memory r, const void* object) -> void* {
                auto* result = static_cast<T*>(r->allocate(sizeof(T), alignof(T)));
                try {
                    std::construct_at(result, clone_in(r, *static_cast<const T*>(object)));
                } catch (...) {
                    r->deallocate(result, sizeof(T), alignof(T));
                    throw;
                }
                return result;
            };
        }
    }

    EntityGraphValue(std::allocator_arg_t, Allocator a, const EntityGraphValue& other)
        : resource_(a.resource()), type_(other.type_), destroy_(other.destroy_),
          clone_(other.clone_) {
        if (!clone_) {
            throw std::invalid_argument(
                "Graph payload requires an explicit allocator-aware copy for cross-domain retention");
        }
        object_ = clone_(resource_, other.object_);
    }

    EntityGraphValue(std::allocator_arg_t, Allocator a, EntityGraphValue&& other)
        : EntityGraphValue(move_to(a.resource(), std::move(other))) {}

    EntityGraphValue(const EntityGraphValue&) = delete;

    EntityGraphValue(EntityGraphValue&& other) noexcept
        : resource_(other.resource_), type_(other.type_),
          object_(std::exchange(other.object_, nullptr)), destroy_(other.destroy_),
          clone_(other.clone_) {}

    EntityGraphValue& operator=(EntityGraphValue&& other) {
        return assign_scoped(*this, std::move(other));
    }

    ~EntityGraphValue() {
        if (object_) {
            destroy_(resource_, object_);
        }
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

    template <class T>
    const T* get() const noexcept {
        return type_ == typeid(T) ? static_cast<const T*>(object_) : nullptr;
    }

private:
    Memory resource_;
    std::type_index type_;
    void* object_ = nullptr;
    void (*destroy_)(Memory, void*) noexcept = nullptr;
    void* (*clone_)(Memory, const void*) = nullptr;
};

struct EntityRelationKey {
    using allocator_type = Allocator;
    String owner_entity, relation;
    std::uint64_t owner_id;
    std::uint64_t view_id = 0;

    EntityRelationKey(std::allocator_arg_t,
                      Allocator a,
                      std::string_view owner,
                      std::uint64_t id,
                      std::string_view relation)
        : owner_entity(canonical_id_space_entity(a.resource(), owner)),
          relation(relation, a.resource()), owner_id(id) {}

    EntityRelationKey(std::allocator_arg_t,
                      Allocator a,
                      std::uint64_t view,
                      std::string_view owner,
                      std::uint64_t id,
                      std::string_view relation)
        : EntityRelationKey(std::allocator_arg, a, owner, id, relation) {
        view_id = view;
    }

    EntityRelationKey(std::allocator_arg_t, Allocator a, const EntityRelationKey& other)
        : owner_entity(other.owner_entity, a.resource()), relation(other.relation, a.resource()),
          owner_id(other.owner_id), view_id(other.view_id) {}

    EntityRelationKey(const EntityRelationKey&) = delete;
    EntityRelationKey(EntityRelationKey&&) noexcept = default;
    bool operator==(const EntityRelationKey&) const = default;
};

struct EntityRelationHash {
    std::size_t operator()(const EntityRelationKey& key) const noexcept {
        auto h = std::hash<std::string_view>{}(key.owner_entity);
        h ^= std::hash<std::uint64_t>{}(key.owner_id) + (h << 6) + (h >> 2);
        h ^= std::hash<std::uint64_t>{}(key.view_id) + (h << 6) + (h >> 2);
        return h ^ std::hash<std::string_view>{}(key.relation);
    }
};
}

class EntityGraphBuilder {
public:
    using allocator_type = Allocator;

    EntityGraphBuilder(std::allocator_arg_t, Allocator a)
        : tables_(a.resource()), relations_(a.resource()),
          option_entities_(nullptr, detail::ScopedDelete<RelationOptionEntities>{a.resource()}) {}

    EntityGraphBuilder(std::allocator_arg_t, Allocator, const EntityGraphBuilder&);

    EntityGraphBuilder(std::allocator_arg_t, Allocator a, EntityGraphBuilder&& other)
        : EntityGraphBuilder(detail::move_to(a.resource(), std::move(other))) {}

    EntityGraphBuilder(const EntityGraphBuilder&) = delete;
    EntityGraphBuilder(EntityGraphBuilder&&) noexcept = default;

    EntityGraphBuilder& operator=(EntityGraphBuilder&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return tables_.get_allocator().resource();
    }

    template <class T>
    void install(std::uint64_t id, T value) {
        auto [table, _] = tables_.try_emplace(typeid(T));
        detail::EntityGraphValue stored(std::allocator_arg,
                                        Allocator(memory_resource()),
                                        std::in_place_type<T>,
                                        std::move(value));
        table->second.insert_or_assign(id, std::move(stored));
    }

    template <class T>
    void install_relation_list(std::string_view owner,
                               std::uint64_t id,
                               std::string_view relation,
                               SmartList<T> list) {
        install_relation_facets(owner, id, relation, list.facets);
        install_relation(owner, id, relation, std::move(list));
    }

    template <class T>
    void install_relation_option(std::string_view owner,
                                 std::uint64_t id,
                                 std::string_view relation,
                                 std::optional<T> value) {
        install_relation(owner, id, relation, std::move(value));
    }

    static EntityGraphBuilder for_json_read(Memory r) {
        EntityGraphBuilder result(std::allocator_arg, Allocator(r));
        result.option_entities_ = detail::make_box<RelationOptionEntities>(r);
        return result;
    }

    template <class T>
    void install_json_relation_list(std::uint64_t view,
                                    std::string_view owner,
                                    std::uint64_t id,
                                    std::string_view relation,
                                    SmartList<T> list) {
        install_relation(view, owner, id, relation, std::move(list));
    }

    template <class T>
    void install_typed_relation_option(std::uint64_t view,
                                       std::string_view owner,
                                       std::uint64_t id,
                                       std::string_view relation,
                                       std::optional<T> value) {
        auto r = memory_resource();
        if (option_entities_ && value && value->is_field_loaded("id")) {
            if (auto entity_id = value->id_value().try_u64()) {
                auto key = std::tuple(view, std::type_index(typeid(T)), *entity_id);
                auto found = option_entities_->index.find(key);
                if (found == option_entities_->index.end()) {
                    option_entities_->index.emplace(
                        key,
                        std::optional<detail::EntityRelationKey>(std::in_place,
                                                                 std::allocator_arg,
                                                                 Allocator(r),
                                                                 view,
                                                                 owner,
                                                                 id,
                                                                 relation));
                } else {
                    found->second.reset();
                }
            }
        }
        install_relation(view, owner, id, relation, std::move(value));
    }

    // Read-result metadata is independent of relation cardinality and mutation state.
    using RelationFacets = decltype(SmartList<CompactRow>::facets);

    void install_relation_facets(std::string_view owner,
                                 std::uint64_t id,
                                 std::string_view relation,
                                 const RelationFacets& facets);

    std::size_t entity_count() const noexcept;

    std::size_t relation_list_count() const noexcept {
        return relations_.size();
    }

private:
    friend class EntityRuntimeState;
    friend void write_debug(DebugWriter&, const EntityGraphBuilder&);
    friend struct detail::EntityRuntimeClone;
    using EntityTable = std::pmr::unordered_map<std::uint64_t, detail::EntityGraphValue>;
    std::pmr::unordered_map<std::type_index, EntityTable> tables_;
    std::pmr::unordered_map<detail::EntityRelationKey,
                            detail::EntityGraphValue,
                            detail::EntityRelationHash>
        relations_;
    using RelationFacetTable = std::pmr::
        unordered_map<detail::EntityRelationKey, RelationFacets, detail::EntityRelationHash>;
    std::optional<RelationFacetTable> relation_facets_;

    struct RelationOptionEntities {
        using allocator_type = Allocator;
        using Key = std::tuple<std::uint64_t, std::type_index, std::uint64_t>;
        std::pmr::map<Key, std::optional<detail::EntityRelationKey>> index;

        RelationOptionEntities(std::allocator_arg_t, Allocator a) : index(a.resource()) {}

        RelationOptionEntities(std::allocator_arg_t,
                               Allocator a,
                               const RelationOptionEntities& other)
            : index(a.resource()) {
            for (const auto& [key, value] : other.index) {
                index.emplace(key, detail::clone_in(a.resource(), value));
            }
        }
    };

    detail::ScopedBox<RelationOptionEntities> option_entities_;

    template <class T>
    void
    install_relation(std::string_view owner, std::uint64_t id, std::string_view relation, T value) {
        install_relation(0, owner, id, relation, std::move(value));
    }

    template <class T>
    void install_relation(std::uint64_t view,
                          std::string_view owner,
                          std::uint64_t id,
                          std::string_view relation,
                          T value) {
        auto r = memory_resource();
        detail::EntityRelationKey key(std::allocator_arg, Allocator(r), view, owner, id, relation);
        detail::EntityGraphValue stored(
            std::allocator_arg, Allocator(r), std::in_place_type<T>, std::move(value));
        relations_.insert_or_assign(std::move(key), std::move(stored));
    }
};

using DynamicFieldChanges =
    std::pmr::map<EntityKey, std::pmr::map<String, DynamicFieldMutation, std::less<>>>;

class EntityChangeSet {
public:
    using allocator_type = Allocator;
    using Changes = std::pmr::map<EntityKey, MutationValues>;

    using DynamicOriginals = std::pmr::map<String, std::optional<Value>, std::less<>>;
    EntityChangeSet(std::allocator_arg_t, Allocator);
    EntityChangeSet(std::allocator_arg_t, Allocator, const EntityChangeSet&);
    EntityChangeSet(std::allocator_arg_t, Allocator, EntityChangeSet&&);

    EntityChangeSet(const EntityChangeSet&) = delete;
    EntityChangeSet(EntityChangeSet&&) noexcept;
    ~EntityChangeSet();

    EntityChangeSet& operator=(EntityChangeSet&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return changes_.get_allocator().resource();
    }

    bool is_empty() const noexcept;
    const DynamicFieldChanges* dynamic_changes() const noexcept;

    void set(const EntityKey&, std::string_view, const Value&);
    const Value* get(const EntityKey&, std::string_view) const;

    const Changes& changes() const noexcept {
        return changes_;
    }

    void clear_entity(const EntityKey&);

    FieldSet field_names(const EntityKey&) const;
    bool operator==(const EntityChangeSet&) const;

private:
    friend class EntityRuntimeState;
    friend struct detail::EntityRuntimeAccess;
    friend void write_debug(DebugWriter&, const EntityChangeSet&);
    struct DynamicData;
    Changes changes_;
    detail::ScopedBox<DynamicData> dynamic_;
    const DynamicFieldDefinitions* dynamic_definitions(const EntityKey&) const;
    const DynamicOriginals* dynamic_originals(const EntityKey&) const;
    void set_dynamic(const EntityKey&,
                     std::string_view,
                     const DynamicFieldMutation&,
                     const DynamicFieldDefinitions&,
                     const Value* original);
};

class ChangeSetStack {
public:
    using allocator_type = Allocator;

    ChangeSetStack(std::allocator_arg_t, Allocator a) : stack_(a.resource()) {}

    ChangeSetStack(std::allocator_arg_t, Allocator a, const ChangeSetStack& other)
        : stack_(other.stack_, a.resource()) {}

    ChangeSetStack(std::allocator_arg_t, Allocator a, ChangeSetStack&& other)
        : ChangeSetStack(detail::move_to(a.resource(), std::move(other))) {}

    ChangeSetStack(const ChangeSetStack&) = delete;
    ChangeSetStack(ChangeSetStack&&) noexcept = default;

    ChangeSetStack& operator=(ChangeSetStack&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return stack_.get_allocator().resource();
    }

    EntityChangeSet& current_mut() {
        if (stack_.empty()) {
            push();
        }
        return stack_.back();
    }

    const EntityChangeSet* current() const noexcept {
        return stack_.empty() ? nullptr : &stack_.back();
    }

    void push() {
        stack_.emplace_back();
    }

    std::optional<EntityChangeSet> pop();
    std::optional<Value> get(const EntityKey&, std::string_view) const;

    void set(const EntityKey& key, std::string_view field, const Value& value) {
        current_mut().set(key, field, value);
    }

    void clear_current();

    void clear_entity(const EntityKey& key) {
        for (auto& set : stack_) {
            set.clear_entity(key);
        }
    }

    FieldSet changed_field_names(const EntityKey&) const;
    bool operator==(const ChangeSetStack&) const = default;

private:
    friend class EntityRuntimeState;
    friend void write_debug(DebugWriter&, const ChangeSetStack&);
    Vector<EntityChangeSet> stack_;
};

struct LedgerCompositionError {
    using allocator_type = Allocator;
    enum class Kind {
        MissingTargetState,
        MissingSourceState,
        ConflictingOriginalVersion,
        ConflictingDynamicDefinitions
    };
    Kind kind;
    String entity, id;
    std::int64_t target = 0, source = 0;

    LedgerCompositionError(std::allocator_arg_t,
                           Allocator a,
                           Kind kind,
                           std::string_view entity = {},
                           std::string_view id = {},
                           std::int64_t target = 0,
                           std::int64_t source = 0)
        : kind(kind), entity(entity, a.resource()), id(id, a.resource()), target(target),
          source(source) {}

    LedgerCompositionError(std::allocator_arg_t, Allocator a, const LedgerCompositionError& other)
        : LedgerCompositionError(std::allocator_arg,
                                 a,
                                 other.kind,
                                 other.entity,
                                 other.id,
                                 other.target,
                                 other.source) {}

    LedgerCompositionError(const LedgerCompositionError&) = delete;
    LedgerCompositionError(LedgerCompositionError&&) noexcept = default;
    bool operator==(const LedgerCompositionError&) const = default;

    Memory memory_resource() const noexcept {
        return entity.get_allocator().resource();
    }

    String to_string(Memory) const;

    const std::exception* error_source() const noexcept {
        return nullptr;
    }
};

using LedgerCompositionResult = Result<std::monostate, LedgerCompositionError>;

class EntityRuntimeState {
public:
    using allocator_type = Allocator;
    using Keys = std::pmr::set<EntityKey>;
    EntityRuntimeState(std::allocator_arg_t, Allocator);
    // Same-domain copies share the lazy ledger and immutable graph. Copies into
    // another explicit Context deep-copy both, preserving cycles within that copy.
    EntityRuntimeState(std::allocator_arg_t, Allocator, const EntityRuntimeState&);

    EntityRuntimeState(std::allocator_arg_t, Allocator a, EntityRuntimeState&& other)
        : EntityRuntimeState(detail::move_to(a.resource(), std::move(other))) {}

    EntityRuntimeState(const EntityRuntimeState&) = delete;
    EntityRuntimeState(EntityRuntimeState&&) noexcept = default;

    EntityRuntimeState& operator=(EntityRuntimeState&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

    EntityRuntimeState with_json_view(std::uint64_t view) const& {
        EntityRuntimeState result(std::allocator_arg, Allocator(resource_), *this);
        result.json_view_ = view;
        return result;
    }

    EntityRuntimeState with_json_view(std::uint64_t view) && {
        detail::resource_arena(resource_);
        json_view_ = view;
        return std::move(*this);
    }

    std::uint64_t json_view_id() const {
        detail::resource_arena(resource_);
        return json_view_;
    }

    bool operator==(const EntityRuntimeState&) const;
    const DynamicFieldValues* loaded_dynamic_fields() const;
    void install_loaded_dynamic_fields(const DynamicFieldValues&);
    bool has_pending_dynamic_mutations() const;
    Result<bool, DynamicFieldError>
    update_dynamic_field(const EntityKey&, std::string_view, const Value&);
    Result<bool, DynamicFieldError> delete_dynamic_field(const EntityKey&, std::string_view);

    detail::EntityLifetimeAnchor::Borrow borrow_lifetime() const {
        return lifetime_.borrow();
    }

    static EntityRuntimeState fresh_with_shared_graph(Memory, const EntityRuntimeState&);
    EntityRuntimeState with_shared_graph(const EntityRuntimeState&) const;
    void adopt_mutations_from(const EntityRuntimeState&) const;
    void attach_runtime_state(const EntityRuntimeState&);
    Result<std::monostate, EntityGraphBuilder> freeze_graph(EntityGraphBuilder) const;

    template <class T>
    const T* resolve_entity(std::uint64_t id) const {
        auto* graph = frozen();
        if (!graph) {
            return nullptr;
        }
        auto table = graph->tables_.find(typeid(T));
        if (table != graph->tables_.end()) {
            auto found = table->second.find(id);
            if (found != table->second.end()) {
                return found->second.template get<T>();
            }
        }
        if (!graph->option_entities_) {
            return nullptr;
        }
        auto found = graph->option_entities_->index.find(
            std::tuple(json_view_, std::type_index(typeid(T)), id));
        if (found == graph->option_entities_->index.end() || !found->second) {
            return nullptr;
        }
        auto edge = graph->relations_.find(*found->second);
        if (edge == graph->relations_.end()) {
            return nullptr;
        }
        const auto* value = edge->second.template get<std::optional<T>>();
        return value && *value ? &**value : nullptr;
    }

    template <class T>
    const SmartList<T>* resolve_relation_list(std::string_view owner,
                                              std::uint64_t id,
                                              std::string_view relation) const {
        auto* stored = relation_value(owner, id, relation);
        return stored ? stored->template get<SmartList<T>>() : nullptr;
    }

    template <class T>
    const std::optional<T>* resolve_relation_option(std::string_view owner,
                                                    std::uint64_t id,
                                                    std::string_view relation) const {
        auto* stored = relation_value(owner, id, relation);
        return stored ? stored->template get<std::optional<T>>() : nullptr;
    }

    const SmartList<CompactRow>* relation_facet(std::string_view owner,
                                                std::uint64_t id,
                                                std::string_view relation,
                                                std::string_view facet) const;

    template <class T>
    RelationHandle<SmartList<T>>
    relation_list(std::string_view owner, std::uint64_t id, std::string_view relation) const {
        auto* stored = relation_value(owner, id, relation);
        if (!stored) {
            return {resource_, LoadedRelation::NotLoaded, nullptr};
        }
        auto* list = stored->template get<SmartList<T>>();
        if (!list) {
            relation_type_mismatch(owner, id, relation);
        }
        return {resource_, list->is_empty() ? LoadedRelation::Empty : LoadedRelation::Loaded, list};
    }

    template <class T>
    RelationHandle<T>
    relation_option(std::string_view owner, std::uint64_t id, std::string_view relation) const {
        auto* stored = relation_value(owner, id, relation);
        if (!stored) {
            return {resource_, LoadedRelation::NotLoaded, nullptr};
        }
        auto* value = stored->template get<std::optional<T>>();
        if (!value) {
            relation_type_mismatch(owner, id, relation);
        }
        return {resource_,
                *value ? LoadedRelation::Loaded : LoadedRelation::Empty,
                *value ? &**value : nullptr};
    }

    bool
    has_relation_view(std::string_view owner, std::uint64_t id, std::string_view relation) const {
        return relation_value(owner, id, relation) != nullptr;
    }

    void push_change_set() const;
    std::optional<EntityChangeSet> pop_change_set() const;
    void clear_current_change_set() const;
    void clear_committed() const;
    void set(const EntityKey&, std::string_view, const Value&) const;
    std::optional<Value> get(const EntityKey&, std::string_view) const;
    EntityChangeSet current_change_set() const;
    void set_comment(std::string_view) const;
    std::optional<String> get_comment() const;
    void set_entity_comment(const EntityKey&, std::string_view) const;
    std::optional<String> get_entity_comment(const EntityKey&) const;
    void mark_as_new(const EntityKey&) const;
    bool is_new(const EntityKey&) const;
    void set_original_snapshot(const EntitySnapshot&) const;
    void set_original_compact_row(std::string_view, const CompactRow&);
    void set_original_compact_row(std::string_view, CompactRow&&);
    std::optional<EntitySnapshot> original_snapshot() const;
    EntityRuntimeState with_loaded_view_from(const EntityRuntimeState&) const;
    void mark_as_delete(const EntityKey&) const;
    bool is_marked_as_delete(const EntityKey&) const;
    FieldSet changed_field_names(const EntityKey&) const;
    Keys deleted_keys() const;
    Keys new_keys() const;
    std::optional<std::int64_t> get_original_version(const EntityKey&) const;
    TraceChain get_trace_chain(const EntityKey&) const;
    void set_original_version(const EntityKey&, std::int64_t) const;

private:
    friend struct detail::EntityRuntimeAccess;
    friend void write_debug(DebugWriter&, const EntityRuntimeState&);
    friend struct detail::EntityRuntimeClone;

    struct LoadedSnapshot {
        using allocator_type = Allocator;
        String entity;
        CompactRow row;

        LoadedSnapshot(std::allocator_arg_t,
                       Allocator a,
                       std::string_view entity,
                       const CompactRow& row)
            : entity(entity, a.resource()), row(std::allocator_arg, a, row) {}

        LoadedSnapshot(std::allocator_arg_t, Allocator a, const LoadedSnapshot& other)
            : LoadedSnapshot(std::allocator_arg, a, other.entity, other.row) {}

        LoadedSnapshot(std::allocator_arg_t, Allocator a, std::string_view entity, CompactRow&& row)
            : entity(entity, a.resource()), row(std::allocator_arg, a, std::move(row)) {}

        LoadedSnapshot(std::allocator_arg_t, Allocator a, LoadedSnapshot&& other)
            : entity(std::move(other.entity), a.resource()),
              row(std::allocator_arg, a, std::move(other.row)) {}

        Memory memory_resource() const noexcept {
            return entity.get_allocator().resource();
        }

        LoadedSnapshot(const LoadedSnapshot&) = delete;
        LoadedSnapshot(LoadedSnapshot&&) noexcept = default;
    };

    Memory resource_;
    mutable detail::EntityLedgerSlot* inner_ = nullptr;
    detail::EntityGraphReference graph_;
    std::uint64_t json_view_ = 0;
    detail::EntityLifetimeAnchor lifetime_;
    mutable std::optional<LoadedSnapshot> loaded_snapshot_;
    mutable std::optional<ArenaBorrow<LoadedSnapshot>> shared_snapshot_;
    std::optional<ArenaBorrow<DynamicFieldValues>> dynamic_fields_;
    const LoadedSnapshot* loaded_snapshot() const;
    void copy_loaded_view_from(const EntityRuntimeState&);
    std::optional<EntitySnapshot> original_values_for(const EntityKey&) const;
    detail::EntityLedgerSlot& shared_ledger_slot() const;
    detail::EntityMutationLedger& context() const;
    const EntityGraphBuilder* frozen() const;
    const detail::EntityGraphValue*
        relation_value(std::string_view, std::uint64_t, std::string_view) const;
    [[noreturn]] void
        relation_type_mismatch(std::string_view, std::uint64_t, std::string_view) const;
    LedgerCompositionResult adopt_mutations_from_checked(const EntityRuntimeState&) const;
    using TraceScopes = std::pmr::map<EntityKey, const TraceScopeToken*>;
    void replace_trace_scopes(const TraceScopes&, const TraceScopes&) const;
    void enrich_allocated_trace_scopes(const std::pmr::map<EntityKey, Value>&) const;
    const TraceScopeToken* trace_scope_for(const EntityKey&, bool unassigned) const;
};

namespace detail {
struct EntityRuntimeAccess {
    using TraceScopes = EntityRuntimeState::TraceScopes;

    // Rust's generated dynamic map retains columns outside the concrete model,
    // including descendant values merged into an ancestor query result. These
    // are native/readonly extras, separate from durable dynamic-field metadata.
    static Result<std::monostate, EntityError>
    capture_dynamic_values(Record& destination, const CompactRow& row, const FieldLayout& layout) {
        auto r = destination.get_allocator().resource();
        using Outcome = Result<std::monostate, EntityError>;
        auto validated = row.validate_dynamic_properties();
        if (!validated) {
            return Outcome::err(r,
                                EntityError(std::allocator_arg,
                                            Allocator(r),
                                            layout.entity(),
                                            std::move(validated).error()));
        }
        for (const auto& [name, value] : row.iter()) {
            if (!layout.index(name) && !layout.is_relation(name)) {
                destination.insert_or_assign(String(name, r), detail::clone_in(r, value));
            }
        }
        return Outcome::ok(r, {});
    }

    // Indexed carriers serialize only selected native fields. Hidden stored
    // defaults must not become present inputs to a typed Checker or graph save.
    static void retain_loaded_native_values(MutationValues& values, const eval::LoadState& loaded) {
        for (auto field = values.fields.begin(); field != values.fields.end();) {
            if (!loaded.is_loaded(field->first)) {
                field = values.fields.erase(field);
            } else {
                ++field;
            }
        }
    }

    // Generated Rust setters trim the stored text while retaining the original
    // mutation operand. Required getters may subsequently read that operand;
    // optional getters read their stored value instead.
    static void update_text(const EntityRuntimeState& state,
                            const EntityKey& key,
                            std::string_view field,
                            String& stored,
                            std::string_view input) {
        Value operand(state.memory_resource(), input);
        detail::trim(input);
        stored.assign(input);
        state.set(key, field, operand);
    }

    static void update_text(const EntityRuntimeState& state,
                            const EntityKey& key,
                            std::string_view field,
                            std::optional<String>& stored,
                            const std::optional<String>& input) {
        auto r = state.memory_resource();
        Value operand = input ? Value(r, *input) : Value(r);
        if (input) {
            std::string_view text = *input;
            detail::trim(text);
            String normalized(text, r);
            stored.emplace(std::move(normalized));
        } else {
            stored.reset();
        }
        state.set(key, field, operand);
    }

    static const DynamicFieldDefinitions* dynamic_definitions(const EntityChangeSet& set,
                                                              const EntityKey& key) {
        return set.dynamic_definitions(key);
    }

    static const EntityChangeSet::DynamicOriginals* dynamic_originals(const EntityChangeSet& set,
                                                                      const EntityKey& key) {
        return set.dynamic_originals(key);
    }

    static void replace_trace_scopes(const EntityRuntimeState& state,
                                     const TraceScopes& scopes,
                                     const TraceScopes& unassigned) {
        state.replace_trace_scopes(scopes, unassigned);
    }

    static void enrich_allocated_trace_scopes(const EntityRuntimeState& state,
                                              const std::pmr::map<EntityKey, Value>& assigned) {
        state.enrich_allocated_trace_scopes(assigned);
    }

    static const TraceScopeToken* trace_scope_for(const EntityRuntimeState& state,
                                                  const EntityKey& key,
                                                  bool unassigned = false) {
        return state.trace_scope_for(key, unassigned);
    }

    static std::optional<EntitySnapshot> original_values_for(const EntityRuntimeState& state,
                                                             const EntityKey& key) {
        return state.original_values_for(key);
    }

    template <class T>
    static T current_field(const EntityRuntimeState& state,
                           const EntityKey& key,
                           std::string_view field,
                           const T& stored) {
        auto r = state.memory_resource();
        auto changed = state.get(key, field);
        if (changed) {
            if constexpr (std::same_as<T, String>) {
                if (auto value = changed->try_text()) {
                    return String(*value, r);
                }
            } else if constexpr (std::same_as<T, bool>) {
                if (auto value = changed->try_bool()) {
                    return *value;
                }
            } else if constexpr (std::unsigned_integral<T>) {
                if (auto value = changed->try_u64()) {
                    return static_cast<T>(*value);
                }
            } else if constexpr (std::signed_integral<T>) {
                if (auto value = changed->try_i64()) {
                    return static_cast<T>(*value);
                }
            } else if constexpr (std::floating_point<T>) {
                if (auto value = changed->try_f64()) {
                    return static_cast<T>(*value);
                }
            } else if constexpr (std::same_as<T, Decimal>) {
                if (auto value = changed->try_decimal()) {
                    return *value;
                }
            } else if constexpr (std::same_as<T, Date>) {
                if (auto value = changed->try_date()) {
                    return *value;
                }
            } else if constexpr (std::same_as<T, Timestamp>) {
                if (auto value = changed->try_timestamp()) {
                    return *value;
                }
            }
        }
        return detail::clone_in(r, stored);
    }

    template <class T, class Getter>
    static eval::EvalResult<T>
    eval_field(Memory r, const eval::LoadState& loaded, std::string_view field, Getter&& getter) {
        if (!loaded.is_loaded(field)) {
            return eval::EvalResult<T>::not_loaded(r, field, field);
        }
        auto value = std::invoke(std::forward<Getter>(getter));
        if constexpr (requires { value.has_value(); }) {
            if (!value.has_value()) {
                return eval::EvalResult<T>::null(r);
            }
        }
        return eval::EvalResult<T>::value(r, std::move(value));
    }

    static const CompactRow* original_row(const EntityRuntimeState& state) {
        auto* snapshot = state.loaded_snapshot();
        return snapshot ? &snapshot->row : nullptr;
    }

    static EntityRuntimeState fresh_with_weak_graph(Memory r, const EntityRuntimeState& source) {
        auto state = EntityRuntimeState::fresh_with_shared_graph(r, source);
        state.graph_ = state.graph_.weak();
        return state;
    }

    static bool has_mutation_context(const EntityRuntimeState&);
    static std::optional<LedgerCompositionError> first_composition_error(const EntityRuntimeState&);

    static LedgerCompositionResult compose(const EntityRuntimeState& target,
                                           const EntityRuntimeState& source) {
        return target.adopt_mutations_from_checked(source);
    }
};
}
template <class T>
concept LedgerEntity = Entity<T> && requires(const T& value) {
    { value.entity_runtime_state() } -> std::same_as<std::optional<EntityRuntimeState>>;
};

template <class Derived>
class LedgerEntityDefaults {
public:
    template <LedgerEntity T>
    LedgerCompositionResult include_pending_mutations_from(const T& source) const {
        const auto& self = static_cast<const Derived&>(*this);
        auto r = self.memory_resource();
        auto target = self.entity_runtime_state();
        if (!target) {
            return LedgerCompositionResult::err(
                r,
                LedgerCompositionError(std::allocator_arg,
                                       Allocator(r),
                                       LedgerCompositionError::Kind::MissingTargetState));
        }
        auto from = source.entity_runtime_state();
        if (!from) {
            return LedgerCompositionResult::err(
                r,
                LedgerCompositionError(std::allocator_arg,
                                       Allocator(r),
                                       LedgerCompositionError::Kind::MissingSourceState));
        }
        return detail::EntityRuntimeAccess::compose(*target, *from);
    }
};
}
