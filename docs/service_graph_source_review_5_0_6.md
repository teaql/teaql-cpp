# Data-service graph source review (Rust 5.0.6)

Current status: **accepted** after the
[complete SG01-SG26 closure](service_graph_source_acceptance_5_0_6.md).
Fresh original1+14 observer tests match9,694 complete observations;
native30/module30 each match12,007. Canonical1,163/format1,805 and stable
input/reference/no-work checks pass. Source totals are now118/4/2; capability11/6
and P0-P7 remain incomplete. Earlier pending language below retains its original
checkpoint scope.

This chronological review covers the 2,085-line
`teaql-runtime/src/data_service/graph.rs`, not the separate graph model in
`teaql-runtime/src/graph.rs` or the audited entry points in `entity_save.rs`.
The locked source SHA-256 is
`552c99766ae688454029c212dfa01f4e6ee27e836e955fa00fe34a8b3b8ca952`.
The reference remains `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Entry and ordinary execution reconciliation

The following rows record direct reading of original bodies at lines 71–408
and the current candidate bodies. They are source-review conclusions, not new
execution results. Existing differential evidence is linked separately below.

| Responsibility | Original behavior and current C++ mapping |
| --- | --- |
| Governed graph save | `save_graph_internal` rejects the wrong root entity before calling the planner, then constructs and reviews the whole policy plan before governed execution. `GraphSaveOperation` preserves that order, including cancellation if governance-wrapper construction throws. It does not introduce a transaction at this service entry. |
| Core graph conversion | `save_entity_graph_from_internal` groups children by relation name while preserving order within each group, copies values/comments, maps Save/Delete to Upsert/Remove, and leaves dirty/original/dynamic state absent. `convert_save_graph` and `CoreEntityGraphSaveOperation` retain these responsibilities; destination allocation is explicitly Context scoped. |
| Ordinary typed graph | `save_entity_graph_internal` extracts the repository-specific graph and invokes governed graph save. `EntitySaveAdapter::Graph` preserves this distinct entry. |
| Status-only typed save | `save_entity_internal` returns an empty service-entity graph before extraction when `need_persist` is false. Otherwise deleted status forces Remove and clears relations. `EntitySaveAdapter::Status` preserves both branches. |
| Status and comment | `save_entity_with_comment_internal` has no no-persistence shortcut. Deleted status forces Remove and clears relations; the supplied comment overrides the extracted comment. `StatusComment` preserves that difference from `Status`. |
| Comment and create entries | Graph-with-comment overrides the root comment after extraction; create-with-comment additionally forces Create. `Comment` and `CreateComment` preserve the order, including extraction errors before comment replacement. |
| Public graph planning entry | `plan_graph` validates its own root comment before rejecting an incorrect root entity, then traverses under that mutation intent and rebuilds adjacent batches. `GraphPlanningOperation` preserves this ordering. Its retained Context-owned root exists before traversal; this storage adaptation does not publish a successful plan before traversal completes. Full traversal review remains separate. |
| Execution entry | `execute_graph_plan_internal` validates the planned root's comment, then establishes plan privacy, before entering execution. `GraphExecutionOperation` preserves that order, including the absent-root intent error before the structural missing-root branch. Parent intent is not a substitute. |
| Ordinary batch execution | Empty batches, empty-field updates and Reference batches perform no writes. Create retains each item's trace. Update requires literal `id`, accepts only I64 as the optional expected `version`, and retains old values and trace. Delete executes items individually with the same ID/version rules. `GraphExecutionOperation` preserves these distinctions and propagates child failures before later batches. |
| Root readback | Remove returns the root without readback. Other operations resolve the descriptor's ID property, require that identity, read through the existing graph-current-row helper, replace root values, and fail on missing persisted data. Root-comment readback trace uses `try_u64`, rather than the planner's stricter positive-ID helper. The candidate keeps that separate rule. |
| Repository entity extraction | `graph_node_from_entity` checks descriptor identity, captures dirty fields/original values/deletion/comment before consuming the entity, parses values, then applies deletion and comment. It does not sample the dynamic carrier used by separate audited extraction. `EntityDataService::graph_node_from_entity` preserves the same getters and order; scoped copies and quiescence protect C++ Context ownership. |

The candidate operations retain wake forwarding and explicit cancellation of
pending children. Those mechanisms adapt Rust future ownership; this source
inspection does not replace existing lifecycle tests or authorize extending a
Context borrow's lifetime.

## Evidence and its limits

[Private save replay](private_save_replay_5_0_6.md) retains actual locked-source
execution of 541 adapter inputs and 520 graph execution/save/readback inputs,
alongside the recursive, command and ledger families. It also records the
previous missing-root intent precedence discrepancy and correction. These are
historical execution checkpoints; they are not silently relabeled as current
whole-source acceptance.

[Graph readback source acceptance](graph_readback_source_audit_5_0_6.md) covers
the separate two-test readback source, including mapped fields, log privacy,
failed observations and unchanged parent scope. [Graph model review](runtime_graph_source_review_5_0_6.md)
covers the separate model source and later planner/legacy witnesses. The
[entity-save matrix](entity_save_responsibility_matrix_5_0_6.md) covers audited
entry and transactional saver responsibilities, not this entire service file.

The running frozen native epoch uses
`parity/native_save_reconciliation_integration_inputs_5_0_6.json.gz` and
`parity/native_save_reconciliation_integration_terminal_5_0_6.json.gz`.
A receipt created by a live controller is not terminal evidence. Its complete
registry/JUnit, input comparison and no-work checks must succeed before it can
support current integration. Even a passing full native suite does not by itself
resolve the remaining source review below.

## Remaining complete-source reconciliation

Reconcile these original sections against their complete candidate bodies and
retained execution witnesses before promoting this source. This finite list
preserves private migration boundaries even if ordinary application code does
not currently call them.

| Original lines | Responsibility still requiring consolidated review |
| --- | --- |
| 20–69 | Generated-ID outcome, rare dynamic-readback payload, trace recovery and specific-versus-fallback chain selection. Review observable map/Debug interfaces separately from Context ownership adaptations. |
| 409–575 | Complete graph traversal: metadata, IDs, timestamps, relation direction, parent linkage, item order and scope tokens. |
| 576–976 | Independent recursive insert/upsert futures, reference/remove shortcuts, insertion fallback, trace timing and authoritative reads. |
| 977–1130 | Reference/remove validation, permitted fields, missing identity, existence, deleted rows and optimistic versions. |
| 1131–1257 | Mutation-value graph parsing and update command projection, dirty fields, original values and marker handling. |
| 1258–1367 | Recursive deletion and complete current-row retrieval; reuse the separate readback test-source decision only for its demonstrated assertions. |
| 1368–1543 | New-ledger dependency order and allocated relation rebinding, including metadata and missing/conflicting identity errors. |
| 1544–1940 | Ledger intent/privacy boundary and complete execution: allocation, relation rebinding, native writes, versioning, trace recovery and returned outcomes. |
| 1941–2063 | Dynamic storage preflight, write dispatch, optimistic authority, readback selection and returned values. |
| 2065–2085 | The original trace-resolution unit test: full specific chain replaces fallback; empty chain inherits it; equal numeric IDs retain typed entity identity; resolving does not consume shared fallback. |

No source disposition, capability status or phase exit changes in this review.
The six pending source records and six partial capabilities remain open.

## Candidate checkpoint hashes

These hashes identify the bodies inspected during the frozen integration epoch.
They do not assert that all code in each file was reviewed.

| Candidate file | SHA-256 |
| --- | --- |
| `teaql_runtime/include/teaql/runtime/detail/data_service/entity_save_adapters.hpp` | `6ea63f001cf8e145aadcf47219cdd2a71bf00767a6279bc5c4d7d9b29d79ab4e` |
| `teaql_runtime/include/teaql/runtime/detail/data_service/entity_service.hpp` | `b6ec2e3f06d88fbdbb9e5895115a2a1ea603364e43778f3926d9c16883ff22e9` |
| `teaql_runtime/include/teaql/runtime/detail/data_service/entity_graph_execution.hpp` | `ff5c97324eb469b1baafb3a7e32b7c01afa24380c9ca9d17edd50aef357c98ea` |
| `teaql_runtime/include/teaql/runtime/detail/data_service/entity_graph_plan.hpp` | `7d94674b05debb8748d8b12deec7efde99fb0d40473ef9bd5a2a99a36c44faf8` |

## Traversal and node-command continuation

Direct review of original lines 409–1367 and the candidate planner, recursive
writer, node commands and current-row operation reconciles these boundaries:

- Planning emits Reference/Remove items before metadata traversal, deduplicates
  assigned identities, propagates explicit parent Create, and captures read
  ancestry before allocation but item tokens after allocation. It does not
  implement the separate recursive writer's relation attachment behavior.
- Recursive insertion validates Reference before adding the node's own scope;
  recursive upsert adds its scope before Reference/Remove validation. Fallback
  insertion clears the comment after retaining its ancestry. To-one children
  precede parent writes; to-many children follow authoritative parent readback.
- To-many upsert requires target ID metadata even for an empty child list,
  checks duplicate assigned identities, and avoids assigning a foreign key to
  Reference children. Recursive insertion deliberately has different behavior.
- Reference rejects child relations and mutable scalar fields, checks existence,
  deletion and an I64 expected version. Remove validation rejects children and
  already-deleted rows but leaves optimistic-version enforcement to deletion.
- Update runs Checker/Fix with an Update marker, removes the marker before
  propagating findings, retains original values and selects only eligible dirty
  properties. Deletion preserves the legacy trace-ID conversion.

The existing valid-intent legacy observations are essential here: original
legacy code can construct blank comment scopes internally, but downstream
trace materialization removes blank audit frames. Candidate filtering at its
materialization point is supported by those actual original outputs, rather
than assumed from similar-looking constructors. See the 72 initial and 284
extended controls in the separate graph-model review.

### Confirmed dynamic reference loss

The original reference reconstruction explicitly retains `node.dynamic_fields`.
The candidate `GraphNodeValidationOperation<E, false>` reconstructed fixed
values and Reference operation but omitted that carrier. Existing node-command
lifecycle tests supplied no dynamic fields, so their passing result did not
cover this responsibility.

The [original twelve-control observation](parity/reference_node_dynamic_original_v2_5_0_6.json.gz)
executes unchanged Rust source through three entries: direct reference
validation, recursive insertion of a Reference, and recursive upsert of a
Reference. Each uses absent, present-empty, loaded NULL and loaded Text dynamic
state. All twelve succeed and preserve the input carrier; full original Debug
results remain in the receipt. The [candidate before correction](parity/reference_node_dynamic_candidate_before_5_0_6.json.gz)
succeeds on all twelve but loses all nine present carriers. Its completed native
library hashes remain unchanged during the isolated probe.

The initial observer injection selected a nonunique source suffix and stopped
before compilation. Its [failure receipt](parity/reference_node_dynamic_original_5_0_6.json.gz)
is preserved separately. The corrected observer is retained as
`scripts/observe_reference_node_dynamic_5_0_6.py`; it appends access and fixture
code to an owned reference view without editing the locked source. Original
probe outputs (168,027,939 bytes) and native probe outputs (23,413,302 bytes)
were removed after retaining results and source.

The ongoing native integration was deliberately interrupted only after both
observers completed. Its build exited 130, its controller exited 1, and CTest
had not started. The terminal receipt records the confirmed defect and owned
state cleanup. It is not a passing full-suite result.

The candidate correction moves the carrier from the operation's Context-owned
node into its returned node. It preserves the existing allocation domain and
uses no shared ownership. The permanent test now exercises all twelve cases,
compares complete definitions/selection/values, explicitly copies results into
GlobalContext and actually destroys UserContext before reading the retained
empty/NULL/Text states. A canonical imported-module consumer is registered.
Focused native, module and lifetime gates are still pending; neither this
correction nor the source file is accepted merely because the edit is present.

The subsequent ledger sections were inspected for allocation-before-trace
recovery, required-create validation before DML, dynamic-only optimistic updates,
whole-write validation before native writes, exact readback membership and audit
enrichment. Final consolidation with their existing witnesses remains pending.

### First correction gate and stale deletion assertion

The first native gate passes six of seven CTests; the first module gate passes
three of four. Both failures occur in the existing `deletion_parent_scope_copy`
assertion, after all twelve new dynamic-retention controls have executed. This
fixture still expected the legacy audit frame to omit the negative identity.
The unchanged original legacy observer requires U64(18446744073709551609) for
I64(-7) in that audit frame; the separate operation Entity frame has no ID.
The fixture is corrected to assert both distinctions. No trace algorithm is
changed. Both failing receipts remain immutable evidence of these attempts:
`reference_node_dynamic_native_5_0_6.json.gz` and
`reference_node_dynamic_module_lifetime_5_0_6.json.gz` in `docs/parity`.
A fresh complete gate is required; lifetime compilation had not yet started.

### Dynamic reference correction qualified

The [fresh follow-up](parity/reference_node_dynamic_followup_5_0_6.json.gz)
passes native **7/7**, canonical modules **6/6**, and focused ASan/UBSan **1/1**
with leak scanning disabled. The registries and terminal JUnit names are checked
exactly, with no skipped tests. All three selected rebuilds report no work;
captured input hashes are unchanged. Canonical verification covers 1,153
consumer/fixture bodies. Both formatting and checking pass for 1,802 handwritten
C++ files. The owned JUnit directory and initial configure log were removed.

This closes the confirmed dynamic reference loss and stale deletion fixture
assertion. Source-model defaults/diagnostics and final complete service-source
integration remain separate open responsibilities. The interrupted full-native
epoch and both failed focused attempts retain their original results.

## Private outcome model reconciliation

The source's first fifty lines keep two distinct types: generated-ID outcomes
and an optional boxed map of authoritative dynamic readbacks. C++ already keeps
those types separate in `detail/ledger_execution_outcome.hpp`. A named `rows`
member replaces Rust Deref/DerefMut on the rare payload; generated-ID lookup and
iteration remain on the outcome facade. Rust Default becomes an explicit
allocator-aware Context construction. Explicit destination copies support C++
retention without introducing implicit shared ownership.

A missing raw diagnostic counterpart was located during complete-source review.
The new `detail/ledger_execution_outcome_debug.hpp` uses the existing DebugWriter
and existing EntityKey/Value/dynamic-field writers, preserving source structure,
field names and map order. It does not publish diagnostic payloads to an
execution sink. Both private types and their diagnostic overloads have explicit
module catalog/facade/isolated-lookup entries.

The [original fifteen-model observation](parity/graph_outcome_models_original_5_0_6.json.gz)
executes unchanged source defaults, generated-ID map lookup and both derived
raw Debug implementations. It crosses zero/one/two generated identities with
absent readback, present empty readback, present empty field selection, loaded
NULL and loaded Text. The single dynamic definition/field avoids arbitrary
HashMap iteration order; the two generated identities test actual ordered-map
iteration. All fifteen observations retain original strings without rewriting
or normalization. The owned Rust view/build output (166,308,586 bytes) was
removed after successful execution.

The candidate probe and strict comparator retain that same finite matrix and
explicitly disable default PMR allocation. Raw model diagnostics carry data and
must continue through the separately qualified privacy-aware runtime path when
published. Current native/module execution of these new diagnostic bodies and
the complete service-source acceptance decision remain pending.

## Complete ledger-body reconciliation map

The complete original sections at lines 1368–2063 have now been read against
`src/data_service/ledger_plan.cpp` and the complete
`detail/data_service/ledger_execution.hpp`. The following map records the source
responsibilities and relevant existing witnesses. Witnesses retain their own
executed scope; this map does not manufacture new test results.

| Source responsibility | Candidate mapping and inspected behavior | Evidence boundary |
| --- | --- | --- |
| New-key dependency ordering | `order_new_ledger_keys` preserves metadata lookup before missing-record skip, ordinary/typed NULL exclusion, exact Value equality, literal-id fallback, forward/reverse direction, duplicate-edge suppression, ordered ready keys and cycle entity names. Canonical equal-key representative handling is explicit. | 1,362 original dependency observations in private-save replay and the installed ledger-plan fixture. |
| Relation rebinding | `rebind_relations` rewrites only declared ID references, collects assignments before mutation, skips NULL, rejects conflicting forward/reverse bindings and leaves the retryable original ledger untouched. | Existing ledger/allocated-trace and real SQLite graph fixtures; arbitrary equal scalar values are not treated as foreign keys. |
| Entry, composition and snapshots | Start checks dynamic-provider availability before mutation intent, then binds privacy and rejects the first graph composition error. It retains distinct new/deleted sets and a copied change set. | Original ledger corpus and complete dynamic graph-save contracts. Metadata definitions remain trusted runtime capabilities. |
| Loaded version authority | Existing versioned deletion/update requires the loaded original version; create-then-delete is cancelled. Missing authoritative current rows classify an update as an insert. | Ledger execution fixture and version/readback/save-preparation source witnesses. |
| Native and extension planning | Deleted keys are excluded; dynamic-only keys obtain ID-only native changes so optimistic native updates still occur. Prepared create payloads include allocated IDs, initial versions, timestamps and source locations. | Original ledger and actual SQLite dynamic graph-save contracts; sparse required-create failure precedes DML. |
| Allocation and trace | All insert IDs are allocated before trace enrichment and relation rebinding. Complete per-key lineage replaces fallback; empty per-key lineage copies fallback. | Existing late-identity lineage fixture plus SQLite allocated-trace complete-source acceptance. The original trace-resolution unit test also passes in the complete original library receipt. |
| Dynamic preflight | Pending dynamic writes require audit scope, the trusted provider, the transaction-bound store, definitions and a U64 owner identity. All writes validate before native DML. | Complete dynamic graph-save fixture, dynamic storage and SQLite provider source reviews. Successful ordinary writes alone do not prove this path. |
| Native write ordering | Required-create validation completes before deletion, insertion and updates. Insert batches combine adjacent dependency-ordered entities; updates retain signature ordering, original values/versions and per-key trace. | Ledger execution, real SQLite rollback/trigger tests and allocated-trace original/native evidence. |
| Dynamic persistence | Store apply precedes readback. Root-entity selection unions changed codes with loaded codes. Every readback has exactly one requested owner, no unselected values, and Set/Delete membership consistent with each mutation. | Dynamic graph-save and storage fixtures, including actual transaction ownership and rollback. |
| Audit enrichment and outcome | Readback uses trusted definitions to reconstruct values; audit changes use original values and authoritative mutation outcomes. The result keeps generated identities separate from optional rare-path readbacks. | Complete entity-save saver/readback tests and the new default/raw diagnostic matrix. |

Original Error, Future, Arc and map syntax is represented through the existing
C++ typed Result/error alternatives, Context-owned operations, checked borrows
and allocator-aware ordered containers. Those language/ownership adaptations
have distinct lifecycle and module evidence. They do not combine the service,
executor, provider, policy, audited-save or transaction responsibilities.

This completes the finite body-location/review map for this source. Acceptance
still requires the new diagnostic module gate, reconciliation of all cited
current consuming evidence and a successful frozen native integration. Source
and capability statuses remain pending/partial until those obligations are met.

### Snapshot timing still to resolve

One ordering difference remains visible during the body review: original
`execute_ledger_plan_scoped` captures `current_change_set()` before the
versioned-deletion metadata-validation loop, whereas the candidate Start phase
captures it after that loop. Default immutable metadata cannot reveal the
difference. A custom metadata callback that changes the shared ledger could
make it observable; no purity contract has been established here that would
justify discarding this ordering. Reproduce the actual original/candidate SPI
behavior, then align the snapshot point if the difference is reachable. This
review does not treat that unresolved ordering as accepted parity.

### Outcome diagnostic gate complete

The [current outcome-model gate](parity/graph_outcome_models_followup_5_0_6.json.gz)
passes native **1/1** and module **2/2**: both frontends compare all fifteen
original observations, and the module gate also compiles/runs the complete
isolated import catalog. Altered diagnostic and altered source-baseline controls
both reject. Canonical consumers, 1,524 exports, reference/generator locks and
module paths verify. Selected native/module rebuilds report no work and all
captured inputs remain unchanged. Formatting and checking pass for 1,804 files.
Owned temporary negative corpora, JUnit files and the configure log are removed.
No new sanitizer run is required for these serialization-only functions; the
separate dynamic-reference lifetime correction already has its focused pass.

The private model/default/raw Debug gap is closed under this finite matrix.
The snapshot-ordering question above and final consuming integration still
prevent whole-source promotion. No capability or phase is marked complete.

### Snapshot-ordering discrepancy reproduced

The [four actual original controls](parity/ledger_snapshot_original_v2_5_0_6.json.gz)
and [candidate before correction](parity/ledger_snapshot_candidate_before_v3_5_0_6.json.gz)
use a trusted MetadataStore whose second Deleted-entity lookup changes the
shared Order ledger. The first lookup occurs in privacy collection; the second
occurs during versioned-deletion validation. Original runtime methods and
metadata SPI remain unchanged. The callback changes a different pending scalar
value, rather than rewriting metadata or simulating persistence in the fixture.

| Trigger | Callback | Original write | Original pending value | Candidate write before correction |
| --- | --- | --- | --- | --- |
| Disabled | No | before | before | before |
| First lookup | Yes | after | after | after |
| Second lookup | Yes | before | after | after |
| Third lookup | Not reached | before | before | before |

All eight runs succeed; exactly the second-lookup control differs. The later
threshold is an unreached control, not evidence of a callback after persistence.
The [difference receipt](parity/ledger_snapshot_differences_5_0_6.json.gz) retains
the exact selected observations. These observations concern captured mutation
values and pending ledger state, not complete SQL/log/privacy equivalence.

The correction moves the existing change-set snapshot immediately after the
new/deleted-key snapshots and before metadata validation, matching the original
source point. It changes no allocation domain, operation ownership, provider or
transaction boundary. Four permanent regression controls exercise the real
service and inspect its emitted native update plus the surviving pending value.
They join the existing ledger execution fixture and canonical module consumer.
The complete ledger-execution differential probe now also has a registered
runtime-only module consumer; previously only its lifecycle/SQLite fixture was
registered through modules.

Initial Rust observer compilation lacked the required executor capabilities
method and mutable Context binding; the first C++ observer did not unwrap its
optional text views. Both are observer failures, not runtime results. A second
C++ attempt selected the MIPS SQLite archive and failed at link time; the final
probe uses the native preset's actual `lib_native` archive. Failed receipts are
retained under the earlier `ledger_snapshot_original`,
`ledger_snapshot_candidate_before`, and `ledger_snapshot_candidate_before_v2`
filenames. Temporary probe outputs were removed by their owned directory guards.
Focused correction gates and the final source/integration decision remain open.

### Snapshot correction gate complete

The [repaired standalone observation](parity/ledger_snapshot_candidate_repaired_5_0_6.json.gz)
now matches all four actual original controls exactly. The
[native gate](parity/ledger_snapshot_native_followup_5_0_6.json.gz) and
[module gate](parity/ledger_snapshot_module_followup_5_0_6.json.gz) each pass
**4/4**, covering the installed callback controls, complete retained ledger
comparison, dynamic graph save and all original entity-save test ports.
Expected registries and complete JUnit names match, without skipped tests.
Both selected rebuilds report no work and captured input hashes are unchanged.
Canonical verification covers 1,155 consumer/fixture bodies; all 1,524 catalog
exports pass. Formatting and checking pass for 1,804 handwritten C++ files.

The successful original and before-correction probe trees removed 167,331,883
and 35,106,520 bytes respectively. The repaired probe and both owned JUnit
directories are removed after retaining their compact results. Earlier observer
compile/link failures remain failed observations. This semantic ordering change
does not change memory ownership or async cleanup, so it uses native/module
checks without repeating sanitizer or MIPS profiles.

The source-visible snapshot-ordering discrepancy is closed. The finite
production/default/trait/test responsibility review is now consolidated; a
passing current frozen native integration and final caller/evidence audit
remain necessary before changing this source's pending disposition.

## Fresh native integration epoch

After the snapshot correction, standalone/source comparisons and root locator
refresh completed, a fresh full native epoch started. Its inputs and terminal
receipts are `parity/native_service_graph_integration_inputs_5_0_6.json.gz` and
`parity/native_service_graph_integration_terminal_5_0_6.json.gz`. Reference,
generator and configure steps pass; the all-target native build is live at this
checkpoint. Do not mutate source, scripts, CMake, JSON ledgers or producer files
while it runs. Narrative documents and compressed receipts are outside the
captured execution inputs. No passing full-suite result is claimed until the
controller, complete registry/JUnit and unchanged-input/no-work checks finish.

### Full native integration terminal result

The [current frozen epoch](native_service_graph_integration_5_0_6.md) now passes
**877/877**, with no failed/error/skipped tests, unchanged11,399 inputs and
references, and no-work stable builds before and after CTest. Controller and
wrapper both terminate successfully. The owning wrapper removes its temporary
logs/state/JUnit directory after retaining their contents in the terminal
receipt. This closes the current full native integration gate for the reviewed
checkpoint. Final whole-source caller/evidence reconciliation and explicit
source/capability dispositions remain separate; temporary parser and generated
identity/stream proposals were not installed into this successful checkpoint.
