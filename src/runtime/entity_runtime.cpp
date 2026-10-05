#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/resources.hpp>
#include <charconv>
#include <teaql/core/detail/debug.hpp>

namespace teaql {
namespace {
void number(String& out, std::int64_t n) {
    char b[32];
    auto end = std::to_chars(b, b + sizeof(b), n).ptr;
    out.append(b, end);
}

String conflict_id(Memory r, const Value& value) {
    if (auto id = value.try_u64()) {
        String s(r);
        char b[32];
        auto end = std::to_chars(b, b + sizeof(b), *id).ptr;
        s.append(b, end);
        return s;
    }
    return detail::debug_value(r, value);
}
}

namespace detail {
struct OriginalVersions {
    using allocator_type = Allocator;
    std::optional<std::pair<EntityKey, std::int64_t>> first;
    std::pmr::map<EntityKey, std::int64_t> overflow;

    OriginalVersions(std::allocator_arg_t, Allocator a) : overflow(a.resource()) {}

    OriginalVersions(std::allocator_arg_t, Allocator a, const OriginalVersions& other)
        : overflow(other.overflow, a.resource()) {
        if (other.first) {
            first.emplace(clone_in(a.resource(), other.first->first), other.first->second);
        }
    }

    std::optional<std::int64_t> get(const EntityKey& key) const {
        if (first && first->first == key) {
            return first->second;
        }
        auto it = overflow.find(key);
        return it == overflow.end() ? std::nullopt : std::optional(it->second);
    }

    void insert(const EntityKey& key, std::int64_t version) {
        if (!first) {
            first.emplace(clone_in(overflow.get_allocator().resource(), key), version);
        } else if (first->first == key) {
            first->second = version;
        } else {
            overflow.insert_or_assign(key, version);
        }
    }

    void merge_from(const OriginalVersions& other) {
        if (other.first) {
            insert(other.first->first, other.first->second);
        }
        for (const auto& [key, version] : other.overflow) {
            insert(key, version);
        }
    }

    void clear() {
        first.reset();
        overflow.clear();
    }
};

// Keep the materialized and compact original-snapshot variants distinct even
// while the public compact-row hydration fast path uses LoadedSnapshot instead.
using OriginalSnapshot = std::variant<EntitySnapshot, CompactRow>;

std::optional<OriginalSnapshot> clone_snapshot(Memory r,
                                               const std::optional<OriginalSnapshot>& value) {
    if (!value) {
        return {};
    }
    return std::visit(
        [&](const auto& snapshot) -> OriginalSnapshot {
            return clone_in(r, snapshot);
        },
        *value);
}

struct EntityMutationLedger {
    using allocator_type = Allocator;
    ChangeSetStack change_sets;
    std::optional<String> comment;
    EntityRuntimeState::Keys deleted_keys, new_keys;
    std::optional<OriginalSnapshot> original_snapshot;
    std::pmr::map<EntityKey, TraceChain> trace_chains;
    OriginalVersions original_versions;
    Vector<LedgerCompositionError> composition_errors;
    bool is_new = false;

    EntityMutationLedger(std::allocator_arg_t, Allocator a)
        : change_sets(std::allocator_arg, a), deleted_keys(a.resource()), new_keys(a.resource()),
          trace_chains(a.resource()), original_versions(std::allocator_arg, a),
          composition_errors(a.resource()) {}

    EntityMutationLedger(std::allocator_arg_t, Allocator a, const EntityMutationLedger& other)
        : change_sets(std::allocator_arg, a, other.change_sets),
          comment(clone_in(a.resource(), other.comment)),
          deleted_keys(other.deleted_keys, a.resource()), new_keys(other.new_keys, a.resource()),
          original_snapshot(clone_snapshot(a.resource(), other.original_snapshot)),
          trace_chains(other.trace_chains, a.resource()),
          original_versions(std::allocator_arg, a, other.original_versions),
          composition_errors(other.composition_errors, a.resource()), is_new(other.is_new) {}
};

struct EntityLedgerSlot {
    using allocator_type = Allocator;
    ScopedBox<EntityMutationLedger> value;

    EntityLedgerSlot(std::allocator_arg_t, Allocator a)
        : value(nullptr, ScopedDelete<EntityMutationLedger>{a.resource()}) {}
};

struct FrozenEntityGraph {
    using allocator_type = Allocator;
    EntityGraphBuilder contents;

    FrozenEntityGraph(std::allocator_arg_t, Allocator a, EntityGraphBuilder&& builder)
        : contents(std::allocator_arg, a, std::move(builder)) {}

    FrozenEntityGraph(std::allocator_arg_t, Allocator a, const FrozenEntityGraph& other)
        : contents(std::allocator_arg, a, other.contents) {}
};

struct EntityGraphSlot {
    using allocator_type = Allocator;
    ScopedBox<FrozenEntityGraph> value;

    EntityGraphSlot(std::allocator_arg_t, Allocator a)
        : value(nullptr, ScopedDelete<FrozenEntityGraph>{a.resource()}) {}
};

// One destination-scoped copy session preserves aliases and graph cycles. The
// map only borrows source pointers during copying and is gone before returning.
struct EntityRuntimeClone {
    Memory resource;
    std::pmr::unordered_map<const EntityLedgerSlot*, EntityLedgerSlot*> ledgers;
    std::pmr::unordered_map<const EntityGraphSlot*, EntityGraphSlot*> graphs;

    explicit EntityRuntimeClone(Memory r) : resource(r), ledgers(r), graphs(r) {}

    EntityLedgerSlot* ledger(const EntityLedgerSlot* source) {
        if (auto it = ledgers.find(source); it != ledgers.end()) {
            return it->second;
        }
        auto* target = &resource_arena(resource).make<EntityLedgerSlot>();
        ledgers.emplace(source, target);
        if (source->value) {
            target->value = make_box<EntityMutationLedger>(resource, *source->value);
        }
        return target;
    }

    EntityGraphSlot* graph(const EntityGraphSlot* source) {
        if (auto it = graphs.find(source); it != graphs.end()) {
            return it->second;
        }
        auto* target = &resource_arena(resource).make<EntityGraphSlot>();
        graphs.emplace(source, target);
        if (source->value) {
            target->value = make_box<FrozenEntityGraph>(resource, *source->value);
        }
        return target;
    }
};

struct EntityCloneScope {
    EntityRuntimeClone local;
    ContextArena& arena;
    bool owner;

    explicit EntityCloneScope(Memory r)
        : local(r), arena(dynamic_cast<ContextArena&>(resource_arena(r))),
          owner(arena.entity_runtime_clone == nullptr) {
        if (owner) {
            arena.entity_runtime_clone = &local;
        }
    }

    ~EntityCloneScope() {
        if (owner) {
            arena.entity_runtime_clone = nullptr;
        }
    }

    EntityRuntimeClone& get() {
        return *arena.entity_runtime_clone;
    }
};

bool EntityRuntimeAccess::has_mutation_context(const EntityRuntimeState& state) {
    return bool(state.inner_->value);
}

std::optional<LedgerCompositionError>
EntityRuntimeAccess::first_composition_error(const EntityRuntimeState& state) {
    if (!state.inner_->value || state.inner_->value->composition_errors.empty()) {
        return {};
    }
    return clone_in(state.resource_, state.inner_->value->composition_errors.front());
}
}

EntityGraphBuilder::EntityGraphBuilder(std::allocator_arg_t,
                                       Allocator a,
                                       const EntityGraphBuilder& other)
    : EntityGraphBuilder(std::allocator_arg, a) {
    detail::EntityCloneScope scope(a.resource());
    for (const auto& [type, table] : other.tables_) {
        auto [out, _] = tables_.try_emplace(type);
        for (const auto& [id, value] : table) {
            out->second.emplace(id, value);
        }
    }
    for (const auto& [key, value] : other.relations_) {
        relations_.emplace(key, value);
    }
}

std::size_t EntityGraphBuilder::entity_count() const noexcept {
    std::size_t count = 0;
    for (const auto& [_, table] : tables_) {
        count += table.size();
    }
    return count;
}

void EntityChangeSet::set(const EntityKey& key, std::string_view field, const Value& value) {
    auto [found, _] = changes_.try_emplace(key);
    found->second.fields.insert_or_assign(String(field, memory_resource()),
                                          Value(memory_resource(), value));
}

const Value* EntityChangeSet::get(const EntityKey& key, std::string_view field) const {
    auto found = changes_.find(key);
    if (found == changes_.end()) {
        return nullptr;
    }
    auto v = found->second.fields.find(field);
    return v == found->second.fields.end() ? nullptr : &v->second;
}

FieldSet EntityChangeSet::field_names(const EntityKey& key) const {
    FieldSet fields(memory_resource());
    if (auto found = changes_.find(key); found != changes_.end()) {
        for (const auto& [name, _] : found->second.fields) {
            fields.emplace(name);
        }
    }
    return fields;
}

bool EntityChangeSet::operator==(const EntityChangeSet& other) const {
    if (changes_.size() != other.changes_.size()) {
        return false;
    }
    auto a = changes_.begin(), b = other.changes_.begin();
    for (; a != changes_.end(); ++a, ++b) {
        if (a->first != b->first || a->second.fields != b->second.fields) {
            return false;
        }
    }
    return true;
}

std::optional<EntityChangeSet> ChangeSetStack::pop() {
    if (stack_.empty()) {
        return {};
    }
    auto out = std::move(stack_.back());
    stack_.pop_back();
    return out;
}

std::optional<Value> ChangeSetStack::get(const EntityKey& key, std::string_view field) const {
    for (auto i = stack_.rbegin(); i != stack_.rend(); ++i) {
        if (auto* v = i->get(key, field)) {
            return Value(memory_resource(), *v);
        }
    }
    return {};
}

void ChangeSetStack::clear_current() {
    if (!stack_.empty()) {
        stack_.back() = EntityChangeSet(std::allocator_arg, Allocator(memory_resource()));
    }
}

FieldSet ChangeSetStack::changed_field_names(const EntityKey& key) const {
    FieldSet out(memory_resource());
    for (const auto& set : stack_) {
        for (const auto& name : set.field_names(key)) {
            out.emplace(name);
        }
    }
    return out;
}

String LedgerCompositionError::to_string(Memory r) const {
    if (kind == Kind::MissingTargetState) {
        return String("target entity has no mutation ledger", r);
    }
    if (kind == Kind::MissingSourceState) {
        return String("source entity has no mutation ledger", r);
    }
    String s("cannot compose ", r);
    s += entity;
    s += '#';
    s += id;
    s += ": receiving ledger expects version ";
    number(s, target);
    s += ", source expects version ";
    number(s, source);
    return s;
}

EntityRuntimeState::EntityRuntimeState(std::allocator_arg_t, Allocator a)
    : resource_(a.resource()),
      inner_(&detail::resource_arena(resource_).make<detail::EntityLedgerSlot>()),
      graph_{&detail::resource_arena(resource_).make<detail::EntityGraphSlot>(), false} {}

EntityRuntimeState::EntityRuntimeState(std::allocator_arg_t,
                                       Allocator a,
                                       const EntityRuntimeState& other)
    : resource_(a.resource()), inner_(other.inner_), graph_(other.graph_.promote()),
      loaded_snapshot_(detail::clone_in(resource_, other.loaded_snapshot_)) {
    detail::resource_arena(other.resource_);
    detail::resource_arena(resource_);
    if (resource_ != other.resource_) {
        detail::EntityCloneScope scope(resource_);
        inner_ = scope.get().ledger(other.inner_);
        graph_.slot = scope.get().graph(other.graph_.slot);
    }
    // Rust Clone promotes the graph-owned weak view; Context still owns storage.
}

EntityRuntimeState EntityRuntimeState::fresh_with_shared_graph(Memory r,
                                                               const EntityRuntimeState& source) {
    EntityRuntimeState result(std::allocator_arg, Allocator(r));
    detail::resource_arena(source.resource_);
    if (r == source.resource_) {
        result.graph_ = source.graph_.preserve();
    } else {
        detail::EntityCloneScope scope(r);
        result.graph_.slot = scope.get().graph(source.graph_.slot);
    }
    result.graph_.is_weak = source.graph_.is_weak;
    return result;
}

EntityRuntimeState EntityRuntimeState::with_shared_graph(const EntityRuntimeState& source) const {
    EntityRuntimeState result(std::allocator_arg, Allocator(resource_), *this);
    if (resource_ == source.resource_) {
        result.graph_ = source.graph_.preserve();
    } else {
        detail::EntityCloneScope scope(resource_);
        result.graph_.slot = scope.get().graph(source.graph_.slot);
    }
    result.graph_.is_weak = source.graph_.is_weak;
    return result;
}

detail::EntityMutationLedger& EntityRuntimeState::context() const {
    detail::resource_arena(resource_);
    if (!inner_->value) {
        inner_->value = detail::make_box<detail::EntityMutationLedger>(resource_);
    }
    return *inner_->value;
}

const EntityGraphBuilder* EntityRuntimeState::frozen() const {
    detail::resource_arena(resource_);
    return graph_.slot->value ? &graph_.slot->value->contents : nullptr;
}

Result<std::monostate, EntityGraphBuilder>
EntityRuntimeState::freeze_graph(EntityGraphBuilder builder) const {
    using R = Result<std::monostate, EntityGraphBuilder>;
    detail::resource_arena(resource_);
    if (graph_.is_weak || graph_.slot->value) {
        return R::err(resource_, std::move(builder));
    }
    graph_.slot->value = detail::make_box<detail::FrozenEntityGraph>(resource_, std::move(builder));
    return R::ok(resource_, {});
}

const detail::EntityGraphValue* EntityRuntimeState::relation_value(
    std::string_view owner, std::uint64_t id, std::string_view relation) const {
    auto* graph = frozen();
    if (!graph) {
        return nullptr;
    }
    detail::EntityRelationKey key(std::allocator_arg, Allocator(resource_), owner, id, relation);
    auto found = graph->relations_.find(key);
    return found == graph->relations_.end() ? nullptr : &found->second;
}

void EntityRuntimeState::relation_type_mismatch(std::string_view owner,
                                                std::uint64_t id,
                                                std::string_view relation) const {
    String s("relation view type mismatch: owner=", resource_);
    s += owner;
    s += " id=";
    char b[32];
    auto end = std::to_chars(b, b + sizeof(b), id).ptr;
    s.append(b, end);
    s += " relation=";
    s += relation;
    throw std::logic_error(std::string(s));
}

void EntityRuntimeState::push_change_set() const {
    context().change_sets.push();
}

std::optional<EntityChangeSet> EntityRuntimeState::pop_change_set() const {
    return inner_->value ? context().change_sets.pop() : std::nullopt;
}

void EntityRuntimeState::clear_current_change_set() const {
    if (inner_->value) {
        context().change_sets.clear_current();
    }
}

void EntityRuntimeState::clear_committed() const {
    if (!inner_->value) {
        return;
    }
    auto& c = context();
    c.change_sets = ChangeSetStack(std::allocator_arg, Allocator(resource_));
    c.deleted_keys.clear();
    c.new_keys.clear();
    c.original_versions.clear();
    c.trace_chains.clear();
    c.original_snapshot.reset();
    c.comment.reset();
    c.is_new = false;
    // Source intentionally keeps composition_errors and the per-view loaded row.
}

void EntityRuntimeState::set(const EntityKey& key,
                             std::string_view field,
                             const Value& value) const {
    context().change_sets.set(key, field, value);
}

std::optional<Value> EntityRuntimeState::get(const EntityKey& key, std::string_view field) const {
    return inner_->value ? inner_->value->change_sets.get(key, field) : std::nullopt;
}

EntityChangeSet EntityRuntimeState::current_change_set() const {
    if (inner_->value) {
        if (auto* set = inner_->value->change_sets.current()) {
            return detail::clone_in(resource_, *set);
        }
    }
    return EntityChangeSet(std::allocator_arg, Allocator(resource_));
}

void EntityRuntimeState::set_comment(std::string_view comment) const {
    context().comment.emplace(comment, resource_);
}

std::optional<String> EntityRuntimeState::get_comment() const {
    return inner_->value ? detail::clone_in(resource_, inner_->value->comment) : std::nullopt;
}

void EntityRuntimeState::mark_as_new(const EntityKey& key) const {
    context().new_keys.emplace(key);
}

bool EntityRuntimeState::is_new(const EntityKey& key) const {
    return inner_->value && inner_->value->new_keys.contains(key);
}

void EntityRuntimeState::set_original_snapshot(const EntitySnapshot& snapshot) const {
    context().original_snapshot.emplace(detail::clone_in(resource_, snapshot));
}

void EntityRuntimeState::set_original_compact_row(std::string_view entity, const CompactRow& row) {
    loaded_snapshot_.emplace(std::allocator_arg, Allocator(resource_), entity, row);
}

std::optional<EntitySnapshot> EntityRuntimeState::original_snapshot() const {
    if (loaded_snapshot_) {
        EntitySnapshot out(resource_);
        out.fields = loaded_snapshot_->row.into_map(resource_);
        return out;
    }
    if (!inner_->value || !inner_->value->original_snapshot) {
        return {};
    }
    return std::visit(
        [&](const auto& value) -> EntitySnapshot {
            if constexpr (std::same_as<std::decay_t<decltype(value)>, EntitySnapshot>) {
                return detail::clone_in(resource_, value);
            } else {
                EntitySnapshot out(resource_);
                out.fields = value.into_map(resource_);
                return out;
            }
        },
        *inner_->value->original_snapshot);
}

void EntityRuntimeState::mark_as_delete(const EntityKey& key) const {
    auto& c = context();
    c.change_sets.clear_entity(key);
    c.deleted_keys.emplace(key);
}

bool EntityRuntimeState::is_marked_as_delete(const EntityKey& key) const {
    return inner_->value && inner_->value->deleted_keys.contains(key);
}

FieldSet EntityRuntimeState::changed_field_names(const EntityKey& key) const {
    return inner_->value ? inner_->value->change_sets.changed_field_names(key)
                         : FieldSet(resource_);
}

EntityRuntimeState::Keys EntityRuntimeState::deleted_keys() const {
    return inner_->value ? Keys(inner_->value->deleted_keys, resource_) : Keys(resource_);
}

EntityRuntimeState::Keys EntityRuntimeState::new_keys() const {
    return inner_->value ? Keys(inner_->value->new_keys, resource_) : Keys(resource_);
}

std::optional<std::int64_t> EntityRuntimeState::get_original_version(const EntityKey& key) const {
    if (inner_->value) {
        if (auto version = inner_->value->original_versions.get(key)) {
            return version;
        }
    }
    if (!loaded_snapshot_ || loaded_snapshot_->entity != key.entity) {
        return {};
    }
    auto* id = loaded_snapshot_->row.get("id");
    auto* version = loaded_snapshot_->row.get("version");
    if (!id || !version || !id->try_u64() || id->try_u64() != key.id.try_u64()) {
        return {};
    }
    return version->try_i64();
}

TraceChain EntityRuntimeState::get_trace_chain(const EntityKey& key) const {
    if (inner_->value) {
        auto found = inner_->value->trace_chains.find(key);
        if (found != inner_->value->trace_chains.end()) {
            return TraceChain(found->second, resource_);
        }
    }
    return TraceChain(resource_);
}

void EntityRuntimeState::set_original_version(const EntityKey& key, std::int64_t version) const {
    context().original_versions.insert(key, version);
}

LedgerCompositionResult
EntityRuntimeState::adopt_mutations_from_checked(const EntityRuntimeState& source) const {
    using R = LedgerCompositionResult;
    auto r = resource_;
    if (!source.inner_->value || inner_ == source.inner_) {
        return R::ok(r, {});
    }
    detail::EntityMutationLedger snapshot(std::allocator_arg, Allocator(r), *source.inner_->value);
    std::optional<std::pair<EntityKey, std::int64_t>> loaded_version;
    if (source.loaded_snapshot_) {
        auto* id = source.loaded_snapshot_->row.get("id");
        auto* v = source.loaded_snapshot_->row.get("version");
        if (id && v) {
            if (auto version = v->try_i64()) {
                loaded_version.emplace(
                    EntityKey(
                        std::allocator_arg, Allocator(r), source.loaded_snapshot_->entity, *id),
                    *version);
            }
        }
    }
    auto& target = context();
    auto conflict = [&](const EntityKey& key,
                        std::int64_t version) -> std::optional<LedgerCompositionError> {
        auto expected = target.original_versions.get(key);
        // Source composition deliberately compares optional numeric IDs, including
        // None == None; public get_original_version has a stricter Some guard.
        if (!expected && loaded_snapshot_ && loaded_snapshot_->entity == key.entity) {
            auto* id = loaded_snapshot_->row.get("id");
            auto* v = loaded_snapshot_->row.get("version");
            if (id && v && id->try_u64() == key.id.try_u64()) {
                expected = v->try_i64();
            }
        }
        if (expected && *expected != version) {
            return LedgerCompositionError(std::allocator_arg,
                                          Allocator(r),
                                          LedgerCompositionError::Kind::ConflictingOriginalVersion,
                                          key.entity,
                                          conflict_id(r, key.id),
                                          *expected,
                                          version);
        }
        return {};
    };
    if (snapshot.original_versions.first) {
        const auto& [key, v] = *snapshot.original_versions.first;
        if (auto e = conflict(key, v)) {
            return R::err(r, std::move(*e));
        }
    }
    for (const auto& [key, v] : snapshot.original_versions.overflow) {
        if (auto e = conflict(key, v)) {
            return R::err(r, std::move(*e));
        }
    }
    if (loaded_version) {
        if (auto e = conflict(loaded_version->first, loaded_version->second)) {
            return R::err(r, std::move(*e));
        }
    }
    for (const auto& set : snapshot.change_sets.stack_) {
        for (const auto& [key, values] : set.changes()) {
            for (const auto& [field, value] : values.fields) {
                target.change_sets.set(key, field, value);
            }
        }
    }
    for (const auto& key : snapshot.deleted_keys) {
        target.deleted_keys.emplace(key);
    }
    for (const auto& key : snapshot.new_keys) {
        target.new_keys.emplace(key);
    }
    target.original_versions.merge_from(snapshot.original_versions);
    if (loaded_version) {
        target.original_versions.insert(loaded_version->first, loaded_version->second);
    }
    for (const auto& [key, traces] : snapshot.trace_chains) {
        auto [entry, _] = target.trace_chains.try_emplace(key);
        for (const auto& trace : traces) {
            entry->second.emplace_back(trace);
        }
    }
    if (!target.original_snapshot) {
        target.original_snapshot = detail::clone_snapshot(r, snapshot.original_snapshot);
    }
    if (!target.comment) {
        target.comment = detail::clone_in(r, snapshot.comment);
    }
    target.is_new |= snapshot.is_new;
    return R::ok(r, {});
}

void EntityRuntimeState::adopt_mutations_from(const EntityRuntimeState& source) const {
    auto result = adopt_mutations_from_checked(source);
    if (!result) {
        context().composition_errors.emplace_back(result.error());
    }
}
}
