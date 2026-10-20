# Shared load-state allocation contract: Rust 5.0.6

Status: all four original test responsibilities are qualified. The active reference
is the exact Rust 5.0.6 commit in `upstream.lock.json`. This review covers the
complete `teaql-core/tests/load_state_allocations.rs`, including its fixture,
allocator controls, four test bodies and retained old-branch reconstruction.
It does not accept the entire `query.rs` file or every adjacent integration test.

## Reproduced allocation defect and repair

The [initial witness](parity/load_state_allocation_initial_5_0_6.json.gz) preserves
the original C++ implementation hash and a failing instrumented execution.
A same-shape merge of three NULL columns makes four arena allocation requests
for 736 bytes, although no new system block is needed. Counting only upstream
blocks would incorrectly report zero allocation. The actual frozen Rust test
requires zero allocation for this case.

`CompactRow::try_extend` now has an allocation-free metadata path after existing
readonly schema/value and persistent-provenance validation. If the ordered column
names and schema remain unchanged, incoming relation markers add nothing and no
incoming read sidecar needs merging, it overwrites values using existing capacity.
It retains first-slot lookup for duplicate column names and last incoming value
precedence. It preserves independent source values by copying into the receiving
scope; dynamically owned value payloads can still require their own copies.
The measured source fixtures use NULL values and exclude payload construction.

Changed columns/schema/relations and incoming read sidecars retain the checked
general merge path. Metadata errors still reject before any scalar overwrite.
No generator, generated adapter, SQL provider, ownership domain or dependency is
added by this repair.

## Complete original test mapping

| Original test | Exact fixture geometry and C++ obligation |
| --- | --- |
| `cold_projection_metadata_cost_is_per_shape_not_per_row` | Widths 4/64/130, each with 1/100/10,000 prepared rows: cold requests are nonzero, calls/bytes are constant across row counts, every snapshot has the same identity, overflow exists only above 64 and every fixed slot is loaded. Fixture rows and output capacity are prepared outside measurement. |
| `same_shape_compact_enhancement_does_not_copy_column_names_or_allocate` | Widths 3/64/141, each with 1/100/10,000 prepared row pairs: every merge retains its original column-layout identity and performs zero allocation. |
| `flat_edge_shape_allocations_do_not_grow_with_row_count` | Width 4 with 1/100/10,000 rows: calls/bytes remain constant, at most ten allocation requests construct shared relation geometry, all rows share that geometry and the relation marker does not become another row value. |
| `shared_availability_allocation_probe` | Widths 4/64/130 with 1/100/10,000 repetitions: cached projection, already-loaded fixed selection and unchanged dynamic selection perform zero allocations and retain identity. The old dynamic-name reconstruction remains a separate measurement/control, never a production implementation. |

The full geometry has **57 measurements** per implementation: 9 cold projections,
9 merges, 3 relation shapes and 36 warm/reference measurements. The verifier
requires every case exactly once. The actual original file runs all four tests;
there is no filter, removed assertion or replacement Rust algorithm. Serial test
harness execution keeps diagnostic rows unambiguous; it changes no fixture or
production code.

## What the counters measure

The C++ harness overrides standard allocation operators, including aligned
allocations, and a test-only `CountingArena` observes every PMR request before
the arena bump allocator. Both counters have an allocating positive control.
The default PMR resource is disabled. Counters are single-thread test state and
own no Context data; the production runtime acquires no singleton or counter.

The existing `Arena::do_allocate` virtual override is now protected so the test
subclass can delegate to the unchanged implementation after recording a request.
No production allocation body, data member, object layout or lifetime changes.
This preserves the existing memory-resource extension point without adding
per-allocation instrumentation to deployed applications.

Cold C++ arena requests are 6/664 bytes at width 4, 6/1,624 bytes at width 64 and
74/5,400 bytes at width 130, independent of row count. Flat relation geometry is
6 requests/544 bytes at all three row counts. These are requested payload bytes,
not resident memory or system-block totals. Rows and prepared value buffers are
excluded, as in the original test. The large host fixtures explicitly increase
their test arena limit; runtime defaults remain unchanged.

Rust heap allocation and C++ arena bookkeeping have different raw counts and
sizes. Both are recorded directly; no normalization claims literal equality.
Acceptance preserves the original zero-allocation, constant-cost and bounded-cost
assertions. Elapsed time is diagnostic data and establishes no speedup claim.

## Semantic and lifecycle gates

The retained 401-case CompactRow corpus is replayed against a freshly compiled
actual Rust core with reference-compatible dependencies, then both C++ frontends.
It checks values, duplicates, ordered columns, private clones, layout identity,
saved prior layouts and map/row conversion rather than allocation counts alone.
The new Cargo bin points directly to the unchanged observer; it adds no dependency
and does not modify the reference, observer bodies or historical corpora.

Existing loaded-snapshot, dynamic-carrier and CompactRow contracts retain checks
for metadata rejection before overwrite, private values, explicit destination
copies and actual UserContext deletion. Targeted sanitizer checks cover the
changed merge path and its existing lifecycle contracts, with leak scanning
disabled. Full compiler/generator/integration and physical-device acceptance
remain separate.

## Reproduction

```sh
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_load_state_allocation_tests \
  teaql_compact_row_api_probe
cmake --preset modules_native
cmake --build --preset modules_native --target teaql_module_load_state_allocation_tests \
  teaql_module_compact_row_api_probe
python3 scripts/verify_load_state_allocations_5_0_6.py \
  --receipt /tmp/load_state_allocation_fresh.json.gz
```

The script refuses existing receipt paths. It executes all original tests,
validates both complete measurement geometries and replays the actual original
CompactRow observer before comparing either frontend. No generator installation
or external database is required.

## Current acceptance evidence

The [complete measurements and source comparison](parity/load_state_allocation_measurements_5_0_6.json.gz)
retain all four actual Rust tests, all 57 original measurements, both native
58-row outputs including their positive control, and all 401 source/native/imported
CompactRow outcomes. The original file, observation driver and corpus are unchanged.
The Cargo manifest adds only a bin pointing at that existing observer; the lock
and dependencies are unchanged.

The [terminal gate record](parity/load_state_allocation_gates_final_5_0_6.json.gz)
records successful no-work builds, **native 10/10**, **imported 10/10** and
**ASan/UBSan 4/4** with leak scanning disabled. The pinned formatter and check
pass 1,657 files, 845 canonical imported bodies verify and source/module path
checks pass. The initial gate collector expected a summary phrase absent from
this pinned CTest output; its [partial record](parity/load_state_allocation_gates_5_0_6.json.gz)
is preserved. The final collector checks the actual summary and every Passed
case. No runtime test failed in that collection correction or was rerun to hide
a failure.

The [implementation review](parity/load_state_allocation_implementation_review_5_0_6.json.gz)
verifies that only `row.cpp` changes among the ten shared G15 implementation files.
Removing the added branch and restoring the two relocated declarations recovers
the exact pre-repair source hash, preserving all previous migration edits.
Restoring the allocator hook's original private declaration recovers the exact
previous arena header bytes. All allocation and cleanup implementation bodies
remain unchanged.

The adjacent [native replay](parity/load_state_source_after_allocations_native_5_0_6.json.gz)
and [imported replay](parity/load_state_source_after_allocations_module_5_0_6.json.gz)
each requalify all 727 original layout/snapshot/carrier/cache observations plus
the explicit ownership observation. They use the new implementation review
rather than silently ignoring the changed `row.cpp` hash. After this repair,
pass `--implementation-review
docs/parity/load_state_allocation_implementation_review_5_0_6.json.gz` to the
reproduction commands in the earlier whole `load_state.rs` review.

The [source acceptance receipt](parity/load_state_allocation_acceptance_5_0_6.json.gz)
binds these records and the complete test-file mapping above. The source ledger
now has **16 accepted / 108 pending** files. Native registration increases from
722 to **723** checks; this focused selection does not establish a full 723-test
integration pass. G13/SQ03, SQ05/P7 and the complete generator goal remain open.
No new MIPS, physical-device, Java or full compiler qualification is claimed.
