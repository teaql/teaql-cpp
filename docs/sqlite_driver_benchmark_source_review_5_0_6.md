# SQLite matched-driver benchmark source review (Rust 5.0.6)

Status: accepted for this benchmark source under DB01-DB13. The final acceptance
section records the complete gate; earlier sections retain historical checkpoints
and their limitations. This is not whole-runtime or cross-language performance
acceptance.

The locked source is `teaql-provider-sqlite/tests/driver_query_benchmark.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`: 453 lines, one opt-in ignored test,
and 32 assertion macro sites. Its SHA-256 is
`80dd2a71e00da7781ff90e3b52798b401eb9cda6bc9b269a2f3f31456066e9a1`.
The transitive allocator helper is
`teaql-runtime/tests/support/allocation_counter.rs`. Ordinary default Cargo
execution skips the benchmark and is not evidence that its assertions passed.

## Complete source responsibilities

| ID | Source responsibility | Required evidence |
| --- | --- | --- |
| DB01 | Indexed `DriverProbe` carrier with id/version/name/nullable note, revision `driver-probe-v1`, four stable indexes/mappings, skipped load-state carrier and typed runtime state | Actual typed hydration, field loadedness and original snapshots; retain explicit omission versus SQL NULL |
| DB02 | Schema lookup resolves only DriverProbe; production SQL executor and actual SQLite transport stay independent | Real schema initialization and metadata-backed runtime query, not a fixture-generated result |
| DB03 | Separate RecordingTransport correctness lane delegates actual compact fetch/execute and captures compiled queries | Exactly one actual statement per list query, exact SQL including comment and exact parameter values/order; masked logs cannot stand in for bindings |
| DB04 | Deterministic raw setup inserts 10,000 rows with version 1, `row-{id}` and alternating NULL/nullable note inside a transaction | Setup and provider/model/connection initialization excluded from every measured sample; no claim that raw setup is application Save |
| DB05 | Sparse/full projections and counts 1/100/10,000 use version > 0 and name prefix, ID ascending, explicit limit/comment/purpose | All six query shapes produce identical native/runtime ordered tuples and projection semantics; runtime compiler SQL equals separately compiled driver SQL |
| DB06 | Actual default-log query validates SQL equality and masked diagnostics before warmup | First-runtime-shape observation is labeled accurately: capture already touched the database, so this is not process/database cold start |
| DB07 | Twenty warmups followed by 31 samples, alternating native/runtime order | Both paths measured with matching SQL/bindings and prepared statement behavior; all samples verified, logs cleared outside the measurement |
| DB08 | Every measured result checks row count and tuple equality, note loadedness and shared immutable snapshot identity | Real pointers/tokens identify one shared snapshot across rows; value equality alone is insufficient |
| DB09 | Log mode is exactly off or default; default mode starts with default runtime options | One log per measured runtime sample with exact comment/purpose and masked params in default mode; no retained logs in off mode |
| DB10 | `allocation_layers`: compact transport fetch, prepared plain decoding, prepared typed decoding | Setup/cloning of input outside counters; result count/tuple/loadedness/shared-snapshot/clean-dirty-state assertions retained |
| DB11 | First and repeated runtime-state cloning measured separately from SQL and typed-field cloning | Cloned states retain exact id/version/name/note presence/value, empty changes, and all original entities remain clean |
| DB12 | Percentiles use ceil(rank)-1; reports include raw samples, p50/p95, throughput and median allocation tuple | Preserve 31 samples and explicit output labels; do not compare noisy times or allocator-dependent counts as exact semantic parity |
| DB13 | Allocator measurement brackets only the action and black-boxes its output | Calls/requested bytes include allocate/zero-allocate/reallocate requests, not deallocation or live/RSS bytes; measurement disabled before result verification and destruction |

## C++ ownership and measurement boundaries

The source's Rust global allocator counter is a measurement tool, not permission
to replace Context ownership or introduce a hidden allocation domain. C++ query,
row, snapshot and expression data must retain their explicit Global/User scope.
Any instrumentation must distinguish logical Context allocation requests from
upstream arena block requests and from SQLite's native allocation. A counter
covering only one of these cannot be labeled a universal allocation total.

The original Rust result drops release allocations after each sample; C++ arena
storage may persist until UserContext closes. The counterpart must explicitly
account for this difference. Before recording performance, choose and document
an application-visible Context lifetime for every sample, place Context setup
and teardown outside the query counter when that is the stated boundary, and
retain real same-scope snapshot assertions. Reusing a Context until an allocation
limit fails, silently resetting an arena, or adding a private query ownership
region does not reproduce the intended workload. Shared metadata/provider
resources may use an explicitly configured Global lifetime; results still belong
to their UserContext and must not be retained past its destruction.

There is no required equality between Rust and C++ wall times or allocation
counts. The intended evidence is equivalent verified work with honest boundary
labels, original raw measurements, toolchain/build/SQLite versions and input
hashes. Do not publish speedup or memory savings from unmatched paths.

## Implementation and qualification sequence

1. Execute the unchanged locked original ignored test explicitly in release,
   in both log modes, using an owned temporary target/database area and the
   exact dependency lock. Retain compact output and source/helper hashes;
   reclaim disposable outputs after all subprocesses exit. Inspect every
   original correctness assertion, not only the final PASS line.
2. Implement the actual C++ indexed fixture, raw prepared SQLite lane and
   delegating recording transport. Keep timing/counter work separate from
   semantic comparisons. Use the runtime's existing typed query/hydration and
   state-cloning protocols; repair shared mechanics before fixture adapters if
   an actual source difference appears.
3. Verify all six query shapes, both logging modes and every allocation-layer
   correctness contract with native and imported consumers. Verify actual
   Context teardown if the fixture adds ownership/lifetime machinery. Register
   routine deterministic contracts separately from opt-in performance runs.
4. Run the opt-in C++ release measurement with documented Context boundaries.
   Preserve all raw samples and stage observations; report measured values as
   observations, not cross-language performance parity. Keep setup, diagnostic
   capture, correctness assertions and destruction outside the stated counters.
5. Reconcile DB01-DB13, every original assertion and helper, generated-model
   versus handwritten-fixture scope, and original/candidate artifacts before
   promoting the source disposition. No baseline or P0-P7 acceptance follows
   from adding the benchmark alone.

At the initial review, this source had no executed counterpart receipt. Existing SQLite provider,
loaded-row and query tests are candidate building blocks, not its acceptance.
The earlier full-native integration epoch was deliberately interrupted to repair
confirmed entity-save scope discrepancies. There is currently no executable-input
freeze preventing benchmark implementation. Preserve that interrupted evidence
without treating it as a passing integration gate.

## Original release execution completed

The [original receipt](parity/sqlite_driver_benchmark_original_5_0_6.json.gz)
now runs the unchanged ignored test explicitly with `--release --offline
--locked --test driver_query_benchmark -- --ignored --nocapture`, once for each
`TEAQL_DRIVER_BENCHMARK_LOG_MODE` value (`off` and `default`). Both runs report
one passed test and no failures. SQLite reports version 3.53.2. The exact source,
allocator helper, dependency lock, commands, environment overrides, complete
stdout/stderr and exit codes are retained. The originals remain unchanged.

The [output validation](parity/sqlite_driver_benchmark_original_validation_5_0_6.json.gz)
checks exact Cartesian coverage rather than just the final PASS line. Each mode
contains 372 raw samples (two implementations, two projections, three row counts,
31 samples), 30 allocation-layer observations (five stages across six shapes),
six actual driver SQL/binding matches, six first-runtime observations and twelve
summary rows. Every expected sample index and stage/shape appears exactly once.
The original assertions additionally verify tuple equality, projected note
loadedness, shared snapshot identities, clean state and log-mode/privacy rules.

The first command elapsed approximately 271 seconds including a fresh release
build; the second command reused that task-owned target and took approximately
4.85 seconds. These command durations are not query latency or comparative
performance claims. The 199,872,375-byte temporary tree was reclaimed after both
processes exited, and its absence was verified. Two compact receipts retain the
measurement evidence; no original build or database tree remains.

Step 1's original execution is complete. C++ implementation, matched measurement
boundaries, native/imported correctness, lifetime evidence and source acceptance
remain pending. This original evidence does not change any source or capability disposition.
The earlier integration epoch was interrupted; its input freeze is no longer active.

## C++ actual-driver correctness lane implemented

The handwritten `DriverProbe` counterpart and delegating recording transport live
in `tests/support/sqlite_driver_benchmark_fixture.hpp`; the runnable fixture is
`teaql_provider_sqlite/tests/driver_query_benchmark.cpp`. The carrier keeps the
original entity/table names, `driver-probe-v1` revision and four field indexes.
It uses production indexed snapshots and runtime state rather than implementing
load-state behavior locally. This is a handwritten benchmark carrier, not a
new generated application SDK or a generator-parity claim.

The native correctness entry passed all twelve logging/projection/count
combinations. Runtime schema initialization precedes raw transactional setup of
10,000 deterministic rows. A separate native SQLite prepared-statement path
binds the independently compiled query's values, and the recording transport
captures the actual runtime query while delegating to the production provider.
The fixture asserts exact SQL including comment, parameter values/order, one
statement, ordered tuple equality, NULL versus omitted note loadedness, shared
snapshot identity and clean decoded state. Default diagnostics retain exact
intent/SQL and masked parameters; logging-off retains no diagnostics.

The connection/provider are explicitly Global-owned; each shape/logging run
creates and destroys its own UserContext. No private query allocation region or
hidden ownership domain is introduced. Native baseline rows are ordinary C++
RAII values, independent of the Context-borrowed runtime result. The only initial
compile issue was an ambiguous local `request` helper name; it was renamed to
`make_query` without changing runtime APIs.

A canonical module target and routine deterministic CTest are registered.
Post-format native/module correctness each passed (1/1); the canonical consumer
check verified 1,151 bodies. Warmups, timed samples,
allocation-layer measurements, first/repeated state-clone stages, release output
and complete DB01-DB13 acceptance remain pending. No timing, speedup or memory
saving claim follows from this correctness lane.

## Optional sampling implementation checkpoint

The executable now accepts `--measure`; ordinary CTest remains a deterministic
correctness run. Sampling performs 20 warmup pairs and 31 measured pairs for each
of the twelve shapes/modes, alternating lane order. Every pair retains the same
ordered tuple, nullable/omitted-field, snapshot identity, clean-state and logging
assertions. The measured runtime transport delegates directly to the provider;
the separate correctness run retains recording and exact SQL/binding assertions.

Each pair has a fresh UserContext. Metadata/service/query setup and independent
native SQL compilation precede both measurements. Timing excludes verification,
log clearing, result destruction and Context destruction. Runtime timing includes
its actual query compilation and execution; native timing starts with the already
compiled SQL and includes SQLite preparation, binding and typed row collection.
This is query-path work, not end-to-end request latency or cold-start performance.

Raw `SAMPLE` columns are log-enabled, full-projection, row count, sample index,
lane, nanoseconds, Context allocation requests, Context requested bytes, C++ heap
requests and C++ heap requested bytes. The existing test-only counter brackets
the action and disables itself on success or exception. SQLite's internal malloc
requests are outside these counters; bytes are requested bytes, not live memory
or RSS. Measurement setup owns no Context data outside the explicit lifetime.

This checkpoint does not complete DB06 or DB10-DB12: first-runtime observations,
five isolated allocation layers and percentile/throughput summaries still need
implementation and qualification. A Debug sampling execution is only a harness
check; release performance evidence remains a separate required gate.

After adding the sampler, pinned formatting passed for 1,802 handwritten files.
Native and imported correctness each passed 1/1, and the canonical consumer
verifier again passed 1,151 bodies. These checks cover both log modes and all
six shapes; they do not establish allocation-layer or release qualification.

The [Debug sampling receipt](parity/sqlite_driver_sampling_debug_5_0_6.json.gz)
completed with exit zero and exactly 744 unique samples: two logging modes,
two projections, three counts, 31 iterations and two lanes. All runtime samples
record nonzero Context requests/bytes; all native samples record nonzero C++
heap requests/bytes and zero Context requests. Every sample passed the same
semantic assertions. This run overlapped module compilation and is strictly a
harness check, not usable performance comparison. No disposable build tree or
database was created; stable preset outputs and the compact receipt are retained.

## Allocation layers and statistical summaries implementation

The counterpart now includes all five isolated stages from DB10/DB11: actual
compact provider fetch, prepared plain decode, prepared typed decode, first
runtime-state clone and repeated runtime-state clone. Row copies and root-state
setup occur outside the counters. Plain decoding uses independent RAII native
rows; typed rows and cloned states use the active UserContext. Assertions check
original id/version/name/note, omission versus NULL, shared shape identity and
clean original/cloned mutation state. Routine correctness invokes these stages
for every shape and log-mode label; direct transport stages do not create SQL
execution diagnostics, so the label does not imply logging at that lower layer.

The optional mode emits 24 summaries in addition to 744 raw samples and 60 layer
observations. Percentiles use sorted nearest-rank (ceil(rank)-1); throughput is
queries per second over total sampled query duration, not rows per second. The
allocation median is the lexicographically sorted four-counter tuple (Context
requests/bytes, heap requests/bytes), not four independent component medians.
`scripts/verify_sqlite_driver_samples.py` independently verifies Cartesian
coverage, recomputes each summary and retains complete output in a gzip JSON
receipt, including command, build label, binary hash and validation outcome.

The initial stage implementation used `empty()` for EntityChangeSet; compilation
correctly rejected it. The fixture now uses the existing `is_empty()` protocol.
No production runtime API was changed to accommodate this test. Qualification
of this new checkpoint is pending; earlier receipts describe earlier source.

The first native/imported layer runs both failed with Context byte-budget
exhaustion: unlike a single query, the layer fixture retains input copies,
decoded batches and state clones until its Context closes. The benchmark now
explicitly configures a 512 MiB per-UserContext/live-user ceiling, matching the
existing hydration allocation probe. This is a ceiling, not a preallocation;
production defaults are unchanged. No arena reset or implicit lifetime extension
is introduced. Revalidation is required after this fixture configuration change.

The 512 MiB fixture configuration passed native and imported correctness (1/1
each), with all 1,151 canonical bodies verified. The
[layer Debug receipt](parity/sqlite_driver_layers_debug_5_0_6.json.gz) validates
744 samples, 60 layer observations and 24 summaries, independently recomputed
from raw samples. This supersedes the failed default-budget layer runs, not the
remaining Release or lifetime gates.

A subsequent source change adds twelve explicit FIRST_RUNTIME_DEFAULT_LOG
observations, after correctness capture and before allocation layers/warmups.
Each uses a fresh UserContext with default logging even for the subsequent
logging-off workload; verification checks its masked SQL and exact intent.
It is not a database/process cold-start observation. The sample verifier now
requires all twelve observations. This latest change still needs qualification.

`scripts/run_sqlite_driver_release.py` configures the pinned native preset in
an owned temporary directory with a Release override, builds only this target,
runs the sample verifier, and reclaims that directory after child processes
finish. Commands, complete build output, source/binary hashes, reclaimed bytes
and removal status are retained in compact receipts. It must be run from the
activated toolchain environment. The script is prepared but not yet executed.

## Measurement adaptations requiring explicit interpretation

The original native Rust lane uses rusqlite `prepare_cached`. The C++ native
pre-cache lane used SQLite prepare/finalize per query, while the production
provider caches SELECT statements (`SqliteCall<Rows>` passes `cached=true`).
This mismatch motivated the native cache correction below; the earlier receipts
cannot qualify cached Release measurement. Exact SQL and bindings are matched within the C++ comparison;
this does not make its statement-cache workload numerically comparable to Rust.
The native C++ lane receives precompiled SQL and bindings, while the runtime
lane includes runtime compilation, hydration, governance and optional logs.
These boundaries must accompany any reported measurements.

The C++ plain layer reads a prepared Context-owned row batch into independent
RAII values; its input batch remains until Context close. Rust consumes its
prepared Vec rows. Deallocations are not counted, and layer observations make
no elapsed-time claim. C++ typed/state inputs and outputs likewise retain their
explicit Context lifetime. The original core value, projection, snapshot and
clean-state assertions remain mandatory in both implementations.

The [verifier controls](parity/sqlite_driver_verifier_controls_5_0_6.json.gz)
accept a complete synthetic transcript and reject missing samples, layers,
first-runtime observations and an incorrect percentile. The synthesized first
observations are only parser controls, not runtime execution evidence. Their
owned temporary directory was removed after execution.

Source review found the native/runtime prepared-statement mismatch while the
first Release build was running. The owned Ninja process was intentionally
interrupted, its parent command reaped, and the runner reclaimed the temporary
build directory. The [interruption receipt](parity/sqlite_driver_release_uncached_interrupted_5_0_6.json.gz)
records failure and cleanup; no Release sample run occurred. Earlier wording
that both C++ lanes prepare/finalize per query was incorrect and has been fixed.
The next required implementation is an explicitly Global-owned native statement
cache with reset/clear-bindings between uses and finalization before connection
destruction. No production provider changes are required for this benchmark fix.

The pre-cache source passed native/module correctness and targeted ASan/UBSan
correctness (leak scanning disabled). These are semantic/lifetime checks, not
acceptance of the unmatched timing path. The interrupted Release tree reclaimed
15,782,571 bytes and its absence was verified.

The next candidate adds `NativeStatements`, explicitly allocated through
`global.make_resource`, with allocator-propagated SQL key copies and owning
SQLite statement handles. It uses persistent preparation, resets/clears bindings
on every checkout exit, reuses the six workload shapes, and checks that explicit
cache cleanup finalizes six statements with no busy cursor left. Its bounded
six-shape benchmark cache is not a new production cache abstraction. This latest
implementation requires fresh native/imported/lifetime and sampling gates.

The [cached correctness receipt](parity/sqlite_driver_cached_correctness_5_0_6.json.gz)
records successful native/imported execution of the corrected cache candidate,
1,151 canonical bodies and 1,802-file formatting/check. Both tests verify six
cache misses across the twelve shape/mode runs, subsequent cache hits, no busy
statement at completion and exact six-statement finalization on explicit clear.
Debug sampling, focused sanitizer and a fresh Release build are running; their
results must not be inferred from this correctness checkpoint.

## Clean-state observer correction

A second helper review found that handwritten Probe inherited
`EntityDefaults::dirty_fields()`, which always returns an empty optional.
Consequently the earlier entity-level clean assertions were vacuous, although
cloned runtime-state change-set assertions exercised real state. Earlier passing
receipts remain valid only for their independently exercised contracts and must
not be cited as complete DB08/DB11 evidence.

Probe now exposes its actual runtime state and derives dirty fields through
`EntityRuntimeState::changed_field_names` for its entity key, matching the
existing generated adapter. After layer measurements, a positive control stages
a real name change, requires `dirty_fields` to contain name, clears the change
set and requires clean state again. This control and all inspection stay outside
allocation measurements. No generated source or production runtime was edited.

The second in-flight Release build was deliberately stopped before measurement;
its [interruption receipt](parity/sqlite_driver_release_dirty_observer_interrupted_5_0_6.json.gz)
retains the reason, process result and temporary-tree cleanup. Cached native and
module correctness and the [cached sanitizer gate](parity/sqlite_driver_cached_sanitizer_5_0_6.json.gz)
precede this observer correction and therefore require a fresh final gate.
The latest observer implementation is undergoing native compilation/verification.

The [real state observer correctness gate](parity/sqlite_driver_state_observer_correctness_5_0_6.json.gz)
now passes native 1/1 and imported 1/1, including the dirty-name/clear positive
control in all twelve shape/mode combinations. Canonical verification remains
1,151 bodies; formatting/check covers 1,802 handwritten files. These results
replace the vacuous entity-level clean-state assertion evidence. Fresh complete
sampling, targeted sanitizer and Release gates for this source are running.

## Current real-observer checkpoint

The [complete Debug sample gate](parity/sqlite_driver_state_observer_debug_5_0_6.json.gz)
passes all 744 samples, 60 layers, 24 independently recomputed summaries and
12 first-runtime/default-log observations with the cached native baseline and
real dirty-state observers. The [focused ASan/UBSan gate](parity/sqlite_driver_state_observer_sanitizer_5_0_6.json.gz)
passes the twelve-shape correctness/layer/dirty-positive-control workload. Its
input hashes match the current source and fixture. Native/imported correctness
was already qualified for those same inputs. The pinned upstream verifier again
confirms Rust 5.0.6 at the locked commit. Release remains in progress.

Allocation instrumentation is owner-thread-local and counts C++ new requests
and wrapped Arena requests separately. This SQLite workload executes on that
owner thread; the receipt is not a measurement of arbitrary worker threads or
SQLite's native malloc traffic. Functional result inspection after every sample
keeps the measured outputs observable; setup, assertions, cache cleanup and
Context teardown remain outside the measured action.

## Complete source acceptance

The [source acceptance receipt](parity/sqlite_driver_source_acceptance_5_0_6.json.gz)
binds the locked original source/helper, final candidate inputs and all retained
gate receipts by SHA-256. The unchanged original runs pass both log modes. Final
native/imported correctness and targeted ASan/UBSan each pass 1/1; all 1,151
canonical consumer bodies and 1,802 handwritten-file format checks pass.

The [Release build](parity/sqlite_driver_release_build_5_0_6.json.gz) uses the
pinned native toolchain with an explicit Release override. Build and measurement
exit successfully, qualification input hashes remain unchanged, and all current
inputs still match the receipt. The [Release transcript](parity/sqlite_driver_release_samples_5_0_6.json.gz)
contains 744 samples, 60 layer observations, 24 independently recomputed summaries
and 12 first-runtime/default-log observations. SQLite is the pinned 3.53.2 static
library recorded in the CMake cache and toolchain lock. No numerical comparison
with Rust timings or allocation counts is asserted.

| Responsibilities | Final exercised counterpart |
| --- | --- |
| DB01-DB02 | Exact indexed DriverProbe identity/layout, production metadata/schema/executor/provider separation, actual SQLite schema and typed hydration |
| DB03-DB05 | Delegating capture; exact actual SQL/comment/bindings; transactional 10,000-row setup; six shapes, deterministic ordered tuples and NULL/omission assertions |
| DB06-DB09 | Default-log first-runtime observation, 20 warmups, 31 alternating sample pairs, complete per-sample value/load-state/shared-identity/real dirty-state checks and exact masked/off diagnostics |
| DB10 | Actual compact fetch and prepared plain/typed decoding; input preparation excluded; complete values and indexed shape assertions |
| DB11 | First/repeated state cloning; retained original id/version/name/note presence/value; actual empty change sets and positive dirty-name/clear observer controls |
| DB12 | Raw transcript plus nearest-rank p50/p95, queries/second and lexicographic allocation medians; independent recomputation and incomplete/corrupt-output rejection controls |
| DB13 | Scoped owner-thread measurement, exception-safe counter disable, result verification outside counters, explicit Context/new/SQLite allocation boundaries and no live/RSS claim |

The native prepared-statement mismatch and inherited empty dirty observer were
corrected before this gate. Earlier passing receipts do not retroactively prove
those responsibilities; both deliberate Release interruptions remain retained.
The final temporary Release tree was reclaimed after all children exited:
44,844,746 bytes removed and directory absence verified. Stable preset outputs
and compact evidence remain. Benchmark source disposition is now accepted;
source totals become **116 accepted / 6 pending / 2 excluded_external**. Capability
statuses remain **11 qualified / 6 partial**, and P0-P7 remain incomplete.
