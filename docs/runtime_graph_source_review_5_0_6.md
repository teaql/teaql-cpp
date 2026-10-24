# Runtime graph model source review (Rust 5.0.6)

Status: pending_review. This review concerns the complete 563-line
`teaql-runtime/src/graph.rs`, SHA-256
`2c0a3e061871b5dee9fef0d3f15987477c3e8051d66e5dbfbd2df1030fbaa93f`, at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Historical acceptance of the four-file
5.0.5 graph/save group does not establish this source's current acceptance.
The service graph planner and typed entity save remain separate source reviews.

## Model responsibilities and current implementation locations

| Responsibility | Candidate location and review requirement |
| --- | --- |
| EntityValues | `teaql_runtime/include/teaql/runtime/graph.hpp`: ordered mutable fields, construction/iteration and conversions remain distinct from CompactRow, MutationValues and EntitySnapshot; explicit destination allocation replaces implicit owned collection conversion |
| GraphOperation and GraphMutationKind | Preserve all four alternatives, ordering/equality/Debug, and Create/Update selection from the Boolean helper |
| TraceScopeToken | `teaql_runtime/src/graph.cpp`: parent links retain lineage, node indexes and exact typed trace data; recovery reverses traversal order and filters only Rust-whitespace-empty comments; checked Context borrowing and explicit destination copies replace Arc ownership |
| Plan item and batch | Retain entity/kind/values/update fields/item index/token/old snapshot and every derived comparison/Debug field; batching must preserve item order and copies |
| GraphMutationPlan | Preserve optional planned root, items/batches/counter/visited identities; push increments the index, rebuild groups adjacent compatible items only, update fields affect only Update grouping, grouped counts use entity/kind, and size/empty/batch count remain distinct |
| sorted_update_fields | Return ordered present keys excluding the specified set, without inventing metadata fields or changing values |
| GraphNode | Preserve entity, mutable values, ordered relations, operation, absent versus empty comment/dirty fields, original snapshot and optional dynamic fields; builders append relations, replace values and retain literal `id` lookup |
| audit_trace_node and trace_scope | `detail/data_service/entity_graph_plan.hpp` and its planner/recursive/command callers implement comment classification, positive typed ID selection and conditional parent-token extension; verify all caller routes, not just direct token recovery |
| ScopedCommentNode | Borrowed stack chain with the same whitespace filtering and root-first reconstruction as persistent tokens; no implicit Context lifetime extension |
| Two original root tests | Hierarchical borrowed/persistent recovery and adjacency-sensitive mutation batching/counts require their exact assertions, independently of production declaration coverage |

## 5.0.6 changes requiring explicit reconciliation

The locked diff from 5.0.5 adds 34 lines and removes two. Both trace recovery
methods change from empty-only filtering to `trim().is_empty()`. GraphNode adds
an optional boxed DynamicFieldValues field, initialized absent, and two private
helpers for audit-node creation and conditional token extension. The derived
Clone, PartialEq and Debug responsibilities consequently also include dynamic
fields; inspecting only the explicit new methods misses that change.

The candidate GraphNode copy constructor copies dynamic fields into its explicit
destination; equality includes absence/presence and content, and graph Debug
includes the field. The dedicated nonempty dynamic corpus below supplies executed Debug and
value-copy evidence; remaining component distinctions still need reconciliation. The actual dynamic graph-save tests include a nonempty graph copied
into Global, deletion of the source UserContext and inspection of retained
values/definitions through the destination. They cover retention but do not
by themselves prove every graph-model equality/Debug case.

The 128-case `runtime_graph_model_5_0_6_cases.json.gz` replay uses the actual
locked Rust types and their derived formatting/comparison. Its driver builds
GraphNode without assigning dynamic_fields, so that corpus covers the absent
case only. Do not describe it as complete nonempty dynamic model evidence.
The separate `runtime_graph_5_0_6_cases.json.gz` has 464 cases; its scope is graph
operations/plans rather than the ten-model formatting corpus. Case counts are
inventory facts until the current binaries execute them successfully.

The newly accepted SQLite allocation-trace source supplies actual planner/save
witnesses for absent/empty/ASCII/Unicode blank comments and U+001C..U+001F local
reasons, positive IDs assigned after planning, both logging modes, safe audits
and physical SQL traces. This complements direct model tests; it does not
replace reference/remove routes, raw structural diagnostics or standalone
ScopedCommentNode behavior.

## Remaining qualification work

- Reconcile both original root test bodies, all public/derived model operations
  and the new helpers against the current candidate and retained actual-source
  observations. Preserve all existing Debug, ordering, batching and lifetime
  requirements rather than limiting acceptance to the small textual diff.
- Establish nonempty dynamic-field clone/equality/Debug coverage or clearly
  identify which actual existing source/candidate witness proves each aspect.
  The dedicated 14-case corpus below covers nonempty values and Debug; do not
  infer independent definition/selection coverage from it.
- Verify direct borrowed/persistent whitespace filtering and positive-ID scope
  behavior across the real graph planning, recursive write and command routes,
  using source-shaped values without test-side fallback lineage.
- Consume the current frozen native integration result and relevant imported
  consumer/lifetime evidence. A successful whole-native run alone does not
  supply missing original observations or broaden a narrow witness.
- Record exact source/input hashes, remaining adaptations and current results
  before changing the source disposition. Keep service graph/entity_save and
  runtime-root acceptance independent.

The full native integration epoch is now terminal with 866/869 passing. This
review makes no source promotion; scoped passing evidence remains distinct from
complete integration acceptance.

## Original root assertion reconciliation

The initial review searched the graph-named fixtures and missed the existing
`tests/parity/teaql_runtime/policy_graph.cpp` source port. Repository-wide source
inspection corrects that omission; no duplicate fixture is needed. The original
source contains 17 assertion sites, six for hierarchical recovery and eleven for
batching/counts. The existing port preserves every observation:

| Original test | Existing direct witness |
| --- | --- |
| Borrowed trace recovery | `test_hierarchical_trace_chain_recovery`: User/Create User/Some(1), Profile/Create Profile/None, empty AuditLog tail; exact length two and full node equality in order |
| Persistent token recovery | The same exact nodes and order through tokens indexed 0, 1 and 2; Context-owned tokens replace Arc ownership |
| Plan batching | `test_graph_mutation_plan_batching_keys_and_counts`: the original six User/Profile pushes, name/email fields, four batch keys in order and item counts 2/2/1/1 |
| Grouped counts | Exactly three keys: User/Create=2, User/Update=3 and Profile/Create=1 |

`graph_contracts` additionally makes an explicit Global copy, deletes the source
UserContext and verifies retained batch fields and their allocator. This is an
ownership adaptation, not a replacement for the original assertions.

The immutable full-native checkpoint
`parity/native_provider_closure_terminal_5_0_6.json.gz` reports
`original_runtime_policy_graph_contracts` and `runtime_graph_contracts` passed.
The policy/graph fixture SHA-256 remains
`d4ca62783f5d0bf3fb70579f5b1b1539daf632e6eb3f6883fbf7833e12749eb6`, identical
to the frozen input snapshot. That full checkpoint failed three other tests;
its passing individual cases are usable scoped evidence, not a whole-suite pass.
Later compact-fetch changes must be considered when assembling final current
source qualification.

The 464-case graph replay (108 node, 324 plan, 32 module-graph cases) and 128-case
model replay are complementary broad observations. Their inputs do not directly
repeat the original User/Profile/AuditLog fixture, which is covered by the direct
source port above. Do not infer a missing original-test port from corpus naming.

## Existing nonempty dynamic model evidence

`tests/parity/dynamic_graph_debug_probe.cpp` and
`dynamic_graph_debug_5_0_6_cases.json` provide 14 actual locked-Rust comparisons,
separate from the absent-only 128-case model corpus. They cover absent and
present-empty carriers, NULL and text values, unbound/bound definitions,
normal/pretty GraphNode Debug, explicit Global clone equality and an independent
source-value assignment that does not alter the copy. Singleton dynamic fields
avoid Rust HashMap diagnostic ordering without normalization.

The frozen native checkpoint records `dynamic_graph_debug_rust_parity` passed.
Its probe SHA-256 is
`3b63258bee9827e0073044b31bf0f77991e4c4630d04c6f11984856f7e06169c`; corpus
SHA-256 is `4847ef316016be046e181bb43c781ace66f60be0d768df2d56a31cf7f8880b74`.
Both match the frozen input snapshot and current files. This removes the earlier
uncertainty about any nonempty Debug/equality evidence. It does not prove
independent definition/selection inequality or a recursive dynamic child, which
this driver does not construct. Existing graph-save lifetime evidence separately
covers source Context destruction after a retained dynamic graph copy.

## Nonempty dynamic model qualification cases

The referenced carrier is `teaql-core/src/dynamic_fields.rs::DynamicFieldValues`,
not a runtime-local duplicate. Its derived equality covers definitions,
selected-code geometry and row values; the C++ carrier's explicit equality
also compares those three components. GraphNode then compares absence/presence
and the carrier content. The outstanding witness must exercise each component,
not merely compare the row-value maps.

Use the existing 14-case corpus for its stated observations. Reconcile or extend
the remaining component distinctions against this checklist:

1. Absent versus present-empty dynamic state, plus a nonempty selected field
   whose value is NULL and a selected field with a non-null value.
2. A faithful copy with equality true; independently changed definitions with
   unchanged values/selection and changed row values with equality false.
   Selected-code geometry tracks actual value keys through all public constructors,
   assign/delete and merge operations. Do not manufacture an invalid carrier with
   identical value keys but different selection. Use selected NULL versus an
   unselected field, and deletion returning to the empty view, for this boundary.
3. A recursive child carrying dynamic state, exercising GraphNode's relation
   copy and comparison rather than only the top-level box.
4. Raw structural Debug showing the `dynamic_fields` field and its carrier
   components for present state. Rust uses hash collections internally: use
   singleton collections for exact text comparisons, and do not normalize away
   meaningful field content to compensate for unspecified multi-key ordering.
5. Explicit source-UserContext to Global copy, followed by source close and
   destination inspection of definitions, selected codes, values and nested
   allocation resources. Original Arc ownership is not permission to extend a
   C++ source Context implicitly.

`runtime_graph_model_probe.cpp` currently constructs its nodes through
`runtime_graph_probe.cpp::node`, which never installs dynamic state. Its
`node_clone_equal` and `node_changed_equal` observations therefore remain
absent-state evidence despite using the production operators. The existing
nonempty graph-save retention witness remains complementary evidence, not a
substitute for the independent equality and formatting cases above.

## Supplemental recursive ownership fixture

`runtime_graph_tests.cpp::dynamic_graph_components` now constructs a child with
one selected NULL dynamic field, compares a definition-only revision change,
distinguishes absent/present-empty/selected-NULL payloads, and copies a parent
with that child into Global. It mutates and deletes the source child's value,
checks recursive equality and selection/value synchronization, closes the source
Context, then inspects the destination definitions, selected string allocator,
NULL value and arena ownership. Default PMR remains disabled by the surrounding
fixture. Native qualification passes the supplemental runtime graph fixture, the original
policy/graph fixture and the 14-case dynamic Debug comparison (3/3 CTest entries)
in `parity/runtime_graph_components_native_5_0_6.json.gz`. Imported-module and
sanitizer checks are in progress. This fixture is a C++ ownership and model
composition check, not a newly executed original Rust oracle.

### Supplemental qualification result

Native passes 3/3 selected entries. The first imported attempt stopped at an
unregistered `original_policy_graph_tests` target; its failed receipt is retained
as `parity/runtime_graph_components_modules_sanitize_5_0_6.json.gz`. The original
policy/graph source fixture was then registered in `modules/contracts.json` and
the modules preset without changing its assertions. The repaired receipt
`parity/runtime_graph_components_modules_sanitize_repaired_5_0_6.json.gz` records
three passing imported commands (including 14/14 dynamic Debug observations),
1,137 canonical consumer/fixture bodies and focused ASan/UBSan 1/1 with leak
scanning disabled. All verification uses stable preset build directories and
creates no disposable binary copies. The new recursive test and its input
fingerprints are retained in these receipts. Source review remains pending until
the complete graph model/helper responsibility reconciliation is finished; these
results do not promote unrelated source files or complete P0-P7.

## Remaining legacy scope caller gap

The locked graph planner uses `GraphNode::audit_trace_node` / `trace_scope`:
nonblank Rust-whitespace comments and strictly positive `try_u64` IDs. However,
`data_service/graph.rs` still has separate retained recursive insert/upsert and
direct-delete methods which construct `ScopedCommentNode` themselves. Those
constructors accept every present comment and convert `Value::I64` using Rust's
`as u64` cast; `ScopedCommentNode::to_trace_chain` subsequently removes
whitespace-empty frames. Do not assume that all callers use the new positive-ID
helper or silently apply its semantics to the legacy methods.

C++ `recursive_graph_write.hpp` and the direct deletion path in
`graph_node_commands.hpp` currently append nonempty comments directly to a
materialized vector and use `graph_trace_id`. This suggests two distinctions
requiring a real execution observation: whitespace-only local frames, and zero
or negative signed IDs. This is a source-review finding, not yet a qualified
runtime mismatch or authorization to change the behavior without comparison.

Input/output inspection of the retained corpora shows why their aggregate pass
counts cannot settle these distinctions:

- `graph_node_command_5_0_6_cases.json.gz`: 1,620 cases, with
  REQUEST_COMMENT_REQUIRED outcomes in 249 node-reference, 250 node-remove and
  216 node-delete cases. Local comments are absent, empty or nonblank strings;
  no whitespace-only local comment is included.
- `recursive_graph_write_5_0_6_cases.json.gz`: 792 cases, with the same early
  intent error in 275 recursive-insert and 347 recursive-upsert cases. Again,
  no whitespace-only local reason is included.

These cases remain valid failure-boundary observations and must not be rewritten
as successful operation evidence. Next, append an access-only observer to an
owned copy of the exact original source, bind a valid MutationIntent through the
real service API, and exercise these methods using absent/empty/ASCII/Unicode
blank/control comments and signed/unsigned boundary IDs. Compare actual command
and query trace vectors, outcomes and call counts against the C++ service with
the same intent. Preserve reference source bodies, keep any new valid-intent
corpus separate from historical inputs, and only repair demonstrated differences.

### Valid-intent original observation

The independent `scripts/observe_legacy_graph_intent_5_0_6.py` observer now
executes the unchanged locked runtime with a valid mutation intent. Its
[original receipt](parity/legacy_graph_intent_original_5_0_6.json.gz) retains all
72 inputs, raw observations, build commands, appended access wrappers and
byte-checked original source provenance. This is supplemental evidence, not a
replacement for the historical failure-boundary corpora. `verify_upstream.py`
confirmed `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` during this run.

| Entry | Cases | Successful outcomes | Mutation calls | Query calls |
| --- | ---: | ---: | ---: | ---: |
| Direct delete | 24 | 16 | 16 | 0 |
| Recursive insert | 24 | 24 | 24 | 24 |
| Recursive upsert | 24 | 24 | 24 | 40 |

The eight direct-delete failures are zero-ID rejection before provider access.
Comments cover absent, empty, ASCII space, tab/newline, U+2003, U+0085, U+001C
and nonblank text. IDs cover U64(7), U64(0) and I64(-7). The observer now records
`entity_id` in every trace frame; the historical recording driver omitted it.
Actual original requests establish that:

- Blank comments contribute no local audit frame, but the ancestor survives.
  U+001C is retained as a nonblank reason.
- A nonblank legacy audit frame for I64(-7) carries U64(18446744073709551609).
  The separate operation entity frame has no converted negative identity.
- In recursive insertion for U64(0), the local audit frame retains ID 0 while
  the operation entity frame carries the allocated ID 100. Upsert delegation
  preserves this distinction too.

The owned source view and build target (168,119,445 bytes) were removed after
successful observation; the compressed receipt retains the useful evidence.
No production C++ body or historical corpus was changed in this step. A matching
C++ valid-intent observer and comparison remain required before repairing these
callers or claiming parity. These 72 cases do not exhaust graph responsibilities,
recursive child composition, provider failures or boundary IDs; this source
remains pending review and P0–P7 remains incomplete.

### C++ legacy scope repair

The C++ recording fixture now accepts an explicit `bound_intent` input and, for
that supplemental mode, records trace entity IDs. Inputs without that field keep
the historical behavior. The independent comparator checks the complete output;
it does not normalize trace frames, IDs, events, queries or outcomes.

The [before-repair observation](parity/legacy_graph_intent_cpp_before_5_0_6.json.gz)
matched only 30/72 cases. All 42 differences were in mutation-request and audit-
event trace vectors: ten direct-delete cases, sixteen recursive-insert cases
and sixteen recursive-upsert cases. Other top-level observations matched.
The three historical parity tests still passed, confirming the coverage gap.

The repair uses Rust-compatible blank-comment filtering at the two legacy
materialization sites and a separate `graph_legacy_trace_id` helper that retains
zero U64 values and casts I64 values to U64. The existing planner's positive-only
`graph_trace_id` remains unchanged. The new helper has module facade, catalog and
isolated lookup entries; no ownership or allocation domain changed.

[Repaired observations](parity/legacy_graph_intent_cpp_repaired_5_0_6.json.gz)
match all 72 original outputs. The new repeatable `legacy_graph_intent_rust_parity`
CTest and historical recursive-write, node-command and graph-execution parity
CTests pass **4/4**; the [native gate receipt](parity/legacy_graph_intent_native_gate_5_0_6.json.gz)
binds source hashes and JUnit evidence. Formatting/checking passed for 1,800 files;
the export verifier passed for 1,521 declarations. The temporary JUnit file was
removed after preserving it in the receipt. Stable native build outputs remain
in the committed preset directory.

This is focused native qualification only. Module execution of these new inputs,
broader recursive composition and error/boundary cases, and complete graph-source
acceptance remain outstanding. The earlier failed full-native checkpoint remains
failed; these four tests do not supersede that complete integration gate.

### Module closure and extended legacy execution

The canonical graph recording fixture is now registered as
`teaql_module_graph_execution_probe`, importing only `teaql_runtime`. Its fixture
bodies remain unchanged by the module adaptation; the consumer verifier now
checks 1,139 consumer/fixture bodies. The module target is included in the
committed module build preset. Five parity CTests are registered in both native
and module configurations, including the two valid-intent corpora.

The [module comparison](parity/legacy_graph_intent_modules_5_0_6.json.gz) matches
the original 72 cases. A separate [extended original observation](parity/legacy_graph_intent_extended_original_5_0_6.json.gz)
adds 284 cases without replacing that corpus:

- 216 provider-failure, event-failure and read-failure variations of the initial
  inputs. Failures that are not reached remain observable successes or earlier
  validation errors; no test treats an unexecuted failure hook as coverage.
- 60 boundary-ID and Unicode-comment cases covering signed zero, I64 extrema,
  U64 maximum, U+2028, U+2029, U+00A0, U+200B and nonblank text, with a blank
  ancestor frame after a nonblank ancestor.
- Eight one-child recursive compositions across insert/upsert, to-one/to-many,
  and blank/nonblank child reasons. Each executes two mutations. To-one writes
  Child then Order; to-many writes Order then Child. Insert executes two queries;
  upsert executes four. Relations deliberately use `attach=false`, so these cases
  qualify trace composition and execution order, not foreign-key attachment.

Across the extended original inputs, outcomes are 79 successes, 64 provider
failures, 64 event-delivery failures, 48 read failures and 29 zero-ID delete
rejections. [Native](parity/legacy_graph_intent_extended_native_5_0_6.json.gz) and
[module](parity/legacy_graph_intent_extended_modules_5_0_6.json.gz) comparisons both
match **284/284** full observations. No additional production repair was needed.

The [closure receipt](parity/legacy_graph_intent_module_closure_5_0_6.json.gz)
retains native **5/5** and module **5/5** CTest results, build output, source hashes,
canonical-body verification and the 1,521-export check. Original Rust bodies were
byte-checked; its disposable view/build tree (168,535,479 bytes) was reclaimed.
Temporary JUnit outputs were removed after copying their contents into the
receipt. Stable preset build directories were reused. No new sanitizer or MIPS
claim is made: this continuation changed observation/registration coverage and
exercised the prior trace-value repair.

These gates close the identified legacy trace comparison gap at this scope.
They do not establish complete graph-source acceptance, foreign-key attachment,
all recursive graph shapes, or a replacement passing full-native checkpoint.
The seven pending source dispositions and six partial generator capabilities
remain unchanged; P0–P7 remains active.

### Original recursive dynamic-component observation

The former C++-only supplemental fixture now has a matching observation against
unchanged original Rust model operations. The appended test
`tests/parity/runtime_graph_components_source_5_0_6.rs` exercises the following
through the real `GraphNode` and `DynamicFieldValues` implementations:

- Definition-only revision changes retain equal values and selected codes but
  make graph equality false.
- Selected NULL differs from an unselected field; absent payload differs from
  present-empty payload.
- A parent/child clone compares equal and produces equal structural Debug.
  Mutation of the original child's value changes recursive equality without
  changing the copy. Deletion restores the public empty selection/value state.
- Dropping the original graph leaves the cloned child's selected NULL intact.
  Rust's owned clone establishes semantics; C++'s explicit Global clone and
  post-UserContext-close allocator checks establish its ownership adaptation.

The [original receipt](parity/runtime_graph_components_original_5_0_6.json.gz)
records **4/4** passing tests: both unchanged original graph root tests, this
supplemental test, and the unchanged ledger-chain fallback test selected by the
`graph::` filter. It retains the exact controller, observer, original source
hashes, locked dependency provenance and command output. All 67 original source
and test bodies in the derived view remain unchanged. The extra ledger test is
reported explicitly, not counted as another graph-model root test.

[Current candidate replay](parity/runtime_graph_components_current_native_modules_5_0_6.json)
records **2/2 native** and **2/2 module** graph/policy contracts and binds the
fixture hashes. An initial native command named a nonexistent target, then the
registered target was selected successfully; this setup correction is retained
in the receipt. No C++ function bodies changed in this continuation. Previous
focused sanitizer evidence remains historical; no new sanitizer run is claimed.

The owned Rust view/build directory (166,325,906 bytes) was removed after success.
This observation closes the missing original witness for the stated dynamic
component comparisons. It does not replace the exact 14-case cross-language
Debug corpus or prove all private planner-helper routes. Complete source review,
remaining service/save responsibilities and current full integration acceptance
are still pending.

### Public planner identity/comment matrix

`observe_graph_planner_scopes_5_0_6.py` now reuses the original public
`EntityDataService::plan_graph` recording driver in an append-only checked source
view. It observes complete planned roots, values, operations, item and batch
structure, token indexes, typed trace frames, visited identities, allocation
counter and provider requests. Timestamp fields are absent, so the historical
driver's current-time rendering branch is not exercised. No result normalization
or test-side implementation of `audit_trace_node` / `trace_scope` is introduced.

The existing 336-case `graph_plan_scopes_5_0_6_cases.json.gz` already covers root
and child Reference/Remove scopes. It remains intact and is replayed alongside
the new corpora; the new work must not be described as the first scope evidence.

- [Create-root original](parity/graph_planner_scopes_original_5_0_6.json.gz):
  896 observations across four child operations, fourteen ID shapes, eight
  absent/blank/control/nonblank reasons and both provider-existence settings.
  All plans succeed and no provider query occurs. The existence flag is thus
  inactive in these cases. Reference children inherit only the parent token;
  each other child operation produces 140 parent-only and 84 extended traces.
  [Native](parity/graph_planner_scopes_native_5_0_6.json.gz) and
  [module](parity/graph_planner_scopes_modules_5_0_6.json.gz) match 896/896.
- [Upsert-root original](parity/graph_planner_upsert_scopes_original_5_0_6.json.gz):
  a separate 896 observations issue 976 queries. There are 448 successful plans
  and 448 errors reporting a missing authoritative `Parent.version` field.
  The historical provider fixture returns only `id` when `exists=true`; these
  errors are genuine readback validation evidence, not successful persisted-
  parent planning. [Native](parity/graph_planner_upsert_scopes_native_5_0_6.json.gz)
  and [module](parity/graph_planner_upsert_scopes_modules_5_0_6.json.gz) match every
  complete observation, including all failures.

Both corpora are registered as repeatable native/module CTests. The original
historical planning and 336-case scope corpora also run through the newly
registered canonical module planner observer. The [gate receipt](parity/graph_planner_scopes_gate_5_0_6.json.gz)
records **native 4/4, module 4/4**, 1,140 canonical consumer/fixture bodies,
1,521 exports, baseline verification and input source hashes. No production C++
body changed. Both owned Rust view/build trees were removed after consumption,
each 168,698,686 bytes; temporary JUnit outputs were reclaimed after retention.

The next unresolved execution boundary is a persisted parent whose provider row
contains all mapped scalar fields, allowing planning to continue into child
Upsert and the other operation paths after the parent existence query. Extend
the recording provider inputs explicitly for this case on both sides, preserve
the historical sparse-row fixture, and compare actual execution rather than
turning the 448 failures into inferred success. No source disposition is promoted
by this matrix; complete graph/service/save review and integration remain open.

### Complete-row persisted-parent planner continuation

The recording providers now accept explicit `rows` inputs when `exists=true`.
Absent `rows` preserves the historical ID-only response, including its genuine
readback failures. Rust's original driver file is unchanged: the owned observer
adapts only the mock provider's field, input assembly and row return; original
runtime methods remain byte-for-byte intact. The C++ recording fixture accepts
the same row payload without changing production functions.

The new [complete-row original observation](parity/graph_planner_complete_rows_original_5_0_6.json.gz)
provides `id`, `version` and `name` through that SPI. All **896** plans succeed and
issue **1,056** queries. The resulting items include 816 Create, 528 Update,
224 Reference and 224 Delete entries. In contrast, the immutable sparse-row
Upsert corpus still contains its 448 missing-`Parent.version` failures. The
complete-row observation therefore reaches persisted-parent child planning
instead of assuming what would happen after early validation.

[Native](parity/graph_planner_complete_rows_native_5_0_6.json.gz) and
[module](parity/graph_planner_complete_rows_modules_5_0_6.json.gz) match **896/896**
full observations, including fields, token indexes, optional positive trace IDs,
query provenance and planned mutation kinds. The explicit mock row's ID remains
999, as in the historical provider, even for a differently filtered ID; this is
a controlled service-SPI observation, not evidence that live SQLite returns a
row that violates its WHERE predicate. No SQL or persistence claim is inferred.

The new corpus is registered in both CTest profiles. The [current gate receipt](parity/graph_planner_complete_rows_gate_5_0_6.json.gz)
records native **5/5** and module **5/5**, replaying all three planner extensions
and both historical planning corpora. It retains source hashes, complete JUnit,
1,140 canonical consumer/fixture checks, 1,521 exports and baseline verification.
Pinned formatting and checking passed for 1,800 handwritten C++ files. The owned
Rust view/build tree (168,965,152 bytes) and temporary JUnit files were reclaimed.

This closes the specific persisted-parent scope boundary identified above.
G17 now links these planner/legacy/model observations explicitly, retaining its
partial status. Complete graph-model responsibility reconciliation, separate
service/save source reviews and a current passing full integration gate remain
required before source or capability promotion.

### Complete model responsibility reconciliation

The [twelve-group responsibility inventory](runtime_graph_responsibilities_5_0_6.json)
now maps the complete 563-line source, including trait-derived behavior and both
original tests, to inspected candidate functions and witnesses. It records
explicit collection/ownership adaptations, literal ID lookup, builder append
semantics, adjacent batching, Debug overflow behavior, independent batch/count
state, raw Debug scope and unordered visited-node diagnostics. This inventory
supports review; it is not an automatic acceptance declaration.

The 128-case model comparator and independent model lifetime fixture lacked
registered module consumers. Both are now registered, importing `teaql_runtime`,
and included in the module build preset. The 464-case module graph comparison
already existed through generic registration; an initial duplicate-test CMake
failure was corrected by retaining that existing entry. The failed setup is
recorded in the module receipt, not relabeled as a passing first attempt.

[Current native model gate](parity/runtime_graph_model_native_closure_5_0_6.json.gz)
and [current module model gate](parity/runtime_graph_model_modules_closure_5_0_6.json.gz)
both pass **6/6**: the 464-case graph, 128-case model and 14-case dynamic Debug
comparisons, the two original graph root tests in the policy/graph fixture,
and the general/independent model lifetime fixtures. Candidate source hashes
remain identical across the two gates. Canonical verification covers 1,143
consumer/fixture bodies and 1,521 exports. Production and test function bodies
were unchanged in this registration/review continuation. Owned temporary JUnit
files were reclaimed after their contents were retained.

The focused graph-model review and these current model gates are reconciled.
A complete frozen native integration epoch is now the next gate before changing
this source's pending disposition. Separate graph-service/entity-save/provider
reviews and the six partial capability decisions are not discharged by this
model-level reconciliation.

### Fresh complete-model replay after the committed checkpoint

The new development controller
`scripts/verify_runtime_graph_source_closure_5_0_6.py` binds the complete source
review to fresh original execution and canonical imported consumers. It prepares
an owned, byte-checked runtime view, preserves all 67 original source/test files,
and appends only the already reviewed dynamic-component observer. The three
existing executable drivers retain their evaluator bodies; only the model
driver's fixture module path is made absolute in the disposable copy. No Rust
implementation, C++ function, generated SDK or producer source is changed.

The [fresh original receipt](parity/runtime_graph_source_original_closure_5_0_6.json.gz)
passes all **606** complete observations: 464 graph operations/plans/module-graph
cases, 128 model formatting/comparison cases, and 14 nonempty dynamic Debug
cases. The unchanged original hierarchical-recovery and batching/counts tests
also pass, as does the supplemental recursive dynamic-component test. The
`graph::` selection additionally runs the unchanged service ledger-fallback
test; its fourth result is reported separately, rather than counted as another
graph-model root test. Original/corpus/driver hashes, commands, complete outputs
and dependency provenance are retained.

The [fresh imported receipt](parity/runtime_graph_source_module_closure_5_0_6.json.gz)
passes **14/14** CTests. These replay the 606 model observations, the original
policy/graph fixture and both model ownership fixtures, plus the four planner
scope corpora, two valid-intent legacy corpora and recursive/command corpora.
The eleven differential entries compare **6,398** complete outputs; the three
fixture entries keep their actual assertion scope. This does not claim that all
6,398 original outcomes were regenerated in this continuation: the helper-route
corpora retain their independently recorded locked-source observations. Current
model and helper implementation hashes are frozen together by the new receipt.
Exact CTest selection, zero skips/failures, successful terminal builds and
no-work checks before/after execution are retained. Canonical verification still
covers 1,163 consumer/fixture bodies.

The original view, source/binary targets and module JUnit directory were removed
after their child processes exited: **273,207,403** and **3,861** bytes respectively.
Stable native and module preset directories are reused. No sanitizer or MIPS
rerun is claimed for this controller/documentation-only change.

The committed pre-commit native attempt remains explicitly interrupted. A fresh
full native epoch uses
`parity/runtime_graph_model_full_native_inputs_5_0_6.json.gz` and
`parity/runtime_graph_model_full_native_terminal_5_0_6.json.gz`. It finished with
**876/877** tests passing, one failure, zero errors and zero skips. Builds and
all 11,412 frozen-input checks passed. The only failure was the obsolete stream
startup expectation rewrite described in the
[strict-source readback](protected_stream_shapes_5_0_6.md).
This remains a failed integration result. The source disposition stays
**pending_review** until a successful complete integration result and the
source-specific acceptance reconciliation are recorded.
Current source totals remain **118 accepted / 4 pending_review /
2 excluded_external**; capabilities remain **11 qualified / 6 partial**.
