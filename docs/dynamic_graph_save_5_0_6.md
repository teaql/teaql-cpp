# Dynamic ledger persistence in Rust 5.0.6

Recorded: 2026-10-12. Runtime implementation checkpoint; G16/G17 and complete
generator phases remain unqualified. Reference:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Execution and ownership

The ledger outcome now retains generated identities and a separate, rare boxed
map of authoritative dynamic readbacks, following `data_service/graph.rs`.
Existing generated-ID lookup remains available at this private execution boundary.
The generated-ID map alias is unchanged. GraphNode has a separate dynamic
payload; native columns and mutation/snapshot types remain distinct.

An extension-only mutation joins native classification as an id-only change.
Versioned rows require a loaded original version and execute the native optimistic
update. New identities are allocated for the whole graph before recovering traces;
metadata-defined foreign keys are rebound to those assigned identities before
inserts. Conflicting bindings fail before changing native payloads.

Before native DML, dynamic preflight requires a graph-capable Context provider,
the operation's pending audit scope, matching trusted definitions and the acquired
transaction's store. It resolves allocated owner IDs and validates the complete
write batch. The former unconditional unsupported guard is replaced by these
actual protocol checks. Unconfigured providers retain the original stable failure.

After native writes, the same transaction applies extensions and reads them back.
Selection includes changed codes and previously selected root codes. Readback
requires exactly the requested owner, no unselected values and SET/delete presence
consistent with the mutation. Typed values validate against the bound definitions.
Changes enrich their existing queued native entity fact using authoritative values
and first originals. Root readback carries canonical ledger lineage. Commit and
post-commit audit delivery retain the separate
[commit-audit contract](commit_audit_5_0_6.md).

Automatic ledger save and ordinary AuditedSaveExt with dynamic state use this
runtime ledger path. Explicit TransactionScope save keeps the original ledger
pending until its enclosing transaction commits, while the returned hydrated view
can have a fresh clean ledger, as source does. Rollback retains retryable original
intent. A sink failure after commit does not undo native or dynamic writes.

Rare operation state owns writes, intents, selections and readback containers;
owner-ID spans refer to stable write storage across suspension. Returned store
operations must belong to the same Context, or they are cancelled and rejected.
Cancellation joins pending store work before freeing its payload. Definitions,
strings and nested maps propagate their destination allocator.

GraphNode's boxed values are internal owned payloads. Checked `field()` loans
require an explicit managed DynamicFieldValues carrier; materialized entities
already install one. Internal payload inspection uses `values()`, and an explicit
`global.make<DynamicFieldValues>()` copy can provide a checked Global loan. No
raw boxed object is treated as a registered Context object. Explicit Global graph
copies clone definitions and values and survive actual UserContext deletion.

GraphNode equality compares optional dynamic contents, rather than pointer
identity. Raw Debug includes the source's dynamic payload field. Definition Debug
keeps its storage binding opaque. Raw model diagnostics do not establish safe
operator/audit projection.

## Evidence and remaining work

Native tests exercise SET, durable NULL, delete, trigger-derived authoritative
values, failed extension DML, corrupt readback, committed sink failure, ordinary
AuditedSaveExt routing, explicit scope commit/rollback, generated IDs, native
foreign-key rebinding and actual UserContext deletion after an explicit Global
copy. A SQLite callback verifies native DML ran exactly once before extension
failure, so rollback checks cannot pass merely because validation rejected early.

Fourteen unchanged public original/C++ observations compare GraphNode and dynamic
value/definition normal/pretty Debug and clone equality, including independent
changes, empty/NULL views, trusted bindings and Unicode/long strings. One dynamic
field avoids randomized Rust HashMap diagnostic order; no normalization is used.
This is a protocol corpus, not a persistence oracle.

The unchanged original SQLite `dynamic_field_query` integration suite passes
23/23 separately. Its source includes persistence, privacy, rollback, projection
and cursor behavior. Passing that Rust suite is not evidence that C++ covers every
one of its cases. The handwritten C++ carrier is not generated-model acceptance.

Complete Checker/Fix/governance coverage of pending non-root dynamic entities,
readonly/presentation authority, complete hooks/trace/privacy/configuration
matrices and generated adapters remain open. The execution path and its targeted
tests do not qualify all of G16/G17 or phases P0-P7. MIPS and physical-device
execution are outside this checkpoint.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
cmake --build --preset native --target teaql_dynamic_graph_save_tests \
  teaql_dynamic_graph_debug_probe teaql_dynamic_ledger_tests \
  teaql_commit_audit_transaction_tests teaql_audited_save_tests \
  teaql_graph_saver_tests teaql_transaction_scope_tests
ctest --test-dir build_native_current --output-on-failure \
  -R '^(dynamic_graph_.*|dynamic_ledger_contracts|commit_audit_transaction_contracts|audited_save_context_contracts|graph_saver_context_contracts|transaction_scope_context_contracts)$'
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
cargo test --manifest-path .reference/teaql-rs/teaql-provider-sqlite/Cargo.toml \
  --offline --locked --test dynamic_field_query
```

Build the public Rust probe from `tests/parity/rust_field_layout/Cargo.toml`
using the explicit host linker/target directory above and
`--offline --locked --bin dynamic_graph_debug_5_0_6`. Regenerate the paired corpus
with `tests/parity/generate_dynamic_graph_debug.py --rust <rust-probe> --cpp <cpp-probe>`.
Final gates pass: native 7/7, targeted ASan/UBSan 7/7 with leak scanning disabled
(`ASAN_OPTIONS=detect_leaks=0`), module import 1/1, and live graph protocol
observations 14/14. The separate original SQLite integration selection is 23/23.
Formatting checks 798 handwritten C++ files with pinned clang-format 18.1.8;
1,404 catalog exports and seven facade archives verify. Source/path checks retain
seven packages, 115 production units and 124 pending source dispositions.

Build the same seven native targets with `--preset sanitize` and run the same
CTest selection in `build_sanitize_current` using `ASAN_OPTIONS=detect_leaks=0`.
Build `teaql_module_contracts` with `--preset modules_native` and run
`module_import_contracts` in `build_modules_native`. Verify facade ownership with
`scripts/verify_module_symbols.py --build-dir build_modules_native`.
These are targeted gates, not full migration or generated-model qualification.
