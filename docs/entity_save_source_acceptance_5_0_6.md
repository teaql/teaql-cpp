# Complete entity-save source review (Rust 5.0.6)

Status: **accepted** for the complete source responsibility at the locked Rust
5.0.6 baseline. The [acceptance record](parity/entity_save_source_acceptance_5_0_6.json.gz)
reconciles ES01-ES18, fresh original 17 tests plus 5,576 observations, native
25/25 and imported modules 25/25. Each C++ frontend matches 9,277 complete
observations. Source totals are **117 accepted / 5 pending_review /
2 excluded_external**; capabilities remain **11 qualified / 6 partial** and
P0-P7 remain incomplete.

## Complete responsibility boundary

The source is `teaql-runtime/src/entity_save.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The
[ES01-ES18 matrix](entity_save_responsibility_matrix_5_0_6.md) accounts for every
production function family and the separate original test modules. The
[chronological review](runtime_entity_save_source_review_5_0_6.md) retains
discrepancies, failed/interrupted gates and corrections; it is not replaced by
a count of passing tests.

| Source family | Responsibilities reviewed |
| --- | --- |
| Fix task locals and accessors | ES01: first-poll clock capture, operation-owned evidence, nested scoped/public versus plain generic calls, last-evidence publication, completion and cancellation |
| `DynGraphSaver`, `GraphSaverFor::new` and its two methods | ES02-ES04: erased capability, executor lookup, distinct graph/ledger plans, policy before begin, transaction-owned authoritative read, commit/rollback and after-commit delivery |
| `decode_saved_ledger_entity` | ES05: optional relation reuse, ordinary/context decoder selection, dynamic snapshot and installation errors |
| `reject_cancelled_new_root`, `can_preserve_loaded_relations` | ES06-ES07: create/delete intersection, pending other entities, new/deleted keys, root scalar fields and changed local relation keys |
| Public graph extraction, private extraction and values conversion | ES08-ES09: pending-dynamic guard, descriptor and entity-hook order, loaded/dirty/original/flag/comment/selection capture, reserved markers and metadata-resolved recursive relations |
| `merge_relation_mutations_into_root` | ES10: child identity, distinct Create/Upsert/Remove/Reference behavior, explicit dirty fields, original versions and recursion |
| `capture_ledger_trace_scopes` and its visitor | ES11: validated root intent, deterministic first ancestry, standalone pending keys, owned unallocated scopes, retries and typed NULL distinction |
| `hydrate_ledger_relations` | ES12: pending union, immutable original values plus current changes, existing-child exclusion, normalized identity, cycle protection and deleted-child behavior without I/O |
| Graph and standalone-dynamic preflight | ES13: Checker/Fix locations, status/dirty marker cleanup before error return, changed fixed values staged into the ledger and per-entity live new/deleted membership |
| `ledger_object_locations` and its visitor | ES14: first deterministic model/JSON path and root path retention |
| `AuditedSaveExt` and its implementation | ES15: generic descriptor/audit/carrier order, dynamic scoped continuation, plain unscoped save and ordinary saved-row decoding |
| Public typed ledger save, its inner entry and shared body | ES16: optional typed carrier, composition/dynamic-provider checks, extraction, saver lookup, staged changes, scopes, dirty decision and typed decoding |
| Public executor-bound save and its inner body | ES17: the caller's executor, no internal commit/rollback, pending ledger only for actual ledger work, authoritative read and ordinary graph fallback |
| `data_service_error_into_runtime` and error branches | ES18: runtime errors retain their kind, provider errors become graph errors, failed commit keeps retry intent and post-commit audit failure cannot be mistaken for rollback |

The typed ledger entry and the generic dynamic continuation share their body
without combining the generic/plain, typed/generated and executor-bound entry
contracts. In the C++ executor fallback, moving the graph into its operation
empties the source dynamic-field owner; its ordinary materializer therefore
does not install the returned graph's dynamic selection. Four successful public
original/native controls establish this data-flow distinction. Visual similarity
to the ledger materializer is not a reason to change it.

The recorded source review also distinguishes configured failures from reached
branches. The direct original ports reach the persisted-update failed-commit
case, retain pending scalar changes and deliver no audit. The after-commit case
asserts its structured kind/code, committed row, cleared ledger and absence of
rollback. Earlier corpus inputs that stopped at missing intent do not substitute
for these assertions.

## Current evidence and consumers

`scripts/verify_entity_save_source_closure_5_0_6.py` runs three separate profiles.
The original profile creates an owned, byte-checked runtime view and appends
access-only bridges and observer fixtures. Every original source and test body,
the locked registry identities and the exact upstream checkout are preserved.
The five original observer evaluators keep their SPI fixtures; only process
main/import/fixture-path adapters change for private test access.

The [fresh original receipt](parity/entity_save_source_original_closure_5_0_6.json.gz)
passes 22 test functions: the original 17 plus five corpus replays. The latter
reproduce all 1,597 Fix, 1,719 preparation, 960 transactional saver, 944 automatic
audited save and 356 executor-bound observations. Complete expected outputs are
compared, not success counts alone. Its owned source/build outputs are removed
after terminal success (180,096,935 bytes). Retained nested-call and focused
ordering/decoder witnesses supplement these historical input sets.

The [native gate](parity/entity_save_source_native_closure_5_0_6.json.gz) and
[final module gate](parity/entity_save_source_module_closure_final_5_0_6.json.gz)
pass the same 25 tests per frontend: all original ports;
preparation, Fix, saver and audited entry contracts/comparisons; runtime-state
bridges; eight explicit-executor scope controls; real SQLite dynamic Save and
commit-audit behavior; completion/cancellation; generated bootstrap, sample-data
and inherited save/graph callers; and the retained relation-graph example.
Exact registry and terminal JUnit names must agree, with no skips. Three missing
canonical generated callers are registered without changing their source bodies
or generated SDK files. The modules configuration has both the regular example
target and a distinct canonical `teaql_module_sqlite_relations_graph` consumer.
The source gate must build and execute that canonical consumer. The two initial
controller failures retain the incorrect target and subsequent missing-command
guard, rather than accepting a registered but unavailable example executable.

The final original and native snapshots are identical. The final module
snapshot differs only in the controller's corrected canonical example target
spelling; replacing that one constant makes the complete controller ASTs
identical. Runtime, fixture, generated SDK, producer, corpus and reference inputs
remain unchanged. Both profiles verify exact registry/JUnit names without
skips, no-work rebuilds and input hashes. Imported validation preserves all
1,159 canonical consumer/fixture bodies. Pinned format/check passes for the
existing 1,805 C++ files; this turn changes no handwritten C++ body or module
export. Existing qualified lifetime/sanitizer evidence remains at its recorded
checkpoint; routine sanitizer and MIPS runs are not repeated for test registration
and source qualification.

The [reachability analysis](parity/entity_save_source_reachability_closure_5_0_6.json.gz)
records actual retained errors and case indexes. For example, transactional
saver inputs reach six commit errors and eight after-commit audit errors;
automatic-save inputs reach 48 begin errors, eight commit errors and eight
after-commit audit errors. Larger configured-fault counts are not substituted
for reached branches. Static review of the small decoder/provider/runtime error
adapters supplements the concrete failure and installation controls; no claim
is made that every unconstrained application callback failure was injected.

## Ownership adaptations and remaining goal scope

Rust owned/Arc values become explicit GlobalContext/UserContext allocation and
borrowed handles. The Context-owned multi-block arena releases registered
objects/resources before its blocks, nested strings/containers keep their
allocator and cross-domain retention copies explicitly. Fixed-scope state uses
UserContext storage and a borrowed activation during polling rather than
process-owned task-local data. Ordinary wrapper values do not extend Context
lifetime. Existing cancellation, transaction lease and checked-close tests
remain necessary; original asynchronous allocator/worker-stack costs are not
claimed identical.

The current source/caller review does not authorize a new nested-transaction API,
accept every function in delegated graph/resolved/provider modules, qualify every
generated model, or finish G13/G17 and P0-P7. The original root/child and dynamic
Save responsibilities remain intact; SQLite-only scope does not remove their
abstractions. The separately approved JSON payload compatibility exception stays
explicit and is not counted as an exact Rust result. No new physical-router,
MIPS or full-suite result follows from this source-specific gate.

## Final continuation and artifact lifecycle

The [final continuation](parity/entity_save_source_final_continuation_5_0_6.json.gz)
rechecks all 11,410 execution inputs and both unchanged references. Only the
source-disposition promotion and G13 evidence append follow the final module
snapshot. Relative to the preceding goal turn, the six changed inputs are the
three module/preset registrations, the new verifier and those two JSON ledgers.
Every production C++ implementation, fixture, generated artifact and producer
file remains unchanged.

Measured disposable cleanup removes 180,125,142 bytes of source/build/JUnit
outputs and 11,264 bytes of this task's Python cache. All three recorded owned
temporary directories are absent; failed-controller cleanup without a measured
size is not added to those totals. Compact receipts and stable native/modules
presets remain. Their measured sizes are 21,040,159,268 and 5,221,812,938 bytes;
these include pre-existing outputs, rather than representing this task's growth.
