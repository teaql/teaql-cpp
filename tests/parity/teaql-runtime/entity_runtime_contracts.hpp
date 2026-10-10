#pragma once
// Exact composition/lazy-root source fixtures; no generator changes.
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/context.hpp>
#include <bit>
#include <charconv>

namespace teaql::test::source_entity_runtime {
inline void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

inline EntityKey key(Memory owner, std::string_view entity, std::uint64_t id) {
    return EntityKey(std::allocator_arg, Allocator(owner), entity, Value(owner, id));
}

struct TestEntity : EntityDefaults<TestEntity>, LedgerEntityDefaults<TestEntity> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "TestEntity";
    Memory owner;
    std::optional<EntityRuntimeState> state;

    TestEntity(std::allocator_arg_t, Allocator a, const EntityRuntimeState* state = nullptr)
        : owner(a.resource()) {
        if (state) {
            this->state.emplace(std::allocator_arg, a, *state);
        }
    }

    Memory memory_resource() const noexcept {
        return owner;
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        return EntityDescriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
    }

    static Result<TestEntity, EntityError> from_compact_row(Memory owner, CompactRow) {
        return Result<TestEntity, EntityError>::err(
            owner,
            EntityError(
                std::allocator_arg, Allocator(owner), ENTITY_NAME, "not used by this test"));
    }

    MutationValues into_values() && {
        return MutationValues(owner);
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        return detail::clone_in(owner, state);
    }
};

static_assert(LedgerEntity<TestEntity>);

inline LedgerCompositionResult compose(const EntityRuntimeState& target,
                                       const EntityRuntimeState& source) {
    auto owner = target.memory_resource();
    TestEntity receiver(std::allocator_arg, Allocator(owner), &target);
    TestEntity sender(std::allocator_arg, Allocator(owner), &source);
    return receiver.include_pending_mutations_from(sender);
}

inline void value_is(const EntityRuntimeState& state,
                     const EntityKey& key,
                     std::string_view field,
                     const Value& expected) {
    auto value = state.get(key, field);
    require(value && *value == expected, "original exact ledger Value and presence");
}

inline void
snapshot(EntityRuntimeState& state, std::string_view entity, Value id, std::int64_t version) {
    auto owner = state.memory_resource();
    Record fields(owner);
    fields.emplace(String("id", owner), std::move(id));
    fields.emplace(String("version", owner), Value(owner, version));
    CompactRow row(std::allocator_arg, Allocator(owner), fields);
    state.set_original_compact_row(entity, row);
}

inline void conflict_is(const LedgerCompositionResult& result,
                        std::string_view id,
                        std::int64_t target,
                        std::int64_t source) {
    require(!result &&
                result.error().kind == LedgerCompositionError::Kind::ConflictingOriginalVersion &&
                result.error().entity == "Child" && result.error().id == id &&
                result.error().target == target && result.error().source == source,
            "complete original composition conflict variant and payload");
}

inline void numeric_id_keys_match_across_sqlite_signed_and_entity_unsigned_values(Memory owner) {
    EntityKey signed_key(
        std::allocator_arg, Allocator(owner), "Child", Value(owner, std::int64_t(1)));
    auto unsigned_key = key(owner, "Child", 1);
    require(signed_key == unsigned_key &&
                (signed_key <=> unsigned_key) == std::strong_ordering::equal,
            "original positive signed/unsigned ID equality and ordering");
    require(signed_key != key(owner, "OtherChild", 1), "original distinct entity type");
    EntityKey negative(
        std::allocator_arg, Allocator(owner), "Child", Value(owner, std::int64_t(-1)));
    require(negative != key(owner, "Child", std::numeric_limits<std::uint64_t>::max()),
            "original negative ID cannot alias maximum unsigned ID");
}

inline void public_composition_reports_missing_ledgers_without_silent_success(Memory owner) {
    EntityRuntimeState state(std::allocator_arg, Allocator(owner));
    TestEntity present(std::allocator_arg, Allocator(owner), &state);
    TestEntity missing(std::allocator_arg, Allocator(owner));
    auto target = missing.include_pending_mutations_from(present);
    auto source = present.include_pending_mutations_from(missing);
    require(!target && target.error().kind == LedgerCompositionError::Kind::MissingTargetState &&
                !source && source.error().kind == LedgerCompositionError::Kind::MissingSourceState,
            "both original missing-ledger error variants");
}

inline void public_composition_copies_intent_without_sharing_source_ledger(Memory owner) {
    EntityRuntimeState target(std::allocator_arg, Allocator(owner)),
        source(std::allocator_arg, Allocator(owner));
    auto child = key(owner, "Child", 42);
    source.set(child, "name", Value(owner, "first"));
    require(compose(target, source).is_ok(), "original first public composition succeeds");
    value_is(target, child, "name", Value(owner, "first"));
    source.set(child, "name", Value(owner, "second"));
    value_is(target, child, "name", Value(owner, "first"));
    require(compose(target, source).is_ok(), "original second public composition succeeds");
    value_is(target, child, "name", Value(owner, "second"));
}

inline void
public_composition_retains_sqlite_signed_snapshot_version_for_unsigned_entity_id(Memory owner) {
    EntityRuntimeState target(std::allocator_arg, Allocator(owner)),
        source(std::allocator_arg, Allocator(owner));
    snapshot(source, "Child", Value(owner, std::int64_t(1)), 7);
    auto child = key(owner, "Child", 1);
    source.set(child, "quantity", Value(owner, std::int64_t(2)));
    require(compose(target, source).is_ok() && target.get_original_version(child) == 7,
            "original SQLite I64 snapshot version reaches U64 entity key");
}

inline void public_composition_rejects_conflicting_versions_without_copying_intent(Memory owner) {
    EntityRuntimeState target(std::allocator_arg, Allocator(owner)),
        source(std::allocator_arg, Allocator(owner));
    auto child = key(owner, "Child", 1);
    snapshot(target, "Child", Value(owner, std::int64_t(1)), 2);
    target.set(child, "quantity", Value(owner, std::int64_t(4)));
    snapshot(source, "Child", Value(owner, std::int64_t(1)), 1);
    source.set(child, "quantity", Value(owner, std::int64_t(3)));
    auto result = compose(target, source);
    conflict_is(result, "1", 2, 1);
    require(target.get_original_version(child) == 2 && source.get_original_version(child) == 1,
            "both original versions survive conflict");
    value_is(target, child, "quantity", Value(owner, std::int64_t(4)));
    value_is(source, child, "quantity", Value(owner, std::int64_t(3)));
}

inline void public_composition_accepts_equal_versions_and_distinct_entity_keys(Memory owner) {
    EntityRuntimeState target(std::allocator_arg, Allocator(owner)),
        source(std::allocator_arg, Allocator(owner));
    auto child = key(owner, "Child", 1), other = key(owner, "OtherChild", 1);
    target.set_original_version(child, 2);
    source.set_original_version(child, 2);
    source.set_original_version(other, 9);
    source.set(child, "quantity", Value(owner, std::int64_t(3)));
    source.set(other, "name", Value(owner, "other"));
    require(compose(target, source).is_ok() && target.get_original_version(child) == 2 &&
                target.get_original_version(other) == 9,
            "original equal and distinct versions");
    value_is(target, child, "quantity", Value(owner, std::int64_t(3)));
    value_is(target, other, "name", Value(owner, "other"));
}

inline void deterministic_randomized_composition_is_atomic_and_seed_replayable() {
    constexpr std::uint64_t seed = 0x434f4d504f534552;
    std::uint64_t random = seed;
    GlobalContext global;
    for (std::size_t trial = 0; trial < 512; ++trial) {
        // Source roots end each trial; an explicit User lifetime mirrors that boundary.
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        EntityRuntimeState target(std::allocator_arg, Allocator(owner)),
            source(std::allocator_arg, Allocator(owner));
        auto child = key(owner, "Child", trial % 17 + 1),
             other = key(owner, "OtherChild", trial % 17 + 1);
        const auto target_version = std::int64_t(trial % 11 + 1);
        const auto source_version = trial % 3 == 0 ? target_version + 1 : target_version;
        target.set_original_version(child, target_version);
        source.set_original_version(child, source_version);
        source.set_original_version(other, source_version + 20);
        for (std::uint64_t mutation = 0; mutation < 32; ++mutation) {
            random = random * 6364136223846793005ULL + 1442695040888963407ULL;
            const auto field = random % 3 == 0   ? "amount"
                               : random % 3 == 1 ? "status"
                                                 : "description";
            source.set(child, field, Value(owner, random >> 16));
            if (mutation % 5 == 0) {
                target.set(child, field, Value(owner, mutation));
            }
            if (mutation % 7 == 0) {
                source.set(other, field, Value(owner, std::rotl(random, 17)));
            }
        }
        auto target_before = target.current_change_set(),
             source_before = source.current_change_set();
        auto result = compose(target, source);
        if (source_version != target_version) {
            char buffer[32];
            auto end = std::to_chars(buffer, buffer + sizeof(buffer), trial % 17 + 1).ptr;
            conflict_is(
                result, std::string_view(buffer, end - buffer), target_version, source_version);
            require(target.current_change_set() == target_before &&
                        target.get_original_version(child) == target_version &&
                        !target.get_original_version(other),
                    "original failed trial is atomic");
        } else {
            require(result.is_ok(), "original compatible randomized trial succeeds");
            for (const auto& [source_key, values] : source_before.changes()) {
                for (const auto& [field, value] : values.fields) {
                    value_is(target, source_key, field, value);
                }
            }
            require(target.get_original_version(child) == target_version &&
                        target.get_original_version(other) == source_version + 20,
                    "original successful trial retains both versions");
        }
        require(source.current_change_set() == source_before,
                "every original randomized source remains retryable");
    }
    require(global.live_user_bytes() == 0, "all 512 trial Users return their arena blocks");
}

inline void generated_void_attachment_retains_version_conflict_for_save_preflight(Memory owner) {
    EntityRuntimeState target(std::allocator_arg, Allocator(owner)),
        source(std::allocator_arg, Allocator(owner));
    auto child = key(owner, "Child", 1);
    target.set_original_version(child, 2);
    source.set_original_version(child, 1);
    source.set(child, "quantity", Value(owner, std::int64_t(3)));
    target.adopt_mutations_from(source);
    auto error = detail::EntityRuntimeAccess::first_composition_error(target);
    require(error && error->kind == LedgerCompositionError::Kind::ConflictingOriginalVersion &&
                error->entity == "Child" && error->id == "1" && error->target == 2 &&
                error->source == 1 && target.get_original_version(child) == 2 &&
                !target.get(child, "quantity"),
            "original void attachment keeps its full conflict and rejects intent");
    value_is(source, child, "quantity", Value(owner, std::int64_t(3)));
}

inline void loaded_snapshot_does_not_allocate_ledger_until_mutation(Memory owner) {
    EntityRuntimeState root(std::allocator_arg, Allocator(owner));
    snapshot(root, "Example", Value(owner, std::uint64_t(7)), 3);
    auto example = key(owner, "Example", 7);
    require(!detail::EntityRuntimeAccess::has_mutation_context(root) &&
                !root.get(example, "name") && root.get_original_version(example) == 3 &&
                !detail::EntityRuntimeAccess::has_mutation_context(root),
            "all original loaded-snapshot reads preserve lazy ledger allocation");
    root.set(example, "name", Value(owner, "updated"));
    require(detail::EntityRuntimeAccess::has_mutation_context(root),
            "original first mutation allocates ledger");
}

inline void graph_composition_retains_version_for_same_id_across_entity_types(Memory owner) {
    EntityRuntimeState parent(std::allocator_arg, Allocator(owner)),
        execution(std::allocator_arg, Allocator(owner));
    snapshot(parent, "Order", Value(owner, std::uint64_t(1)), 1);
    snapshot(execution, "InferenceExecution", Value(owner, std::uint64_t(1)), 2);
    auto order_key = key(owner, "Order", 1), execution_key = key(owner, "InferenceExecution", 1);
    execution.set(execution_key, "execution_status", Value(owner, "COMPLETED"));
    require(!parent.get_original_version(execution_key),
            "original unrelated loaded type has no version");
    parent.adopt_mutations_from(execution);
    require(parent.get_original_version(order_key) == 1 &&
                parent.get_original_version(execution_key) == 2,
            "original graph composition preserves both same-ID distinct-type versions");
}

inline void clone_before_first_mutation_materializes_one_shared_ledger(Memory owner) {
    EntityRuntimeState root(std::allocator_arg, Allocator(owner));
    auto child = detail::clone_in(owner, root);
    auto child_key = key(owner, "Child", 2);
    child.set(child_key, "name", Value(owner, "updated"));
    value_is(root, child_key, "name", Value(owner, "updated"));
    require(detail::EntityRuntimeAccess::has_mutation_context(root),
            "original clone shares materialized ledger");
}

inline void explicit_graph_composition_adopts_existing_child_mutations(Memory owner) {
    EntityRuntimeState parent(std::allocator_arg, Allocator(owner)),
        child(std::allocator_arg, Allocator(owner));
    auto child_key = key(owner, "Child", 17);
    child.mark_as_new(child_key);
    child.set(child_key, "display_name", Value(owner, "before attach"));
    child.set_original_version(child_key, 3);
    parent.adopt_mutations_from(child);
    require(parent.is_new(child_key) && parent.get_original_version(child_key) == 3,
            "original graph attachment preserves new marker and version");
    value_is(parent, child_key, "display_name", Value(owner, "before attach"));
}

inline void relation_handles_distinguish_loaded_empty_and_not_loaded(Memory owner) {
    EntityRuntimeState root(std::allocator_arg, Allocator(owner));
    EntityGraphBuilder graph(std::allocator_arg, Allocator(owner));
    SmartList<std::uint64_t> loaded(std::allocator_arg, Allocator(owner));
    loaded.data.push_back(7);
    loaded.loaded(true);
    graph.install_relation_list("Owner", 1, "loaded", std::move(loaded));
    graph.install_relation_list(
        "Owner", 1, "empty", SmartList<std::uint64_t>(std::allocator_arg, Allocator(owner)));
    graph.install_relation_option<std::uint64_t>("Owner", 1, "present", std::uint64_t(9));
    graph.install_relation_option<std::uint64_t>("Owner", 1, "null", std::nullopt);
    require(root.freeze_graph(std::move(graph)).is_ok(), "original relation fixture freezes");
    auto present_list = root.relation_list<std::uint64_t>("Owner", 1, "loaded");
    require(present_list.state() == LoadedRelation::Loaded && present_list.value() &&
                present_list.value()->data.size() == 1 && present_list.value()->data[0] == 7,
            "original complete loaded list value");
    auto empty = root.relation_list<std::uint64_t>("Owner", 1, "empty");
    require(empty.state() == LoadedRelation::Empty && empty.value() && empty.value()->data.empty(),
            "original present empty list");
    auto missing = root.relation_list<std::uint64_t>("Owner", 1, "missing");
    require(missing.state() == LoadedRelation::NotLoaded && !missing.value(),
            "original missing list");
    auto present = root.relation_option<std::uint64_t>("Owner", 1, "present");
    require(present.state() == LoadedRelation::Loaded && present.value() && *present.value() == 9,
            "original present to-one value");
    auto null = root.relation_option<std::uint64_t>("Owner", 1, "null");
    auto absent = root.relation_option<std::uint64_t>("Owner", 1, "absent");
    require(null.state() == LoadedRelation::Empty && !null.value() &&
                absent.state() == LoadedRelation::NotLoaded && !absent.value(),
            "original loaded-null and unloaded to-one states remain distinct");
}

inline void run_original_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    numeric_id_keys_match_across_sqlite_signed_and_entity_unsigned_values(owner);
    public_composition_reports_missing_ledgers_without_silent_success(owner);
    public_composition_copies_intent_without_sharing_source_ledger(owner);
    public_composition_retains_sqlite_signed_snapshot_version_for_unsigned_entity_id(owner);
    public_composition_rejects_conflicting_versions_without_copying_intent(owner);
    public_composition_accepts_equal_versions_and_distinct_entity_keys(owner);
    deterministic_randomized_composition_is_atomic_and_seed_replayable();
    generated_void_attachment_retains_version_conflict_for_save_preflight(owner);
    loaded_snapshot_does_not_allocate_ledger_until_mutation(owner);
    graph_composition_retains_version_for_same_id_across_entity_types(owner);
    clone_before_first_mutation_materializes_one_shared_ledger(owner);
    explicit_graph_composition_adopts_existing_child_mutations(owner);
    relation_handles_distinguish_loaded_empty_and_not_loaded(owner);
}
}
