# Complete data-service graph review (Rust 5.0.6)

Status: **accepted** after the [complete source qualification](parity/service_graph_source_acceptance_5_0_6.json.gz).
Fresh original1+14 observer tests reproduce 9,694 complete observations; current
native30/module30 each match 12,007 observations. Canonical1,163 and pinned
format1,805 pass. Source totals are **118 accepted / 4 pending_review /
2 excluded_external**; capabilities stay **11 qualified / 6 partial** and P0-P7
remain incomplete. The reference is
`teaql-runtime/src/data_service/graph.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`552c99766ae688454029c212dfa01f4e6ee27e836e955fa00fe34a8b3b8ca952`.
The separate graph model and audited entry sources have independent decisions.

## Complete responsibility map

The [chronological review](service_graph_source_review_5_0_6.md) records the
original body reconciliation, reproduced defects and their correction gates.
The following finite map covers every production/default/trait/test family in
the 2,085-line source, including private migration paths.

| ID | Original responsibility | C++ mapping and qualification boundary |
| --- | --- | --- |
| SG01 | Generated identities and optional dynamic readbacks | Distinct `LedgerExecutionOutcome` and `DynamicReadbacks` types; allocator-aware defaults, generated-map lookup/iteration and mutable `rows` replace Deref/DerefMut. Raw derived diagnostics remain separate from safe publication. |
| SG02 | Trace recovery and specific/fallback resolution | Token recovery supplies an empty chain when absent; a complete per-item chain replaces fallback, while an empty chain copies it without consuming sibling ancestry. |
| SG03 | Governed ordinary graph save | Wrong-root rejection precedes planning; whole-plan policy review precedes governed execution. This service entry creates no transaction. |
| SG04 | Core graph conversion | Copy values/comments, group children by relation in stable order, map Save/Delete to Upsert/Remove and retain absent dirty/original/dynamic state. |
| SG05 | Five typed entity adapters | Preserve the status-only no-persistence shortcut, deleted relation clearing, comment replacement and forced-create order as distinct entry contracts. |
| SG06 | Planning entry | Validate root intent before root identity, retain one scoped traversal, install the planned root and rebuild adjacent batches only on completion. |
| SG07 | Complete recursive plan traversal | Preserve Reference/Remove shortcuts, metadata and relation direction, visited assigned identities, parent creation, ID/version/timestamp preparation, item order and pre/post-allocation trace timing. |
| SG08 | Batch execution and root readback | Intent precedes privacy, empty/Reference work skips DML, updates/deletes require literal ID and optional I64 version, per-item trace and old values survive, Remove skips readback and other roots use authoritative current rows. |
| SG09 | Repository entity extraction | Descriptor identity, getter order, dirty/original/deleted/comment capture and value conversion stay distinct from audited dynamic-carrier extraction. |
| SG10 | Recursive insertion | Validate Reference before extending its scope; preserve Remove behavior, to-one before parent writes and to-many after readback. This path does not inherit the planner's traversal rules. |
| SG11 | Recursive upsert | Extend scope before Reference/Remove validation; retain fallback insertion ancestry, to-one/to-many order, duplicate-ID rejection, target-ID metadata requirements and unmodified Reference foreign keys. |
| SG12 | Reference validation | Reject child relations and mutable scalars; validate metadata, identity, existence, deletion and I64 optimistic version. Return the node without dropping its selected dynamic fields. |
| SG13 | Remove validation | Reject children and already-deleted rows; preserve identity/existence checks and leave optimistic enforcement to deletion. |
| SG14 | Mutation-value graph parsing | Preserve reserved marker consumption, object/list/null relations and exact malformed-child failures without collapsing mutation, snapshot or scalar values. |
| SG15 | Update command projection | Update Checker/Fix and marker cleanup precede error propagation; eligible dirty mapped fields, original values, version and legacy trace conversion remain intact. |
| SG16 | Recursive deletion | Preserve recursive child handling and delete dispatch with the source's distinct comment/ID behavior. |
| SG17 | Current-row retrieval | Retain mapped projection, ID predicate, request intent/trace, single-row conversion and provider errors; consume the separately accepted readback source tests only for their actual assertions. |
| SG18 | New-ledger dependency ordering | Metadata precedes missing-record skip; typed/ordinary NULL are excluded, foreign matching uses Value equality, duplicate edges are suppressed and canonical key representatives, ordered ready keys and cycle diagnostics match. |
| SG19 | Allocated relation rebinding | Rewrite declared references only, collect before mutation, reject conflicting forward/reverse bindings and preserve the retryable original ledger. |
| SG20 | Ledger entry | Dynamic provider check precedes intent, then ledger privacy and composition validation. Do not replace this order with audited-entry or transactional-saver order. |
| SG21 | Ledger snapshots and classification | Capture new/deleted/change snapshots before deletion metadata callbacks; require loaded versions, cancel new/deleted entities and classify missing current rows as inserts. |
| SG22 | Allocation and prepared creates | Allocate the complete new graph before trace enrichment/rebinding; validate actual sparse payloads with IDs, versions, timestamps and model locations before any DML. |
| SG23 | Native ledger writes | Deletes precede dependency-ordered adjacent insert batches, then signature-ordered updates; preserve old values, original versions, per-key trace and source event positions. |
| SG24 | Dynamic write preflight | Require audit scope, trusted provider, executor-owned store, definitions and U64 owner identity; validate every write before native DML. |
| SG25 | Dynamic persistence and outcome | Apply writes, read exact owners with changed/loaded selection union, reject unselected or inconsistent Set/Delete results, reconstruct trusted values and enrich audit before returning rare-path readbacks. |
| SG26 | Original trace-resolution test | Preserve complete-specific replacement, empty-specific fallback, empty fallback, typed equal numeric IDs and reusable shared fallback. The original test and actual C++ per-item lineage/caller controls have separate evidence. |

Candidate implementation families are `entity_graph_plan.hpp`,
`entity_graph_execution.hpp`, `entity_save_adapters.hpp`,
`recursive_graph_write.hpp`, `graph_node_commands.hpp`, `entity_service.hpp`,
`ledger_execution.hpp`, `ledger_execution_outcome.hpp` and its Debug header under
`teaql_runtime/include/teaql/runtime/detail`, plus
`teaql_runtime/src/data_service/ledger_plan.cpp`. Delegation preserves independent
service, executor, provider, policy and audited transaction responsibilities.

The reference's generated-ID outcome is not just a map: the optional boxed
dynamic payload and its raw Debug/default/collection contracts remain present.
The fifteen retained diagnostic controls use one dynamic definition/field to
avoid depending on unspecified HashMap iteration. The two-ID cases exercise
actual ordered-map iteration; no arbitrary randomized HashMap order equivalence
is claimed.

## Current source and consuming gates

`scripts/verify_service_graph_source_closure_5_0_6.py` prepares a fresh owned,
byte-checked original runtime view. Appended access bridges expose existing
methods; they do not replace the graph planner, recursive writer, node commands
or ledger executor. Retained observer evaluators and SPI fixtures keep their
bodies, with main/import/path adapters for unit-test access. Legacy observers
retain their explicit valid request intent; planner observers retain their
documented optional full-row provider fixture. The historical ledger observer
asserts rare readbacks are absent before projecting generated IDs, rather than
silently discarding supported dynamic results.

The [fresh original gate](parity/service_graph_source_original_closure_corrected_5_0_6.json.gz)
passes the complete original trace-resolution test, eight
retained corpus replays, both legacy valid-intent matrices, three planner scope
matrices and the fifteen raw outcome controls. Complete outputs are compared,
including errors, provider calls, plans, batches, locations and ancestry; case
counts alone are not source acceptance. Actual dynamic storage, authoritative
versions, allocation traces and rollback remain covered by their independent
source-qualified fixtures and the joined candidate gate.

The [native](parity/service_graph_source_native_closure_5_0_6.json.gz) and
[module](parity/service_graph_source_module_closure_5_0_6.json.gz) gates each pass
30 explicitly selected tests. Four previously
missing canonical consumers cover ledger ordering and its comparator, typed
save adapters and recursive SQLite writes. It requires the full exact registry,
actual executable commands, terminal JUnit without skipped tests, stable input
and reference hashes, no-work rebuilds and canonical body preservation.

The first reference observer failed compilation because its inner module
documentation followed helper items. Its
[failed receipt](parity/service_graph_source_original_closure_5_0_6.json.gz)
remains intact. Moving the unmodified driver before those helper items fixes
only observer assembly. The original source prefixes stay byte-identical.
Both successful C++ input snapshots are identical; the successful original
snapshot differs only in that verifier file. Removing the original-observer
builder leaves identical complete controller ASTs, so C++ build/selection/run
logic and all implementation, fixture, corpus and generated inputs are unchanged.

The successful original run executes fifteen test functions: the original one,
eight retained-family tests, five legacy/planner supplements and one outcome
diagnostic test. These retain 6,635 base observations, 356 explicit-intent legacy
observations, 2,688 planner scope observations and 15 outcome observations.
The additional generated inheritance graph comparison contributes 2,313 actual
retained outputs on each C++ frontend, giving 12,007 per frontend. The separate
owned original source/build/corpus directory is removed after terminal success
(185,846,333 bytes). Pinned formatting passes for 1,805 C++ files; this turn
changes no handwritten C++ function or module export. Existing lifetime gates
remain at their own recorded checkpoints rather than being relabeled as new
sanitizer, MIPS or physical-device execution.

The earlier snapshot timing and dynamic Reference loss defects remain recorded
in the chronological review. Their permanent callback and selected-value
regressions must be included in the current consuming fixtures. Historical
full-native877 evidence keeps its original checkpoint; it is not relabeled to
cover later runtime corrections or the new test registrations.

## Explicit adaptations and remaining goal scope

Context-owned operations, allocation-aware containers and checked borrowed
metadata replace Rust Future/Arc ownership. Registered objects and actual
resources finish cleanup before arena reclamation. Results borrow their Context;
retention into Global uses an explicit destination copy. Poll activation,
transaction leases, pending cancellation and ancestor tokens may not extend
Context lifetime through hidden shared ownership. Existing lifecycle evidence
retains its recorded implementation hashes; test registration alone adds no
new runtime ownership mechanism or reason to repeat MIPS/sanitizer profiles.

This source decision does not accept every delegated resolved/provider function,
all generated models or the remaining G08/G12/G13/G14/G15/G17 obligations.
It does not add PostgreSQL/MySQL, cloud, GPU adapters or physical-router claims.
The user-approved generated JSON payload exception remains explicit and is not
turned into an exact original match. P0-P7 and the overall generator goal remain
independent of this source-specific qualification.

## Final continuation and artifact lifecycle

The [final continuation](parity/service_graph_source_final_continuation_5_0_6.json.gz)
rechecks all 11,411 execution inputs and both unchanged references. Only the
source disposition and G13 evidence append follow the corrected original gate.
Relative to the preceding goal turn, exactly six execution inputs change: the
three module/preset registrations, the verifier and those two JSON ledgers.
Production C++, fixture, generated SDK, producer and module export bodies stay
unchanged.

All four recorded owned temporary directories are absent. Measured cleanup
removes 185,870,113 bytes of source/build/corpus/JUnit outputs and 25,418 bytes
of task-created Python cache. Failed-run cleanup without a measured size is not
added to that total. Compact receipts and the stable native/modules preset
directories remain. Their measured sizes are 21,040,213,353 and 5,473,205,527
bytes, including pre-existing outputs rather than just this task's growth.
