# Initialized snapshot and row cache review: Rust 5.0.6

Status: 80 exact public-API semantic cases and one explicit ownership adaptation
pass, including hint counts and lifecycle gates. G15 and the complete P1/P7
responsibilities remain open.

This checkpoint follows complete original `load_state.rs` and the
CompactRowLayout/CompactRow portion of `query.rs` at the locked Rust 5.0.6
commit. The independent observer compiles those unchanged original core sources
through their public APIs. It introduces no private bridge or replacement cache.

## Cache responsibilities

| Original contract | C++ implementation and observation |
| --- | --- |
| FieldLayout carries immutable geometry and type-local weak shape hints | Immutable Global-owned FieldLayout and an explicit User SnapshotPool keep type metadata separate from result lifetime. Snapshot equality includes the actual layout identity, even for equal metadata. |
| Live hits inspect only their hash bucket; misses prune dead hints | SnapshotPool first resolves matching checked borrows, then prunes dead registrations on a miss. Explicit destruction cannot resurrect an old allocation identity. |
| Admission is bounded to 512 hints per layout | The matrix crosses 511/512/513/520 live shapes and repeats first, last and newly created projections, before and after releasing half. Hint counts and surviving state availability are observed. Saturation prevents admission, not valid results. |
| Row layouts memoize a primary snapshot and weak secondary typed snapshots | ColumnLayout memoizes checked Context borrows and keys secondary entries by managed layout identity. Repeated rows share availability without sharing mutable value changes. |
| Schema replacement retains column/relation geometry and resets memoized state | None, empty and nonempty schemas, equal schema reuse and changed readonly types preserve source geometry. Actual schema Debug includes the separately qualified cached u64 hash. |
| View-local mutation does not change cached row state | Toggle and restore a fixed field through LoadedSnapshot::with_loaded. Check the unchanged sibling, restored identity, overflow slots, named relations, durable selections and derived/unknown fields. |

The layout matrix has 72 cases: 2/65/130 fields, empty or mixed alias projections,
relations absent/present, three readonly-schema modes and equal/different model
names on distinct layout identities. Eight saturation/release cases add boundary
and cleanup observations. One final case records the ownership models separately.
The common output is a declared public-API observation: bits, sorted overflow/
selected sets, availability, schema, cache identities, hint counts and row value
isolation. Sorting unordered sets happens in both observation encoders; it does
not rewrite the actual runtime or its Debug output.

## Weak ownership and Context ownership

Rust's primary Arc keeps a snapshot alive while its row layout lives. A secondary
Weak does not keep a snapshot alive after its last returned Arc is dropped.
Dropping the row releases its primary snapshot if no other strong owner remains.

C++ borrows do not own snapshots. Snapshot allocations remain in the User arena
after wrapper or row destruction. Releasing an object explicitly, or deleting
its UserContext, invalidates its checked ID. The release matrix consequently uses
Rust last-Arc drop and C++ explicit object destruction, not an implicit C++ owner
that extends lifetime. The separate ownership observation must report these
different mechanisms; it is not compared as though both used reference counting.

Native output is copied explicitly into Global. Stack PMR values leave scope
before actual User deletion, all charged User blocks must be reclaimed, and the
retained result is then serialized. The ownership case also checks an escaped
snapshot borrow after deletion. Default PMR allocation is disabled. Global type
metadata does not retain User shape hints.

## Initialized diagnostics

Both observers retain actual initialized layout/snapshot diagnostics. Original
HashMap order and secondary address keys are process-dependent. C++ diagnostics
describe scoped identities and actual snapshot data; FieldLayout does not invent
a type-local Rust weak map. No diagnostic string is normalized or rewritten to
claim text equality between these representations.

Source hint counts are read from the original public FieldLayout Debug: in this
controlled fixture, each `(Weak)` token is one actual hint and fixed names contain
no such token. C++ reads SnapshotPool::hint_count. Complete raw before/after
diagnostics accompany those numeric observations, so the source state is retained
for inspection. This evidence does not claim portable Weak formatting, cache hash
keys, or initialized diagnostic text parity.

## Reproduction and remaining scope

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_initialized_snapshot_cache_probe
python3 scripts/verify_initialized_snapshot_cache_5_0_6.py
ctest --preset native --output-on-failure -R '^initialized_snapshot_cache_rust_parity$'
```

The verifier checks locked offline source compilation, registry versions and
checksums, live original results and retained source-file hashes. The corpus keeps
raw original/native diagnostics independently from the stable common comparison.
No generated artifact or production algorithm changes in this checkpoint.

Supported-model governed reconstruction, general readonly/durable hydration,
provider/cache/facet/aggregate composition, privacy/provenance and final P1/P7
integration remain required. This is core cache evidence, not SQLite execution
or complete G15 qualification. No MIPS or physical-device run is implied.

## Terminal verification

Selected native checks pass 5/5, targeted ASan/UBSan 2/2 with leak scanning
disabled, and the canonical module consumer 1/1. The same 80-case source/native
comparison reproduces with the reference registry versions and checksums.
The module verifier checks 585 canonical bodies and the pinned formatter checks
1405 handwritten C++ files. See the [source record](parity/initialized_snapshot_cache_source_5_0_6.json)
and [terminal record](parity/initialized_snapshot_cache_5_0_6.json).

The first module/sanitizer attempts consumed the earlier corpus before the added
hint-count fields had been recorded from source. All eight saturation expectations
omitted those fields; the compiled observers correctly emitted them. Complete
[initial outputs](parity/initialized_snapshot_cache_initial_validation_5_0_6.json.gz)
and [stale-expectation differences](parity/initialized_snapshot_cache_initial_gaps_5_0_6.json.gz)
are retained. After source preparation and complete live comparison finished,
module and sanitizer follow-ups passed. This was fixture execution ordering,
not a production cache defect; no source algorithm was changed to make it pass.
