# Relation source responsibility review (Rust 5.0.6)

Status: complete named-source review accepted. See the
[final acceptance](relation_source_acceptance_5_0_6.md) for terminal evidence and
scope. No unrelated capability or generator phase is promoted.

The locked source is `teaql-runtime/src/data_service/relation.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The
[machine-readable inventory](parity/relation_responsibility_inventory_5_0_6.json)
binds its SHA-256 and maps all 43 function definitions exactly once: 39 production
functions (including nested `supported`), three tests and one test fixture.
Every candidate implementation and evidence-document path exists. This is a
coverage index, not proof that the referenced tests exercise every responsibility.

## Responsibility map

### R01: Facet runtime adapter

Source: `user_context`, `fetch_facet_smart_list`.

Retains the existing UserContext and delegates through scoped services. Public facet evidence and owner async observations must be correlated with the final source-wide gate.

Candidate evidence: [public_facets_5_0_6](public_facets_5_0_6.md), [hydration_owner_facets_async_5_0_6](hydration_owner_facets_async_5_0_6.md).

### R02: Identity sets and key snapshots

Source: `from_value`, `unique_relation_values`, `relation_key_rows`.

Normalizes nonnegative integer identity, excludes NULL keys, and snapshots declared local keys before overlays. Signed/unsigned representative choice is not guaranteed by the Rust unstable sort.

Candidate evidence: [relation_planning_replay_5_0_6](relation_planning_replay_5_0_6.md), [relationship_identity_contracts_5_0_6](relationship_identity_contracts_5_0_6.md), [dynamic_relation_hydration_5_0_6](dynamic_relation_hydration_5_0_6.md).

### R03: Top-N planning and telemetry

Source: `relation_top_n_operation`, `relation_top_n_execution_plan`, `provider_owns_relation_probe_batch`.

Provider batching requires at least two identities, provider capability and no explicit threshold; capture mode controls execution route. The C++ batching decision is integrated into the operation, rather than a separately named helper.

Candidate evidence: [relation_dispatch_enhancement_replay_5_0_6](relation_dispatch_enhancement_replay_5_0_6.md).

### R04: Repository and explicit relation plans

Source: `relation_loads`, `relation_plans`, `relation_query`, `build_relation_plans`, `build_relation_plans_from_loads`, `flat_relation_plans`, `supported`.

Preserves behavior selection, missing metadata/relation errors, recursive explicit query plans, and all-descendant decoder eligibility. Nested supported is included in this inventory.

Candidate evidence: [relation_planning_replay_5_0_6](relation_planning_replay_5_0_6.md).

### R05: Child query construction and trace

Source: `relation_child_repo`, `query_for_plan`, `base_relation_query`, `ensure_stable_top_n_order`, `query_for_compact_plan`.

Retains relation-path trace, target entity, projection keys, cleared nested loads/facets, stable group or ID ordering, and per-parent partitioning. Planning snapshots do not establish physical SQL execution.

Candidate evidence: [relation_planning_extensions_5_0_6](relation_planning_extensions_5_0_6.md), [relation_planning_replay_5_0_6](relation_planning_replay_5_0_6.md).

### R06: Native fetch and selected dynamic values

Source: `fetch_relation_rows`, `fetch_native_relation_rows`.

Empty keys avoid native I/O; bounded probes preserve order and stop on error. Selected dynamic values load after native rows with trusted child intent. G16 acceptance supersedes its earlier partial checkpoint, but does not accept this entire source.

Candidate evidence: [relation_dispatch_enhancement_replay_5_0_6](relation_dispatch_enhancement_replay_5_0_6.md), [dynamic_relation_hydration_5_0_6](dynamic_relation_hydration_5_0_6.md), [dynamic_storage_acceptance_5_0_6](dynamic_storage_acceptance_5_0_6.md).

### R07: Embedded enhancement and attachment

Source: `enhance_relations_internal`, `enhance_query_relations_internal`, `enhance_plan`, `attach_relation_rows`.

Snapshots keys before descendant overlays; loads descendants by batch; suppresses inverse convenience wiring for an explicit child; preserves identity fallback and loaded sidecars. The new 24-case receipt closes only the loaded-state observation gap.

Candidate evidence: [relation_hydration_replay_5_0_6](relation_hydration_replay_5_0_6.md), [dynamic_relation_hydration_5_0_6](dynamic_relation_hydration_5_0_6.md), [relation_loaded_state_5_0_6](relation_loaded_state_5_0_6.md).

### R08: Flat graph hydration and installation

Source: `hydrate_flat_plans_internal`, `hydrate_compact_flat_plans_internal`, `hydrate_flat_plan`, `hydrate_compact_flat_plan`, `hydrate_compact_flat_leaf`, `install_compact_flat_relation`, `install_unfetched_forward_detail`.

Preserves decoder registration/errors, leaf dispatch, identity graph and edge-owned unfetched detail, reusable non-ID membership, inverse state and partial installation ordering. SQLite membership-source acceptance is supporting evidence, not automatic acceptance of the runtime source.

Candidate evidence: [relation_hydration_replay_5_0_6](relation_hydration_replay_5_0_6.md), [hydration_flat_facets_5_0_6](hydration_flat_facets_5_0_6.md), [relation_membership_source_audit_5_0_6](relation_membership_source_audit_5_0_6.md).

### R09: Relation aggregates and key ownership

Source: `relation_aggregate_key_rows`, `enhance_relation_aggregates_internal`, `enhance_relation_aggregate`.

Validates sibling relation names before fetch and snapshots only local fields; aliases cannot change later memberships. Earlier direct enhancement observations use a child-fetch SPI; complete-method coverage must be reconciled.

Candidate evidence: [relation_dispatch_enhancement_replay_5_0_6](relation_dispatch_enhancement_replay_5_0_6.md), [relation_membership_source_audit_5_0_6](relation_membership_source_audit_5_0_6.md).

### R10: Object groups and child overlays

Source: `enhance_object_group_bys_internal`, `enhance_child_queries_internal`.

Retains ID projection/filter, last-row identity lookup, missing-object NULL and checked dynamic merges. Intent rejection occurs before child fetch. Simplified SPI evidence remains distinguishable from full runtime execution.

Candidate evidence: [relation_dispatch_enhancement_replay_5_0_6](relation_dispatch_enhancement_replay_5_0_6.md), [dynamic_relation_hydration_5_0_6](dynamic_relation_hydration_5_0_6.md), [dynamic_storage_acceptance_5_0_6](dynamic_storage_acceptance_5_0_6.md).

### R11: Owner facet execution

Source: `relation_facets`.

Complete owner membership clears page/order/partition; missing or NULL owner becomes false filter. Intent and trace use child scope. Async evidence covers completion, failure, cancellation and Context close.

Candidate evidence: [hydration_owner_facets_5_0_6](hydration_owner_facets_5_0_6.md), [hydration_owner_facets_async_5_0_6](hydration_owner_facets_async_5_0_6.md), [hydration_flat_facets_5_0_6](hydration_flat_facets_5_0_6.md).

### T01: Original source tests and fixture

Source: `limited_many_plan`, `single_parent_probe_does_not_delegate_to_a_multi_parent_provider_batch`, `topn_001_002_003_004_plan_respects_default_threshold_and_sqlite_policy`, `topn_010_plan_telemetry_is_safe_and_complete`.

Three original tests plus their fixture. Two named planner/telemetry counterparts exist; singleton batching is covered through execution-route evidence. Fresh original-test replay and complete test-to-evidence reconciliation remain required.

Candidate evidence: [relation_dispatch_enhancement_replay_5_0_6](relation_dispatch_enhancement_replay_5_0_6.md).

## Fresh original test replay

[The original test receipt](parity/relation_source_original_tests_5_0_6.json.gz)
records 3/3 passing planner tests against a complete source-preserving view,
without appended observers or replacement bodies. Test names are derived from
the locked source and matched against the terminal Cargo output. The source hash
matches this inventory. The owned temporary source/build tree was removed:
160880381 bytes, with cleanup confirmed.

```sh
source scripts/toolchains/activate.sh
python3 scripts/replay_relation_source_tests_5_0_6.py --output <fresh-receipt.json.gz>
```

## Dispatch coverage gap and supplemental closure

Inspection of the active `relation_execution_5_0_6_cases.json.gz` corpus finds
432 cases with parent counts 0, 1, 2, 3 and 21, and thresholds absent, 0, 2 and 9.
The original singleton-provider test also asserts parent count 32 and explicit
threshold 32. Those exact stimuli are absent from this corpus. Existing dispatch
coverage supports the branch behavior, but it must not be described as a literal
replay of every source-test assertion. The inventory records the corpus hash and
these historically missing stimuli. The [supplemental boundary contract](relation_probe_boundaries_5_0_6.md)
now covers all original batching stimuli through actual C++ dispatch, including
32 parents and threshold 32: native/module each pass 32/32; both existing 432-case
differential regressions also pass. These are source-derived integration assertions,
not a replacement original-output corpus.

## Current implementation review findings

The current production bodies were checked against the locked methods for the
following cross-cutting ordering and ownership responsibilities:

- Plan builders preserve ordered grouping of dotted behavior loads, explicit
  relation ordering, recursive metadata errors and nested decoder eligibility.
  Query construction clears planner-owned descendants/facets and adds only missing
  direct group/ID ordering fields for sliced queries.
- Embedded enhancement snapshots membership before descendant overlays, performs
  descendants over the child batch, and attaches before owner facets. Explicit
  inverse selection suppresses convenience wiring. The repaired outer SmartList
  loaded flag preserves nested state independently.
- Ordinary flat hydration installs descendants before parent relation views and
  keeps shape availability distinct from compact-flat marking. Non-ID buckets
  remain reusable; ID buckets are consumed once.
- Compact forward hydration installs ancestors before descendants, then publishes
  edge-owned exact projections without replacing richer identity-table peers.
  Missing targets use an edge-owned identity-only option; explicit NULL marks
  availability, while a missing key retains its separate state.
- Facet paths clear page/order/partition for complete owner membership. Compact
  forward owners are validated before facet I/O; other facet paths preserve the
  original ordering of availability marking, facet completion, graph installation
  and decoder failure. The native union includes full partial-state corpora.
- Runtime facet adapters retain the same Context and scoped service. Poll-based
  C++ operations explicitly forward wakes and cancel descendant, native-fetch,
  dynamic and facet children before Context arena reclamation. This is the C++
  ownership adaptation of Rust future-drop cleanup, not shared lifetime extension.

These findings support the existing responsibility groups and do not replace
executed evidence. The final native union passes **26/26**, recorded by
`scripts/verify_relation_native_closure_5_0_6.py`, which binds current service
headers/implementations and checks the exact 26-test selection. The [terminal native receipt](parity/relation_native_closure_5_0_6.json.gz)
retains successful builds of all 18 targets, exact test discovery, CTest output,
JUnit results and unchanged source hashes across execution. It does not promote
the source disposition automatically. Final module-union reconciliation passes the same 26 commands; the named-source acceptance record is complete.

## Closure disposition

1. Fresh complete-source replay passes all three original planner tests. Two
   direct C++ planner/telemetry ports and supplemental actual-dispatch boundaries
   account for their assertions, including singleton and explicit-threshold cases.
2. R09/R10 now have 754 complete original/native/module observations. Historical
   child-fetch-SPI receipts remain unchanged and retain their narrower scope.
3. R01-R11 body review, ownership adaptations and relevant original observations
   are consolidated in the final acceptance review. These are finite semantic
   witnesses alongside source review, not a claim of exhaustive input testing.
4. The final native26/module26 union uses current implementation hashes and
   includes graph/facet cancellation and Context-close contracts. No new lifetime
   implementation was introduced, so unrelated sanitizer/MIPS suites were not run.
5. The relation source alone is accepted. The other ten pending source rows and
   remaining generator P0-P7 gates require their own reviews and evidence.

The 24-case attachment comparison retains its exact observed-field scope. Source
counts are now 112 accepted / 10 pending / 2 excluded_external.
