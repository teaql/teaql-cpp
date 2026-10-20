# Native SQLite transaction streaming

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Status: the missing provider responsibility is repaired in production after the
frozen native epoch became terminal. Installed native **8/8** and imported
**4/4** regressions pass. The independent coherent source view also passes its
targeted sanitizer gate. The [bounded source acceptance](sqlite_transaction_source_acceptance_5_0_6.md)
covers the two complete transaction/lease source files. Full capability acceptance
remains open; the historical full native 735/736 result is unchanged.

## Actual source and current C++ behavior

The complete 94-line `teaql-provider-sqlite/src/transaction.rs` implements
SqlTransport, SqlTransaction and StreamingSqlTransport on SqliteTransaction.
Its stream delegates to stream_sql_unleased with the existing transaction
lease, rather than trying to acquire the root connection lease again.

The unchanged original `tests/transaction_lease.rs` has three tests. A fresh
run against the actual frozen crate passes all three, including repeated
transaction probes, streaming three transaction-visible rows, root cursor
lease retention and cancellation followed by connection reuse.
The [original test receipt](parity/sqlite_transaction_streaming_original_tests_5_0_6.json.gz)
preserves the actual command, terminal exit code and all three named outcomes.

Before the repair, C++ SqliteNativeTransaction implemented SqlTransportTransaction
and DynamicFieldStore, but not StreamingSqlTransport. An actual imported
consumer began the real provider transaction and checked its runtime
interfaces. The [gap witness](parity/sqlite_transaction_streaming_gap_5_0_6.json.gz)
returns `transaction_streaming_supported: false`; it then rolls back successfully.
This is a missing interface/behavior, not a database error or a type-name-only
inventory observation.

The existing [144-case lease matrix](sqlite_async_lease_5_0_6.md) and
[366 historical transaction inputs](sqlite_async_schedule_5_0_6.md) retain their
actual scope and earlier passing evidence. Neither invokes the missing native
transaction streaming interface. Buffered data-service transaction streams are
a separate layer and cannot substitute for this provider protocol.

## Reviewable implementation proposal

The [prepared patch](parity/sqlite_transaction_streaming_patch_5_0_6.json.gz)
contains both proposed files, exact original/proposed hashes, its unified diff,
preparation script, pinned formatting results and a successful `git apply --check`.
At that preparation checkpoint it remained outside production inputs until the
frozen epoch became terminal; the installation and current gates are below.

1. Add the existing AsyncStream tracking overload to SqlTransaction's protected
   helpers. PendingOperations already tracks stream closure/cancellation; use
   that mechanism so an open stream prevents consuming commit/rollback.
2. Have the private native SQLite transaction implement the existing optional
   StreamingSqlTransport interface. Do not require all generic SQL transactions
   to implement streaming or merge provider and data-service abstractions.
3. Let the existing SqliteStream cursor accept an optional transaction identity.
   Ordinary provider streams keep their existing null-identity default. A native
   transaction stream passes its actual transaction identity and reuses the
   existing connection, prepared statement, binding, row/chunk and cleanup logic.
4. Validate owning Context and active transaction before creating/tracking the
   stream. Reuse existing actual cursor cleanup and Context quiescence; do not
   introduce a second buffering algorithm, ownership domain or hidden owner.

The complete proposed provider translation unit is checked independently in a
[shadow compilation](parity/sqlite_transaction_streaming_shadow_compile_5_0_6.json.gz).
Its `complete` and exit code describe compilation only. Mixing a shadow object
with old library/module definitions would not establish coherent runtime or
lifetime acceptance, so that is not claimed as the final verification gate.

The shadow compilation is now terminal and successful. The two preexisting
warnings about DecimalFormat initialization and lease-expression parentheses
are retained in its stderr; no unrelated cleanup enters this proposal. The
[provisional join](parity/sqlite_transaction_streaming_provisional_5_0_6.json.gz)
links the actual source tests, failing-parity witness, applicable patch and
successful compile, and verifies all 10,329 frozen inputs and both references
unchanged. At that earlier checkpoint, runtime and lifetime gates were still
outstanding; their subsequent coherent results follow below.

The following [coherent shadow build](parity/sqlite_transaction_streaming_coherent_5_0_6.json.gz)
recompiles all affected library dependencies from a separate source view. Only
the two proposed production files differ; other sources are immutable links to
the frozen checkpoint. No current native/module output is reused as a repaired
library. Its `complete` flag records terminal build success independently.

The [complete test port](parity/sqlite_transaction_streaming_coherent_tests_5_0_6.json.gz)
contains all three original lease bodies and four C++ lifecycle bodies covering
pending/partial/consumed/foreign access, cancellation, actual Context deletion
and error-cursor cleanup. The coherent build has now completed successfully,
and its [runtime receipt](parity/sqlite_transaction_streaming_coherent_runtime_5_0_6.json.gz)
records all seven bodies passing against freshly built libraries. The same
coherent prototype preserves **510/510** actual-source outcomes: 144 lease,
183 asynchronous historical schedules and 183 synchronous historical inputs.
All eleven recorded compile/link/execute commands exit zero. Existing corpus
bytes and original observer bodies are unchanged; their three-bin temporary
[reference project](parity/sqlite_transaction_streaming_reference_5_0_6.json.gz)
checks exact original registry identities rather than using the drifted broad
field-layout lock. The three unchanged native probe sources also have a separate
[successful compilation receipt](parity/sqlite_transaction_streaming_probe_compile_5_0_6.json.gz).

The [targeted sanitizer build](parity/sqlite_transaction_streaming_sanitize_build_5_0_6.json.gz)
and [sanitizer runtime gate](parity/sqlite_transaction_streaming_sanitize_runtime_5_0_6.json.gz)
are now terminal and successful against the same source view. Both selected
executables pass: the full lease/stream lifetime port and the complete existing
dynamic-store lifetime body, whose native transaction interface also shares
the modified inheritance layout. All four compile/link/execute commands exit
zero. Leak scanning is disabled; the pinned host SQLite archive remains
uninstrumented. These are coherent prototype sanitizer results, not a claim that
the entire installed sanitizer preset has been rebuilt or passed.
The [coherent module build](parity/sqlite_transaction_streaming_module_build_5_0_6.json.gz)
and [imported runtime replay](parity/sqlite_transaction_streaming_module_runtime_5_0_6.json.gz)
are now terminal and successful. All seven named-module facades use the same
proposed headers and preserve every native header-library hash. The complete
canonical seven-body consumer and all **510/510** fresh original/imported
transaction outcomes pass, with all eleven recorded commands exiting zero.
No older module PCM/archive supplies repaired behavior.

The [installation payload](parity/sqlite_transaction_streaming_installation_5_0_6.json.gz)
now includes the two production changes, the complete pinned-formatted test
source, its native CTest registration and canonical module/probe registrations.
All five files have exact before/after hashes and the combined patch passes
`git apply --check`. The [installation receipt](parity/sqlite_transaction_streaming_installed_5_0_6.json.gz)
records its subsequent application after the frozen epoch ended. Full pinned
format/check passes across 1,675 handwritten C++ files. The installed native
selection passes [8/8](parity/sqlite_transaction_streaming_installed_native_5_0_6.json.gz).
The first installed module configure correctly rejects three incomplete probe
registrations; its [failure](parity/sqlite_transaction_streaming_installed_modules_5_0_6.json.gz)
is retained. Adding their required package fields and including all four new
contracts in the module preset restores configuration and coherent compilation.
The [module follow-up](parity/sqlite_transaction_streaming_installed_modules_followup_5_0_6.json.gz)
passes all four selected imported regressions. All 881 canonical consumer and
fixture bodies retain their implementation/assertion bodies.
The [coherent native join](parity/sqlite_transaction_streaming_coherent_join_5_0_6.json.gz)
also checks the complete seven-body/510-outcome result, a successful no-work
shadow rebuild and all 10,329 unchanged frozen execution inputs/reference identities.

## Installed regression gates

The installed repair and full semantic port of the three original lease tests
preserve their fixture responsibilities. Four additional C++ bodies cover:

- An unpolled or partially consumed transaction stream preventing terminal
  consumption without marking the transaction consumed.
- Stream exhaustion and early cancellation releasing the pending borrow, after
  which commit/rollback succeeds.
- Repeated probes and transaction streams using the held lease while root I/O
  and a second begin remain pending.
- Actual transaction UserContext destruction rolling back, stopping its cursor
  and releasing waiters before arena reclamation.
- Transaction cancellation, foreign output Context refusal, consumed transaction
  refusal and resource/statement cleanup on error.

The repository now retains a dedicated
[reference project](../tests/parity/rust_sqlite_transaction_reference_5_0_6/Cargo.toml)
and [reproduction helper](../scripts/verify_sqlite_transaction_streaming_5_0_6.py).
It uses repository-relative paths to the unchanged original observers, verifies
all registry versions/checksums against the fixed reference lock, and checks the
original observer/corpus identities before building. The 144 lease and 366
ordered historical transaction inputs remain byte-identical. The helper compares
fresh actual Rust outcomes, retained expectations and each selected C++ frontend
with number-kind-aware equality and strict LF framing. The
[installed replay](parity/sqlite_transaction_streaming_installed_source_replay_5_0_6.json.gz)
passes all **510/510 outcomes per frontend**. These older matrices do
not themselves exercise native transaction streams; the full seven-body port
provides that missing evidence.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_native_transaction_stream_tests \
    teaql_sqlite_async_lease_probe teaql_sqlite_async_schedule_probe teaql_sqlite_transport_probe
cmake --build --preset modules_native --target teaql_module_native_transaction_stream_tests \
    teaql_module_sqlite_async_lease_probe teaql_module_sqlite_async_schedule_probe \
    teaql_module_sqlite_transport_probe
ctest --preset native -R '^provider_sqlite_transaction_stream_contracts$'
ctest --preset modules_native -R '^module_provider_sqlite_transaction_stream_contracts$'
python3 scripts/verify_sqlite_transaction_streaming_5_0_6.py \
    --module-build-dir build_modules_native --record /tmp/sqlite_transaction_replay.json.gz
```

Keep the original three-test result and missing-interface witness separate from
repaired runtime results. Leak scanning remains disabled. No full current suite,
MIPS, physical device, generator or complete G13/G17/P0-P7 acceptance follows
from this provider repair.
