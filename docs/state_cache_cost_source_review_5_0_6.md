# Retained state-cache cost source review: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Complete source: `teaql-runtime/tests/state_cache_cost.rs` (52 lines).
SHA256: `f6b03439799159abead94c6c5a5eb74ad7813c8bd3158755cfa11854e983b999`.

Status: this complete one-test source is accepted after the installed original,
native and imported-module gates. Earlier isolated receipts retain their
exploratory scope; they are not relabeled as formal module evidence.

## Complete responsibilities

The file contains one test,
`shared_state_cache_hit_cost_with_retained_shapes`, and imports the separately
inventoried allocation-counter helper. It measures retained metadata cache hits;
it does not measure SQL execution, entity hydration, retained heap or query latency.

The C++ counterpart is `teaql_runtime/tests/state_cache_cost_tests.cpp`. It uses
actual `FieldLayout`, `LoadedSnapshot` and Context-owned `SnapshotPool` operations,
without a generated model or a replacement cache implementation.

| Original responsibility | C++ counterpart |
| --- | --- |
| Exact layout | Entity `StateCacheCost`, revision `v1`, slots id=0/version=1/name=2, matching physical/member aliases and no relations. |
| Target shape | Retain the projection of id/name. Every measured result must preserve target identity, load id/name and omit version. |
| Competing retained shapes | Preserve 0/63/511 distinct id/`#retained_N` projections for populations of 1/64/512. The vector size and actual pool hint count are checked before and after measurement. |
| Warm cache | Perform 100 actual snapshot-copy-and-intern operations before each population's measured samples. |
| Complete sample matrix | Seven samples of 10,000 operations for each of the three populations: all 21 `(hints, reads, sample)` keys, without missing or duplicate rows. |
| Measured operation | Copy the target snapshot with its explicit allocator and call `into_shared(pool)` on every iteration. Copying only the borrow handle is not the measured operation. The initial result-handle copy is inside the counted region. |
| Zero allocation | Require zero arena requests/bytes and zero ordinary/aligned heap calls/bytes for every sample. |
| Timing | Record elapsed nanoseconds separately. Neither the original nor C++ imposes a latency threshold; no timing equality or speedup is claimed. |

All competing shapes remain alive during every measurement. A fresh explicit
GlobalContext/UserContext pair scopes each population, so no process-global or
thread-local cache owns Context data. Counter state is test instrumentation only.

## Allocation instrumentation and ownership adaptation

The original helper wraps Rust global alloc, zeroed alloc and realloc. C++ uses
the existing test-only global-new and Arena allocation-entry wrappers. These are
different observation surfaces: both C++ layers are reported separately rather
than substituting arena block growth for allocation requests. The cache operation
uses runtime C++ allocation paths; this test does not claim to instrument arbitrary
third-party `malloc` or external allocations.

Positive controls must observe allocations through both User and Global arena
resources, ordinary new and aligned new before the measured cache cases run.
The same symbol wrapping is installed for native and imported-module executables,
with the size_t ABI selected by the target pointer size. No production allocator
or cache implementation changes are part of this source qualification.

Rust uses retained Arc references; C++ uses Context-bound borrows and explicitly
allocated immutable shapes. All borrows are destroyed before their owning Context.
The identity and live-sharing semantics are preserved within the approved scope;
implicit extension of Context lifetime is not introduced.

The separately inventoried `support/allocation_counter.rs` and full hydration
allocation probe remain pending. Accepting this test does not accept all helper
allocation APIs, thread behavior or hydration budgets.

## Qualification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_state_cache_cost_source_5_0_6.py \
  --record docs/parity/state_cache_cost_installed_5_0_6.json.gz
```

Recording requires a fresh receipt path. The controller verifies the locked
reference and executes the complete unchanged Rust test in the byte-checked
runtime view, with locked dependency identities and one test thread. The
external-exporter development dependencies omitted by that established view
remain outside this test's scope.

It builds native and imported counterparts, runs the new cost test alongside
existing snapshot geometry/lifetime contracts, and checks canonical imported
consumer bodies. A direct execution records each complete 21-sample grid and
requires positive controls. Binary hashes, exact CTest selection, no-work rebuilds
and unchanged input/reference guards are required. Timing columns are retained
but excluded from cross-language allocation comparisons.

The earlier [allocation audit](remaining_allocation_source_audit_5_0_6.md)
retains the isolated original/native exploration and its matched 21-sample grid.
Those receipts are preserved rather than relabeled as installed module evidence.

## Terminal result

The [installed receipt](parity/state_cache_cost_installed_5_0_6.json.gz) passes
**original1/native2/module2**, including existing snapshot geometry/lifetime
regressions. Both frontends preserve the exact 21-sample grid, zero requests/bytes
at both C++ allocation layers, positive controls and unchanged geometry. Canonical
consumers, exact selection, binary hashes, no-work and input/reference guards pass.
The source remains byte-identical to the locked reference.

[Source acceptance](parity/state_cache_cost_acceptance_5_0_6.json.gz) applies only
to this complete 52-line test. Active totals become **94 accepted, 28 pending,
two external exclusions**. Capabilities remain **11 qualified, six partial**;
the allocation helper, full hydration budgets and P0–P7 exits remain open.
No sanitizer, MIPS, physical-device or timing-equivalence claim is added.
