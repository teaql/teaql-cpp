# Transaction-local commit audit in Rust 5.0.6

This checkpoint implements the pending business-fact queue introduced by frozen
`teaql-runtime/src/commit_audit.rs`. The active reference is
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. It is a runtime prerequisite for
G16/G17, not qualification of those capabilities or the complete generator.

## Contract and ownership adaptation

Each transaction operation owns a separate allocator-aware pending batch in its
UserContext. A temporary borrowed activation exists only while its callback or
execution child is synchronously polled. It restores the previous activation on
return, suspension and exception. Nested operations do not join the outer queue;
a different UserContext does not match the active batch. This replaces Rust's
Tokio task-local scope with explicit owner-thread execution. No thread-local
storage, hidden singleton, shared lifetime extension or Context-owned queue is
introduced. GlobalContext carries only the temporary borrowed activation pointer.

`send_event` captures governance and bootstrap provenance before enqueueing.
Without a matching operation scope, it delivers immediately as before. Successful
commit clears the committed ledger, then delivers the pending facts directly;
direct delivery cannot requeue them into an enclosing activation. Rollback,
failed commit and cancellation discard pending facts. Cleanup joins child and
terminal operations before dropping the queue. Operator SQL logs remain separate
and can describe failed or cancelled work.

Dynamic changes enrich the newest matching native entity/id fact. They append
`#code` fields and changes and update values/new_values where present, preserving
actual authoritative values supplied by the caller. Missing scope or native fact
returns `DYNAMIC_FIELD_AUDIT_SCOPE_REQUIRED`. This helper alone does not persist
or read back dynamic fields; the dynamic ledger guard remains until that entire
transaction flow is implemented and verified.

Delivery attempts every committed fact even after a sink returns an error. The
first failure is owned by `RuntimeError::AuditAfterCommit`; Display states that
the transaction committed and must not be retried blindly. Raw Debug retains
the nested error, and the default Error source remains empty, matching Rust.
Cross-domain copies explicitly deep-copy the nested payload.

Source `commit_audit.rs` describes projection before public sinks. The concrete
source Context implementation calls its raw sink with raw events and projects
its safe sink separately. C++ preserves that existing raw/safe protocol split;
the new queue does not establish additional raw-sink privacy or qualify all
5.0.6 logging/provenance contracts.

## Evidence and remaining work

The unchanged original module's five private unit tests cover future payload
boxing, unscoped enqueue, Context identity, concurrent task isolation and nested
queues. Native queue tests separately cover explicit poll activation, nested
queues, enrichment, retained pending commits, failure/cancellation, continued
fan-out and deep Global retention after actual UserContext deletion. These are
separate evidence, not a claim that C++ directly invokes Rust's private functions.

The public error corpus contains 20 direct original/C++ observations covering
empty, Unicode/escaped, secret and long event errors with zero, one, two and five
nested wrappers. Display, ordinary/pretty Debug and default source behavior match
without normalization.

Real SQLite tests exercise successful commit, body rollback, deferred foreign-key
commit failure, suspended-body cancellation and sink failure after commit. Sinks
verify SQLite autocommit is restored and committed rows are visible. Both facts
are attempted even if the first sink fails. Automatic ledger saves verify that
the original ledger is cleared before delivery and that an audit error cannot
undo the committed version/value. Historical transaction fixtures now use the
transaction capability and assert physical UPDATE/readback log order exactly
once, as required by the frozen SQL mutation source.

Native regression passes 9/9; targeted ASan/UBSan passes 6/6 with leak scanning
disabled (`ASAN_OPTIONS=detect_leaks=0`); module import passes 1/1. The live public
error comparison passes 20/20, and the original private unit selection passes 5/5.
All 1,404 catalog exports and seven facade archives verify. Source/path checks
cover seven packages, 115 production units and 124 source dispositions; these
checks establish structure/provenance only.
Full dynamic graph write/readback integration, generated carriers, the complete
G16/G17 matrix and phases P0-P7 remain open. No MIPS or physical-device run is
claimed for this checkpoint.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_commit_audit_original \
cargo test --manifest-path .reference/teaql-rs/teaql-runtime/Cargo.toml \
  --offline --locked commit_audit::tests:: -- --nocapture
cmake --build --preset native --target teaql_commit_audit_tests \
  teaql_commit_audit_transaction_tests teaql_commit_audit_error_probe \
  teaql_graph_saver_tests teaql_transaction_scope_tests teaql_context_event_tests
ctest --test-dir build_native_current --output-on-failure \
  -R '^(commit_audit.*|graph_saver_context_contracts|transaction_scope_context_contracts|context_event_contracts)$'
python3 tests/parity/generate_commit_audit_error.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/commit_audit_error_5_0_6 \
  --cpp build_native_current/teaql_commit_audit_error_probe
```

Build the Rust public probe from `tests/parity/rust_field_layout/Cargo.toml` with
`--offline --locked --bin commit_audit_error_5_0_6`, the explicit host linker above,
and `CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506` before regeneration.

For lifetime verification, build the six commit-audit/graph-saver/scope/event
targets with `--preset sanitize` and run the six-test selection above in
`build_sanitize_current` with `ASAN_OPTIONS=detect_leaks=0`. For module verification,
build `teaql_module_contracts` with `--preset modules_native` and run
`module_import_contracts` in `build_modules_native`. Verify catalog and archive
ownership with `scripts/verify_module_exports.py` and
`scripts/verify_module_symbols.py --build-dir build_modules_native`.

The broader native selection also builds/runs `teaql_transaction_move_only_tests`,
`teaql_transaction_provider_error_tests` and `teaql_audited_save_tests`.
Provider-error fixtures now supply valid query comment/purpose and mutation reason,
so move-only error checks reach the provider rather than failing at intent validation.

The next checkpoint must prepare and validate all dynamic writes before native
DML, apply them through the acquired transaction, read authoritative selected
values through that same store, enrich the queued native facts and carry readbacks
into saved entities. Only then can the temporary dynamic ledger guard be removed.
