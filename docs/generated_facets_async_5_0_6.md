# Facet suspension, cleanup and error boundaries: Rust 5.0.6

Status: scoped asynchronous and error checkpoint verified; full G08/P5 remains
partial. The separate quoted-Line attachment decision remains pending.

## Source evidence

The fixed runtime and producer are unchanged: runtime v5.0.6
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, producer
`45e1572c4956ca963410d8ea436ead3c8857087a`. The complete generated Rust crate
remains unmodified. The facet oracle now contains 1,184 exact observations,
including 816 actual SQLite executions. The added 96 calls exercise missing
relations, a registry excluding the facet model, a missing outer table and a
missing facet table. Both includeAll modes, nonblank/missing facet comments and
list/one/page/rows/count/exists mode boundaries are checked. Actual query Debug,
SQL/parameters/comment/purpose, result maps, totals and error strings match
without output normalization.

Source state is changed only through its public Context metadata and registry
setters. The missing relation uses a metadata store containing the original
Order/Person descriptors with only the customer relation removed. Missing tables
are produced by real SQLite renames. Registry rejection uses the source's public
EntityRegistry contract. These are real request failures, not fabricated error
messages or handwritten substitutes for frozen generated execution.

The extended source matrix required no production runtime change: facet-local
missing-relation errors retain their source boundary, repository and executor
failures are wrapped once, and raw/count/exists execution continues to skip the
generated facet orchestration layer.

## C++ ownership and suspension evidence

The deferred executor test uses actual EntityQuery, EntityDataService and facet
list coordination with registered Context resources and real generated models.
It substitutes only executor scheduling and returned compact rows; no query or
facet coordination algorithm is duplicated in the test. Unused transaction and
mutation SPI methods reject, preserving the full executor registration contract.

- Outer work genuinely returns Pending. Facets have not started when the outer
  child suspends, and its wake reaches the awaiting list operation.
- Outer completion starts one genuinely pending facet and retains the typed outer
  rows while suspended. The facet's wake reaches the same awaiting parent.
- Changing an independently copied request while work is pending cannot alter the
  executing snapshot or add a facet to its result.
- Both children finish exactly once when completion attaches the compact facet.
- Actual UserContext destruction is tested during outer and facet suspension,
  with and without a preceding explicit cancel. Only the live child is cancelled;
  completed and never-started stages retain their correct boundaries.
- Each executor work item owns a 32 KiB Context host buffer. Cancellation verifies
  its contents before the arena blocks are reclaimed, and removes its wake target.
- A null default PMR resource ensures nested owned storage follows explicit scopes.

This scheduling test is a C++ lifetime adaptation gate. It does not claim a live
Rust-versus-C++ scheduling comparison, thread-worker/GPU shutdown acceptance, or
complete concurrency support.

Artifacts: teaql_runtime/tests/generated_facets_async_tests.cpp; extended
Rust/C++ probes and generated_facets_5_0_6_cases.json.gz; existing source fixture
and schema hashes remain unchanged. Native CTest passes 4/4 (new deferred cleanup, facet Context ownership, expanded
facet source replay and existing generated execution corpus). Targeted ASan/UBSan
passes 2/2 (new deferred cleanup and expanded source replay). Formatting checks
pass for 926 handwritten C++ files; locked source/module-path integrity and diff
checks pass. All 17 capability records remain unqualified. No generator, export,
module or production-runtime code changed during this follow-up, so previous
reproduction/compiler/module gates were not rerun. No MIPS or physical-device run is
claimed; SQLite internals are not sanitizer-instrumented and leak scanning stays
disabled under the current profile contract.

## Remaining work

Qualify additional inherited/guarded/temporal/numeric models, actual aggregation
cache expiry/propagation, full logical trace/privacy matrices, registry or resource
replacement while suspended, worker-thread host-memory shutdown, and broader
nested error/result protocols. GPU adapters remain outside the migration scope. Resolve the quoted-Line source-failure disposition
before complete G08 acceptance. Core abstract layers and explicit Context lifetimes
remain unchanged.
