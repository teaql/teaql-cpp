# Relation planning replay against complete original methods

The 112 complete historical entity-service planning stimuli now execute the
original locked 5.0.6 EntityDataService methods through append-only access
bridges. The prior field-wrapper/extracted-method planning fixture is replaced.
Public cross-checks and all historical inputs/observations remain unchanged.
The unused original executor panics on any planning I/O.

The [source provenance](parity/relation_planning_5_0_6_source_provenance.json)
records the completed 21-observer build, intact prefixes of 610 original archive
files, appended access bridges and original registry versions/checksums. The
[driver review](parity/relation_planning_5_0_6_driver_review.json) retains observer
hashes and exact adaptation diffs. Historical files are not relabeled.

## Initial differences and correction

The initial complete comparison found **73/112** differences; see the preserved
[initial outputs](parity/relation_planning_5_0_6_initial_gaps.json).

Most differences were observer labels: the Rust fixture uses fixed expression
tags while C++ directly emitted enum ordinals. An added expression variant had
shifted the C++ ordinals. The observer now emits the existing fixture's explicit
Column/Value/Binary/And/Or labels and rejects unsupported fixture expressions.
This does not change production enum values or expression semantics.

The shared runtime relation key collector also retained ordinary and typed NULL
values. The original runtime filters both before sorting/deduplication. C++ now
uses the existing Value null predicate before collecting keys. All **112/112**
complete actual original/native outputs match, including **17** planning errors.

## Execution and ownership regression

The existing real SQLite top-N regression now includes missing, ordinary NULL
and typed NULL parent keys alongside valid duplicate identities. It preserves
per-parent offset/limit and ID tie-breaking, capture-dependent probe batching,
caller snapshot isolation and explicit Global retention.

The mock execution regression separately requires a missing/NULL-only parent
set to return an empty result without provider calls or capability lookups.
Existing pending, cancellation, foreign-domain and Context-close behavior is
retained under a failing default PMR resource.

The first added fixture used an initializer list that implicitly copied a
move-only Value; explicit scoped prvalues fix its two compiler errors. Those
[initial build errors](parity/relation_planning_5_0_6_initial_build_errors.json)
remain recorded. Initial native checks passed 8/8 and module checks 1/1, while
ASan exposed the added local null-parent vector surviving deliberate Context
reset. That vector now belongs to the Context, so destruction precedes arena
reclamation. The [first verification](parity/relation_planning_5_0_6_initial_verification.json)
preserves the failure instead of treating the initial sanitizer run as green.

## Reproduction and open contracts

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_entity_service_probe
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family entity_service \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
```

After terminal builds, the changed native contract passes **1/1**, targeted
ASan/UBSan **1/1** (leak scan disabled), canonical module **1/1** and **107**
consumer/fixture bodies verify. The preceding broader native selection passed
**8/8**; the final rerun focuses on the changed ownership fixture. Pinned
format/check covers **993** handwritten files. See the
[terminal record](parity/relation_planning_replay_5_0_6.json).
One active comparator migrates to the separately versioned planning corpus; the
[binding inventory](parity/active_corpus_baseline_after_relation_planning_5_0_6.json)
now counts 132 active, 66 historical and 21 custom selections. Static binding
counts do not prove semantic acceptance.
The source audit also finds adjacent unqualified contracts: copied facets must
be cleared before child planning, and grouped/aggregate top-N must use group
fields rather than an ungrouped entity ID for stable ordering. They are not
covered by the 112 historical inputs. See the explicit
[follow-up record](parity/relation_planning_5_0_6_follow_up.json).
Full relation execution/SQL/privacy/generated/model configurations and P7 remain
open; all 17 full capabilities remain partial.

Subsequent scoped resolution: [relation_planning_extensions_5_0_6.md](relation_planning_extensions_5_0_6.md) resolves the two adjacent facet-clearing/grouped-order gaps with 1152 actual original/native observations and relevant SQLite/lifetime/module gates. The open-contract statements above describe the earlier checkpoint; broader configurations and complete capability acceptance remain open.
