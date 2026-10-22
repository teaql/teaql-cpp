# Nested audited Save Fix scopes (Rust 5.0.6)

Status: the focused correction is qualified. Fresh original-source/native
comparison matches all four public-call controls; final native 6/6, imported
module 2/2 and ASan/UBSan 2/2 pass with the final fixture. This is not complete
entity-save source acceptance or a new generator parity verdict.

## Original public-call contract

The reference is `teaql-runtime/src/entity_save.rs` at Rust 5.0.6 commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Each public typed ledger save,
executor-bound ledger save and generic dynamic-field save creates its own
`GRAPH_FIX_TIME` and `GRAPH_FIX_EVIDENCE` scope. A plain generic save creates no
scope and inherits the caller's active scope.

The [before-correction receipt](parity/nested_public_save_scope_before_5_0_6.json.gz)
compiles an owned view preserving every original runtime and test body. An
appended observer calls the real public entry points. During outer extraction,
it records `outer_before`, polls a nested save, inspects published evidence,
then records `outer_after`. Inner extraction records `inner`; a deliberately
invalid relation value makes both operations finish before provider I/O. The
fixture executor panics or throws if queried or mutated. Evidence-collector
identity avoids depending on whether two clock reads share a millisecond.

| Inner public call | Inherits outer collector | Last evidence immediately after inner | Final outer evidence |
| --- | --- | --- | --- |
| Typed ledger save | No | `inner` | `outer_before`, `outer_after` |
| Generic dynamic-field save | No | `inner` | `outer_before`, `outer_after` |
| Plain generic save | Yes | Empty (no inner publication) | `outer_before`, `inner`, `outer_after` |
| Executor-bound ledger save | No | `inner` | `outer_before`, `outer_after` |

The original source establishes all four outcomes. Before correction, C++
reuses the parent's collector in all four paths. The three scoped inner calls
therefore publish both `outer_before` and `inner`, and contaminate final outer
evidence with `inner`. The plain generic path already matches.

## Runtime correction and permanent witnesses

`GraphFixedOperation` now creates a new Context-owned `GraphFixState` on its
first poll, even when another save on that UserContext is active. Activation
still restores the outer state after polling. A plain generic save receives
no extra wrapper. The generic dynamic continuation has one wrapper at its
actual scope boundary; it does not introduce another scoped save internally.
Clock sampling remains deferred until polling. Completion publication,
exception and cancellation behavior use the existing shared implementation.

`tests/support/nested_save_scope_fixture.hpp` exercises the four real nested
entry paths from `graph_fix_tests.cpp`, including Context close, zero live arena
bytes and actual Context destruction at iteration exit. The existing null
default memory resource guard remains active. State, models, evidence and the
executor stay in the declared UserContext lifetime; no dependency or ownership
domain is added.

`scripts/verify_nested_save_scope_5_0_6.py` freshly replays the byte-preserved
original source and compares the native output. The
[final installed comparison](parity/nested_public_save_scope_installed_final_5_0_6.json.gz)
matches all four controls. Its disposable original build is removed after
successful terminal execution (167,842,739 bytes).
`scripts/verify_nested_save_scope_closure_5_0_6.py` records focused profile builds,
the exact selected CTest registry, terminal JUnit results, stable input hashes,
no-work rebuilds, reference/path checks and owned temporary cleanup. Sanitizer
leak scanning stays disabled under the project rule.

## Qualification boundary

The first native six-test and module two-test checks passed, but the
[first sanitizer run](parity/nested_public_save_scope_sanitize_5_0_6.json.gz)
found a use-after-free in the new fixture: its stack `Vector<String>` retained
arena elements until after explicit Context close. The fixture now allocates
that observation vector through `user_context.make`. The
[next sanitizer run](parity/nested_public_save_scope_sanitize_corrected_fixture_5_0_6.json.gz)
and the first counterfactual probe exposed the remaining stack `Model`: the
allocator-aware audited constructor can copy its values, so moving it does not
guarantee an empty source. The final fixture also allocates that model through
`user_context.make`. Registered destruction therefore precedes block reclamation
for both retained objects. The failed sanitizer/counterfactual receipts and the
earlier native and module receipts remain historical; they do not qualify the
final fixture. The [final counterfactual compiler](parity/nested_public_save_scope_previous_compiler_final_5_0_6.json.gz)
uses the exact retained previous scope header without reverting production
code. With the final fixture, [fresh original comparison](parity/nested_public_save_scope_previous_final_5_0_6.json.gz)
reproduces all three differences under ASan/UBSan with no sanitizer diagnostic.
The compiler and comparison receipts identify the actual overlay, complete
probe source, library/binary identities and header bytes. The owned compiler
directory (111,889,849 bytes) and original build (167,842,739 bytes) are removed.

Final consumers use the same final fixture and installed scope implementation:

- [Native 6/6](parity/nested_public_save_scope_native_final_5_0_6.json.gz): Fix
  contracts/comparison, automatic Save contracts/comparison and executor-bound
  Save contracts/comparison.
- [Imported modules 2/2](parity/nested_public_save_scope_modules_final_5_0_6.json.gz):
  canonical Fix contracts/comparison; 1,156 consumer/fixture bodies verified.
- [ASan/UBSan 2/2](parity/nested_public_save_scope_sanitize_final_5_0_6.json.gz):
  final Fix lifecycle/nesting and automatic Save contracts, leak scanning off.

Each final profile verifies its exact test registry, successful terminal JUnit
results without skips, stable recorded inputs and no-work rebuilds. Pinned
format/check covers 1,805 C++ files. The complete execution snapshot contains
11,409 inputs and exactly six intended changes from the preceding audit-parser
checkpoint, before the subsequent evidence-only ledger update. Source and
capability statuses remain unchanged: 116 accepted / 6 pending /
2 excluded_external and 11 qualified / 6 partial. P0-P7 remain incomplete.

The [final continuation receipt](parity/nested_public_save_scope_final_continuation_5_0_6.json.gz)
rechecks all 11,409 inputs and both unchanged references. It permits only the
evidence-only G03/G13 ledger append after the execution snapshot. All 15 recorded
owned temporary directories are absent; measured cleanup totals 783,274,820
bytes. Compact receipts and the three stable preset directories are retained.
Failed-run cleanup without a measured size is not added to that total.

These witnesses test nesting and evidence publication at controlled extraction
errors. They do not establish successful nested transactions, independently
committing nested saves, provider fault coverage or complete `entity_save.rs`
acceptance. The previously completed 877-test native epoch predates this change
and is not relabeled. MIPS, physical-router execution and generator output are
outside this focused correction. The approved JSON payload compatibility
exception remains separate.
