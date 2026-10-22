# Runtime entity save source review (Rust 5.0.6)

Current closure: [complete ES01-ES18 acceptance](entity_save_source_acceptance_5_0_6.md).
Fresh original17+5576, native25/module25 and 9,277 complete observations per C++
frontend pass, with canonical1,159 and format1,805. Source totals are now117/5/2;
capability11/6 and P0-P7 remain incomplete. Earlier pending language and failed
or interrupted gates below describe their original checkpoints.

Status: accepted. This review covers the 2,206-line
`teaql-runtime/src/entity_save.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`ff04761be7223d57174a3ce3ed315f87a86ec728e48117e2a3ffb229a6427e5b`.
The graph model and service graph executor are separate source responsibilities.
This initial responsibility map does not establish complete candidate equivalence.

## Source responsibilities and candidate navigation

| Source responsibility | Candidate location | Boundary requiring qualification |
| --- | --- | --- |
| Fix time and evidence task scope | `detail/graph_fix_scope.hpp`, `graph_fix_tests.cpp`, graph-fix oracle | One operation's time/evidence must survive async polling without owning Context data in TLS; restore outer scope and publish evidence on success/failure |
| `DynGraphSaver` and `GraphSaverFor` | `detail/data_service/graph_saver.hpp` | Trusted Context executor lookup; keep graph and ledger capabilities distinct |
| Graph save transaction | `graph_saver.hpp` | Validate intent and review complete immutable policy plan before begin; collect events, commit before delivery; rollback execution failure |
| Ledger save transaction and authoritative readback | `graph_saver.hpp`, `ledger_execution_outcome.hpp` | Bind ledger privacy and intent; apply generated identities/dynamic readbacks; read on the transaction-owned executor before commit; clear only committed ledger state |
| Typed saved-row decoding | `entity_save.hpp::install_saved_dynamic_fields`, audited save operations | Preserve loaded relations only when eligible; require indexed state before dynamic installation; retain decoder errors |
| Cancelled new root | `entity_save_preparation.cpp` | Create-then-delete cannot fabricate a persisted return entity |
| Loaded relation preservation | `entity_save_preparation.cpp` | Reject reuse after new/deleted keys, other-entity dynamic changes, other-entity scalar changes or changed local relation keys |
| Entity extraction | `entity_save.hpp` | Capture loaded scalar list, dirty fields, old snapshot, dynamic state and comment before consuming values; deletion clears child graph |
| Values-to-graph conversion | `entity_save_preparation.cpp` | Interpret modeled relations and reserved mutation markers without merging scalar, snapshot and mutation types |
| Relation mutation merge | `entity_save_preparation.cpp` | Create sets values; Upsert uses explicit dirty fields only; Remove marks deletion; Reference does not stage writes; preserve original versions and recurse |
| Ledger scope capture | `entity_save_preparation.cpp` | Require root request intent; deterministic first-route ancestry; explicitly composed pending items; unassigned scope ownership; fresh capture on retry |
| Relation hydration | `entity_save_preparation.cpp` | Loaded snapshot and ledger composition, visited identities, deletions and descendants must be reconciled without inventing implicit database reads |
| Graph and pending dynamic preflight | `entity_save_preparation.cpp` | Typed Checker/Fix paths, dirty/loading evidence and skipped/deleted members; independently pending dynamic entities use their actual model location or root fallback |
| Ledger object locations | `entity_save_preparation.cpp` | Deterministic first model-relative path, root empty path, entity-qualified identity |
| Generic `AuditedSaveExt` | `detail/data_service/audited_auto_save.hpp` | Dynamic carriers route to ledger save; missing carrier is an error; ordinary graph path remains separate |
| Generated ledger entry | `detail/data_service/audited_save.hpp` | Composition errors precede execution; hydrate/preflight/merge/capture ordering; preserve relations only with sufficient unchanged-state evidence |
| Explicit-executor ledger entry | `detail/data_service/audited_save.hpp` | Never commit or rollback caller-owned transaction; return retryable ledger ownership to the caller; authoritative readback still uses supplied executor |
| Error conversion | Saver and audited save operations | Preserve runtime errors; wrap provider errors as graph errors; committed audit delivery failure must not become a retryable rolled-back write |

Paths above are relative to `teaql_runtime/include/teaql/runtime` unless a
`.cpp` is named; preparation is `teaql_runtime/src/entity_save_preparation.cpp`.
These are implementation navigation links, not a claim that every listed branch
has already passed source-level acceptance.

## Original tests and existing witnesses

The original file contains 17 tests: relation snapshot preservation; transaction
readback race; failed readback rollback; authoritative soft-delete tombstone;
failed commit preserving retryable ledger; post-commit audit failure; cancelled
new root; policy denial before begin; six ledger ancestry/retry/allocation/
composition tests; model/JSON locations; missing-intent ordering; and typed
Checker dirty-field preservation.

At the start of this review, `tests/parity/teaql_runtime/entity_save.cpp` contained
eight correspondingly named ports, including readback/race, rollback, tombstone, cancellation, policy denial,
relation-state preservation, location and dirty-field contracts. Additional
witnesses exist in request trace, commit audit, dynamic preflight, generated save
and SQLite allocation-trace tests. Exact-name search alone neither proves a
missing port nor establishes semantic coverage. Reconcile each original assertion
with the actual candidate test behavior before source promotion.

The retained `audited_save`, `audited_auto_save` and `graph_fix` oracle corpora
are complementary. Their acceptance must be assessed for which branch was
actually reached; an intent/preflight error cannot prove the transaction or
post-commit branch that follows it.

## Current review boundary

The [fresh unchanged-original replay](parity/entity_save_original_tests_5_0_6.json.gz)
passes all **17/17** source tests independently of the frozen full-native
integration epoch. Its append-free runtime view preserves all
67 original source/test files, with locked dependency provenance and complete
command output retained. Its task-owned temporary view/build tree
(166,272,251 bytes) was removed after success.
No candidate implementation, frozen JSON ledger, generator or execution registry
is modified during that epoch. A passing original replay alone will establish
reference behavior, not C++ parity or complete entity-save acceptance.

Next reconcile the seventeen source tests, then the public/private production
responsibilities above, including generic versus ledger versus caller-owned
transaction paths. Preserve failures and committed-operation semantics explicitly.
Graph-model closure, SQLite-only scope and compact ownership adaptations do not
permit reducing any of these responsibilities.

## Commit boundary assertion reconciliation

Read-only inspection of the exact original tests and candidate fixtures reveals
an important distinction between configured fault injection and reached failure.
The [inspection receipt](parity/entity_save_commit_boundary_review_5_0_6.json.gz)
records corpus and source hashes, exact case indexes and outcome distributions.
It is analysis of retained observations, not a new execution claim.

The 960-case `graph_saver_5_0_6_cases.json.gz` contains 440 inputs with commit
failure enabled. Of those, 400 stop at missing intent; other early outcomes are
readback, cancellation, identity and version errors. Only six actually return
`graph write error: commit failed`. Those six are ledger saves of new entities,
retain pending/new keys and deliver zero events. They establish the new-entity
commit-failure boundary, not the original persisted-update fixture.

Twenty-four inputs configure event failure. Eight reach
`AUDIT_DELIVERY_AFTER_COMMIT`, with one attempted event after the commit note:
four graph paths and four ledger paths. The ledger paths clear pending/new keys;
the graph paths leave their unrelated seeded ledger alone. Remaining configured
failures stop earlier and are not evidence of post-commit delivery.

The candidate fixtures provide complementary concrete assertions:

- `graph_saver_tests.cpp::ledger_completion_and_errors` retains new-key state on
  failed commit and checks consumed-transaction cleanup. It does not install the
  audit sink needed to assert the full original persisted-update conjunction.
- `commit_audit_tests.cpp` asserts no delivered facts on explicit transaction
  commit failure, including a suspended terminal operation. It is not itself an
  automatic persisted-update ledger save.
- `commit_audit_transaction_tests.cpp::automatic_ledger_save` uses actual SQLite to
  update an existing row, inject sink failure, assert `AuditAfterCommit`, assert
  an empty ledger and read back the committed name/version. This is concrete
  committed-update evidence, separate from the original mock's call assertions.
- The source-port file `tests/parity/teaql_runtime/entity_save.cpp` currently
  contains eight named original ports, but not the two named commit-failure and
  audit-failure tests. This observation does not deny the complementary fixtures
  above.

Before complete source acceptance, explicitly reconcile or add the original
persisted-update failed-commit conjunction: the commit failure is reached, its
error is retained, audit delivery is absent, and pending scalar changes remain
retryable. Also retain the post-commit structured error/code and no-rollback
observations when consolidating the corresponding audit test. Do not weaken
these assertions or convert early failures into downstream coverage. Candidate
edits remain deferred while the full-native input snapshot is frozen.

## Typed NULL scope ownership: original observation

The [eight-case original observer](parity/ledger_typed_null_scope_original_5_0_6.json.gz)
completed successfully against the locked Rust 5.0.6 source. It preserves all
original implementation bodies and appends a test-only observer. The matrix
crosses graph-discovered children and standalone pending new keys with ordinary
NULL, typed text NULL, signed zero and unsigned zero. Every child has its own
audit reason under Parent ID 10; the supplied allocation maps the child key to
ID 200.

| Input child ID | Before enrichment | After enrichment, both discovery paths |
| --- | --- | --- |
| Ordinary NULL | No child trace ID | Child trace ID 200 |
| Typed text NULL | No child trace ID | No child trace ID |
| Signed zero | No child trace ID | Child trace ID 200 |
| Unsigned zero | No child trace ID | Child trace ID 200 |

Parent ID 10, both audit reasons and both trace kinds are preserved in all eight
observations. This establishes actual original behavior, rather than inferring
it only from the Rust pattern matching expression.

Candidate inspection identifies a discrepancy in
`teaql_runtime/src/entity_save_preparation.cpp`: the local `unallocated` predicate
calls `Value::is_null()`, which includes typed NULL, for both graph and standalone
scope registration. The Rust predicate includes ordinary NULL and integer zero
only. The existing graph-saver observer omits trace entity IDs, so its successful
comparisons do not establish this ownership/enrichment contract. A candidate
execution reproducer and correction remain required; no candidate execution
match or source acceptance is claimed here. The approved generated JSON payload
exception does not apply to this independent trace ownership contract.

After the current frozen native integration epoch terminates, add the matching
candidate matrix with trace IDs and preserve the failing observation before
narrowing the local unallocated-ID predicate. Keep general `Value::is_null()`
semantics unchanged. Verify both discovery paths and ordinary NULL/zero controls,
then run relevant native/module and lifetime checks for scope enrichment.

The observer's owned temporary source/build directory was removed after recording
its evidence (166,298,934 bytes). No candidate or generator input was changed
while the full-native epoch was running.

### Transaction state machine reconciliation

Read-only inspection of `TransactionalGraphSave::poll` in
`detail/data_service/graph_saver.hpp` confirms the intended ordering: an execution
success starts commit; an execution error starts rollback. A terminal provider
error returns before ledger clearing or audit delivery. A successful rollback
returns the saved execution error; only successful commit reaches
`clear_committed()` and `deliver_committed_audits()`. This matches the original
Rust branches, including rollback-error precedence over the execution error.
The delivery helper wraps the first sink error as `AuditAfterCommit` after
attempting the queued events. These are inspected implementation facts, not a
replacement for the missing combined persisted-update regression.

The direct source-port mock in `tests/parity/teaql_runtime/entity_save.cpp`
always succeeds in `Tx::commit_transaction`; it changes the mock row to simulate
an unrelated writer after commit. Thus simply invoking its current fixture
cannot exercise the original `simulate-commit-failure` path. The required test
extension must introduce an explicit commit-failure branch before that simulated
writer, retain the original success behavior, and install an event sink while
asserting retained scalar ledger changes. The post-commit sink-failure case must
also assert the structured error, diagnostic code and absence of rollback.
Cleanup invokes transaction cancellation as the C++ resource-release protocol;
that must not be mislabeled as a SQL rollback call.

An independent candidate trace probe was considered during the frozen epoch.
A read-only Ninja dependency dry run requested CMake regeneration instead of
reporting the libraries up to date. It therefore did not establish the prerequisite
for consuming those outputs. No candidate probe was compiled or run from them;
keep the trace-ID candidate execution pending until the owning build terminates.

### Frozen integration epoch disposition

The graph-model native integration epoch was deliberately interrupted after the
TypedNull ownership discrepancy was confirmed from original execution and
candidate source review. Its retained terminal receipt records build exit 130;
CTest did not run, so this epoch supplies no full-suite acceptance. Only the
owned Ninja child was interrupted; the controller and wrapper exited, preserving
the build log and reclaiming their temporary directory. Incremental preset build
outputs remain available for the correction. This is an intentional transition
to defect repair, not a timeout, passing integration result or replacement for
the historical 866/869 checkpoint.

### Candidate reproduction and correction in progress

The new eight-case `unallocated_scope_identity` fixture is part of the existing
`request_trace_scope_contracts` executable. Its initial compile exposed a private
method access in the test; switching to the existing `EntityRuntimeAccess`
adapter corrected the fixture without widening the runtime API. The executable
then built successfully and failed on the typed-NULL trace-ID assertion (exit
134), confirming the candidate discrepancy. The
[before-correction receipt](parity/ledger_typed_null_scope_candidate_before_5_0_6.json.gz)
records that first failure without claiming all eight cases completed.

The local unallocated predicate now tests the ordinary NULL variant explicitly;
signed and unsigned zero branches are unchanged. General `Value::is_null()` and
JSON behavior remain unchanged. Post-correction formatting and native verification
are running; module/lifetime gates and source acceptance are not yet claimed.

### Focused native correction checkpoint

The eight-case scope matrix now passes in `request_trace_scope_contracts` after
the local predicate correction. The direct entity-save source fixture now ports
ten original tests, adding the persisted-update commit failure and post-commit
sink failure contracts. Both additions execute through the production saver and
ledger. The commit failure reaches mutation, authoritative readback and the
failing commit, returns its error, delivers no audit, and leaves scalar changes
pending. The sink failure produces `AuditAfterCommit` with its diagnostic code,
records commit and no rollback, attempts one audit, and leaves the ledger empty.

Both exact native CTests passed. An initial incorrect selector matched zero tests;
it is not acceptance evidence. The corrected invocation used
`--no-tests=error` and `original_runtime_entity_save_contracts`. The
[native checkpoint](parity/entity_save_scope_native_checkpoint_5_0_6.json.gz)
retains source hashes and these limits. Formatting/check and the module scope
consumer are running after the test additions; sanitizer and full-source/full
integration qualification remain outstanding.

The [focused sanitizer gate](parity/entity_save_scope_sanitizer_5_0_6.json.gz)
passes **2/2** after the correction: scope contracts including the eight-case
matrix, and all ten direct original save contracts. ASan/UBSan use the committed
preset with leak scanning disabled. Formatting and format checking both pass
for 1,800 handwritten files. Upstream and module-path provenance checks pass.
The sanitizer build reused its committed output directory; no extra disposable
build tree was created. Module execution remains pending at this checkpoint.

### Scope correction and transaction test gate completed

The [final focused receipt](parity/entity_save_scope_closure_5_0_6.json.gz)
records **native 2/2, module 2/2 and ASan/UBSan 2/2**. The entity-save source
fixture is now registered in the canonical module contract manifest and native
module preset; all ten tests run through the same function bodies under imports.
Canonical verification passes for 1,144 consumer/fixture bodies. The final native
run rebuilt the formatted fixture before executing both exact CTest names.
The earlier missing module registration is closed for this fixture.

This closes the TypedNull scope discrepancy and the two named commit/audit test
coverage gaps. It does not accept the complete entity-save source, graph-service
source, remaining capabilities or P0-P7. The interrupted full native epoch remains
interrupted; do not replace its outcome with this focused gate. Build artifacts
remain in the three committed reusable preset directories, with no additional
disposable build tree retained by this continuation.

### Complete original test inventory port, verification in progress

The candidate fixture now contains and invokes all **17** original test names.
The seven additional ports after the ten-test checkpoint cover nested ancestry
with deleted children and equal numeric IDs across entity types; retry rebuilding;
allocated ancestor ownership and retry ID replacement; distinct NULL/zero keys
of one entity type; deleted-child discovery from a composed loaded snapshot;
signed provider IDs matched to unsigned generated references; and missing-intent
ordering for graph/ledger saves with logging enabled and disabled.

Trace comparison checks kind, entity type, optional identity, reason and chain
length. Allocation controls assert inherited siblings do not acquire their own
trace identity, recapture replaces earlier assigned IDs, source graph IDs remain
unchanged and clear removes captured scopes. Hydration fixtures register metadata
without any executor, preserving the original no-I/O contract. Missing-intent
fixtures install a policy registry that throws if called and assert the provider
call list stays empty across all six original missing/Unicode-blank inputs, both
save paths and both logging settings.

The four allocation/ancestry additions first passed native; the complete
seventeen-test fixture then built and passed the exact native CTest. Name/call
inventory reconciliation found no omitted original test. These facts do not
substitute for assertion review or complete production-source qualification.
Post-format native and canonical module execution are running; the earlier
10-test sanitizer receipt is historical and is not relabeled as a 17-test gate.

### Retained corpus execution reachability

The [retained-output analysis](parity/entity_save_retained_corpus_reachability_5_0_6.json.gz)
records corpus hashes, case counts, input modes and reached call notes. It is a
read-only analysis, not a new replay. Unlike the older graph-saver corpus's large
missing-intent subset, the audited-save corpora contain no
`REQUEST_COMMENT_REQUIRED` outcomes: 356 explicit-executor cases include 264
provider-call cases, 232 query-call cases and 244 event-call cases. The 944
plain/automatic cases (472 each) include 747 begin-call cases, 663 provider-call
cases, 338 rollback-call cases, 361 commit-call cases and 352 event-call cases.
These count cases containing a call note, not successful transaction terminals
or total calls. Preserve original error outputs when qualifying their branches.

The 1,597 graph-fix cases divide into 1,344 constructors, 6 scope cases, 72
publication cases and 175 payload cases. Constructor validation volume must not
be presented as 1,597 asynchronous scope executions. The report labels output
shapes conservatively rather than treating every non-error-shaped result as a
successful business save.

Hydration's candidate local-reference predicate already tests the ordinary NULL
variant explicitly and compares foreign/local values through `EntityKey`, matching
the inspected Rust branch. The corrected allocation-scope predicate is separate;
no hydration implementation change is justified by that earlier scope defect.

### All seventeen source tests verified

The [seventeen-contract receipt](parity/entity_save_seventeen_contracts_5_0_6.json.gz)
records the final formatted source hash, all original test names and successful
native, module-import and ASan/UBSan execution of the complete candidate fixture.
Each frontend's one CTest invokes all seventeen contracts. The original replay
remains the separately retained unchanged-source 17/17 receipt. Canonical module
verification passes for 1,144 bodies; formatting/check passes for 1,800 files.
These results close the original named-test inventory, not the production-source
responsibility review. In particular, a newly identified explicit-executor
empty-ledger scope-capture ordering difference needs independent observation.

### Explicit-executor empty-ledger scope discrepancy

The first observer attempted blank audit reasons through `audit_as`; the original
Rust wrapper rejected construction with a panic before the save entry point.
That [initial failure](parity/explicit_save_scope_original_5_0_6.json.gz) is retained
and does not establish a Checker-count mismatch. The corrected observer uses
valid reasons and extends only fixture output with the retained root trace.

[Eight original observations](parity/explicit_save_scope_valid_original_5_0_6.json.gz)
cross runtime-state absence/presence, two valid reasons and an observing Checker
on/off. Original runtime bodies remain unchanged. The
[candidate before-correction comparison](parity/explicit_save_scope_before_5_0_6.json.gz)
matches **4/8**; all four mismatches have an empty ledger carrier. Every observed
field except `root_scope` matches. Rust retains the root audit-reason scope after
the call; C++ returns an empty trace. No Checker-count discrepancy is claimed.

The explicit-executor operation captured scopes only inside its dirty-ledger
branch. Capture now runs after pending dynamic preflight and before the dirty
check, matching the Rust ordering and the already-correct automatic entry point.
General value semantics and caller-owned transaction responsibility are unchanged.
The eight-case comparator is registered for both native and module consumers;
historical audited-save outputs remain unchanged unless the new observation flag
is supplied. Formatting and focused native qualification are in progress.

The explicit-executor probe previously had no canonical module target. Its new
module CTest condition would therefore have skipped registration without one.
The probe is now in `modules/contracts.json` and the `modules_native` target list,
with exactly one entry in each. Both the new eight-case scope comparison and the
historical 356-case audited-save comparison are registered against that target.
This records a registration repair; execution is still pending. The successful
original observer reclaimed 167,958,860 bytes of disposable source/build outputs.

### Explicit-executor correction qualified

The [closure receipt](parity/explicit_save_scope_closure_5_0_6.json.gz) records
**native 3/3, module 3/3 and focused ASan/UBSan 2/2**. All eight new observations
and all 356 historical audited-save cases match on each applicable frontend;
native and module also execute the seventeen original entity-save contracts.
Canonical verification passes for 1,145 bodies. The missing probe module target
is fixed, not merely hidden behind a condition. This closes the observed
empty-ledger scope discrepancy without claiming complete source acceptance.

### Remaining preparation and Fix integration gate

The retained save-preparation corpus has 1,719 cases: 91 extraction, 1,188 typed
conversion, 240 preflight, 145 merge, 31 hydration and 24 flag cases. Graph Fix
has the separately recorded 1,597-case responsibility breakdown. Both probes
and their independent Context fixtures lacked canonical module registrations;
they are now registered along with their preset targets and module comparisons.
Function bodies are unchanged in this registration step. A focused native gate
also rebuilds and executes the existing graph-saver, automatic-save and runtime
state bridge comparisons/fixtures, because these are the adjacent entry points
needed for the entity-save production responsibility review. These gates are
running; no passing result is inferred from registration alone.

### Adjacent save responsibility gate completed

The [native adjacent gate](parity/entity_save_adjacent_native_5_0_6.json.gz)
passes **10/10**: original-output comparisons and independent Context fixtures
for preparation, graph Fix, graph saver, automatic audited save and runtime state
bridging. The [new module gate](parity/entity_save_preparation_fix_modules_5_0_6.json.gz)
passes **4/4** for preparation and Fix, including 1,719 and 1,597 exact original
observations respectively. Canonical verification passes for 1,149 bodies.
Native and module builds reuse their committed directories; this registration
step changes no C++ function body and adds no disposable build tree.

These gates complement the seventeen-test source port and the two corrected
scope discrepancies. Full entity-save acceptance still requires the complete
production responsibility reconciliation and final integration evidence. In
particular, successful dynamic-field installation in generated/SQLite paths must
not be described as direct coverage of every decoder/installation failure branch.
The historical full-native failure and later deliberately interrupted epoch remain
retained, without either being relabeled as a passing full suite.

### Missing indexed dynamic snapshot boundary

A [fresh original observer](parity/saved_dynamic_snapshot_original_5_0_6.json.gz)
uses the unchanged private saved-row decoder with a model whose loaded snapshot
is absent. Without dynamic results it succeeds. With a present, valid but empty
dynamic result it returns exactly `graph write error: indexed state missing from
dynamic save result`. Original runtime bodies are preserved; the owned source
view/build directory was removed after retaining evidence (166,319,304 bytes).

The candidate Context fixture now directly checks the corresponding installation
helper's absent/present branches, including the structured Graph error and exact
message. The first fixture incorrectly borrowed definitions from an unregistered
stack result and failed with `Managed object borrow requires a registered Context
object`. Registering the definitions through `user_context.make` corrected the
fixture in accordance with the existing ownership contract; no runtime behavior
was changed. The corrected native test passes. Final formatting and native/module
reverification are running. This specifically qualifies the missing-snapshot
precondition, not every dynamic overlay or entity installer error branch.

The [dynamic snapshot boundary closure](parity/saved_dynamic_snapshot_closure_5_0_6.json.gz)
records successful final native and canonical-module fixture execution, the
unchanged runtime implementation, original two observations, corrected Context
registration and source hash. Formatting/check passes for 1,800 files and
canonical verification for 1,149 bodies. No new sanitizer run was needed for this
fixture-only assertion addition; earlier runtime repair sanitizer evidence remains
separately scoped. This closes the missing-snapshot precondition evidence gap.

### Next saved-row decoder reconciliation

Read-only review while the SQLite benchmark Release gate runs identifies the
remaining decoder error-wrapping boundary. The existing saved-dynamic fixture
proves absent fields and present fields without an indexed snapshot. The source
then has two distinct fallible steps: `LoadedSnapshot::with_dynamic_fields`
(owner/layout mismatch), and the entity's `install_loaded_dynamic_fields` hook.
Both must become Graph errors, preserving their source message. The owner
mismatch already has lower-level dynamic-carrier corpus witnesses; that alone
does not demonstrate the saved-row decoder's Graph conversion or that an
installer is skipped after snapshot overlay failure. Successful dynamic graph
save witnesses also cannot establish either failure branch.

The next focused observer should exercise the unchanged original decoder with
an indexed carrier and (1) a mismatched dynamic owner, (2) matching owner plus a
failing installer, and (3) matching owner plus successful installation. Candidate
checks should assert error kind/message, installer invocation ordering and
preservation of the indexed snapshot on success. This is a qualification gap,
not an observed production discrepancy. No runtime/source behavior was changed
by this review, and no source disposition is promoted.

### Saved dynamic installation: original six-case observation

The [unchanged-original observer](parity/saved_dynamic_installation_original_5_0_6.json.gz)
passes six observations: owner mismatch, failing installer and successful
installation, each with relation preservation disabled/enabled. Owner mismatch
returns `graph write error: dynamic field owner disagrees with fixed entity
layout` and never invokes the installer. Installer failure invokes it once and
returns `graph write error: Order: installer rejected saved dynamic fields`.
Success invokes it once, preserves all three fixed loaded fields and retains
the original snapshot identity for the empty dynamic selection. Both failures
are structured Graph errors. Runtime implementation bodies remain unchanged;
166,343,846 bytes of disposable original view/build outputs were reclaimed.

A corresponding indexed C++ carrier and six-case check now live in
`save_preparation_tests.cpp`. They call the production saved-dynamic installation
helper after ordinary/contextual compact decoding, assert exact errors and call
counts, and verify loaded fixed fields on success. This fixture uses explicitly
registered UserContext-owned dynamic definitions and the production loaded-state
pool. Native compilation/verification is running; no C++ pass or complete
entity-save acceptance is inferred from adding the fixture.

The [installation closure receipt](parity/saved_dynamic_installation_closure_5_0_6.json.gz)
records native 1/1 and imported-module 1/1, each exercising all six combinations
with exact error/call-order and success assertions. Formatting/check passes for
1,802 files and canonical verification for 1,151 bodies. The runtime helper is
unchanged; no new sanitizer run was required for these fixture-only semantic
assertions using already established scoped hydration/installation contracts.
This closes the overlay/installer Graph conversion evidence gap, not complete
entity-save production or final integration acceptance.

### Generic dynamic audited-save Fix scope review

Read-only comparison during the current native integration epoch identifies a
possible remaining entry-point difference. Rust's generic `AuditedSaveExt`
creates its Fix time/evidence scope after detecting dynamic values/pending
mutations and successfully obtaining the runtime carrier. C++ wraps the typed
`Ledger=true` entry with `with_graph_fix_scope`, but the non-generated generic
entry constructs `AuditedAutomaticSave<T, false>` directly; its runtime dynamic
route selection does not visibly add a corresponding scope. Context evidence
recording is inactive without a GraphFixActivation.

An original-only observer is running outside frozen repository inputs. It seeds
prior Fix evidence and checks a missing-saver failure in plain versus dynamic
routes, plus dynamic input without a carrier. This distinguishes scope completion
(which should publish its new evidence, possibly empty) from failure before a
scope exists (which should retain prior evidence). This is currently a source
inspection finding, not a confirmed candidate execution discrepancy. No frozen
C++ input has been edited; reproduce candidate behavior before selecting a fix.

The [original generic-route observer](parity/generic_dynamic_fix_scope_original_5_0_6.json.gz)
now confirms the publication boundary: after a missing-saver failure, the plain
route retains one previously published evidence entry, while the dynamic route
with a runtime carrier publishes an empty result. A dynamic route missing its
carrier retains the prior entry because it fails before entering the scope.
The [first observer compile failure](parity/generic_dynamic_fix_scope_observer_compile_failure_5_0_6.json.gz)
was fixture-only (incorrect comment return type and an unavailable executor
crate); the corrected observer uses the original String-returning contract and
existing Tokio test runtime.

A candidate reproduction is compiling outside repository execution inputs,
using already completed native libraries whose modification times predate the
integration epoch and whose hashes are checked before/after. It uses the existing
handwritten dynamic School carrier without a generated-ledger marker, preserves
the production generic route and compares plain/dynamic evidence publication.
The initial candidate probe needed the existing provider include path; that
compile failure is separately retained and is not a runtime discrepancy. No
frozen source input has been edited for these one-off observations.

### Generic dynamic Fix discrepancy reproduced and repair started

The [candidate before-fix receipt](parity/generic_dynamic_fix_scope_candidate_before_5_0_6.json.gz)
returns `plain,1` and `dynamic,1`, exiting 1 against the original expected
`plain,1` and `dynamic,0`. It uses the existing handwritten School carrier with
no generated marker and stable, previously completed native libraries; their
hashes remain unchanged. Original and candidate temporary outputs are removed.
This confirms an observable production entry-point discrepancy, not merely a
missing test or an observer compilation issue.

The [full-native epoch](parity/native_driver_save_integration_terminal_5_0_6.json.gz)
was deliberately interrupted at build exit 130 after this confirmation. Its
input snapshot remains retained; no CTest execution occurred. The owned log/state
directory was reclaimed after the controller and child exited. Its source freeze
is now over. This does not supersede earlier failed full-native evidence.

The repair adds a prepared continuation to AuditedAutomaticSave. The generic
entry still samples the descriptor, applies audit and resolves its carrier once.
Only a valid dynamic route creates a continuation under the existing
`with_graph_fix_scope` wrapper. Composition checks, preflight and persistence
then run inside that scope, reusing its time/evidence publication and cancellation
mechanics. Plain routes and missing-carrier failure stay outside the new scope;
typed/generated ledger entries retain their existing wrapper. No new ownership
domain or duplicated Fix implementation is introduced. A missing-saver regression
for the handwritten generic carrier is added to dynamic_graph_save_tests.cpp.
Formatting and targeted native verification are running; imported, lifetime and
related entry-point gates remain required before claiming correction acceptance.

### Generic dynamic continuation: native cancellation checkpoint

The corrected native entry-point selection passes all six tests: dynamic graph
Save, Graph Fix, runtime-state bridge contracts/parity and automatic audited Save
contracts/parity. The generic carrier now publishes an empty completed Fix scope
on missing-saver failure, while the plain route preserves prior evidence. Two
additional pending-operation controls verify explicit cancellation and direct
UserContext destruction: cancellation occurs exactly once under the original
Fix clock/scope, without publishing incomplete evidence. Test-local Context-backed
values are destroyed before closing the Context; only Global-owned observation
counters are read afterward.

The first cancellation fixture omitted the required `finish_and_wait()` hook;
its [compile-failure receipt](parity/generic_dynamic_fix_scope_cancellation_fixture_failure_5_0_6.json.gz)
is retained separately. Adding the hook corrected the fixture, not production
behavior. The [follow-up receipt](parity/generic_dynamic_fix_scope_followup_5_0_6.json.gz)
records current native output and tracks the subsequent format, module and
sanitizer gates. Those gates must finish successfully before this repair can be
qualified. This checkpoint does not promote the complete entity-save source or
supersede the interrupted full-native integration epoch.

### Plain versus dynamic saved-row decoding: original observation

The [new original observer](parity/plain_saved_dynamic_original_5_0_6.json.gz)
uses a registered `DynGraphSaver` returning an empty dynamic-field selection for
an otherwise valid saved row. Its generic model intentionally has no indexed
snapshot. A plain input succeeds and returns a model without dynamic fields;
a dynamic input with a runtime carrier fails with `indexed state missing from
dynamic save result`. Both assertions execute through the unchanged original
`AuditedSaveExt` entry. The disposable original workspace is reclaimed, with
its size and source provenance retained in the receipt.

This distinguishes the source's ordinary `from_compact_row` path from
`decode_saved_ledger_entity`; the latter's installation precondition must not
be imposed on plain generic saves. The current C++ shared completion body calls
`install_saved_dynamic_fields` for both routes. An independent candidate observer
is being compiled against completed native libraries to establish whether the
source difference is observable. Its first fixture used raw-pointer assignment
to `ScopedBox`; that compile failure is retained separately and corrected using
`reset` with a registered Context payload. No production change or source
acceptance is claimed by this investigation.

The [candidate reproduction](parity/plain_saved_dynamic_candidate_before_5_0_6.json.gz) now confirms the mismatch: the same plain generic path returns `indexed state missing from dynamic save result` and exits 1, while the original succeeds. The completed native library hashes remain unchanged and the owned probe directory is removed. Repair is deferred until the active Fix-scope sanitizer gate terminates so its recorded inputs remain stable.

### Generic Fix-scope gate completed; saved-row repair started

The Fix-scope follow-up terminated successfully: native **6/6**, canonical
module consumers **2/2** and ASan/UBSan **2/2**, with leak scanning disabled.
All 1,802 handwritten files pass pinned formatting/check; recorded repair inputs
remain unchanged throughout the gate. This closes the focused generic dynamic
Fix publication/cancellation correction, not the entire entity-save source.
No disposable build tree was introduced; committed preset directories are reused.

After that gate terminated, the independent plain saved-row discrepancy was
repaired by limiting dynamic-field installation in `AuditedAutomaticSave` to its
ledger route. The plain route retains ordinary compact-row reconstruction.
`audited_auto_save_tests.cpp` now supplies a saver returning dynamic fields to a
model without indexed state: generic plain Save must succeed, while the ledger
entry must still reject missing indexed state. Relevant native builds/tests are
running. Module/format gates and final repair acceptance remain pending; prior
Fix-scope evidence identifies its earlier checkpoint rather than qualifying this
subsequent edit automatically.

The plain saved-row correction now passes **native 4/4**: automatic audited Save
contracts/parity, dynamic graph Save and runtime-state bridge parity. The added
plain/ledger control reaches its intended successful/error outcomes. The
[follow-up receipt](parity/plain_saved_dynamic_followup_5_0_6.json.gz) captures
compiled input hashes and native output, and tracks pending formatting, imported
module and focused sanitizer gates. The original observer removed 166,504,021
bytes of disposable outputs; the successful candidate reproducer removed
12,767,058 bytes. No extra full build configuration is retained.

### Explicit-executor graph fallback: data-flow reconciliation

The apparent shared-decoder concern does not reproduce on the explicit-executor
path. `AuditedWithExecutor` moves `*node_` into the by-value
`EntityDataServiceAccess::save_graph_internal` argument. `GraphNode` has its
defaulted move constructor and a `ScopedBox` dynamic-field member, so the source
node no longer owns that selection. `materialize(..., false)` reads this moved-from
node's empty dynamic pointer, not the graph operation's returned fields. The
ledger branch retains its node and installs its authoritative readbacks as intended.
No production correction is made to this path on the basis of visual similarity.

The [original observation](parity/explicit_saved_dynamic_original_5_0_6.json.gz)
exercises all four combinations of optional runtime state and an empty selected
dynamic-field container through the public executor-bound save entry. Every case
reaches successful ordinary row reconstruction and returns no pending ledger.
The [native control](parity/explicit_saved_dynamic_candidate_5_0_6.json.gz) also
passes all four combinations, checking the returned name, absence of a pending
ledger and absence of installed dynamic fields. Its completed library hashes
remain unchanged. This is a focused return-path control, not an exact comparison
of every diagnostic, event or serialized output field.

The [first original attempt](parity/explicit_saved_dynamic_original_early_readback_5_0_6.json.gz)
provided no readback rows and stopped at `persisted Order record could not be read
back` in every case. It is retained as early-error evidence, not return-path
coverage. The final observer supplies the saved row and explicitly requires all
four successful non-ledger outcomes. Both original temporary workspaces and the
candidate probe directory were reclaimed by their controllers.

### Plain saved-row gate completed

The [final focused receipt](parity/plain_saved_dynamic_followup_5_0_6.json.gz)
passes native **4/4**, imported-module **2/2** and ASan/UBSan **2/2**, with leak
scanning disabled and repair inputs unchanged. Both formatter and format check
pass for 1,802 handwritten files. This closes the ordinary generic Save decoding
correction while preserving the dynamic/ledger indexed-state precondition.
The [explicit-executor reconciliation receipt](parity/explicit_saved_dynamic_closure_5_0_6.json.gz)
separately records four successful original/native fallback controls, no required
production change there, and 1,151 canonical bodies plus reference/path checks.

Complete production-source acceptance remains pending, including final
entry-point responsibility reconciliation (descriptor sampling, audit application,
carrier lookup and error precedence) and consuming integration. These focused
results do not supersede the interrupted full-native epoch or qualify P0-P7.
All one-off probes have exited and their temporary directories were removed;
reusable native/module/sanitizer preset outputs are retained.

### Save entry ordering and optional typed carrier

The [eight original observations](parity/save_entry_order_original_5_0_6.json.gz)
cross generic/ledger entry, presence of dynamic fields and presence of a runtime
carrier. A test-only model records descriptor reads, audit application and the
core/typed carrier hook. The original generic entry reads its descriptor before
applying audit; the typed ledger entry applies audit, reads the optional typed
carrier and then extracts the descriptor. Only a generic dynamic save with no
carrier returns the missing-carrier error. A typed ledger save with an empty
carrier proceeds to the missing-saver boundary, even with selected dynamic fields.

The [candidate before correction](parity/save_entry_order_candidate_before_5_0_6.json.gz)
reproduces four differences, all on the typed ledger route. Its common entry reads
an extra descriptor before audit, and its carrier check also rejects a typed
empty carrier when dynamic fields exist. The
[comparison receipt](parity/save_entry_order_differences_5_0_6.json.gz) retains each
original/candidate sequence and route. Raw original errors remain retained;
resource type spelling is not normalized into a claim of identical diagnostics.
Both observers used completed builds and removed their disposable output trees.

`AuditedAutomaticSave` now limits the pre-audit descriptor sample and mandatory
carrier check to the generic entry. Typed ledger saves preserve the optional
carrier contract; the later extraction, preflight and persistence body is unchanged.
Eight regression controls in `audited_auto_save_tests.cpp` assert exact observed
call sequences and error boundaries. Native verification is running; format,
module and relevant regression gates must complete before accepting the repair.
This is not complete entity-save source or P0-P7 acceptance.

The entry-order correction now passes native **4/4**, including all eight new
sequence/route controls and retained automatic-save/runtime-bridge comparisons.
The [follow-up receipt](parity/save_entry_order_followup_5_0_6.json.gz) records
current compiled inputs and native output while the format, module, focused
sanitizer and canonical/reference gates run. Original and candidate probes
removed 166,551,856 and 12,707,612 bytes respectively. No additional persistent
build configuration is introduced. Complete source and integration acceptance
remain open.

### Consolidated responsibility map and Checker state rechecks

The [eighteen-row responsibility matrix](entity_save_responsibility_matrix_5_0_6.md)
separates production responsibilities, focused witnesses and remaining closure.
Review of the preparation helpers found an observable difference in standalone
pending dynamic preflight: C++ snapshots `deleted_keys` and `new_keys` before
iterating, while Rust reads those sets for each pending entity. A registered
Checker can change the shared ledger while inspecting an earlier entity.

The [original callback observer](parity/pending_dynamic_recheck_original_5_0_6.json.gz)
checks two pending entities. No callback change yields `1:update, 2:update`;
marking entity 2 deleted while checking entity 1 yields only `1:update`; marking
entity 2 new yields `1:update, 2:create`. The
[candidate before correction](parity/pending_dynamic_recheck_candidate_before_5_0_6.json.gz)
reports two Update checks in all three cases. The
[two-difference receipt](parity/pending_dynamic_recheck_differences_5_0_6.json.gz)
retains exact observed lists. These are preflight-helper observations, not a claim
that the complete Save/persistence operation has run. Temporary original and
candidate output trees are reclaimed, and candidate library hashes stay stable.
Repair is deferred until the active entry-order gate terminates to preserve its
execution checkpoint.

### Entry-order closure and dynamic status correction started

The [entry-order follow-up](parity/save_entry_order_followup_5_0_6.json.gz)
completed with native **4/4**, imported modules **2/2**, focused ASan/UBSan **2/2**,
1,802-file format/check, 1,151 canonical bodies and the exact upstream check.
Recorded inputs were unchanged. The eight original sequences and optional typed
carrier boundary are now retained as executable regression controls.

After that checkpoint completed, standalone dynamic preflight was changed to
query `is_marked_as_delete(key)` and `is_new(key)` for each entity. These are
existing direct membership queries over the same ledger sets, preserving current
state without copying whole sets repeatedly. Three callback regressions cover
no change, deleting the next entity and creating the next entity. Native
preflight/preparation/graph-save validation is running; remaining gates are open.

Further source reconciliation must verify ordinary saver intent precedence
against missing executor lookup. The original `GraphSaverFor::save_graph_dyn`
validates MutationIntent before requiring its executor; the candidate currently
requires the executor before reaching graph planning. Existing provided-executor
missing-intent tests establish no I/O, but do not settle this combined-error
boundary. This is a source-review question requiring execution, not a passing
parity claim or an authorized compatibility exception.

The dynamic status correction passes native **4/4**: the three new callback
controls, preparation contracts, all 1,719 retained preparation comparisons and
dynamic graph Save. The [follow-up receipt](parity/pending_dynamic_recheck_followup_5_0_6.json.gz)
records current input hashes and native output while formatting, four imported
module tests and two focused sanitizer tests run. No full-source acceptance or
full integration success follows from this checkpoint.

### Saver intent precedence: combined failure reproduced

The [original eight-case observer](parity/saver_intent_precedence_original_5_0_6.json.gz)
invokes the actual `GraphSaverFor` with its executor deliberately absent. Both
graph and ledger routes reject absent, empty and whitespace comments as
`REQUEST_COMMENT_REQUIRED` before resource lookup; a valid comment reaches the
missing-executor error. The fixture executor's I/O and transaction methods panic
if called, so these observations cannot hide an unintended provider operation.

The [candidate before correction](parity/saver_intent_precedence_candidate_before_5_0_6.json.gz)
returns missing-executor for all four graph inputs; its ledger outcomes already
match. The [three-difference receipt](parity/saver_intent_precedence_differences_5_0_6.json.gz)
retains the original errors and candidate classifications; complete candidate
error text is also retained. Executor type spelling remains a language-specific
diagnostic detail. One-off original/candidate directories were reclaimed after
terminal execution, and candidate library hashes were unchanged.

The ordinary transactional saver needs source-order intent validation before
requiring the executor. The active pending-dynamic correction gate must terminate
before editing its shared runtime inputs. No compatibility exception or full
source acceptance is claimed for this discrepancy.

### Dynamic status gate complete; saver intent correction started

The [dynamic status follow-up](parity/pending_dynamic_recheck_followup_5_0_6.json.gz)
terminated successfully with native **4/4**, canonical module **4/4**, ASan/UBSan
**2/2** (leak scanning disabled), 1,802-file format/check, 1,151 canonical bodies
and the locked upstream check. Captured inputs stayed unchanged. The three
callback controls retain the original traversal snapshot while rechecking each
entity's current deletion/creation membership. The 1,719 preparation observations
also pass through native and module frontends.

After this gate finished, ordinary `TransactionalGraphSave` gained intent
validation before executor lookup. Its pending-planning guard avoids repeating
the entry check when polling an already-started plan. The ledger route retains
its existing capture/validation order. Eight regression controls cover both
routes and absent/empty/whitespace/valid comments, requiring the exact source
intent diagnostic and the expected missing-resource boundary for valid intent.
Native saver and automatic-save tests/comparisons are running. This ordering-only
change adds no ownership or asynchronous cleanup mechanism; no new sanitizer run
is planned for it. Format/module and final source/integration checks remain open.

The intent-precedence correction passes native **4/4**, including the eight new
combined-error controls and retained saver/automatic-save Rust comparisons.
Preparing the imported gate revealed that `graph_saver_probe` had no canonical
module consumer. Its source-preserving registration and native module-preset
target are now added; this is a test consumer, not a production export change.
The [follow-up receipt](parity/saver_intent_precedence_followup_5_0_6.json.gz)
distinguishes native compiled inputs from the subsequent module registration
and tracks format, imported comparisons and canonical/reference checks.
The new registration does not retroactively establish earlier module coverage.

### Saver intent and full imported comparison closure

The first imported run selected only three tests: its new probe executable was
built, but the comparator CTest was not yet registered. The original receipt
retains this limitation explicitly. The corrected
[module closure](parity/saver_intent_precedence_module_closure_5_0_6.json.gz)
asserts the complete four-name registry before execution and verifies the same
four terminal JUnit names. All **4/4** pass, including the full graph-saver and
automatic-save comparisons. Canonical verification covers 1,152 bodies; the
locked generator review verifies 42 sources and 17 capability records, with six
still unqualified. No production export or generated artifact was changed.

The currently confirmed entity-save discrepancies now have focused correction
witnesses. The full current native integration epoch is the next consuming gate,
with code, scripts, manifests, JSON ledgers and producer inputs frozen by its
collector. Narrative review and compressed evidence receipts may continue to be
updated without changing execution inputs. Source/capability acceptance remains
pending; historical full-suite failures and interrupted epochs remain retained.

### Nested public Save collectors

The [nested Save review](nested_save_scope_5_0_6.md) identifies and corrects a
further ES01 boundary after that full native epoch. Rust creates a fresh Fix
collector/time scope for each public ledger, dynamic generic and executor-bound
save, including nested calls on the same Context. Plain generic saves inherit
the outer scope and do not publish an inner result. C++ had treated nested
public calls as internal adapters and reused the outer collector. A fresh
byte-preserved original observer and the corrected fixture with the retained
old header reproduce the three differences under ASan/UBSan without a sanitizer
diagnostic. All four installed outcomes now match fresh original observations.

Final native6/module2/sanitizer2, canonical1,156 and format/check1,805 pass.
Initial fixture lifetime errors and their failed sanitizer/counterfactual runs
are retained separately: the final fixture allocates both its observation vector
and retained source model through UserContext, so destruction precedes close.
This is a focused scope/evidence correction; successful nested transactions and
complete source/caller acceptance remain separate. Source/capability counts and
P0-P7 are unchanged, and the earlier 877-test epoch is not relabeled.
