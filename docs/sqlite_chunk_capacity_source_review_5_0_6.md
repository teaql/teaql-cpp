# SQLite cursor chunk capacity: Rust 5.0.6

Status: SC01-SC06 and the complete three-test allocation source are qualified.
Native/imported/targeted sanitizer gates pass after reproducing and repairing the
shared-provider differences. Whole-provider and complete migration acceptance
remain separate.

The reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. This review covers
all three tests, their fixture/query/drain helpers in the complete 106-line
`teaql-provider-sqlite/tests/stream_chunk_allocations.rs`, and the native cursor
allocation loop in `teaql-provider-sqlite/src/lib.rs`. The unchanged 47-line
allocation-counter helper has also been read; its counters are test instrumentation.
Exact line counts must follow the retained source rather than this narrative if
the reference is explicitly upgraded.

## Actual source and pre-repair behavior

The [original receipt](parity/sqlite_chunk_capacity_original_5_0_6.json.gz)
records all **three original tests passing**, exact-reference dependencies and
**858** live original observations. The observer calls the actual provider and
transaction streaming interfaces; it does not implement buffering or SQL locally.

The matrix crosses thirteen row counts (including empty, tiny, exact boundary,
tails and 4,096), eleven chunk sizes (including zero and non-powers of two),
three consumption limits (all, none and one), and root/transaction cursors.
Each observation retains public buffer capacity, every ordered ID, chunk index,
final flag, EOF state, complete provider reuse and autocommit after rollback.
The corpus and source comparisons do not normalize outcomes.

The [compiled pre-repair receipt](parity/sqlite_chunk_capacity_differences_5_0_6.json.gz)
reproduces **250/858** differences. For 223 rows at size 73, original capacities
are `[73, 73, 73, 4]`; C++ previously returns `[128, 128, 128, 4]`.
The direct original-bound C++ port also fails on geometric over-allocation.
Compilation and linking exited successfully before either executable was run.
The original inputs, expected outputs and failed evidence remain preserved.

## Complete test responsibilities

| ID | Original responsibility | Native mapping and evidence |
| --- | --- | --- |
| SC01 | Real fixture and measured drains | Preserve the same table, recursive seed, query/comment and seven `(rows, size)` configurations. Both warm and measured passes check every I64 ID and final count through the real provider. |
| SC02 | Positive chunk capacity | Start with `min(chunk_size, 4)` capacity, grow to the known positive bound, and retain exact full/tail capacities 73/4 for the complete 223-row fixture. Preserve all four indexes and the source row bound. |
| SC03 | Early drop and cursor/lease release | Read exactly 73 rows, close the cursor, then drain all 223 ordered rows through the same provider. C++ explicit stream cancellation maps to source drop. |
| SC04 | Zero-sized native stream | Preserve one three-row final tail followed by EOF. The original zero-size compatibility path has a four-element initial allocation for this row type; subsequent growth is unbounded by chunk size. It does not adopt the separate dynamic-field zero-size rejection. |
| SC05 | Allocation measurement | The Rust helper counts process allocator calls/bytes on its current test thread and measures elapsed time. The test discards elapsed time and only reports counts/bytes. C++ reports explicitly named arena-upstream block calls/bytes, measured after fixture setup and one warm drain. These scopes differ and their numerical totals are not equated. |
| SC06 | Explicit C++ cleanup | A supplementary test actually destroys the UserContext while its 73-row cursor holds the lease. Another Context's pending transaction resumes; rollback and all 223 rows remain usable. Every fixture backing block returns to its explicit upstream after its Contexts end. This does not claim Rust Context deletion while a live safe borrow exists. |

The canonical C++ probe provides the three complete original contract bodies
plus the additional Context-deletion body through `--contracts`. Its normal
mode supplies the exact 858-case observations, explicitly copying retained JSON
into Global before deleting its UserContext. The default PMR resource is disabled.

## Shared implementation and lifetime boundaries

The implementation changes only the native `SqliteStream` loop. It reserves
the short prefix after SQLite supplies a row and before decoding it, then grows
to the positive chunk bound when that prefix fills. Empty cursors allocate no
row buffer. The zero-size path explicitly reserves the source's four-row prefix
and preserves subsequent vector growth and single-tail behavior.

Root, transaction and dynamic streams share this loop. Buffered fetch_stream
retains its separate terminal-chunk rules. The preceding lease/binding repair,
poison precedence, query/stream protocols and explicit Context domains are
unchanged. There is no new runtime dependency, allocator domain or shared owner.

This bounds an individual positive-sized chunk buffer, not the total retained
stream result. C++ rows/chunks remain Context borrows and their arena storage is
reclaimed at Context close. Cursor cancellation releases native SQLite work;
it does not independently reclaim every arena allocation. The C++ measurement
tag is `CPP_STREAM_ARENA_BLOCKS`, so it cannot be mistaken for Rust's
`STREAM_CHUNKS` process-allocation numbers. No total-memory or speedup claim is made.

Capacity observations qualify the locked native libstdc++ implementation. No new
MIPS, physical-router, external-provider or GPU result is inferred. The entire
provider source, remaining capability/source obligations, current full native
integration and every P0-P7 exit retain their own requirements.

## Reproduction

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_sqlite_chunk_capacity_5_0_6.py \
    --profiles native \
    --record /tmp/sqlite_chunk_capacity_native_fresh.json.gz
```

The verifier builds locked original source, checks all three original tests and
all 858 recorded outcomes before using compiled C++ consumers. For the selected
imported/lifetime gates, use `--profiles modules_native sanitize`. It also retains
all 87 binding/poison controls, native transaction cleanup and real dynamic-stream
integration. Each receipt path must be fresh; historical evidence is never overwritten.

## Terminal qualification

The [repaired execution](parity/sqlite_chunk_capacity_repaired_5_0_6.json.gz)
passes **native 7/7, imported 6/6 and selected ASan/UBSan 6/6**, with leak scanning
disabled. Each frontend matches all **858** public capacity/result observations
and retains all **87** binding/poison controls. The complete source contract port,
actual UserContext deletion, waiting-transaction wakeup, native transaction and
real dynamic-query integration all pass. Selected final builds report no work.
The original three tests remain unchanged and pass in the same source gate.

The [preservation proof](parity/sqlite_chunk_capacity_preservation_5_0_6.json.gz)
checks 10,643 pre-change execution inputs. Exactly one production loop adds nine
lines; the other existing changes are four build registrations. Seven new files
install the observers/project/corpus/verifier. Independent producer, existing
SDKs, interfaces, previous observers/corpora and both reference identities remain
unchanged. No generated artifact is edited or regenerated.

The [static record](parity/sqlite_chunk_capacity_static_5_0_6.json.gz) verifies
paths, **1,516** exports, **1,011** canonical bodies, generator ledger, Python
syntax and whitespace. Pinned format/apply and check pass **1,708** handwritten
C++ files. Native discovery is **782**, imported discovery **573**; those are
inventories, not full suites.

Only `tests/stream_chunk_allocations.rs` advances, to **66 accepted / 56 pending /
2 external**. The [scoped qualification](parity/sqlite_chunk_capacity_qualification_5_0_6.json.gz)
records complete test-source acceptance and explicitly leaves whole-provider and
whole-goal acceptance false. All seventeen capability verdicts remain unchanged,
including six partial capabilities. Current full integration, the remaining
provider/root and caller source contracts, standing source-behavior decisions
and every P0-P7 exit remain required. The interrupted 778-test build is not
relabeled; no new whole native epoch is running.
