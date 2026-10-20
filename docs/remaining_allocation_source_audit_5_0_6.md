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
