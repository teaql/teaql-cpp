# SQLite operation-owned transaction leases

Status: scoped asynchronous lease and synchronous transaction source comparisons
pass. Complete provider/transaction and P7 acceptance remain open. Runtime:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` (5.0.6).

## Original protocol

The original `teaql-provider-sqlite/src/transaction.rs` returns a distinct,
non-cloneable SqliteTransaction owning an asynchronous connection lease. Its
transport methods use that transaction. Commit/rollback consume it; dropping
an active transaction rolls back before releasing the lease. Parent provider
asynchronous queries/writes and a second begin wait for that lease.

The C++ runtime already implements these responsibilities with an owner-thread
wait queue and Context-owned transaction cleanup. This checkpoint verifies the
actual protocol. The synchronous parent API still rejects leased connections.

## Direct original/C++ comparison

The Rust driver polls real provider futures. The C++ driver polls actual
operations: Global owns the connection; separate UserContexts own the transaction
and waiter. This allows actual transaction Context destruction while the waiter
remains alive. Retained observations use explicit destination copies.

**144/144** observations vary zero/one/three rows, optional transaction writes,
commit/rollback/drop, parent read/write/cancelled write/second begin, and one or
three pending polls. They compare affected counts, transaction-visible rows,
every pending result, cancellation state, release notification, completed waiter
result, final rows and native autocommit. No output normalization is used.

Rust future drop maps to C++ operation cancellation. Rust transaction drop maps
to **actual transaction UserContext destruction**, including rollback before
arena reclamation. The waiter does not implicitly extend the first Context.
Release notification is measured after cancellation, independently of the C++
cancellation notification. Cancellation-notification counts are not asserted
identical across languages.

The historical transaction corpus has 366 inputs: 183 synchronous and 183
asynchronous. A separate versioned corpus replays **all 183 synchronous inputs**
against actual 5.0.6, matching full steps/errors/rows/autocommit. The original
driver/corpus/comparator remain intact. The original asynchronous subset needs
a scheduling migration: 69 profiles begin repeatedly and 12 issue an outside
operation, where the old driver expected immediate readiness. Owned leases now
make these futures pending. The remaining 183 inputs are not relabeled, dropped
or qualified by this new 144-case matrix.

## Gates

Both new corpora match fresh live Rust and native C++. Relevant native checks
pass **4/4**: both comparisons, ID-space allocation/lease rejection and SQL
transaction execution. Targeted ASan/UBSan asynchronous comparison passes **1/1**
with all **144** inputs, including actual Context destruction and pending/cancelled
work. Leak scanning is disabled; host SQLite is not instrumented. No full-suite
or new module-interface acceptance is inferred.

Pinned format/check passes **985** handwritten C++ files. This checkpoint changes
development drivers/evidence, not production behavior or public APIs. No new
dependency, MIPS/physical-device, GPU or external integration is claimed.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_sqlite_async_lease_probe teaql_sqlite_transport_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin sqlite_async_lease_5_0_6 --bin sqlite_transport_5_0_6
python3 tests/parity/generate_sqlite_async_lease_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_async_lease_5_0_6 --cpp build_native_current/teaql_sqlite_async_lease_probe
python3 tests/parity/replay_sqlite_sync_transaction_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_transport_5_0_6 --cpp build_native_current/teaql_sqlite_transport_probe
ctest --preset native -R '^(sqlite_async_lease_rust_parity|sqlite_sync_transaction_rust_parity|provider_sqlite_id_contracts|sql_transaction_execution_contracts)$' --output-on-failure
ASAN_OPTIONS=detect_leaks=0 ctest --preset sanitize -R '^sqlite_async_lease_rust_parity$' --output-on-failure
```

Full historical asynchronous scheduling, all resource/error/interleaving
configurations and complete G13/G17/provider/P7 gates remain open.

Subsequent [historical scheduling replay](sqlite_async_schedule_5_0_6.md) migrates
all 183 remaining asynchronous inputs with an explicit scheduler and qualifies
the complete ordered historical input coverage through the 366-case composite
comparator. The broader resource/error/interleaving and P7 gates remain open.
