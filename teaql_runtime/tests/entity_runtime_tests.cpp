#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/context.hpp>
#include "../../tests/parity/teaql_runtime/entity_runtime_contracts.hpp"
#include <iostream>
using namespace teaql;
static_assert(std::is_copy_constructible_v<RelationHandle<int>>);
static_assert(!std::is_constructible_v<RelationHandle<int>, LoadedRelation, const int*>);

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static EntityKey key(Memory r, std::string_view name = "Child", std::uint64_t id = 7) {
    return EntityKey(std::allocator_arg, Allocator(r), name, Value(r, id));
}

static void immutable_original_row_sharing() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& root = user_context->make<EntityRuntimeState>();
    Record values(r);
    values.emplace(String("id", r), Value(r, std::uint64_t{7}));
    values.emplace(String("version", r), Value(r, std::int64_t{3}));
    values.emplace(String("name", r), Value(r, String(4096, 'x', r)));
    CompactRow row(std::allocator_arg, Allocator(r), std::move(values));
    auto* value_buffer = row.values().data();
    root.set_original_compact_row("Child", std::move(row));
    check(detail::EntityRuntimeAccess::original_row(root)->values().data() == value_buffer,
          "hydration transfers the original value buffer without copying it");
    EntityRuntimeState alias(std::allocator_arg, Allocator(r), root);
    auto* original = detail::EntityRuntimeAccess::original_row(root);
    check(original == detail::EntityRuntimeAccess::original_row(alias) &&
              original->values().data() == value_buffer,
          "first clone promotes one immutable row and preserves its value buffer");
    auto issued = user_context->allocation_stats().issued_objects;
    EntityRuntimeState repeated(std::allocator_arg, Allocator(r), alias);
    check(detail::EntityRuntimeAccess::original_row(repeated) == original &&
              user_context->allocation_stats().issued_objects == issued,
          "later clones reuse the original row without new managed objects");
    {
        auto copy = alias.original_snapshot();
        copy->fields.insert_or_assign(String("name", r), Value(r, "changed outside"));
        check(root.original_snapshot()->fields.at(String("name", r)).try_text()->size() == 4096,
              "returned original values are independent materializations");
    }
    root.set(key(r), "name", Value(r, "dirty"));
    check(alias.get(key(r), "name")->try_text() == "dirty" &&
              alias.original_snapshot()->fields.at(String("name", r)).try_text()->size() == 4096,
          "shared mutable ledger remains separate from immutable original row");
    Record replacement(r);
    replacement.emplace(String("id", r), Value(r, std::uint64_t{8}));
    CompactRow next(std::allocator_arg, Allocator(r), std::move(replacement));
    root.set_original_compact_row("Child", std::move(next));
    check(root.original_snapshot()->fields.size() == 1 && alias.get_original_version(key(r)) == 3 &&
              detail::EntityRuntimeAccess::original_row(alias) == original,
          "replacing one view's snapshot does not change its cloned views");
    auto& retained = global.make<EntityRuntimeState>(alias);
    check(detail::EntityRuntimeAccess::original_row(retained) != original,
          "cross-domain retention deeply owns an independent original row");
    check(retained.get_original_version(key(global.memory_resource())) == 3,
          "destination metadata and original version stay intact");
}

static void field_access_and_evaluation() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& root = user_context->make<EntityRuntimeState>();
    auto entity = key(r);
    root.set(entity, "name", Value(r, "overlaid"));
    check(detail::EntityRuntimeAccess::current_field(root, entity, "name", String("stored", r)) ==
              "overlaid",
          "required getters read the shared mutation ledger before stored values");
    root.set(entity, "name", Value(r, false));
    check(detail::EntityRuntimeAccess::current_field(root, entity, "name", String("stored", r)) ==
              "stored",
          "incompatible mutation conversion falls back to stored field");
    auto unloaded = eval::LoadState::not_loaded(r);
    bool invoked = false;
    auto missing = detail::EntityRuntimeAccess::eval_field<int>(r, unloaded, "quantity", [&] {
        invoked = true;
        return 0;
    });
    check(missing.is_not_loaded() && !invoked && missing.missing().failed_node == "quantity" &&
              missing.missing().attempted_path == "quantity",
          "evaluation reports exact missing path without invoking the getter");
    auto loaded = eval::LoadState::fully_loaded(r);
    auto zero = detail::EntityRuntimeAccess::eval_field<int>(r, loaded, "quantity", [] {
        return 0;
    });
    auto false_value = detail::EntityRuntimeAccess::eval_field<bool>(r, loaded, "active", [] {
        return false;
    });
    auto null =
        detail::EntityRuntimeAccess::eval_field<std::optional<int>>(r, loaded, "quantity", [] {
            return std::optional<int>{};
        });
    check(zero.is_value() && zero.get() == 0 && false_value.is_value() && !false_value.get() &&
              null.is_null(),
          "zero, false and optional NULL preserve distinct loaded results");
}

static void immutable_trace_scopes() {
    GlobalContext global;
    auto& retained = [&]() -> EntityRuntimeState& {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& state = user_context->make<EntityRuntimeState>();
        EntityKey root_key(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t{0}));
        EntityKey child_key(
            std::allocator_arg, Allocator(r), "Payment", Value(r, std::uint64_t{301}));
        EntityKey inherited_key(
            std::allocator_arg, Allocator(r), "OrderItem", Value(r, std::uint64_t{201}));
        TraceNode root_trace(
            std::allocator_arg, Allocator(r), TraceKind::AuditReason, "Order", "submit");
        auto& root = user_context->make<TraceScopeToken>(nullptr, root_trace, 0);
        TraceNode child_trace(
            std::allocator_arg, Allocator(r), TraceKind::AuditReason, "Payment", "authorize", 301);
        auto& child = user_context->make<TraceScopeToken>(&root, child_trace, 1);
        detail::EntityRuntimeAccess::TraceScopes scopes(r), unassigned(r);
        scopes.emplace(detail::clone_in(r, root_key), &root);
        scopes.emplace(detail::clone_in(r, child_key), &child);
        scopes.emplace(detail::clone_in(r, inherited_key), &root);
        unassigned.emplace(detail::clone_in(r, root_key), &root);
        detail::EntityRuntimeAccess::replace_trace_scopes(state, scopes, unassigned);
        std::pmr::map<EntityKey, Value> allocated(r);
        allocated.emplace(detail::clone_in(r, root_key), Value(r, std::uint64_t{100}));
        detail::EntityRuntimeAccess::enrich_allocated_trace_scopes(state, allocated);
        auto* rebound = detail::EntityRuntimeAccess::trace_scope_for(state, root_key);
        check(rebound->track.entity_id == 100 && root.track.entity_id == std::nullopt &&
                  rebound == detail::EntityRuntimeAccess::trace_scope_for(state, inherited_key) &&
                  rebound ==
                      detail::EntityRuntimeAccess::trace_scope_for(state, child_key)->parent() &&
                  rebound == detail::EntityRuntimeAccess::trace_scope_for(state, root_key, true),
              "allocated IDs rebind shared ancestors once without mutating the original tokens");
        auto issued = user_context->allocation_stats().issued_objects;
        detail::EntityRuntimeAccess::enrich_allocated_trace_scopes(state, allocated);
        check(user_context->allocation_stats().issued_objects == issued,
              "repeat enrichment preserves unchanged token identities");
        return global.make<EntityRuntimeState>(state);
    }();
    auto r = global.memory_resource();
    EntityKey root_key(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t{0}));
    EntityKey child_key(std::allocator_arg, Allocator(r), "Payment", Value(r, std::uint64_t{301}));
    auto* root = detail::EntityRuntimeAccess::trace_scope_for(retained, root_key);
    auto* child = detail::EntityRuntimeAccess::trace_scope_for(retained, child_key);
    check(root && child && child->parent() == root && root->memory_resource() == r &&
              retained.get_trace_chain(child_key).front().entity_id == 100,
          "explicit Global ledger copies preserve shared ancestry after actual User deletion");
    retained.clear_committed();
    check(!detail::EntityRuntimeAccess::trace_scope_for(retained, root_key) &&
              !detail::EntityRuntimeAccess::trace_scope_for(retained, root_key, true),
          "commit clears both assigned and unassigned scope tables");
}

static void local_comments_and_root_reason() {
    GlobalContext global;
    auto& retained = [&]() -> EntityRuntimeState& {
        auto source_user_context = global.make_user_context();
        auto r = source_user_context->memory_resource();
        auto& root = source_user_context->make<EntityRuntimeState>();
        auto& source = source_user_context->make<EntityRuntimeState>();
        root.set_comment("receiving reason");
        root.set_entity_comment(key(r), "old local comment");
        source.set_comment("source reason must not propagate");
        source.set_entity_comment(key(r), std::string(4096, 'x'));
        source.set_entity_comment(key(r, "Other"), "independent entity annotation");
        root.adopt_mutations_from(source);
        check(root.get_comment() == "receiving reason" &&
                  root.get_entity_comment(key(r))->size() == 4096,
              "composition merges entity annotations without replacing receiving root reason");
        return global.make<EntityRuntimeState>(root);
    }();
    auto r = global.memory_resource();
    check(retained.get_entity_comment(key(r))->size() == 4096 &&
              retained.get_entity_comment(key(r, "Other")) == "independent entity annotation",
          "entity annotation keys and long strings survive actual source User deletion");
    retained.clear_committed();
    check(!retained.get_comment() && !retained.get_entity_comment(key(r)),
          "commit clears both distinct annotation responsibilities");
}

static void keyed_originals_and_loaded_views() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto make_row = [&](std::string_view name) {
        Record values(r);
        values.emplace(String("id", r), Value(r, std::uint64_t{7}));
        values.emplace(String("name", r), Value(r, name));
        return CompactRow(std::allocator_arg, Allocator(r), std::move(values));
    };
    auto& receiving = user_context->make<EntityRuntimeState>();
    auto& source = user_context->make<EntityRuntimeState>();
    receiving.set_original_compact_row("Child", make_row("receiving"));
    auto fallback = detail::EntityRuntimeAccess::original_values_for(receiving, key(r));
    check(fallback->fields.at(String("name", r)).try_text() == "receiving" &&
              !detail::EntityRuntimeAccess::has_mutation_context(receiving),
          "keyed original reads use the loaded view without creating mutation intent");
    check(!detail::EntityRuntimeAccess::original_values_for(receiving, key(r, "Other")),
          "same ID in another entity type cannot read this original snapshot");
    receiving.set(key(r), "name", Value(r, "changed"));
    source.set_original_compact_row("Child", make_row("source"));
    source.set(key(r), "value", Value(r, true));
    receiving.set_original_compact_row("Child", make_row("new receiving view"));
    receiving.adopt_mutations_from(source);
    auto original = detail::EntityRuntimeAccess::original_values_for(receiving, key(r));
    check(original->fields.at(String("name", r)).try_text() == "receiving",
          "composition retains the receiving ledger's earlier keyed original");
    auto view = receiving.with_loaded_view_from(source);
    check(view.original_snapshot()->fields.at(String("name", r)).try_text() == "source" &&
              detail::EntityRuntimeAccess::original_values_for(view, key(r))
                      ->fields.at(String("name", r))
                      .try_text() == "receiving",
          "loaded view rebinding preserves the receiving ledger and replaces only view data");
    auto& other = user_context->make<EntityRuntimeState>();
    other.set_original_compact_row("Other", make_row("other entity"));
    other.set(key(r, "Other"), "value", Value(r, false));
    receiving.adopt_mutations_from(other);
    check(detail::EntityRuntimeAccess::original_values_for(receiving, key(r, "Other"))
                  ->fields.at(String("name", r))
                  .try_text() == "other entity",
          "composition preserves distinct typed keys with the same numeric ID");
    auto& retained = global.make<EntityRuntimeState>(receiving);
    receiving.clear_committed();
    check(!detail::EntityRuntimeAccess::original_values_for(receiving, key(r, "Other")) &&
              detail::EntityRuntimeAccess::original_values_for(receiving, key(r))
                      ->fields.at(String("name", r))
                      .try_text() == "new receiving view",
          "commit clears keyed originals while preserving the per-view loaded fallback");
    auto retained_original = detail::EntityRuntimeAccess::original_values_for(
        retained, key(global.memory_resource(), "Other"));
    check(retained_original->fields.at(String("name", global.memory_resource())).try_text() ==
              "other entity",
          "destination copies preserve their independently owned keyed dictionary");
    auto& surviving = [&]() -> EntityRuntimeState& {
        auto source_user_context = global.make_user_context();
        auto source_r = source_user_context->memory_resource();
        auto& source_state = source_user_context->make<EntityRuntimeState>();
        Record fields(source_r);
        fields.emplace(String("id", source_r), Value(source_r, std::uint64_t{7}));
        fields.emplace(String("name", source_r), Value(source_r, String(4096, 'x', source_r)));
        source_state.set_original_compact_row(
            "Child", CompactRow(std::allocator_arg, Allocator(source_r), std::move(fields)));
        source_state.set(key(source_r), "value", Value(source_r, true));
        return global.make<EntityRuntimeState>(source_state);
    }();
    auto surviving_original =
        detail::EntityRuntimeAccess::original_values_for(surviving, key(global.memory_resource()));
    check(surviving_original->fields.at(String("name", global.memory_resource()))
                  .try_text()
                  ->size() == 4096,
          "keyed originals survive actual source User deletion after explicit Global copying");
}

struct Node {
    using allocator_type = Allocator;
    EntityRuntimeState state;
    String name;

    Node(std::allocator_arg_t, Allocator a, const EntityRuntimeState& state, std::string_view name)
        : state(std::allocator_arg, a, state), name(name, a.resource()) {}

    Node(std::allocator_arg_t, Allocator a, const Node& other)
        : Node(std::allocator_arg, a, other.state, other.name) {}

    Node(const Node&) = delete;
    Node(Node&&) noexcept = default;

    Node& operator=(Node&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }
};

static void ledger_lifetimes() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& root = user->make<EntityRuntimeState>();
    auto& alias = user->make<EntityRuntimeState>(root);
    check(root == alias, "copies share the lazy ledger before mutation");
    {
        Record values(r);
        values.emplace(String("id", r), Value(r, std::int64_t(7)));
        values.emplace(String("version", r), Value(r, std::int64_t(4)));
        CompactRow row(std::allocator_arg, Allocator(r), values);
        root.set_original_compact_row("Child", row);
        check(root.get_original_version(key(r)) == 4 &&
                  !detail::EntityRuntimeAccess::has_mutation_context(root),
              "loaded snapshot reads do not allocate ledger");
    }
    check(!alias.original_snapshot(), "loaded snapshots are per view");
    alias.set(key(r), "payload", Value(r, String(100, 'x', r)));
    check(root.get(key(r), "payload")->try_text()->size() == 100, "lazy shared write visible");
    root.set_comment("persisted intent");
    root.mark_as_new(key(r));
    root.push_change_set();
    root.set(key(r), "upper", Value(r, true));
    auto& retained = global.make<EntityRuntimeState>(root);
    check(!(retained == root), "cross-domain ledger copied, not borrowed");
    root.mark_as_delete(key(r));
    check(root.changed_field_names(key(r)).empty() && root.is_new(key(r)),
          "delete clears all stack levels but retains new marker");
    user->close();
    check(user->allocation_stats().live_blocks == 0, "all User blocks reclaimed");
    auto gr = global.memory_resource();
    check(retained.get(key(gr), "payload")->try_text()->size() == 100 &&
              retained.get(key(gr), "upper")->try_bool() == true,
          "deep-copy values survive source close");
    check(retained.get_original_version(key(gr)) == 4, "loaded row copied with ledger");
    retained.clear_committed();
    check(retained.current_change_set().is_empty() && !retained.get_comment() &&
              retained.get_original_version(key(gr)) == 4,
          "commit clears intent but preserves loaded row");
}

static void composition() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& target = user->make<EntityRuntimeState>();
    auto& source = user->make<EntityRuntimeState>();
    auto k = key(r);
    target.set_original_version(k, 2);
    source.set(k, "text", Value(r, "keep source"));
    source.set_original_version(k, 3);
    auto error = detail::EntityRuntimeAccess::compose(target, source);
    check(!error && target.current_change_set().is_empty() && source.get(k, "text"),
          "conflicting compose is atomic and source remains intact");
    target.adopt_mutations_from(source);
    check(detail::EntityRuntimeAccess::first_composition_error(target).has_value(),
          "void attachment retains conflict");
    target.clear_committed();
    check(detail::EntityRuntimeAccess::first_composition_error(target).has_value(),
          "source clear_committed retains composition error");
    check(bool(detail::EntityRuntimeAccess::compose(target, source)),
          "cleared original version can accept source intent");
    source.set(k, "text", Value(r, "changed later"));
    check(target.get(k, "text")->try_text() == "keep source",
          "composition copies without aliasing source ledger");
    auto& empty = user->make<EntityRuntimeState>();
    auto& fresh = user->make<EntityRuntimeState>();
    empty.set_original_compact_row("Child", CompactRow(std::allocator_arg, Allocator(r)));
    fresh.adopt_mutations_from(empty);
    check(!detail::EntityRuntimeAccess::has_mutation_context(fresh),
          "pristine source does not materialize target ledger");
}

static void graph_and_copy() {
    GlobalContext global;
    auto source = global.make_user_context();
    auto r = source->memory_resource();
    auto& root = source->make<EntityRuntimeState>();
    {
        EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
        builder.install(1, Node(std::allocator_arg, Allocator(r), root, "first"));
        builder.install(2, Node(std::allocator_arg, Allocator(r), root, "second"));
        SmartList<Node> list(std::allocator_arg, Allocator(r));
        list.emplace_back(root, "list child");
        list.with_summary("long", String(100, 's', r));
        builder.install_relation_list("PurchaseOrder", 1, "children", std::move(list));
        builder.install_relation_option<Node>("PurchaseOrder", 1, "null", std::nullopt);
        check(builder.entity_count() == 2 && builder.relation_list_count() == 2,
              "graph contains typed tables and independent relation views");
        check(bool(root.freeze_graph(std::move(builder))), "freeze publishes graph once");
    }
    auto* first = root.resolve_entity<Node>(1);
    auto* second = root.resolve_entity<Node>(2);
    check(first && second && first->state == root && second->state == root,
          "graph entities share root ledger");
    auto view = root.relation_list<Node>("purchase_order", 1, "children");
    auto missing_view = root.relation_list<Node>("PurchaseOrder", 1, "missing");
    check(view.state() == LoadedRelation::Loaded && view.value()->data[0].name == "list child",
          "relation owner canonicalized");
    check(root.relation_option<Node>("PurchaseOrder", 1, "null").is_empty(),
          "loaded null remains distinct from missing");
    check(!root.relation_list<Node>("PurchaseOrder", 1, "missing").is_loaded(),
          "no implicit relation load");
    bool mismatch = false;
    try {
        root.relation_list<std::uint64_t>("PurchaseOrder", 1, "children");
    } catch (const std::logic_error&) {
        mismatch = true;
    }
    check(mismatch && !root.resolve_relation_list<std::uint64_t>("PurchaseOrder", 1, "children"),
          "strict handle mismatch panics while optional resolver returns None");
    auto& copy = global.make<EntityRuntimeState>(root);
    source->close();
    source.reset();
    bool expired = false;
    try {
        (void)view.value();
    } catch (const ReferenceError& error) {
        expired = error.kind() == ReferenceError::Kind::Closed;
    }
    check(expired, "relation handle rejects actual User deletion before reading graph storage");
    expired = false;
    try {
        (void)missing_view.state();
    } catch (const ReferenceError& error) {
        expired = error.kind() == ReferenceError::Kind::Closed;
    }
    check(expired, "missing relation handles also retain a checked Context lifetime");
    auto gr = global.memory_resource();
    auto* a = copy.resolve_entity<Node>(1);
    auto* b = copy.resolve_entity<Node>(2);
    check(a && b && a->state == copy && b->state == copy,
          "cross-domain cyclic graph preserves ledger aliases");
    check(a->state.resolve_entity<Node>(2) == b,
          "cross-domain back-reference resolves copied graph");
    a->state.set(key(gr), "name", Value(gr, "shared copied ledger"));
    check(copy.get(key(gr), "name")->try_text() == "shared copied ledger",
          "copied graph references same copied ledger");
    check(copy.relation_list<Node>("PurchaseOrder", 1, "children")
                  .value()
                  ->summary.at(String("long", gr))
                  .memory_resource() == gr,
          "nested relation summary propagates allocator");
    {
        EntityGraphBuilder rejected(std::allocator_arg, Allocator(gr));
        rejected.install(3, std::uint64_t(42));
        auto result = copy.freeze_graph(std::move(rejected));
        check(!result && result.error().entity_count() == 1 &&
                  !copy.resolve_entity<std::uint64_t>(3),
              "second freeze returns intact builder");
    }
}

struct Resource {
    using allocator_type = Allocator;
    Memory r;
    int* destroyed;
    bool active = true;

    Resource(std::allocator_arg_t, Allocator a, int& count) : r(a.resource()), destroyed(&count) {}

    Resource(std::allocator_arg_t, Allocator a, Resource&& other)
        : r(a.resource()), destroyed(other.destroyed), active(std::exchange(other.active, false)) {
        if (r != other.r) {
            throw std::invalid_argument("resource cannot migrate");
        }
    }

    Resource(Resource&& other) noexcept
        : r(other.r), destroyed(other.destroyed), active(std::exchange(other.active, false)) {}

    Resource(const Resource&) = delete;

    ~Resource() {
        if (active) {
            ++*destroyed;
        }
    }
};

struct LedgerModel : EntityDefaults<LedgerModel>, LedgerEntityDefaults<LedgerModel> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "LedgerModel";
    Memory r;
    const EntityRuntimeState* state;

    LedgerModel(std::allocator_arg_t, Allocator a, const EntityRuntimeState* state = nullptr)
        : r(a.resource()), state(state) {}

    LedgerModel(std::allocator_arg_t, Allocator a, const LedgerModel& other)
        : r(a.resource()), state(other.state) {
        if (state && r != other.r) {
            state = &detail::resource_arena(r).make<EntityRuntimeState>(*state);
        }
    }

    Memory memory_resource() const {
        return r;
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<LedgerModel, EntityError> from_compact_row(Memory r, CompactRow) {
        return Result<LedgerModel, EntityError>::ok(r,
                                                    LedgerModel(std::allocator_arg, Allocator(r)));
    }

    MutationValues into_values() && {
        return MutationValues(r);
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        if (!state) {
            return {};
        }
        return detail::clone_in(r, *state);
    }
};

static_assert(LedgerEntity<LedgerModel>);

static void public_composition() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& target = user->make<EntityRuntimeState>();
    auto& source = user->make<EntityRuntimeState>();
    auto& receiver = user->make<LedgerModel>(&target);
    auto& sender = user->make<LedgerModel>(&source);
    auto& missing = user->make<LedgerModel>();
    auto first = missing.include_pending_mutations_from(missing);
    check(!first && first.error().kind == LedgerCompositionError::Kind::MissingTargetState,
          "public composition checks target first");
    auto second = receiver.include_pending_mutations_from(missing);
    check(!second && second.error().to_string(r) == "source entity has no mutation ledger",
          "public composition reports missing source");
    source.set(key(r), "name", Value(r, "typed composition"));
    check(bool(receiver.include_pending_mutations_from(sender)),
          "public LedgerEntity default composes states");
    check(target.get(key(r), "name")->try_text() == "typed composition",
          "typed default preserves mutation intent");
}

static void eager_payload_cleanup() {
    int count = 0;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& root = user->make<EntityRuntimeState>();
    {
        EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
        builder.install(1, Resource(std::allocator_arg, Allocator(r), count));
        builder.install(1, Resource(std::allocator_arg, Allocator(r), count));
        check(count == 1, "replacing graph entity destroys native payload once");
        SmartList<Resource> native_list(std::allocator_arg, Allocator(r));
        native_list.emplace_back(count);
        builder.install_relation_list("Owner", 1, "resources", std::move(native_list));
        check(bool(root.freeze_graph(std::move(builder))), "freeze accepts move-only resources");
    }
    bool denied = false;
    try {
        global.make<EntityRuntimeState>(root);
    } catch (const std::invalid_argument&) {
        denied = true;
    }
    check(denied, "cross-domain copy never silently retains non-copyable native payload");
    check(detail::resource_arena(global.memory_resource()).stats().live_blocks > 0,
          "Global remains usable after failed copy");
    check(global.make<String>("still alive") == "still alive",
          "copy scope restored after exception");
    user->close();
    check(count == 3 && user->allocation_stats().live_blocks == 0,
          "native graph payload cleaned before User arena reclamation");
}

static void runtime_attachment_preserves_views() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& target = user_context->make<EntityRuntimeState>();
    auto& source = user_context->make<EntityRuntimeState>();
    Record fields(r);
    fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
    fields.emplace(String("version", r), Value(r, std::int64_t{3}));
    fields.emplace(String("name", r), Value(r, "original"));
    source.set_original_compact_row("Child", CompactRow(std::allocator_arg, Allocator(r), fields));
    source.set(key(r), "name", Value(r, "before attachment"));
    source.attach_runtime_state(target);
    check(source.original_snapshot()->fields.at(String("name", r)).try_text() == "original" &&
              source.get_original_version(key(r)) == 3,
          "attachment retains the entity loaded snapshot and optimistic version");
    check(target.get(key(r), "name")->try_text() == "before attachment",
          "attachment adopts preexisting mutations");
    source.set(key(r), "name", Value(r, "after attachment"));
    check(target.get(key(r), "name")->try_text() == "after attachment",
          "later updates use the receiving ledger");
    auto next_user_context = global.make_user_context();
    auto& other = next_user_context->make<EntityRuntimeState>();
    bool rejected = false;
    try {
        source.attach_runtime_state(other);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && !detail::EntityRuntimeAccess::has_mutation_context(other),
          "cross-domain attachment rejects before modifying the target");
    auto& retained = global.make<EntityRuntimeState>(source);
    check(retained.get(key(global.memory_resource()), "name")->try_text() == "after attachment",
          "explicit destination copying retains the attached ledger");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        ledger_lifetimes();
        immutable_original_row_sharing();
        field_access_and_evaluation();
        keyed_originals_and_loaded_views();
        local_comments_and_root_reason();
        immutable_trace_scopes();
        composition();
        public_composition();
        runtime_attachment_preserves_views();
        graph_and_copy();
        eager_payload_cleanup();
        teaql::test::source_entity_runtime::run_original_contracts();
        std::pmr::set_default_resource(previous);
        std::cout << "Entity identity graph and ledger contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
