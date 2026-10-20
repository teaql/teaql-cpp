# Remaining allocation-source audit: Rust 5.0.6

This is an implementation inventory, not acceptance evidence. The locked
reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. At the initial audit, all three sources below remained pending. The retained
cache test is now formally accepted; see the terminal update below. The other two
sources remain pending. Existing readonly-property zero-allocation evidence does not
cover these broader responsibilities.

## Retained snapshot cache hits

`teaql-runtime/tests/state_cache_cost.rs` has one test,
`shared_state_cache_hit_cost_with_retained_shapes` (52 lines).

It constructs the exact id/version/name layout, keeps an id/name target alive,
and retains 0, 63 or 511 additional shapes using distinct `#retained_N` names.
For each resulting population of 1, 64 or 512 shapes, it warms 100 target hits,
then measures seven samples of 10,000 clone-and-intern operations. Every sample
must return the same target identity, retain id/name and omit version, and report
zero allocation calls and requested bytes. The retained shape count is asserted.
The 21 elapsed-time samples are observations, not latency pass/fail thresholds.

The C++ counterpart should exercise `LoadedSnapshot` and the actual
Context-owned `SnapshotPool`, using allocator-aware snapshot copies followed by
`into_shared(pool)`. Retain every competing shape outside the measured region.
The existing `loaded_snapshot_tests.cpp` checks interning, cache saturation and
lifetime behavior, but it does not measure all 21 cache-hit samples. A standalone
measured test must retain the real lookup and not substitute a borrowed-handle
copy for copy-and-intern.

Use the existing test-only arena/global-new instrumentation with positive
controls. Record arena request count/bytes separately from heap calls/bytes;
zero must hold for both. The original Rust global allocator measurement and
C++ arena instrumentation are different observation surfaces and should be
reported explicitly. Run the unchanged original test, native and imported
counterparts, and preserve exact per-sample outcomes and source hashes.

## Full typed hydration

`teaql-runtime/tests/hydration_allocations.rs` has one test,
`typed_hydration_allocation_probe` (196 lines), a macro-derived indexed Probe,
a plain struct decoder and prepared row builders.

The original independently exercises sparse and full projections at 1, 100 and
10,000 rows. For each combination it measures four paths:

| Path | Original assertion |
| --- | --- |
| Typed hydration | At most `3 * rows + 1` global allocation calls; every id/name/version/note value, note availability, shared snapshot and clean dirty state are checked. |
| Identity graph batch | At most `3 * rows + 32` calls; frozen first/last entity snapshots share the warmed shape. Actual boxes and identity tables are counted; temporary per-row state structures are not excused. |
| Context-bound hydration | At most `2 * rows + 1` calls; all snapshots and note availability remain correct, and a comment on row one does not affect row two. |
| Plain hydration | Full id/version/name/note value checks; calls/bytes/time form the baseline measurement without an allocation ceiling assertion. |

Input rows, metadata warmup, database I/O and logging are outside the measured
regions. Graph freezing occurs after the measured batch build, matching the
original boundary. The complete matrix contains 24 measured outcomes.

A generated C++ model must preserve id/version/name plus nullable note. The
source-specific fixed layout is not a reason to hand-patch generated output.
The existing graph, bulk load-state and readonly tests prove useful semantics,
but they do not establish these complete hydration allocation budgets.

Before accepting this source, specify the C++ budget interpretation explicitly.
Rust global allocation calls cannot silently become only C++ arena block counts:
that would hide per-row state allocation requests behind a growing arena. Report
both allocation layers, the prepared input boundary, payload costs and metadata
costs. Any C++ ownership optimization must preserve complete value, availability,
identity and mutation isolation assertions. Do not claim cross-language time or
byte equality from unrelated allocation mechanisms.

## Allocation measurement helper

`teaql-runtime/tests/support/allocation_counter.rs` (47 lines) installs a Rust
global allocator which counts alloc, zeroed alloc and realloc requests while a
thread-local measurement flag is enabled. It excludes deallocation, resets each
measurement, starts timing before enabling the flag, black-boxes the result,
disables counting before reading elapsed time, and returns the action result.

The existing C++ helper is a test executable instrument, not a production
allocator or hidden owner of Context data. Its ordinary/aligned new/delete and
arena wrappers already support focused readonly measurements. Source acceptance
must document the observation differences, verify positive controls, and tie the
helper to executed consumers. A header declaration or an unrelated green test is
insufficient. Counter state must not leak into runtime allocation or Context
ownership design.

## State-cache exploratory execution

Two isolated receipts now provide preliminary evidence while the broader SDK
integration gate keeps its inputs frozen:

- [Unchanged original](parity/state_cache_cost_original_exploration_5_0_6.json.gz):
  the complete original test passes in the byte-checked runtime view. All 21
  samples report zero Rust global allocation calls and requested bytes.
- [Native exploration](parity/state_cache_cost_exploration_5_0_6.json.gz): a test
  source retained in the receipt is compiled outside the repository build
  outputs with the existing native compiler command and already-built libraries.
  Compile, link and execution succeed. Both Context scopes and ordinary/aligned
  heap positive controls pass. All 21 samples preserve target identity and
  id/name availability, retain exactly 1/64/512 shapes and report zero arena
  requests/bytes and heap calls/bytes. Library hashes remain unchanged.

The C++ probe calls the real allocator-aware snapshot copy and `into_shared`
lookup on every iteration. It does not replace the operation with a handle copy.
Timing is recorded without a cross-language speed claim. The source is not yet
installed as a repository CMake target or run through imported modules; these
receipts do not promote the source or any capability. Formal integration follows
the currently running SDK gate and the dynamic-query projection correction.

The [matched exploratory grid](parity/state_cache_cost_comparison_exploration_5_0_6.json.gz)
checks the exact `(retained shapes, reads, sample)` key set in both receipts:
1/64/512 shapes, 10,000 reads and sample indices 0 through 6, with no duplicates
or missing samples. All Rust allocation call/byte outcomes and both C++ allocation
layers are zero. Timing columns are deliberately not compared. The receipt
retains input hashes and does not change the pending source verdict.


## Formal retained-cache follow-up

The [complete state-cache review](state_cache_cost_source_review_5_0_6.md) and
[installed receipt](parity/state_cache_cost_installed_5_0_6.json.gz) now qualify
that one source with original1/native2/module2, exact21-sample grids, allocation
positive controls, snapshot regressions and canonical/no-work/input guards. The
preceding exploration paragraphs retain their earlier checkpoint scope. The
full hydration probe and allocation-counter source remain pending.

## Full hydration native exploration

The dedicated generated `cpp_hydration_allocation::HydrationProbe` now has an
executed native probe in `tests/parity/hydration_allocation_probe.cpp`. Reproduce
with the activated native toolchain and existing native libraries:

```bash
source scripts/toolchains/activate.sh
python3 scripts/probe_hydration_allocations_5_0_6.py
```

The [exploratory receipt](parity/hydration_allocation_exploration_5_0_6.json.gz)
retains the source, compiler/link commands, generated SDK and library hashes,
binary hash, exact 24-row matrix and allocation positive controls. Compilation,
linking and execution completed successfully. This is not source acceptance;
the original and imported-module comparison and budget qualification remain open.
The script uses the native dynamic-relation compilation environment and links
existing libraries without changing their outputs; it checks their hashes after
execution. It does not build missing or stale native libraries on the caller's
behalf. Its wrapping symbol is specific to the native 64-bit ABI.

All sparse/full projections and 1/100/10,000-row sizes execute typed,
Context-state-bound, graph-batch and plain paths. Typed outputs check every
scalar, note availability, shared snapshot identity and clean dirty state.
Context-bound outputs also check private comments. Graph freezing is outside the
measurement; first/last graph snapshots share the warmed identity. Prepared input
rows and registry setup are outside measurement, while graph-builder creation
and output-vector storage are inside. A test-only 512 MiB Context limit permits
the cumulative matrix; the initial run exhausted the unchanged default budget.

For 10,000 rows, both projections produced these arena request counts:

| Path | Arena requests | Locked Rust allocation ceiling |
| --- | ---: | ---: |
| Typed | 70,475 | 30,001 |
| Context-bound | 70,470 | 20,001 |
| Graph batch | 90,484 | 30,032 |
| Plain | 1 | No ceiling |

These observation surfaces differ, so this table identifies an unresolved cost
gap rather than claiming a failed Rust-equivalent allocator assertion. Low heap
call counts do not establish the allocation budget: requests inside the arena
must remain visible. The short name uses C++ small-string storage; no equal
payload-allocation or cross-language timing claim follows. Graph/table storage
and private original snapshots must be separated from avoidable temporary state
before qualifying an adapted budget. Generated field order is id/name/note/version;
prepared SQL columns remain id/version/name[/note], exercising mapped decoding.

One candidate for investigation is the generated decoder's final
`set_original_compact_row(ENTITY_NAME, row)` call: it uses the copying overload,
although the runtime also has a move overload. This is a source observation, not
an attribution of all measured costs or authorization to discard original values
or relation metadata. Any optimization must preserve read metadata, original
snapshots and subsequent mutation behavior, then regenerate the SDKs.

## Allocator-aware entity move correction

The first follow-up changes the C++ producer to emit allocator-aware entity moves
through the existing `detail::move_to` runtime helper. That helper moves within
the same resource and invokes the allocator-aware copy across resources. It is
already used by handwritten runtime carriers; no alternate ownership domain or
new allocation API is introduced. The original-row copy remains unchanged in
this checkpoint.

The old generated allocator-aware move delegated unconditionally to its copy
constructor. A native debugger trace of the first typed row identified graph-slot
creation, original-row copying, and ledger/snapshot promotion during insertion
into the allocator-aware result vector. The latter work followed the generated
move-to-copy delegation. This attribution applies to that row and path, not to
every request in the complete matrix.

Only the dedicated hydration SDK is regenerated at this checkpoint. Other SDKs
still retain the previous producer fingerprint; a complete SDK migration and its
integration gates remain required before general generator qualification.
`generate_hydration_allocation_contracts_5_0_6.py --refresh-cpp` regenerates both
languages in staging, requires the locked Rust artifacts to remain byte-identical,
and permits only the C++ model header plus provenance to change on installation.

The [native follow-up](parity/hydration_allocation_move_exploration_5_0_6.json.gz)
retains all 24 measured rows. At 10,000 rows, typed hydration falls from 70,475 to
30,161 arena requests for both projections. Context-bound hydration still uses
70,472 and graph batches 90,485 requests. These remain observations, not accepted
budget substitutions. Arena object registration and other batch costs remain
visible; no ceiling is relaxed to fit the results.

The probe additionally verifies that moving a generated model within its Context
performs zero arena/heap allocations and retains its indexed snapshot identity,
long string, id/version and comment. An allocator-aware move into Global storage
must preserve the source and deep-copy to the destination, then retain values,
NULL/load state, comment and an original snapshot after actual source Context
deletion. The original exploratory receipt retains its old source and SDK hashes;
it is not relabeled as evidence for the changed constructor.

The [Java receipt](parity/hydration_move_java_5_0_6.json.gz) records 75/75 passing
`CppGenerationTest` cases, including normal and inherited emitted constructors.
It also retains the preceding failed run: one existing scope-registration test
raised an unsupported-scope error while packaging overlapped with that test run.
The subsequent run was serialized after generation completed and passed. The
failure is retained rather than treated as a passing or skipped test.

The [sanitizer follow-up](parity/hydration_allocation_move_sanitize_exploration_5_0_6.json.gz)
executes the same 24 rows and move/actual-Context-deletion checks against the
existing ASan/UBSan native libraries, with leak scanning disabled as required.
Arena counts match the native follow-up. The
[SDK reproduction receipt](parity/hydration_move_reproduction_5_0_6.json.gz)
confirms exact dedicated Rust/C++ artifacts and provenance. These focused gates
do not replace full generated-API, imported-module or all-SDK migration checks.

The remaining Context-bound path calls generated `on_loaded`, which currently
assigns `root.with_shared_graph(*source)`. The existing const runtime overload
copies the loaded view and promotes its private snapshot/ledger to shared slots.
A consuming graph-binding path is a candidate for the next runtime-first change;
it must preserve private mutation state, snapshot contents, graph identity and
cross-domain checks rather than merely suppress allocation accounting.

Current repository-wide status: module-path verification and the full formatting
check pass. `verify_generator_parity.py` currently fails its producer fingerprint
assertion (line 143), because retained SDK manifests outside the dedicated fixture
still identify the previous generator implementation. This is an outstanding
migration gate, not a baseline/source parity acceptance or a reason to weaken the
fingerprint check. Regenerate the complete active SDK inventory and requalify its
integration after the runtime/generator optimization is finalized.

## Consuming graph binding: implementation checkpoint

A runtime rvalue overload of `EntityRuntimeState::with_shared_graph` now transfers
the receiver's private ledger, native original snapshot and dynamic read values
without first cloning them. Its graph binding and JSON view come from the source,
matching the existing lvalue operation. The lvalue operation retains copy
semantics; consumers must explicitly move a receiver to select the new path.
Same-domain binding preserves the existing graph slot and its weak/strong flag.
Cross-domain binding invokes the existing graph clone scope before replacing the
receiver's graph slot; no source-owned graph pointer is retained implicitly.
Both allocation resources are validated before mutation.

The runtime-only self-test extends the retained loaded-view probe with weak and
strong sources, empty and active private ledgers, original-row payload-buffer retention,
no additional registered arena objects, source JSON view selection, private
comments, publication authority and self-binding. A separate destination Context
must retain both its private loaded values and a copied source graph after actual
source Context deletion. Native and scoped sanitizer builds are being qualified
before updating the generated `on_loaded` adapter. No source or capability
acceptance is promoted by this implementation checkpoint.

The [unchanged original hydration receipt](parity/hydration_allocation_original_exploration_5_0_6.json.gz)
now records the complete locked Rust `typed_hydration_allocation_probe` test:
1/1 passes and all 24 matrix entries are present. At 10,000 rows, both sparse and
full projections report 20,001 typed calls, 10,001 Context-bound calls, 20,015
graph-batch calls and 10,001 plain calls. These are measured values beneath the
source's ceilings, not replacement ceilings. The isolated byte-checked runtime
view, command output and original source hash are retained. This adds original
allocation evidence without accepting the outstanding C++ cost/ownership work.

The [runtime follow-up receipt](parity/consuming_graph_binding_runtime_5_0_6.json.gz)
now records native2/sanitizer2 and Java75 success. The initial new test incorrectly
required the address of the inline row wrapper to survive moving its containing
state. That failure is preserved in
[the initial assertion receipt](parity/graph_binding_initial_assertion_5_0_6.json.gz).
The corrected assertion checks the owned row-values buffer identity, which is
the relevant no-copy property. The existing Rust loaded-view corpus remains green.

After the runtime gate passed, the generated `on_loaded` adapter was changed to
`root = std::move(root).with_shared_graph(*source)`. The dedicated refresh accepts
only this exact support-header replacement in addition to the reviewed model
header, and still requires unchanged locked Rust output. The
[native generated follow-up](parity/hydration_allocation_graph_binding_native_5_0_6.json.gz)
passes all 24 samples and the generated move/deletion checks. At 10,000 rows it
reports 30,163 typed, 30,158 Context-bound and 50,171 graph-batch arena requests
for each projection. Approximately four requests per Context-bound/graph row
have been removed. Registration bookkeeping and remaining ownership costs are
still counted; the allocation budget remains explicitly unaccepted.

The [generated sanitizer matrix](parity/hydration_allocation_graph_binding_sanitize_5_0_6.json.gz)
now passes all 24 rows plus the move/deletion contracts. All arena and heap count/
byte columns match the native matrix; elapsed time is deliberately excluded from
comparison. Both receipts identify identical probe/runtime inputs and SDK hashes.
The [dedicated reproduction](parity/hydration_graph_binding_reproduction_5_0_6.json.gz)
is exact. The runtime receipt also retains a final native no-work build. Full
formatting and module-path checks pass. All-SDK fingerprint migration and imported
module qualification remain pending, and the hydration source stays unaccepted.

## Transfer the consumed decoder input row

The existing runtime rvalue `set_original_compact_row` already transfers the
native value buffer and clears read sidecars before retaining the snapshot.
The strengthened `immutable_original_row_sharing` contract installs both a loaded
relation and dynamic-field values, then compares copied and moved original rows:
both preserve identical native scalar values and strip the sidecars. The moved
row retains its prepared native buffer. Existing immutable sharing, independent
materialization, mutation overlay and cross-domain retention assertions still
execute. The [runtime receipt](parity/original_row_transfer_runtime_5_0_6.json.gz)
records native1/sanitizer1 and Java75 success.

The generator now passes `std::move(row)` only at the final original-snapshot
installation, after native fields, forward relations and indexed availability
have been decoded. No runtime implementation was changed for this step. The
generated probe additionally checks that the actual decoder retains the input
value-buffer identity, then exercises its existing local move and cross-domain
retention after source deletion.

The [native 24-row matrix](parity/hydration_allocation_original_row_native_5_0_6.json.gz)
reports 20,163 typed, 20,158 Context-bound and 40,171 graph-batch arena requests
at 10,000 rows, identically for sparse/full projections. The decrease of exactly
10,000 requests on each path follows elimination of one native-row vector copy
per entity. Context-bound and graph budgets remain unqualified; metadata and
cleanup requests have not been excluded from the measurement.

The next allocation-site review is `Arena::construct`: nontrivial objects
currently allocate a cleanup record and object storage separately. Co-locating
those two storage regions could reduce request count without removing cleanup,
object IDs or lifetime checks. Any such kernel change requires alignment, nested
construction, throwing constructors, explicit destruction, budget and actual
Context-close verification; it is not implemented or accepted at this checkpoint.

The [original-row transfer sanitizer matrix](parity/hydration_allocation_original_row_sanitize_5_0_6.json.gz)
passes the same 24 rows and generated buffer-transfer/Context-deletion assertions.
The [comparison and reproduction receipt](parity/original_row_transfer_reproduction_5_0_6.json.gz)
checks identical native/sanitizer allocation counts and requested bytes for the
same source/runtime/SDK inputs, excluding timing, and retains exact dedicated SDK
reproduction. These focused gates do not close the whole-source allocation budget
or all-SDK/module gates.

## Combined cleanup/object allocation: pending kernel qualification

`Arena::construct` now reserves nontrivial object storage and its cleanup record
in one arena request. The object starts at a base aligned for both types; the
cleanup record follows the object at its own aligned offset. Padding is counted
inside the requested size, and size arithmetic has a compile-time overflow guard.
The cleanup chain is published only after successful object construction, exactly
as before. Object registration, live bits, destruction callbacks, explicit
resource domains and terminal block reclamation remain separate responsibilities.
Trivially destructible objects retain their existing storage path.

The Context-memory test adds a request-counting Arena to verify one storage
request for payloads and warmed identified objects, including 64/256-byte
alignment. It checks explicit destruction followed by repeated close, nested
construction/destruction ordering, successful child cleanup after a parent
constructor throws, and no destructor registration after a capacity rejection.
The existing Context-memory/reference/entity-runtime contracts remain part of
the focused gate. The payload-first layout is the qualification candidate;
intermediate compilation outputs are not acceptance evidence.

The hydration probe now also checks shared generated field-layout/entity-name
identity and enforces the original call ceilings independently on arena requests
and heap calls. It still reports all requested bytes and does not compare timings
or impose a Rust byte ceiling. The unchanged thresholds are typed `3*n+1`,
Context-bound `2*n+1`, graph batch `3*n+32`, with plain hydration unbounded.
Until executed evidence and the remaining source/module review are complete,
source acceptance stays false.

`scripts/regenerate_hydration_adapters_5_0_6.py` prepares an all-active-SDK migration.
Before permitting updates it requires both current native and sanitizer hydration
receipts to pass those thresholds, checks runtime/SDK/producer fingerprints,
regenerates to staging, and allows only the three reviewed adapter transformations.
Undoing those transformations and applying the pinned formatter must reproduce
each old file exactly. Historical Rust and C++ fixtures remain protected. The
inverse transform has been checked against the dedicated SDK at committed HEAD;
the all-SDK migration itself has not yet run.

The first native and sanitizer smoke selections each pass the three Context
memory/reference/entity-runtime tests. Because the public arena header was
refined to the payload-first layout during that initial compilation, these smoke
results do not qualify a stable final input set. Both profiles are now rebuilding
against the fixed layout, followed by eight focused tests covering memory,
references, entity runtime, asynchronous shutdown, provider shutdown, loaded-view
parity/contracts and retained cache costs. Only after those commands complete
will the controller execute the full hydration matrix with allocation ceilings.
The all-SDK update remains gated on the resulting fresh receipts.

The prepared all-SDK integration controller now selects native26/module20,
including runtime and generated runtime-state bridge contracts/parity alongside
graph/JSON/ledger and ownership regressions. It retains the complete generated
positive/negative compiler gate, canonical module consumers, no-work builds and
unchanged-input checks. This controller has not run yet. SDK installation also
requires the final native8/sanitizer8 kernel safety receipt, not just allocation
measurements.

A separate remaining helper discrepancy is confirmed by source inspection:
Rust's allocation counter enables/counts per thread, while the existing C++ test
helper uses executable-global counters. The current hydration and cache-cost
probes perform their measured work on one thread, so their receipts do not claim
thread-isolated counting. Qualification of `support/allocation_counter.rs` still
requires a thread-isolation adaptation and tests; that source remains pending.
Do not change instrumentation during the active final kernel measurement epoch.

## Stable combined-allocation outcome and lazy graph follow-up

The [stable kernel receipt](parity/combined_cleanup_runtime_pre_graph_5_0_6.json.gz)
records native8/sanitizer8 and no-work builds before the next graph change.
The full hydration ceiling check nevertheless fails. The
[detailed native failure](parity/hydration_allocation_combined_native_failure_5_0_6.json.gz)
retains partial output and the failing command: sparse 10,000-row graph batch
uses 30,170 arena requests against 30,032 allowed. Typed/context paths at that
size use 10,163/10,158 requests and are below their respective limits. The probe
now flushes each measured row before checking its ceiling, and its controller
retains failed commands as incomplete receipts. No failing receipt can authorize
SDK migration. The old buffered failures did not preserve per-case output.

An unbound entity previously allocated a graph slot during construction, even
when a later Context attachment immediately replaced that slot. The new runtime
candidate leaves an unbound graph unmaterialized. First copying/sharing or graph
publication materializes its slot through the same registered `Arena::make` path;
object IDs, allocation ceilings and cleanup are not bypassed. Empty graph reads
and diagnostic rendering handle the unmaterialized state without allocation.
Copying an empty root materializes a shared identity before either view publishes,
so later publication remains visible through both. Weak/strong roles, JSON views,
private ledger/loaded values and explicit cross-domain graph cloning are retained.
Construction still validates the allocation resource and thread before binding
its lifetime anchor.

The loaded-view self-test now verifies allocation-free unbound construction/read,
exactly one registered slot on first sharing, publication through aliases, and
copy-before-publication identity. The stable candidate is rebuilding under both
profiles with fifteen focused tests: the preceding eight plus entity-runtime and
graph model/contracts/parity regressions. Hydration measurements run only after
that selection passes. The SDK migration controller now requires this enlarged
kernel receipt. Lazy-graph and hydration source acceptance remain pending.

## Lazy graph kernel checkpoint

The first native/sanitizer replay each passed fourteen of fifteen tests. The
retained [initial failure](parity/lazy_graph_initial_clone_count_5_0_6.json.gz)
identifies an obsolete allocation expectation: first Clone now materializes
both the deferred ledger binding and the deferred graph binding, rather than
only the ledger binding after eager graph construction. The model test now
requires exactly two registered bindings, no mutation ledger, and no additional
bindings on subsequent Clone. Sharing and mutation visibility checks remain.

The [completed kernel checkpoint](parity/combined_cleanup_runtime_5_0_6.json.gz)
records native15/sanitizer15 passing and both subsequent builds reporting no
work. The strict [native hydration probe](parity/hydration_allocation_combined_native_5_0_6.json.gz)
and [sanitizer hydration probe](parity/hydration_allocation_combined_sanitize_5_0_6.json.gz)
pass all twenty-four samples and positive controls. Allocation counts and byte
counts match between profiles; elapsed times are not compared. At 10,000 rows,
typed and Context hydration each request one arena allocation for the reserved
result vector; graph batches request 20,014, below the unchanged 30,032 ceiling.
The first Context attachment still registers the shared graph binding. All
registered objects retain their existing ID and lifetime checks.

This permits the restricted all-SDK adapter regeneration gate to proceed. It
does not accept the complete hydration source: imported-module integration,
all-SDK reproduction, allocation-counter thread isolation and final source
review remain separate requirements. No source or capability is promoted here.

The [all-SDK update receipt](parity/generated_hydration_sdks_update_5_0_6.json.gz)
now records all eighty active SDKs reproduced and installed. Changed generated
code is restricted to allocator-aware moves, consuming graph attachment and
original-row transfer; 2,467 protected artifacts remain unchanged. A fresh,
strict byte-for-byte reproduction is running before the repository generator
verifier and native/imported-module integration. Installation alone does not
qualify those downstream gates.

## Allocation-counter thread isolation candidate

During the unchanged-input SDK integration epoch, an isolated candidate is kept
outside the worktree. Only the test helper's `Counts` and measurement flag become
`thread_local`; this storage owns measurement scalars, never Context data. The
[retained candidate experiment](parity/allocation_counter_thread_isolation_candidate_5_0_6.json.gz)
includes the exact candidate and original helper sources, the test and command
outputs. The original shared helper fails the same test; the candidate passes
native and ASan/UBSan with leak scanning disabled.

Two threads overlap measurement lifetimes but serialize actions with acquire/
release handshakes. This makes the original-helper comparison deterministic
without introducing a counter data race. Each thread must begin disabled and
retain its own counts, requested bytes and enable flag despite another thread's
reset and disable. Exact positive controls cover ordinary, array, zero-size and
aligned allocations, arena-entry requests, non-counted frees and disabled
allocations. Arena calls use an explicit stub in this isolated experiment; it
proves helper isolation, not real Context or allocator integration. Installation,
actual Context consumers, module verification and source acceptance remain open.
No active build input was changed for this experiment.

The [second all-SDK reproduction](parity/generated_hydration_sdks_reproduced_5_0_6.json.gz)
has completed: all eighty SDKs are byte-identical to the installed outputs.
`verify_generator_parity.py` now passes for the forty-two locked generator
sources and seventeen capability records; six capabilities remain unqualified.
The native/imported-module integration controller is running with frozen inputs.
This closes the previously reported producer-fingerprint mismatch, not the
remaining source or capability acceptance requirements.

## Real Context counter-isolation follow-up

The staged helper now also passes with actual thread-owned GlobalContext and
UserContext arenas: [native](parity/allocation_counter_real_context_candidate_5_0_6.json.gz)
and [ASan/UBSan](parity/allocation_counter_real_context_sanitize_candidate_5_0_6.json.gz).
No arena stub is used in these two receipts. Each thread creates and warms its
own Contexts outside measurement, then holds an overlapping measurement interval
while acquire/release handshakes serialize observations. The parent observes
exactly two arena requests totaling 62 requested bytes; the worker independently
observes two totaling 100. Ordinary/aligned/array/zero-size heap controls retain
the preceding exact counts, and deallocation/disabled operations add no counts.
Default PMR is disabled, linked library hashes stay unchanged, and Contexts are
destroyed after measurement on their creating threads. Leak scanning remains
disabled for the sanitizer run.

These are isolated candidate executions with the real runtime, not installed
helper acceptance. The repository helper and all captured integration inputs
remain unchanged. Installation must be followed by the readonly-property,
state-cache and full hydration consumers plus canonical module execution. The
complete helper review must also map each consumer's reset/timing/result and
exception-disabling boundaries to the source `measured` contract; thread-local
counter storage alone is not that complete review.

## Queued installation after hydration integration

The [post-integration controller record](parity/post_hydration_candidate_installation_5_0_6.json.gz)
retains a queued, self-contained installer. It waits for the observed integration
process to exit, requires its complete success receipt, and verifies the full
captured input/reference map and destination hashes before writing anything.
Failure or drift stops installation. While its state is `waiting_for_integration`,
no candidate has been installed by this controller.

The prepared changes install only the reviewed TLS helper, its real concurrent
Context test, the seven-contract trace-chain counterpart, and the expanded core
LIKE test. The existing hydration probe becomes a native/canonical-module CTest
target. Both allocation executables receive the same pointer-width-specific
arena wrapper used by existing cache measurements. Module contracts and preset
targets are registered without modifying generated output.

After installation the controller runs pinned formatting and exact native10/
module10 selections covering allocation, hydration, trace-chain, core functional,
readonly, cache, request-scope, loaded-view and entity-runtime behavior. A focused
sanitizer allocation-counter gate follows. Canonical module-body, no-work and
unchanged-input guards are required. The controller never promotes source or
capability dispositions; terminal execution and source review remain separate.

## Plain-decoder helper fidelity gap

A full helper-body audit found a remaining limitation beyond the valid 24-row
matrix. The source `plain` helper supports present Text notes as well as missing
and Null notes, and matches U64 IDs/I64 versions exactly. The installed probe
only checks its prepared missing/Null note inputs and uses permissive numeric
accessors. Those samples still prove their recorded budgets and values, but do
not establish the helper's complete branch behavior.

The [staged C++ helper](parity/plain_hydration_helper_candidate_5_0_6.json.gz)
uses exact variant checks and an allocator-propagated optional String. It passes
four valid forms (absent, Null, empty Text, long Text) and rejects signed IDs,
unsigned versions, numeric names and numeric notes. The
[original helper observer](parity/plain_hydration_helper_original_5_0_6.json.gz)
copies the exact Plain struct/function excerpt from the locked source into a
supplemental temporary test, preserving every original runtime source/test body.
Its four accepted/four rejected cases agree; actual source panic diagnostics are
retained rather than normalized. The C++ candidate uses checked exceptions and
does not claim byte-identical Rust panic formatting.

This helper candidate is not part of the already queued installation controller.
After that controller terminates, integrate the helper and its checks into the
hydration executable and rerun the complete 24-row native/imported matrix with
unchanged ceilings. Do not promote hydration_allocations.rs solely from the
queued matrix, even if it passes. Existing standalone and queued receipts retain
their exact earlier scope.

The [complete staged hydration probe](parity/hydration_plain_complete_native_candidate_5_0_6.json.gz)
now incorporates that helper and its eight branch checks into the full existing
probe. Native execution passes all twenty-four matrix samples, move/row-transfer
contracts, positive controls and unchanged ceilings. Every allocation-count and
requested-byte column matches the preceding native combined-allocation receipt;
elapsed times are excluded. The receipt retains the full candidate source,
compile/link commands and unchanged runtime/generated/helper/library hashes.
It remains outside active build inputs. Installation and canonical module replay
must occur after the currently queued controller terminates, preserving that
controller's immutable input snapshot and earlier scope.
