#include <limits>
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <teaql/runtime/resources.hpp>
#include <charconv>
#include <teaql/core/detail/debug.hpp>
#include <teaql/runtime/graph_debug.hpp>
#include <teaql/core/dynamic_fields_debug.hpp>

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
    std::pmr::map<EntityKey, String> entity_comments;
    EntityRuntimeState::Keys deleted_keys, new_keys;
    std::optional<OriginalSnapshot> original_snapshot;
    std::pmr::map<EntityKey, OriginalSnapshot> original_snapshots;
    EntityRuntimeAccess::TraceScopes trace_chains, unassigned_trace_scopes;
    OriginalVersions original_versions;
    Vector<LedgerCompositionError> composition_errors;
    bool is_new = false;

    EntityMutationLedger(std::allocator_arg_t, Allocator a)
        : change_sets(std::allocator_arg, a), entity_comments(a.resource()),
          deleted_keys(a.resource()), new_keys(a.resource()), original_snapshots(a.resource()),
          trace_chains(a.resource()), unassigned_trace_scopes(a.resource()),
          original_versions(std::allocator_arg, a), composition_errors(a.resource()) {}

    EntityMutationLedger(std::allocator_arg_t, Allocator a, const EntityMutationLedger& other)
        : change_sets(std::allocator_arg, a, other.change_sets),
          comment(clone_in(a.resource(), other.comment)),
          entity_comments(other.entity_comments, a.resource()),
          deleted_keys(other.deleted_keys, a.resource()), new_keys(other.new_keys, a.resource()),
          original_snapshot(clone_snapshot(a.resource(), other.original_snapshot)),
          original_snapshots(a.resource()), trace_chains(a.resource()),
          unassigned_trace_scopes(a.resource()),
          original_versions(std::allocator_arg, a, other.original_versions),
          composition_errors(other.composition_errors, a.resource()), is_new(other.is_new) {
        TraceScopeCopyCache scopes(a.resource());
        for (const auto& [key, token] : other.trace_chains) {
            trace_chains.emplace(clone_in(a.resource(), key),
                                 clone_trace_scope(a.resource(), token, scopes));
        }
        for (const auto& [key, token] : other.unassigned_trace_scopes) {
            unassigned_trace_scopes.emplace(clone_in(a.resource(), key),
                                            clone_trace_scope(a.resource(), token, scopes));
        }
        for (const auto& [key, snapshot] : other.original_snapshots) {
            original_snapshots.emplace(clone_in(a.resource(), key),
                                       std::visit(
                                           [&](const auto& value) -> OriginalSnapshot {
                                               return clone_in(a.resource(), value);
                                           },
                                           snapshot));
        }
    }
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

void with_entity_runtime_copy(Memory r, FunctionRef<void()> copy) {
    EntityCloneScope scope(r);
    copy();
}

bool EntityRuntimeAccess::has_mutation_context(const EntityRuntimeState& state) {
    return state.inner_ && bool(state.inner_->value);
}

std::optional<LedgerCompositionError>
EntityRuntimeAccess::first_composition_error(const EntityRuntimeState& state) {
    if (!state.inner_ || !state.inner_->value || state.inner_->value->composition_errors.empty()) {
        return {};
    }
    return clone_in(state.resource_, state.inner_->value->composition_errors.front());
}
}

EntityGraphBuilder::EntityGraphBuilder(std::allocator_arg_t,
                                       Allocator a,
                                       const EntityGraphBuilder& other)
    : EntityGraphBuilder(std::allocator_arg, a) {
    last_read_view_ = other.last_read_view_;
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
    if (other.option_entities_) {
        option_entities_ =
            detail::make_box<RelationOptionEntities>(a.resource(), *other.option_entities_);
    }
    if (other.relation_facets_) {
        relation_facets_.emplace(a.resource());
        for (const auto& [key, facets] : *other.relation_facets_) {
            relation_facets_->emplace(key, facets);
        }
    }
}

void EntityGraphBuilder::install_relation_facets(std::string_view owner,
                                                 std::uint64_t id,
                                                 std::string_view relation,
                                                 const RelationFacets& facets) {
    install_relation_facets(0, owner, id, relation, facets);
}

void EntityGraphBuilder::install_relation_facets(std::uint64_t view,
                                                 std::string_view owner,
                                                 std::uint64_t id,
                                                 std::string_view relation,
                                                 const RelationFacets& facets) {
    reserve_read_view(view);
    if (facets.empty()) {
        return;
    }
    auto r = memory_resource();
    if (!relation_facets_) {
        relation_facets_.emplace(r);
    }
    detail::EntityRelationKey key(std::allocator_arg, Allocator(r), view, owner, id, relation);
    relation_facets_->erase(key);
    relation_facets_->emplace(std::move(key), facets);
}

std::uint64_t EntityGraphBuilder::allocate_read_view() {
    if (last_read_view_ == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Embedded relation view limit exhausted");
    }
    return ++last_read_view_;
}

void EntityGraphBuilder::reserve_read_view(std::uint64_t view) noexcept {
    last_read_view_ = std::max(last_read_view_, view);
}

std::size_t EntityGraphBuilder::entity_count() const noexcept {
    std::size_t count = 0;
    for (const auto& [_, table] : tables_) {
        count += table.size();
    }
    return count;
}

struct EntityChangeSet::DynamicData {
    using allocator_type = Allocator;
    DynamicFieldChanges changes;
    std::pmr::map<EntityKey, ArenaBorrow<DynamicFieldDefinitions>> definitions;
    std::pmr::map<EntityKey, DynamicOriginals> originals;

    DynamicData(std::allocator_arg_t, Allocator a)
        : changes(a.resource()), definitions(a.resource()), originals(a.resource()) {}

    DynamicData(std::allocator_arg_t, Allocator a, const DynamicData& other)
        : changes(other.changes, a.resource()), definitions(a.resource()), originals(a.resource()) {
        auto& target = detail::resource_arena(a.resource());
        for (const auto& [key, binding] : other.definitions) {
            const auto& schema = binding.get();
            auto& origin = detail::resource_arena(schema.memory_resource());
            const auto& copy =
                origin.can_lend_to(target) ? schema : target.make<DynamicFieldDefinitions>(schema);
            definitions.emplace(detail::clone_in(a.resource(), key),
                                ArenaBorrow<DynamicFieldDefinitions>(std::allocator_arg, a, copy));
        }
        for (const auto& [key, fields] : other.originals) {
            auto [entry, inserted] = originals.try_emplace(key);
            for (const auto& [code, value] : fields) {
                entry->second.emplace(String(code, a.resource()),
                                      detail::clone_in(a.resource(), value));
            }
        }
    }
};

EntityChangeSet::EntityChangeSet(std::allocator_arg_t, Allocator a) : changes_(a.resource()) {}

EntityChangeSet::EntityChangeSet(std::allocator_arg_t, Allocator a, const EntityChangeSet& other)
    : changes_(other.changes_, a.resource()) {
    if (other.dynamic_) {
        dynamic_ = detail::make_box<DynamicData>(a.resource(), *other.dynamic_);
    }
}

EntityChangeSet::EntityChangeSet(std::allocator_arg_t, Allocator a, EntityChangeSet&& other)
    : EntityChangeSet(detail::move_to(a.resource(), std::move(other))) {}

EntityChangeSet::EntityChangeSet(EntityChangeSet&&) noexcept = default;
EntityChangeSet::~EntityChangeSet() = default;

bool EntityChangeSet::is_empty() const noexcept {
    return changes_.empty() && (!dynamic_ || dynamic_->changes.empty());
}

const DynamicFieldChanges* EntityChangeSet::dynamic_changes() const noexcept {
    return dynamic_ ? &dynamic_->changes : nullptr;
}

const DynamicFieldDefinitions* EntityChangeSet::dynamic_definitions(const EntityKey& key) const {
    if (!dynamic_) {
        return nullptr;
    }
    auto found = dynamic_->definitions.find(key);
    return found == dynamic_->definitions.end() ? nullptr : &found->second.get();
}

const EntityChangeSet::DynamicOriginals*
EntityChangeSet::dynamic_originals(const EntityKey& key) const {
    if (!dynamic_) {
        return nullptr;
    }
    auto found = dynamic_->originals.find(key);
    return found == dynamic_->originals.end() ? nullptr : &found->second;
}

void EntityChangeSet::set_dynamic(const EntityKey& key,
                                  std::string_view code,
                                  const DynamicFieldMutation& mutation,
                                  const DynamicFieldDefinitions& definitions,
                                  const Value* original) {
    auto r = memory_resource();
    if (!dynamic_) {
        dynamic_ = detail::make_box<DynamicData>(r);
    }
    if (!dynamic_->definitions.contains(key)) {
        auto& arena = detail::resource_arena(r);
        auto& origin = detail::resource_arena(definitions.memory_resource());
        const auto& copy = origin.can_lend_to(arena)
                               ? definitions
                               : arena.make<DynamicFieldDefinitions>(definitions);
        dynamic_->definitions.emplace(
            detail::clone_in(r, key),
            ArenaBorrow<DynamicFieldDefinitions>(std::allocator_arg, Allocator(r), copy));
    }
    auto [entry, inserted] = dynamic_->originals.try_emplace(key);
    if (!entry->second.contains(code)) {
        entry->second.emplace(String(code, r),
                              original ? std::optional<Value>(Value(r, *original)) : std::nullopt);
    }
    auto [changes, created] = dynamic_->changes.try_emplace(key);
    changes->second.insert_or_assign(String(code, r), detail::clone_in(r, mutation));
}

void EntityChangeSet::clear_entity(const EntityKey& key) {
    changes_.erase(key);
    if (dynamic_) {
        dynamic_->changes.erase(key);
        dynamic_->definitions.erase(key);
        dynamic_->originals.erase(key);
    }
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
    if (dynamic_) {
        auto found = dynamic_->changes.find(key);
        if (found != dynamic_->changes.end()) {
            for (const auto& [code, mutation] : found->second) {
                String name("#", memory_resource());
                name += code;
                fields.emplace(std::move(name));
            }
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
    if (bool(dynamic_) != bool(other.dynamic_)) {
        return false;
    }
    if (!dynamic_) {
        return true;
    }
    if (dynamic_->changes != other.dynamic_->changes ||
        dynamic_->originals != other.dynamic_->originals ||
        dynamic_->definitions.size() != other.dynamic_->definitions.size()) {
        return false;
    }
    for (const auto& [key, definitions] : dynamic_->definitions) {
        auto found = other.dynamic_->definitions.find(key);
        if (found == other.dynamic_->definitions.end() ||
            definitions.get() != found->second.get()) {
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
    if (kind == Kind::ConflictingDynamicDefinitions) {
        String error("cannot compose incompatible dynamic definitions for ", r);
        error += entity;
        return error;
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
      graph_{&detail::resource_arena(resource_).make<detail::EntityGraphSlot>(), false},
      lifetime_(resource_) {}

EntityRuntimeState::EntityRuntimeState(std::allocator_arg_t,
                                       Allocator a,
                                       const EntityRuntimeState& other)
    : resource_(a.resource()), inner_(&other.shared_ledger_slot()), graph_(other.graph_.promote()),
      lifetime_(resource_) {
    detail::resource_arena(other.resource_);
    detail::resource_arena(resource_);
    if (resource_ != other.resource_) {
        detail::EntityCloneScope scope(resource_);
        inner_ = scope.get().ledger(inner_);
        graph_.slot = scope.get().graph(other.graph_.slot);
    }
    copy_loaded_view_from(other);
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
    result.json_view_ = source.json_view_;
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
    result.json_view_ = source.json_view_;
    return result;
}

void EntityRuntimeState::copy_loaded_view_from(const EntityRuntimeState& other) {
    json_view_ = other.json_view_;
    dynamic_fields_.reset();
    if (const auto* fields = other.loaded_dynamic_fields()) {
        install_loaded_dynamic_fields(*fields);
    }
    loaded_snapshot_.reset();
    shared_snapshot_.reset();
    if (const auto* snapshot = other.loaded_snapshot()) {
        if (resource_ == other.resource_) {
            if (!other.shared_snapshot_) {
                auto& shared = detail::resource_arena(resource_).make<LoadedSnapshot>(
                    std::move(*other.loaded_snapshot_));
                other.shared_snapshot_.emplace(std::allocator_arg, Allocator(resource_), shared);
                other.loaded_snapshot_.reset();
            }
            shared_snapshot_ = other.shared_snapshot_;
        } else {
            loaded_snapshot_.emplace(std::allocator_arg, Allocator(resource_), *snapshot);
        }
    }
}

EntityRuntimeState
EntityRuntimeState::with_loaded_view_from(const EntityRuntimeState& source) const& {
    EntityRuntimeState result(std::allocator_arg, Allocator(resource_), *this);
    result.copy_loaded_view_from(source);
    return result;
}

EntityRuntimeState EntityRuntimeState::with_loaded_view_from(const EntityRuntimeState& source) && {
    detail::resource_arena(resource_);
    if (this != &source) {
        copy_loaded_view_from(source);
    }
    return std::move(*this);
}

detail::EntityLedgerSlot& EntityRuntimeState::shared_ledger_slot() const {
    auto& arena = detail::resource_arena(resource_);
    if (!inner_) {
        inner_ = &arena.make<detail::EntityLedgerSlot>();
    }
    return *inner_;
}

detail::EntityMutationLedger& EntityRuntimeState::context() const {
    shared_ledger_slot();
    if (!inner_ || !inner_->value) {
        auto ledger = detail::make_box<detail::EntityMutationLedger>(resource_);
        if (const auto* snapshot = loaded_snapshot()) {
            if (const auto* id = snapshot->row.get("id")) {
                ledger->original_snapshots.emplace(
                    EntityKey(std::allocator_arg, Allocator(resource_), snapshot->entity, *id),
                    detail::clone_in(resource_, snapshot->row));
            }
        }
        inner_->value = std::move(ledger);
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

const SmartList<CompactRow>* EntityRuntimeState::relation_facet(std::string_view owner,
                                                                std::uint64_t id,
                                                                std::string_view relation,
                                                                std::string_view facet) const {
    auto* graph = frozen();
    if (!graph || !graph->relation_facets_) {
        return nullptr;
    }
    detail::EntityRelationKey key(
        std::allocator_arg, Allocator(resource_), json_view_, owner, id, relation);
    auto found = graph->relation_facets_->find(key);
    if (found == graph->relation_facets_->end()) {
        return nullptr;
    }
    auto result = found->second.find(facet);
    return result == found->second.end() ? nullptr : &result->second;
}

const detail::EntityGraphValue* EntityRuntimeState::relation_value(
    std::string_view owner, std::uint64_t id, std::string_view relation) const {
    auto* graph = frozen();
    if (!graph) {
        return nullptr;
    }
    detail::EntityRelationKey key(
        std::allocator_arg, Allocator(resource_), json_view_, owner, id, relation);
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
    return inner_ && inner_->value ? context().change_sets.pop() : std::nullopt;
}

void EntityRuntimeState::clear_current_change_set() const {
    if (inner_ && inner_->value) {
        context().change_sets.clear_current();
    }
}

void EntityRuntimeState::clear_committed() const {
    if (!inner_ || !inner_->value) {
        return;
    }
    auto& c = context();
    c.change_sets = ChangeSetStack(std::allocator_arg, Allocator(resource_));
    c.deleted_keys.clear();
    c.new_keys.clear();
    c.original_versions.clear();
    c.trace_chains.clear();
    c.unassigned_trace_scopes.clear();
    c.original_snapshot.reset();
    c.original_snapshots.clear();
    c.comment.reset();
    c.entity_comments.clear();
    c.is_new = false;
    // Source intentionally keeps composition_errors and the per-view loaded row.
}

void EntityRuntimeState::set(const EntityKey& key,
                             std::string_view field,
                             const Value& value) const {
    context().change_sets.set(key, field, value);
}

std::optional<Value> EntityRuntimeState::get(const EntityKey& key, std::string_view field) const {
    return inner_ && inner_->value ? inner_->value->change_sets.get(key, field) : std::nullopt;
}

EntityChangeSet EntityRuntimeState::current_change_set() const {
    if (inner_ && inner_->value) {
        if (auto* set = inner_->value->change_sets.current()) {
            return detail::clone_in(resource_, *set);
        }
    }
    return EntityChangeSet(std::allocator_arg, Allocator(resource_));
}

bool EntityRuntimeState::operator==(const EntityRuntimeState& other) const {
    const auto* left = loaded_dynamic_fields();
    const auto* right = other.loaded_dynamic_fields();
    return ((!left && !right) || (left && right && *left == *right)) &&
           (this == &other || (inner_ && inner_ == other.inner_));
}

const DynamicFieldValues* EntityRuntimeState::loaded_dynamic_fields() const {
    return dynamic_fields_ ? &dynamic_fields_->get() : nullptr;
}

void EntityRuntimeState::install_loaded_dynamic_fields(const DynamicFieldValues& values) {
    auto& copy = detail::resource_arena(resource_).make<DynamicFieldValues>(values);
    dynamic_fields_.emplace(std::allocator_arg, Allocator(resource_), copy);
}

bool EntityRuntimeState::has_pending_dynamic_mutations() const {
    detail::resource_arena(resource_);
    if (!inner_ || !inner_->value) {
        return false;
    }
    for (const auto& set : inner_->value->change_sets.stack_) {
        if (const auto* changes = set.dynamic_changes(); changes && !changes->empty()) {
            return true;
        }
    }
    return false;
}

Result<bool, DynamicFieldError> EntityRuntimeState::update_dynamic_field(const EntityKey& key,
                                                                         std::string_view code,
                                                                         const Value& value) {
    using Output = Result<bool, DynamicFieldError>;
    if (!dynamic_fields_) {
        return Output::err(resource_,
                           DynamicFieldError(resource_, "DYNAMIC_FIELD_DEFINITIONS_MISSING", code));
    }
    auto& fields = const_cast<DynamicFieldValues&>(dynamic_fields_->get());
    if (fields.definitions().owner_type() != key.entity) {
        return Output::err(resource_,
                           DynamicFieldError(resource_, "DYNAMIC_FIELD_OWNER_MISMATCH", code));
    }
    auto definition = fields.definitions().fields().find(code);
    if (definition != fields.definitions().fields().end()) {
        code = definition->first;
    }
    const bool loaded = fields.selected_codes().contains(code);
    auto found = fields.values().find(code);
    std::optional<Value> original;
    if (found != fields.values().end()) {
        original.emplace(resource_, found->second);
    }
    auto checked = fields.assign(code, value);
    if (!checked) {
        return Output::err(resource_, std::move(checked).error());
    }
    context().change_sets.current_mut().set_dynamic(key,
                                                    code,
                                                    DynamicFieldMutation::set(resource_, value),
                                                    fields.definitions(),
                                                    original ? &*original : nullptr);
    return Output::ok(resource_, !loaded);
}

Result<bool, DynamicFieldError> EntityRuntimeState::delete_dynamic_field(const EntityKey& key,
                                                                         std::string_view code) {
    using Output = Result<bool, DynamicFieldError>;
    if (!dynamic_fields_) {
        return Output::err(resource_,
                           DynamicFieldError(resource_, "DYNAMIC_FIELD_DEFINITIONS_MISSING", code));
    }
    auto& fields = const_cast<DynamicFieldValues&>(dynamic_fields_->get());
    if (fields.definitions().owner_type() != key.entity) {
        return Output::err(resource_,
                           DynamicFieldError(resource_, "DYNAMIC_FIELD_OWNER_MISMATCH", code));
    }
    auto definition = fields.definitions().fields().find(code);
    if (definition != fields.definitions().fields().end()) {
        code = definition->first;
    }
    const bool loaded = fields.selected_codes().contains(code);
    auto found = fields.values().find(code);
    std::optional<Value> original;
    if (found != fields.values().end()) {
        original.emplace(resource_, found->second);
    }
    auto checked = fields.erase(code);
    if (!checked) {
        return Output::err(resource_, std::move(checked).error());
    }
    context().change_sets.current_mut().set_dynamic(key,
                                                    code,
                                                    DynamicFieldMutation::erase(resource_),
                                                    fields.definitions(),
                                                    original ? &*original : nullptr);
    return Output::ok(resource_, loaded);
}

void EntityRuntimeState::set_comment(std::string_view comment) const {
    context().comment.emplace(comment, resource_);
}

std::optional<String> EntityRuntimeState::get_comment() const {
    return inner_ && inner_->value ? detail::clone_in(resource_, inner_->value->comment)
                                   : std::nullopt;
}

void EntityRuntimeState::set_entity_comment(const EntityKey& key, std::string_view comment) const {
    context().entity_comments.insert_or_assign(detail::clone_in(resource_, key),
                                               String(comment, resource_));
}

std::optional<String> EntityRuntimeState::get_entity_comment(const EntityKey& key) const {
    if (inner_ && inner_->value) {
        auto found = inner_->value->entity_comments.find(key);
        if (found != inner_->value->entity_comments.end()) {
            return String(found->second, resource_);
        }
    }
    return {};
}

void EntityRuntimeState::mark_as_new(const EntityKey& key) const {
    context().new_keys.emplace(key);
}

bool EntityRuntimeState::is_new(const EntityKey& key) const {
    return inner_ && inner_->value && inner_->value->new_keys.contains(key);
}

void EntityRuntimeState::set_original_snapshot(const EntitySnapshot& snapshot) const {
    context().original_snapshot.emplace(detail::clone_in(resource_, snapshot));
}

void EntityRuntimeState::set_original_compact_row(std::string_view entity, const CompactRow& row) {
    CompactRow stripped(
        std::allocator_arg, Allocator(resource_), *row.shared_columns(), row.values());
    loaded_snapshot_.emplace(std::allocator_arg, Allocator(resource_), entity, std::move(stripped));
    shared_snapshot_.reset();
}

void EntityRuntimeState::set_original_compact_row(std::string_view entity, CompactRow&& row) {
    CompactRow stripped(std::allocator_arg, Allocator(resource_), std::move(row));
    stripped.clear_loaded_relations();
    loaded_snapshot_.emplace(std::allocator_arg, Allocator(resource_), entity, std::move(stripped));
    shared_snapshot_.reset();
}

const EntityRuntimeState::LoadedSnapshot* EntityRuntimeState::loaded_snapshot() const {
    return shared_snapshot_   ? &shared_snapshot_->get()
           : loaded_snapshot_ ? &*loaded_snapshot_
                              : nullptr;
}

std::optional<EntitySnapshot> EntityRuntimeState::original_values_for(const EntityKey& key) const {
    if (inner_ && inner_->value) {
        auto found = inner_->value->original_snapshots.find(key);
        if (found != inner_->value->original_snapshots.end()) {
            return std::visit(
                [&](const auto& value) -> EntitySnapshot {
                    using T = std::remove_cvref_t<decltype(value)>;
                    if constexpr (std::same_as<T, EntitySnapshot>) {
                        return detail::clone_in(resource_, value);
                    } else {
                        EntitySnapshot out(resource_);
                        out.fields = value.copy_map(resource_);
                        return out;
                    }
                },
                found->second);
        }
    }
    const auto* snapshot = loaded_snapshot();
    if (snapshot && snapshot->entity == key.entity) {
        const auto* id = snapshot->row.get("id");
        if (id && id->try_u64() == key.id.try_u64()) {
            EntitySnapshot out(resource_);
            out.fields = snapshot->row.copy_map(resource_);
            return out;
        }
    }
    return {};
}

std::optional<EntitySnapshot> EntityRuntimeState::original_snapshot() const {
    if (loaded_snapshot()) {
        EntitySnapshot out(resource_);
        out.fields = loaded_snapshot()->row.into_map(resource_);
        return out;
    }
    if (!inner_ || !inner_->value || !inner_->value->original_snapshot) {
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
    return inner_ && inner_->value && inner_->value->deleted_keys.contains(key);
}

FieldSet EntityRuntimeState::changed_field_names(const EntityKey& key) const {
    return inner_ && inner_->value ? inner_->value->change_sets.changed_field_names(key)
                                   : FieldSet(resource_);
}

EntityRuntimeState::Keys EntityRuntimeState::deleted_keys() const {
    return inner_ && inner_->value ? Keys(inner_->value->deleted_keys, resource_) : Keys(resource_);
}

EntityRuntimeState::Keys EntityRuntimeState::new_keys() const {
    return inner_ && inner_->value ? Keys(inner_->value->new_keys, resource_) : Keys(resource_);
}

std::optional<std::int64_t> EntityRuntimeState::get_original_version(const EntityKey& key) const {
    if (inner_ && inner_->value) {
        if (auto version = inner_->value->original_versions.get(key)) {
            return version;
        }
    }
    if (!loaded_snapshot() || loaded_snapshot()->entity != key.entity) {
        return {};
    }
    auto* id = loaded_snapshot()->row.get("id");
    auto* version = loaded_snapshot()->row.get("version");
    if (!id || !version || !id->try_u64() || id->try_u64() != key.id.try_u64()) {
        return {};
    }
    return version->try_i64();
}

TraceChain EntityRuntimeState::get_trace_chain(const EntityKey& key) const {
    if (inner_ && inner_->value) {
        auto found = inner_->value->trace_chains.find(key);
        if (found != inner_->value->trace_chains.end()) {
            return found->second->recover_trace_chain(resource_);
        }
    }
    return TraceChain(resource_);
}

const TraceScopeToken* EntityRuntimeState::trace_scope_for(const EntityKey& key,
                                                           bool unassigned) const {
    if (!inner_ || !inner_->value) {
        return nullptr;
    }
    const auto& scopes =
        unassigned ? inner_->value->unassigned_trace_scopes : inner_->value->trace_chains;
    auto found = scopes.find(key);
    return found == scopes.end() ? nullptr : found->second;
}

void EntityRuntimeState::replace_trace_scopes(const TraceScopes& scopes,
                                              const TraceScopes& unassigned) const {
    TraceScopes copied(resource_), copied_unassigned(resource_);
    detail::TraceScopeCopyCache cache(resource_);
    auto copy = [&](const TraceScopes& source, TraceScopes& destination) {
        for (const auto& [key, token] : source) {
            if (!token) {
                throw std::invalid_argument("Trace scope entries require a token");
            }
            destination.emplace(detail::clone_in(resource_, key),
                                detail::clone_trace_scope(resource_, token, cache));
        }
    };
    copy(scopes, copied);
    copy(unassigned, copied_unassigned);
    auto& ledger = context();
    ledger.trace_chains = std::move(copied);
    ledger.unassigned_trace_scopes = std::move(copied_unassigned);
}

void EntityRuntimeState::enrich_allocated_trace_scopes(
    const std::pmr::map<EntityKey, Value>& assigned) const {
    if (assigned.empty()) {
        return;
    }
    auto& ledger = context();
    std::pmr::unordered_map<const TraceScopeToken*, std::uint64_t> identities(resource_);
    for (const auto& [key, scope] : ledger.unassigned_trace_scopes) {
        auto found = assigned.find(key);
        if (found != assigned.end()) {
            if (auto id = found->second.try_u64(); id && *id > 0) {
                identities.insert_or_assign(scope, *id);
            }
        }
    }
    if (identities.empty()) {
        return;
    }
    detail::TraceScopeCopyCache cache(resource_);
    auto rebind = [&](const TraceScopeToken* scope) {
        Vector<const TraceScopeToken*> pending(resource_);
        for (auto* token = scope; token && !cache.contains(token); token = token->parent()) {
            pending.push_back(token);
        }
        while (!pending.empty()) {
            auto* token = pending.back();
            pending.pop_back();
            auto* parent = token->parent() ? cache.at(token->parent()) : nullptr;
            auto found = identities.find(token);
            bool changed_id = found != identities.end() && token->track.entity_id != found->second;
            if (changed_id || parent != token->parent()) {
                auto track = detail::clone_in(resource_, token->track);
                if (found != identities.end()) {
                    track.entity_id = found->second;
                }
                auto& replacement = detail::resource_arena(resource_).make<TraceScopeToken>(
                    parent, track, token->node_index);
                cache.emplace(token, &replacement);
            } else {
                cache.emplace(token, token);
            }
        }
        return cache.at(scope);
    };
    for (auto& [key, scope] : ledger.trace_chains) {
        scope = rebind(scope);
    }
    for (auto& [key, scope] : ledger.unassigned_trace_scopes) {
        scope = rebind(scope);
    }
}

void EntityRuntimeState::set_original_version(const EntityKey& key, std::int64_t version) const {
    context().original_versions.insert(key, version);
}

LedgerCompositionResult
EntityRuntimeState::adopt_mutations_from_checked(const EntityRuntimeState& source) const {
    using R = LedgerCompositionResult;
    auto r = resource_;
    if (!source.inner_ || !source.inner_->value || inner_ == source.inner_) {
        return R::ok(r, {});
    }
    detail::EntityMutationLedger snapshot(std::allocator_arg, Allocator(r), *source.inner_->value);
    std::optional<std::pair<EntityKey, std::int64_t>> loaded_version;
    if (source.loaded_snapshot()) {
        auto* id = source.loaded_snapshot()->row.get("id");
        auto* v = source.loaded_snapshot()->row.get("version");
        if (id && v) {
            if (auto version = v->try_i64()) {
                loaded_version.emplace(
                    EntityKey(
                        std::allocator_arg, Allocator(r), source.loaded_snapshot()->entity, *id),
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
        if (!expected && loaded_snapshot() && loaded_snapshot()->entity == key.entity) {
            auto* id = loaded_snapshot()->row.get("id");
            auto* v = loaded_snapshot()->row.get("version");
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
        if (set.dynamic_) {
            for (const auto& [key, definitions] : set.dynamic_->definitions) {
                for (const auto& receiving : target.change_sets.stack_) {
                    const auto* previous = receiving.dynamic_definitions(key);
                    if (previous && *previous != definitions.get()) {
                        return R::err(
                            r,
                            LedgerCompositionError(
                                std::allocator_arg,
                                Allocator(r),
                                LedgerCompositionError::Kind::ConflictingDynamicDefinitions,
                                key.entity));
                    }
                }
            }
        }
    }
    for (const auto& set : snapshot.change_sets.stack_) {
        for (const auto& [key, values] : set.changes()) {
            for (const auto& [field, value] : values.fields) {
                target.change_sets.set(key, field, value);
            }
        }
        if (const auto* dynamic = set.dynamic_changes()) {
            for (const auto& [key, changes] : *dynamic) {
                const auto* definitions = set.dynamic_definitions(key);
                const auto* originals = set.dynamic_originals(key);
                for (const auto& [code, mutation] : changes) {
                    const Value* original = nullptr;
                    if (originals) {
                        auto found = originals->find(code);
                        if (found != originals->end() && found->second) {
                            original = &*found->second;
                        }
                    }
                    target.change_sets.current_mut().set_dynamic(
                        key, code, mutation, *definitions, original);
                }
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
        if (!target.trace_chains.contains(key)) {
            target.trace_chains.emplace(detail::clone_in(r, key), traces);
        }
    }
    for (const auto& [key, scope] : snapshot.unassigned_trace_scopes) {
        target.unassigned_trace_scopes.try_emplace(detail::clone_in(r, key), scope);
    }
    for (const auto& [key, original] : snapshot.original_snapshots) {
        if (!target.original_snapshots.contains(key)) {
            target.original_snapshots.emplace(
                detail::clone_in(r, key),
                std::visit(
                    [&](const auto& value) -> detail::OriginalSnapshot {
                        return detail::clone_in(r, value);
                    },
                    original));
        }
    }
    if (!target.original_snapshot) {
        target.original_snapshot = detail::clone_snapshot(r, snapshot.original_snapshot);
    }
    for (const auto& [key, comment] : snapshot.entity_comments) {
        target.entity_comments.insert_or_assign(detail::clone_in(r, key), String(comment, r));
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

void EntityRuntimeState::attach_runtime_state(const EntityRuntimeState& target) {
    if (resource_ != target.resource_) {
        throw std::invalid_argument(
            "Runtime attachment requires the same Context allocation domain");
    }
    target.adopt_mutations_from(*this);
    *this = target.with_loaded_view_from(*this);
}
}

namespace teaql {
namespace {
template <class Range, class Element>
void write_runtime_map(DebugWriter& writer, const Range& values, Element element) {
    writer.sequence(
        values,
        [&](auto& out, const auto& entry) {
            write_debug(out, entry.first);
            out.raw(": ");
            element(out, entry.second);
        },
        "{",
        "}");
}

template <class T>
void write_runtime_optional(DebugWriter& writer, const std::optional<T>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_debug(out, *value);
        });
    } else {
        writer.raw("None");
    }
}

void write_runtime_versions(DebugWriter& writer, const detail::OriginalVersions& versions) {
    writer.structure("OriginalVersions", [&](auto& fields) {
        fields.field("first", [&](auto& out) {
            if (versions.first) {
                out.tuple("Some", [&](auto& inner) {
                    inner.sequence(
                        std::array{0, 1},
                        [&](auto& item, int index) {
                            if (index == 0) {
                                write_debug(item, versions.first->first);
                            } else {
                                item.integer(versions.first->second);
                            }
                        },
                        "(",
                        ")");
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("overflow", [&](auto& out) {
            write_runtime_map(out, versions.overflow, [](auto& inner, auto value) {
                inner.integer(value);
            });
        });
    });
}

void write_runtime_ledger(DebugWriter& writer, const detail::EntityMutationLedger& value) {
    writer.structure("EntityMutationLedger", [&](auto& fields) {
        fields.field("change_sets", [&](auto& out) {
            write_debug(out, value.change_sets);
        });
        fields.field("comment", [&](auto& out) {
            write_runtime_optional(out, value.comment);
        });
        fields.field("entity_comments", [&](auto& out) {
            write_runtime_map(out, value.entity_comments, [](auto& inner, const auto& comment) {
                inner.text(comment);
            });
        });
        fields.field("deleted_keys", [&](auto& out) {
            out.sequence(
                value.deleted_keys,
                [](auto& inner, const auto& key) {
                    write_debug(inner, key);
                },
                "{",
                "}");
        });
        fields.field("new_keys", [&](auto& out) {
            out.sequence(
                value.new_keys,
                [](auto& inner, const auto& key) {
                    write_debug(inner, key);
                },
                "{",
                "}");
        });
        fields.field("original_snapshot", [&](auto& out) {
            if (value.original_snapshot) {
                out.tuple("Some", [&](auto& inner) {
                    std::visit(
                        [&](const auto& snapshot) {
                            using T = std::decay_t<decltype(snapshot)>;
                            inner.tuple(std::same_as<T, EntitySnapshot> ? "Materialized"
                                                                        : "Compact",
                                        [&](auto& item) {
                                            write_debug(item, snapshot);
                                        });
                        },
                        *value.original_snapshot);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("trace_chains", [&](auto& out) {
            write_runtime_map(out, value.trace_chains, [](auto& inner, const auto* token) {
                write_debug(inner, *token);
            });
        });
        fields.field("unassigned_trace_scopes", [&](auto& out) {
            write_runtime_map(
                out, value.unassigned_trace_scopes, [](auto& inner, const auto* token) {
                    write_debug(inner, *token);
                });
        });
        fields.field("original_snapshots", [&](auto& out) {
            write_runtime_map(out, value.original_snapshots, [](auto& inner, const auto& snapshot) {
                std::visit(
                    [&](const auto& item) {
                        using T = std::decay_t<decltype(item)>;
                        inner.tuple(std::same_as<T, EntitySnapshot> ? "Materialized" : "Compact",
                                    [&](auto& payload) {
                                        write_debug(payload, item);
                                    });
                    },
                    snapshot);
            });
        });
        fields.field("original_versions", [&](auto& out) {
            write_runtime_versions(out, value.original_versions);
        });
        fields.field("composition_errors", [&](auto& out) {
            out.sequence(value.composition_errors, [](auto& inner, const auto& error) {
                write_debug(inner, error);
            });
        });
        fields.field("is_new", [&](auto& out) {
            out.boolean(value.is_new);
        });
    });
}

// Source Mutex Debug ends with a non-exhaustive marker. A scoped scratch writer
// retains inherited options and lets the outer writer add its current indentation.
void write_runtime_mutex(DebugWriter& writer, const detail::EntityMutationLedger& value) {
    String text(writer.memory_resource());
    DebugWriter nested(text, writer.options());
    nested.structure("Mutex", [&](auto& fields) {
        fields.field("data", [&](auto& out) {
            write_runtime_ledger(out, value);
        });
        fields.field("poisoned", [&](auto& out) {
            out.boolean(false);
        });
    });
    if (writer.options().pretty) {
        text.insert(text.size() - 1, "    ..\n");
    } else {
        text.insert(text.size() - 2, ", ..");
    }
    writer.raw(text);
}

void write_runtime_frozen(DebugWriter& writer, const EntityGraphBuilder& value) {
    String text(writer.memory_resource());
    DebugWriter nested(text, writer.options());
    write_debug(nested, value);
    text.replace(0, std::string_view("EntityGraphBuilder").size(), "FrozenEntityGraph");
    writer.raw(text);
}
}

void write_debug(DebugWriter& writer, const EntityChangeSet& value) {
    writer.structure("EntityChangeSet", [&](auto& fields) {
        fields.field("changes", [&](auto& out) {
            write_runtime_map(out, value.changes_, [](auto& inner, const auto& changes) {
                write_debug(inner, changes);
            });
        });
        fields.field("dynamic_changes", [&](auto& out) {
            if (!value.dynamic_) {
                out.raw("None");
                return;
            }
            out.tuple("Some", [&](auto& inner) {
                write_runtime_map(
                    inner, value.dynamic_->changes, [](auto& payload, const auto& changes) {
                        write_runtime_map(
                            payload, changes, [](auto& mutation_writer, const auto& mutation) {
                                if (const auto* set = mutation.value()) {
                                    mutation_writer.tuple("Set", [&](auto& item) {
                                        write_debug(item, *set);
                                    });
                                } else {
                                    mutation_writer.raw("Delete");
                                }
                            });
                    });
            });
        });
        fields.field("dynamic_metadata", [&](auto& out) {
            if (!value.dynamic_) {
                out.raw("None");
                return;
            }
            out.tuple("Some", [&](auto& inner) {
                inner.structure("DynamicMutationMetadata", [&](auto& metadata) {
                    metadata.field("definitions", [&](auto& payload) {
                        write_runtime_map(payload,
                                          value.dynamic_->definitions,
                                          [](auto& item, const auto& binding) {
                                              write_debug(item, binding.get());
                                          });
                    });
                    metadata.field("originals", [&](auto& payload) {
                        write_runtime_map(payload,
                                          value.dynamic_->originals,
                                          [](auto& item, const auto& originals) {
                                              write_runtime_map(
                                                  item,
                                                  originals,
                                                  [](auto& original_writer, const auto& original) {
                                                      write_runtime_optional(original_writer,
                                                                             original);
                                                  });
                                          });
                    });
                });
            });
        });
    });
}

void write_debug(DebugWriter& writer, const EntityGraphBuilder& value) {
    writer.structure("EntityGraphBuilder", [&](auto& fields) {
        fields.field("entity_types", [&](auto& out) {
            out.integer(value.tables_.size());
        });
        fields.field("entities", [&](auto& out) {
            out.integer(value.entity_count());
        });
        fields.field("relation_lists", [&](auto& out) {
            out.integer(value.relation_list_count());
        });
    });
}

void write_debug(DebugWriter& writer, const ChangeSetStack& value) {
    writer.structure("ChangeSetStack", [&](auto& fields) {
        fields.field("stack", [&](auto& out) {
            out.sequence(value.stack_, [](auto& inner, const auto& set) {
                write_debug(inner, set);
            });
        });
    });
}

void write_debug(DebugWriter& writer, const EntityRuntimeState& value) {
    detail::resource_arena(value.resource_);
    writer.structure("EntityRuntimeState", [&](auto& fields) {
        fields.field("inner", [&](auto& out) {
            out.structure("LazyLedgerSlot", [&](auto& slot) {
                slot.field("shared", [&](auto& shared) {
                    shared.tuple("OnceLock", [&](auto& binding) {
                        if (!value.inner_) {
                            binding.raw("<uninit>");
                            return;
                        }
                        binding.tuple("OnceLock", [&](auto& ledger) {
                            if (value.inner_->value) {
                                write_runtime_mutex(ledger, *value.inner_->value);
                            } else {
                                ledger.raw("<uninit>");
                            }
                        });
                    });
                });
            });
        });
        fields.field("graph", [&](auto& out) {
            out.tuple(value.graph_.is_weak ? "Weak" : "Strong", [&](auto& inner) {
                if (value.graph_.is_weak) {
                    inner.raw("(Weak)");
                } else {
                    inner.tuple("OnceLock", [&](auto& item) {
                        if (value.graph_.slot->value) {
                            write_runtime_frozen(item, value.graph_.slot->value->contents);
                        } else {
                            item.raw("<uninit>");
                        }
                    });
                }
            });
        });
        // Ordinary C++ graphs isolate JSON result views in their graph domains;
        // the shared ordinary graph uses the source's zero coordinate.
        fields.field("json_view", [&](auto& out) {
            out.integer(value.json_view_);
        });
        fields.field("loaded_snapshot", [&](auto& out) {
            out.structure("LoadedSnapshotCarrier", [&](auto& carrier) {
                carrier.field("storage", [&](auto& storage) {
                    storage.structure(
                        "Mutex",
                        [&](auto& mutex) {
                            mutex.field("data", [&](auto& data) {
                                const auto* snapshot = value.loaded_snapshot();
                                if (!snapshot) {
                                    data.raw("Empty");
                                    return;
                                }
                                data.tuple(
                                    value.shared_snapshot_ ? "Shared" : "Inline", [&](auto& inner) {
                                        inner.structure("LoadedEntitySnapshot", [&](auto& loaded) {
                                            loaded.field("entity", [&](auto& item) {
                                                item.text(snapshot->entity);
                                            });
                                            loaded.field("row", [&](auto& item) {
                                                write_debug(item, snapshot->row);
                                            });
                                        });
                                    });
                            });
                            mutex.field("poisoned", [&](auto& data) {
                                data.boolean(false);
                            });
                        },
                        true);
                });
            });
        });
        fields.field("dynamic_fields", [&](auto& out) {
            if (const auto* dynamic = value.loaded_dynamic_fields()) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *dynamic);
                });
            } else {
                out.raw("None");
            }
        });
    });
}
}
