# Incremental ledger test source review: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Complete source: `teaql-runtime/tests/test_incremental_ledger.rs`.
SHA256: `d9257e9cd5cc3de5284eeb1abb5d51b4bf29abd191fd9bc8525d0c8711f1b3a1`.
The source is unchanged from the historical baseline; acceptance still requires
current-reference execution and review rather than relabeling the old receipt.

## Complete responsibilities

| Original test/helper | C++ mapping |
| --- | --- |
| `SequentialIdGenerator` | Independent `InternalIdGenerator` fixture returns the old value of atomic fetch-add starting at 1, with sequential consistency. It does not substitute the runtime counter's increment-before-return convention. |
| `test_incremental_ledger_observability` | `incremental_ledger_observability` reproduces IDs 1/2/99/100/101/200/201, every scalar write, deletion, repeated update, version-field tampering, registered original versions and replacement request comment. Assertions cover every printed field value, latest write, separate original-version authority, deleted keys and sorted field-signature batches. |
| `successful_commit_cleanup_removes_all_replayable_root_state` | `successful_commit_cleanup` checks empty changed/new/deleted sets, removed original version and comment, then a fresh single-entity change after cleanup. |
| `deterministic_randomized_ledger_sequences_preserve_identity_and_cleanup_invariants` | `randomized_ledger_sequences` retains all eight exact seeds, 2,048 steps per seed and three independent roots. It reproduces both scalar writes, marking new, deletion with field removal, original-version registration and committed cleanup. Every step compares all roots' fields, new/deleted key sets and registered versions with independent standard-container expectations; numeric IDs remain namespaced by entity. |
| `deterministic_nested_change_sets_preserve_latest_value_and_support_rollback` | `nested_change_sets` retains seed `0x4348414e47455345`, 4,096 steps and depth limit 8. Writes, guarded push/pop and clear operate on the same deterministic sequence; popped scalar changes and latest value across the remaining stack match the independent model. |

C++ unsigned 64-bit arithmetic provides the source's wrapping generators. Values
in the randomized and nested tests retain the exact U64 variant; field/key
ordering is normalized by ordered standard-container expectations. Those
containers are test oracles, not alternative production ledgers. Production
roots and their snapshots allocate in the explicitly scoped UserContext, with
a separate Context for each randomized seed.

The observability test in Rust prints batches using an unordered outer map.
C++ verifies the semantic batch membership and field values instead of treating
print order or translated headings as a runtime contract. This simulated
batching test does not execute SQL, reject a write, or provide authorization:
manual version changes remain observable while original versions stay distinct.
Likewise, invoking `clear_committed` directly tests cleanup mechanics, not a
physical database commit or audit-delivery lifecycle.

## Current qualification gate

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_incremental_ledger_source_5_0_6.py \
  --record docs/parity/incremental_ledger_installed_5_0_6.json.gz
```

The verifier requires a fresh receipt path. It executes all four unchanged Rust
integration tests through a byte-checked complete runtime view and locked
dependency identities. It then builds and runs the existing four-scenario C++
executable in native and imported-module modes, checks the exact CTest names,
canonical module consumers, binary hashes, no-work rebuilds and unchanged
captured inputs/references. Each C++ profile has one CTest registration, not
20,480 tests: 16,384 randomized steps and 4,096 nested steps are internal loops.
Only disabled external-exporter development dependencies are omitted from the
Rust view.

This review accepts only the complete integration-test source after its
terminal gate. EntityRuntimeState's implementation, graph orchestration,
generated mutation APIs and actual transaction behavior have independent
responsibilities. No production or generator implementation is changed here;
no new sanitizer, MIPS, physical-router or complete phase claim is made.

The first controller attempt passed all four Rust tests but failed to recognize
the observability test's result: `--nocapture` interleaved its printed output
with the harness result line. The controller now uses `--show-output`, retaining
both complete printed evidence and separately parseable result lines. The
initial `parity/incremental_ledger_registry_failure_5_0_6.json.gz` is preserved;
it is not an accepted gate or a runtime failure.

## Terminal result

[Installed gate](parity/incremental_ledger_installed_5_0_6.json.gz) passes
**original4, native1/module1**, with complete original iteration ranges,
canonical module consumers, no-work builds and unchanged inputs/references.
[Source acceptance](parity/incremental_ledger_acceptance_5_0_6.json.gz)
records this complete test file only. Active source totals become
**89 accepted, 33 pending, two external exclusions**; capabilities remain
**11 qualified, six partial**. Full phase and goal completion remain open.
