# Historical asynchronous transaction scheduling on Rust 5.0.6

Status: all historical transaction inputs have separately versioned current-source
evidence. Complete provider/interleaving and P7 acceptance remain open. Runtime:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` (5.0.6).

## Scheduling adaptation

The old asynchronous driver expected each call to be immediately ready. Rust
5.0.6 returns an independent transaction holding a connection lease: a repeated
begin or an outside-provider operation can wait. Blocking on that waiter before
ending its owner would prevent progress.

The new Rust/C++ development adapters preserve **all 183 historical asynchronous
input action sequences**, SQL, parameters and initial data. They explicitly use
the same scheduler:

1. Submit and poll each input action once. Record its immediate result or pending
   state, including native autocommit.
2. After every action, poll each submitted waiter once in submission order.
   Record resumed results with their original input action index. A completed
   second begin becomes the active transaction for subsequent input actions.
3. At input exhaustion, cancel/drop remaining waiters before destroying the
   active transaction. Observe final stored rows and autocommit.

This is an explicit scheduler for the changed operation protocol, not a claim
that old immediate-readiness expectations still hold. It does not copy the
transaction/lease implementation: both adapters call actual provider operations.
Complete steps, resumes, errors, cancellations, teardown and final state compare
without normalization.

## Ownership and evidence

Rust stores actual futures and non-cloneable transaction values. C++ puts each
transaction/begin operation in its own UserContext; parent writes and observations
belong to another UserContext borrowing the application-owned Global connection.
Consuming or dropping a transaction destroys its actual Context after completion;
outstanding begin contexts are cancelled and reclaimed before active teardown.
Observation copies use the receiving allocator. No shared ownership extends a
C++ Context lifetime.

All **183/183** asynchronous schedules match actual locked Rust and C++. They
include the 69 repeated-begin and 12 outside-operation input profiles previously
identified as needing migration, along with constraint/SQL errors, commit and
rollback sequencing and unfinished active transactions. The observed source
results contain **75 pending submissions**, **75 resumptions** and **72 active
transactions dropped at exhaustion**. These particular historical inputs leave
no pending waiter at exhaustion; cancellation behavior is independently covered
by the [144-case lease matrix](sqlite_async_lease_5_0_6.md), not inferred here.

The composite comparator checks that the ordered inputs in the two new corpora
equal their respective subsets of the preserved historical 366-case corpus.
Both routes then compare current outputs against the exact active commit. Fresh
live replay passes **183 synchronous + 183 asynchronous = 366/366**. The active
`sqlite_provider_transaction_rust_parity` test selects this composite route.
Historical driver, corpus and old comparator remain intact.

## Final gates

Relevant native transaction checks pass **5/5**: composite coverage, separate
sync/async comparisons, the additional lease matrix and SQL transaction
execution. Targeted ASan/UBSan schedule comparison passes **1/1**, all **183**
inputs, including actual transaction Context destruction and resumed operations.
Leak scanning is disabled and the host SQLite archive is not instrumented.
Pinned format/check passes **986** handwritten C++ files. Frozen references stay
clean; runtime/source/module-path and generator ledger verification pass.

Production runtime and public interfaces are unchanged by these driver/evidence
additions. No full suite, new module interface, MIPS/physical-device, GPU or
external integration claim is inferred. Complete provider errors/resources,
arbitrary scheduler interleavings and G13/G17/P7 gates remain open.

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_sqlite_async_schedule_probe teaql_sqlite_transport_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin sqlite_async_schedule_5_0_6 --bin sqlite_transport_5_0_6
python3 tests/parity/replay_sqlite_async_schedule_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_async_schedule_5_0_6 --cpp build_native_current/teaql_sqlite_async_schedule_probe
python3 tests/parity/compare_sqlite_transaction_5_0_6.py --cpp build_native_current/teaql_sqlite_transport_probe --cpp-async build_native_current/teaql_sqlite_async_schedule_probe --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_transport_5_0_6 --rust-async /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_async_schedule_5_0_6
ASAN_OPTIONS=detect_leaks=0 ctest --preset sanitize -R '^sqlite_async_schedule_rust_parity$' --output-on-failure
```
