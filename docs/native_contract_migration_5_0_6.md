# Native contract migration and trace scope source replay

Status: targeted native migration, lifetime and module gates pass. Full native
and P7 acceptance remain open. Runtime baseline is
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` (5.0.6).

## Diagnostic and final native evidence

A fresh selection of 20 remaining native contracts fails **20/20** before this
migration. Its separate [terminal record](parity/native_remaining_diagnostic_5_0_6.json)
preserves the failures; the earlier full-suite 292/489 record remains unchanged.

The final selection passes **21/21**: 17 of those maintained native contracts,
two mutation callback-close/reentrancy cases, and two new direct-source comparison
families. Its [terminal record](parity/native_contract_migration_5_0_6.json)
does not describe a full-suite rerun. SQLite ID-space, automatic audited plain
entity entry and relation execution from the 20-case diagnostic remain outside
this completed selection and still require migration.

Subsequent [SQLite entry follow-up](sqlite_entry_followup_5_0_6.md) resolves those
three remaining fixtures, completing all 20 families from that diagnostic in a
final selected **28/28** native run. The 20-failure and earlier 21-pass records
remain historical scoped evidence. Full native/P7 acceptance remains open.

## Restored valid fixture entry points

The current original Rust runtime tests explicitly bind mutation intent and
comment/purpose at protected query entry points. Historical C++ ports lacked
these prerequisites, so many failed before their asserted provider, checker,
policy, callback, cache, transaction or cleanup behavior ran.

Maintained native fixtures now bind explicit intent for private mutation and
readback operations. Graph/ledger entry nodes and runtime roots supply their
own comments. Query helpers own annotated copies of their inputs, retaining
caller query values. Cache invalidation uses a separate mutation-service copy
so binding its readback intent cannot change the caller's count request purpose.
No runtime default intent, invalid-input exception, provider bypass or fixture
default in the shared original Rust reference is added.

The restored native paths cover runtime mutations, recursive SQLite writes,
graph planning, plain save adapters, graph node validation/delete, transactional
ledger save/readback, original service boundaries, validation, events, typed
queries, relation aggregates/cache and pagination. Existing pending/resume,
callback-close, cancellation, metadata expiry, actual Context deletion and
explicit Global-copy assertions remain.

The candidate test now catches the actual `ExpressionNotLoadedError` for E
unwrap. Its model predicate still returns false for the default unloaded foreign
ID, matching the unchanged generated model; it is distinct from E's NotLoaded
result. The missing E state is not weakened to a loaded false value.

The transaction SQL observer check accounts for the actual persisted-snapshot
readback after a successful UPDATE: one initial SELECT, one UPDATE and one
readback SELECT. It verifies two Select entries, one Update entry and a returned
persisted snapshot, preserving the no-duplicate-notification responsibility.
The original Rust executor's `execute_mutation_single` explicitly fetches that
snapshot after one affected row; no logging event is suppressed to meet a count.

## Actual runtime differences repaired

Two source-reviewed runtime defects are corrected:

- `ScopedCommentNode::to_trace_chain` now filters comments using Rust Unicode
  White_Space, as does the persistent token. Nonempty comment text is retained
  verbatim and ancestry stays in original order. The original 5.0.6 `graph.rs`
  applies `.trim().is_empty()` in both traversals.
- Public graph planning now captures a Remove node's own nonblank audit scope
  before bypassing descriptor lookup. Reference nodes retain only the parent
  scope. Audit IDs use `Value::try_u64()` and retain only positive values;
  negative/zero IDs do not become unsigned trace identities. Empty/whitespace
  annotations do not create scope tokens. This follows `GraphNode::audit_trace_node`,
  `trace_scope` and the Remove arm in original `data_service/graph.rs`.

Current LikePattern native assertions also preserve both the expanded pattern
and original input, rather than expecting the historical raw Text operand.
Production expression lowering is unchanged by these assertion migrations.

## New source comparison families

`scoped_comments_5_0_6_cases.json` records **242/242** actual original/C++
observations of both stack and persistent scope APIs. Cases cover all Rust
Unicode White_Space characters, empty/mixed comments, non-whitespace zero-width
marks/BOM, quoted/Unicode text, ordering, all nine trace kinds, optional IDs and
a 128-level chain. Complete retained node kind/entity/ID/comment fields compare
without normalization. Historical trace corpora remain intact.

`graph_plan_scopes_5_0_6_cases.json.gz` records **336/336** actual public
`plan_graph` observations: root and nested Reference/Remove nodes, 12 identity
forms, absent/empty/ASCII/Unicode/padded comments, complete plan/node/batch/scope
output and provider query observations. The historical public planning driver
is separately replayed against the locked 5.0.6 crates. The old driver and corpus
remain unchanged. These scoped inputs contain no timestamp descriptors or
values, so the historical driver's current-time rendering branch is never used;
there is no output normalization for this matrix.

Both families pass fresh live Rust plus C++ replay. This is not complete graph
execution, privacy/governance, clock or historical-corpus qualification.

## Final scoped gates

Targeted ASan/UBSan passes **12/12**, covering the updated runtime mutation and
reentrancy, node commands, recursive writes, save adapter, graph planning,
move-only transaction/ledger, graph lifecycle, candidate and both source replay
families. Leak scanning is disabled. The host SQLite archive is not instrumented;
these checks qualify project code and its lifetime/cleanup boundaries.

Modules consumers reuse the canonical graph lifecycle and graph-planning test
sources. Both import-only contracts pass **2/2**; the planning consumer is added
to the committed modules manifest and build preset. There is no duplicate local
implementation of the graph semantics in these consumers.

Pinned formatting/check passes **984** handwritten C++ files. Runtime/source,
module-path and generator-ledger checks pass. Frozen reference worktrees remain
clean. No generator source/artifact change or new runtime dependency is involved
in this migration, and no full-profile acceptance is inferred from selected
checks.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_scoped_comments_probe teaql_entity_graph_plan_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin scoped_comments_5_0_6 --bin entity_graph_plan_5_0_6
python3 tests/parity/generate_scoped_comments_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/scoped_comments_5_0_6 --cpp build_native_current/teaql_scoped_comments_probe
python3 tests/parity/generate_graph_plan_scopes_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/entity_graph_plan_5_0_6 --cpp build_native_current/teaql_entity_graph_plan_probe
```

No MIPS/physical-device, GPU or external integration acceptance is claimed. Full
sanitizer leak scanning remains disabled. All 17 full generator capabilities and
the overall P0–P7 goal remain open.
