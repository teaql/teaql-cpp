# Loaded graph reconstruction protocol: Rust 5.0.6

The [installed replay](parity/loaded_graph_protocol_installed_5_0_6.json.gz)
matches **3,568/3,568** actual-source/native/imported results per frontend.
It retains all 2,032 historical reconstruction inputs and expected outputs,
then adds 1,536 shared-identity and projection controls. No production function
changes are needed for these outcomes. This is focused evidence for
`data_service/resolved.rs`, whose complete disposition remains pending.

## Original implementation and dependency provenance

The original runtime is fixed at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The verifier prepares a disposable
`ReferenceRuntimeView`, preserving every original source, test and vector byte.
It uses the original runtime's dependency versions and registry checksums;
only the already disabled external-exporter development dependencies are omitted.
The prior independent private-save workspace remains historical evidence.

The complete retained Rust fixture supplies models, input/output conversion,
registry setup and observations. Its only adaptations are absolute fixture-path
spelling, an alias for the current crate and an appended test entry. All existing
fixture functions remain intact. A test-only access bridge delegates directly to
the original `attach_flat_relation_graph`; the original recursive
`flatten_relation_graph`, Context registry and graph builder execute normally.
No reconstruction algorithm is copied into a fixture. Executor methods panic
if either queries or mutations are attempted.

The unchanged original
`ordinary_typed_query_future_has_bounded_heap_frame` test also passes. The
added graph observer is counted separately: **original1 / caller1**. Its heap
frame measurement is Rust-specific evidence, not a C++ frame-size equivalence.

## Shared identity and projection controls

The new controls independently vary forward/reverse keys, one/many cardinality,
parent traversal order, cached versus raw child rows, full versus identity-only
versus duplicate child projections, nested loaded children, facet metadata,
decoder registration and duplicate owner identities. Conflicting raw payloads
check that a present loaded relation is authoritative.

Observations retain complete mutated rows, relation shape markers, remaining
loaded caches, both owner edges, nested relations, the shared Child table entry,
typed lists/options, recursive facet metadata and exact errors. Thus traversal
order cannot silently merge a filtered identity-only edge with a visible sibling
projection. The inspected owner/child identities remain those documented in the
retained fixture; this does not claim every possible model or graph shape.

The old corpus remains byte-identical, and its fresh original-source prefix
matches every retained expectation. New expectations come only from actual
original execution and live in a separate corpus.

## Installed verification and boundaries

Native **3/3** and imported **3/3** selected registered gates pass: the new
protocol corpus, unchanged 2,032-case corpus and complete existing lifetime
program. Canonical imported consumers validate, and final selected builds have
no work. The unchanged C++ probe disables default PMR and uses explicit Context
allocation. Its lifetime program destroys UserContext, rejects a retained closed
borrow and verifies independent nested Global graph/facet copies afterward.

No new dependency, C++ ownership domain or generated API is introduced. This
checkpoint adds no full native-suite, sanitizer, Release, MIPS, physical-router,
SQL/privacy, complete `resolved.rs` or generator phase acceptance claim.

Reproduce the complete source and both frontend gates with a fresh receipt:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_loaded_graph_protocol_5_0_6.py \
  --record /tmp/loaded_graph_protocol_replay.json.gz \
  --cpp build_native_current/teaql_loaded_row_graph_probe \
  --module build_modules_native/modules/teaql_module_loaded_row_graph_probe \
  --check-gates
```
