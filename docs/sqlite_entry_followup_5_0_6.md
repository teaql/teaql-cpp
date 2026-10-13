# SQLite and plain audited entry follow-up

Status: the three remaining native contract fixtures, targeted sanitizer and
combined/module checks pass. Full P7 and provider
capability acceptance remain open. Runtime baseline:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` (5.0.6).

## Fixture corrections

The synchronous SqliteIdSpaceGenerator is bound to the parent provider. Its
transaction rollback fixture now uses that provider's synchronous
`begin_transaction`/`rollback_transaction` entry points, preserving allocation
inside the transaction and reuse of the rolled-back ID. A separate assertion
starts a leased asynchronous transaction and verifies that the parent generator
cannot bypass it; explicit rollback then releases the lease. No provider lease
check is weakened and no implicit transaction binding is introduced.

The relation execution fixture now supplies its own nonblank comment and
purpose before entering ContextDataService. Its parent/plan snapshot,
per-parent offset/limit/tie-breaker, batched versus individually logged query,
Global retention and actual Context destruction assertions remain unchanged.

The plain audited entity fixture implemented `get_comment` but inherited the
default no-op setter. Both Rust and C++ `Audited::into_entity` apply the audit
reason through `set_comment`; the fixture now implements that setter with its
own allocator. Its Parent descriptor is also registered before preflight. The
fixture remains a plain Entity with no ledger API, distinct from the ledger
branch tested alongside it. No default reason or runtime preflight bypass is
added. Shared save-preparation and public namespace native regressions pass.

The ID test's private SQL setup function is renamed `execute_fixture_sql` to
avoid collision with the imported `teaql::sql` namespace. This changes fixture
spelling only; all allocation/storage/concurrency assertions remain.

## Actual provider replay

The historical provider input sets are replayed against the actual locked
5.0.6 source. Newly named corpora preserve all observed outputs without
normalization:

| Family | Original/C++ matching observations |
| --- | --- |
| SQLite transport | 798 |
| SQLite ID-space | 261 |

The transport set covers fetch/execute, repeated and buffered calls, streaming,
capabilities and synchronous units. The ID-space set covers original table,
storage, allocation/floor and transaction call sequences. The old driver and
corpora remain unchanged. The separately copied 5.0.6 Rust driver adapts the
now-distinct SqliteTransaction type for compilation; these two input families do
not execute its asynchronous transaction branch. Async transaction semantics
and the historical transaction/business-ID families require separate source
qualification and are not inferred from these results.

Both new corpora pass fresh live Rust and native C++ replay. Active transport and
ID-space CTest comparators select the new artifacts and continue checking the
active commit. A commit-label change is not used as evidence.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin sqlite_transport_5_0_6
python3 tests/parity/replay_sqlite_provider_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_transport_5_0_6 --cpp build_native_current/teaql_sqlite_transport_probe
python3 tests/parity/compare_sqlite_transport_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_transport_5_0_6 --cpp build_native_current/teaql_sqlite_transport_probe
python3 tests/parity/compare_sqlite_id_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_transport_5_0_6 --cpp build_native_current/teaql_sqlite_transport_probe
```

## Verification limits

The final combined native selection passes **28/28**: all 20 contract families
from the preceding diagnostic, two mutation reentrancy cases, four actual-source
comparison families and two shared fixture regressions. The separately stored
[terminal record](parity/native_contract_migration_followup_5_0_6.json) preserves
this scope. This is not a full native-suite rerun.

Canonical import-only consumers pass **3/3** for the three corrected contract
fixtures. The ID-space consumer is added to the committed modules manifest and
build preset; existing relation and automatic-save consumers retain their source
adapters. Frozen references and all historical provider artifacts remain intact.

Targeted ASan/UBSan passes **3/3** for ID-space, relation execution and automatic
audited save, with leak scanning disabled. The host SQLite archive is not
instrumented, so these checks qualify project lifetime/resource boundaries.
Pinned formatting/check passes **984** handwritten C++ files. Production runtime
and generator implementation are unchanged in this follow-up. No new dependency,
MIPS/physical-device, GPU or external integration claim is made. Complete native,
historical-source and generator capability/P7 gates remain open.

Subsequent [lease checkpoint](sqlite_async_lease_5_0_6.md) adds 144 actual asynchronous protocol comparisons and replays all 183 synchronous historical transaction inputs. The remaining 183 historical asynchronous schedules stay unqualified; the old full transaction family is preserved.
