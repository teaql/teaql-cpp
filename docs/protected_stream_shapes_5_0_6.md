# Protected stream shapes and configuration: Rust 5.0.6

Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This checkpoint addresses G09 EX10/EX12 and protected execution integration.
Complete G09, G13-G17 and P0-P7 acceptance retain their independent requirements.

## Current strict-source readback

The earlier startup disposition below is historical. The installed deferred
generated-stream runtime now produces **13,104/13,104 raw original results**.
The [current CLI source binding](stream_shapes_cli_source_binding_5_0_6.md)
attaches the complete original corpus and strict native readback to the actual
registered test, with 15 negative controls and four selected checks passing.
The [current strict-source replay](parity/stream_shapes_strict_source_current_native_5_0_6.json.gz)
also passes every existing connection-release, reuse, Policy, logging and
constructor-failure boundary assertion. No expected source observation or
boundary assertion is changed, and no runtime implementation changes here.

The completed full native epoch exposes an obsolete comparator requirement:
`startup_disposition` rewrote 6,012 original results to require the former
eager C++ behavior. Thus its registered stream check fails while reporting all
13,104 raw equalities. This is not a newly observed runtime scheduling gap. The
[historical negative control](parity/stream_shapes_strict_source_historical_control_5_0_6.json.gz)
confirms that raw source expectations reject exactly the 6,012 retained old C++
outputs, while preserving all original inputs. It does not claim a fresh run of
the old runtime.

The [frozen full-native attempt](parity/runtime_graph_model_full_native_terminal_5_0_6.json.gz)
finished with **876/877** tests passing, one failure, zero errors and zero skips.
Its only failure is `stream_shapes_rust_parity`; builds and frozen-input checks
passed. The comparator now compares the raw original observations directly,
without startup rewrites. The original corpus, all existing boundary assertions
and historical paired records stay unchanged. Focused registered verification
is recorded separately; it does not turn that failed full epoch into a passing
result. The owned strict replay controller (2,249 bytes) was removed after its
process exited.

The [focused registered receipt](parity/stream_shapes_strict_registered_commit_5_0_6.json.gz)
passes both `stream_shapes_rust_parity` and
`module_stream_shapes_rust_parity` after successful target builds. Each checks
**13,104/13,104 raw equalities** with no startup adaptations and all existing
boundary assertions enabled. The owned JUnit directory (927 bytes) was removed
after both test processes exited. This is two focused tests, not a new full
native integration epoch.

## Actual source and declared observations

The new observers call the unchanged generated inheritance-graph library and
actual Context/service/executor/SQLite paths. They reuse its existing schema,
data and synthetic private-name canary. Original generated sources, historical
observers and historical corpora remain unchanged. Dependency registry versions
and checksums are checked against the locked original runtime.

The 13,104 inputs cover School, Academy, ResearchAcademy and Enrollment; fourteen
query shapes; ordinary/ID-set/continuous configuration; default, zero, one and
four-row chunk requests; zero/one/full consumption; allow/restrict/deny Policy;
and logging on/off. Additional inputs exercise zero-sized, offset and beyond-end
slices, plus blank comment/purpose with and without polling.

Shapes include ordinary scalar requests, readonly values, raw field/filter/outer
SQL, facets, scalar grouping/count/sum, unavailable STDDEV, actual missing tables,
and each of the three unsupported enhancement categories: relations, child types
and object grouping. Streams do not execute list-only facet enhancement. Keeping
an optimization hint does not imply that a streaming request uses that physical
pagination protocol.

Outputs preserve complete query and before/after Policy Debug, typed JSON and
loaded fields, computed values, exact errors/panics, public stream-open outcome,
SQLite SELECT authorization callbacks and every stable retained SQL diagnostic
field. Operational timestamps/durations are outside the cross-process boundary.
Debug and policy snapshots are trusted test observations, not safe-publication
APIs. Provider failures remain failures rather than becoming empty results.

After every stream closes, the observers issue an actual generated Platform
exists request through the same Context and connection. Allowed requests must
succeed; a denied request must reach Policy and reject before SELECT. A temporarily
renamed source table is restored before reuse. Rust additionally checks pool mutex
availability; native checks SQLite statement idleness. Native disables the default
PMR allocator and actually deletes each UserContext, requiring zero live User bytes.

## Reproduced guard gap and shared repair

The [initial paired observations](parity/stream_shapes_initial_5_0_6.json.gz)
retain all original/native outputs. Of 13,104 raw observations, 3,960 agree exactly.
The explicit startup disposition below accounts for 6,012 differences. The
remaining 3,132 cases expose one implementation gap: typed native `stream(0)`
proceeds while the generated Rust request panics with
`stream chunk size must be positive` before Policy or SQL.

Shared application-facing `EntityQuery::stream` now rejects zero with that exact
message before changing query state. Generated adapters already delegate to this
method, including rvalue chaining; no producer or generated artifact needs a
change. The low-level core SelectQuery still accepts zero, matching its separate
original contract. The [complete pre-repair sources](parity/stream_shapes_before_5_0_6.json.gz)
preserve both layers. The marker-source verifier proves the complete later header
delta is this guard before replaying its unchanged historical constraint review.

## Historical startup and diagnostic disposition

Original generated `request_support.rs::fetch_entity_stream` wraps repository
resolution in a lazy `async_stream`. Its public request returns a stream before
Policy, enhancement validation or diagnostic cursor construction. Native public
execution starts those stages before returning its checked borrowed stream.
This is the previously documented native-start/Rust-poll adaptation, now observed
with no item polling as well as failures.

For consumed requests, only the public open outcome differs when Policy or an
unsupported enhancement rejects execution: Rust yields that exact failure as an
item; native returns it from startup. Complete errors, Policy snapshots, rows,
SELECT counts, safe diagnostics and connection reuse still compare exactly.

For an unconsumed request, Rust performs no Policy or SQL work. Native runs Policy
and the enhancement guards at startup. The comparator takes the complete Policy
snapshot and any startup rejection from the otherwise identical actual original
first-poll input. It checks that no SELECT or row delivery occurs in either case.
No arbitrary error or query difference is allowed.

An allowed native stream has already created its diagnostic cursor. Cancelling
it without polling therefore publishes a zero-row Cancelled record when logging
is enabled. Rust's unchanged `teaql-sql/src/diagnostic_stream.rs` independently
requires this same terminal behavior once its diagnostic wrapper exists; its
`unpolled_drop_is_cancelled_without_delivery` test records that contract. The
lazy generated wrapper never constructs that cursor in the unpolled Rust case.

The comparator derives the required cancellation record from the complete safe
SQL record of the actual original first-poll sibling, changing only outcome to
Cancelled, delivered count to zero and its corresponding count summary. SQL,
bindings, masking, trace, intent and all other diagnostic fields remain exact.
It does not delete additional diagnostics or normalize private data. Raw source
and native observations are retained separately; these cases are not labeled raw
cross-runtime equality or identical scheduling.

## Verification

All 13,104 final native results satisfy the declared contract: 7,092 are raw
equalities and 6,012 use the exact startup disposition above. The complete
original corpus also reproduces against the successfully built original bin.
The [repaired paired record](parity/stream_shapes_repaired_5_0_6.json.gz) retains
both raw outputs and the exact indices of raw differences and dispositions.

Both deliver the same 2,646 typed entities and preserve 8,784 allowed reuse
requests plus 4,320 pre-SELECT reuse denials. Source/native Policy callbacks are
6,264/9,396; retained safe SQL records are 1,800/2,700. The additional 3,132
startup Policy callbacks and 900 zero-row Cancelled records follow the exact
unpolled disposition above. None of those safe diagnostic records contains the
synthetic private-name canary. Query/Policy Debug and raw provider errors retain
their separate trusted-observation boundary.

Selected native CTest checks pass 4/4 (ordinary generated execution, dynamic
streams, the runnable generated example and the retained 4,864-case composed
matrix). A separate core query-model check passes 1/1, including its deliberate
zero-sized low-level StreamConfig. Imported-module comparisons pass 2/2 for the
new stream matrix and retained composed matrix. All compiler/linker/test processes
have exited successfully.

Formatting/check covers 1,637 C++ files, and canonical module verification covers
788 consumer/fixture bodies. The only production change is the argument guard;
ownership, cursor cleanup, async scheduling, provider and generator algorithms
are unchanged. Existing lifetime/sanitizer evidence remains checkpoint-specific;
this change does not rerun sanitizer or MIPS profiles. The
[terminal record](parity/stream_shapes_terminal_5_0_6.json.gz) binds the source,
raw paired data, current inputs and complete verification output.

## Reproduction and remaining scope

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_stream_shapes_probe
ctest --preset native --output-on-failure -R '^stream_shapes_rust_parity$'
```

The comparison accepts `--rust /path/to/stream_shapes` to replay the immutable
source corpus. Build that bin with `--offline --locked` from
`tests/parity/rust_dynamic_json_privacy_5_0_6/Cargo.toml`, with the original
registry versions/checksums verified before using the result.

The checkpoint does not qualify full configured source/result conversion,
constant/candidate/dynamic-field streams, every execution entry or the remaining
optimization/store/assembly combinations. Their prior evidence and independent
requirements remain active. No external provider, GPU adapter or physical-device
claim follows from this matrix.
