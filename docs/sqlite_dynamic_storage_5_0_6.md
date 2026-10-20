# SQLite durable extension storage

Current G16 qualification is recorded in the [complete acceptance audit](dynamic_storage_acceptance_5_0_6.md). The text below preserves this earlier checkpoint and its original remaining-work boundaries.

Status: concrete SQLite store and native transaction integration implemented;
runtime provider configuration, generated graph save, dynamic audit/readback and
cursor enhancement qualification remain open. Reference: Rust v5.0.6
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, primarily
`teaql-provider-sqlite/src/dynamic_fields.rs` and `transaction.rs`.

## Storage and codecs

The native connection initializes `teaql_dynamic_field_storage_v1` with the
original namespace/owner-type/decimal-owner-ID/code composite key, definition
revision, declared type, explicit NULL flag and optional payload. Namespace and
field codes are bound parameters. Only trusted provider/runtime plans choose
namespaces; this adds no tenancy/permission or arbitrary table-selection API.

Reads validate namespace/selection, return every requested owner including empty
entries, and bind at most 400 IDs per statement. Selected stored fields must match
the declared revision/type. Driver failures report only
DYNAMIC_FIELD_STORAGE_FAILED: storage; malformed stored data reports the original
field/error code without exposing stored payloads or SQLite diagnostics.

Set(NULL), including matching TypedNull input, retains a stored row and decodes
as untyped NULL. Delete removes the row. All ten declared types preserve their
Value variants. Bool, full-width signed/unsigned integers, Text/LargeText, JSON,
Date and Timestamp use the source JSON representation. Decimal remains exact
decimal text. F64 stores its 64-bit hexadecimal representation as a JSON string,
including negative zero; nonfinite operands reject before the first statement,
and nonfinite/malformed stored bit strings reject on read.

Whole-batch validation checks namespace, positive owner ID, names, finite numbers
and exact declared operand types before applying any write. The lower-level store
validates storage operands; trusted definition/profile provenance is additionally
the responsibility of the pending runtime provider/graph preparation layer.

The codec prepares one upsert/delete pair per apply batch and one read statement
per 400-ID chunk, then finalizes them through RAII before releasing its lease.
This historical checkpoint initially used short-lived native statements. The
[shared cache follow-up](sqlite_dynamic_statement_cache_5_0_6.md) now returns dynamic
statements to the existing connection cache with explicit simultaneous-borrow
tracking. Historical results below retain their original verification scope.

## Binding, exclusive connection access and cleanup

SqliteMutationExecutor exposes its DynamicFieldStore through the SQL transport;
the provider-independent SqlDataServiceExecutor forwards this existing SPI. An
ambient provider can initialize/read but always rejects writes, even if raw SQL
has changed autocommit. Writes are available through the concrete native
SqlTransportTransaction, and the data-service transaction forwards that store.
Clones share provider binding/source identity; separately constructed executors
on the same connection have different coordinates. Source identities derive from
non-reused Context allocation identities and are never driver data or log fields.

The C++ connection lease now retains the operation-owned transaction coordinate
between statements. Ordinary reads/writes/begins queue while a different
transaction owns the connection. Calls carrying the owning transaction's private
coordinate can continue even when an ambient waiter arrived first. Terminal
completion releases/wakes waiters after SQLite returns to autocommit. This
replaces the earlier per-statement-only exclusion that could let unrelated work
join an active transaction; it does not use a global or Context transaction flag.

Dynamic operations snapshot their namespace, definitions, IDs, selection, writes
and request intent into the operation's explicit allocation domain. Transaction
pending tracking includes dynamic operations, preventing consumption before even
unpolled work completes or is canceled. Terminal errors perform native rollback
and wake waiting work, matching consumed Rust transaction Drop behavior. Cancel/
Context quiescence releases queued leases and rolls back the owning transaction
before returning host arena blocks. C++ post-consumption read/use rejection is an
ownership adaptation; Rust cannot retain a usable consumed transaction value.

## Evidence and reproduction

The dedicated oracle calls the unchanged real Rust SQLite store, rather than a
fake executor or copied codec. 123 original/C++ observations compare validation,
apply, own-transaction reads, pending ambient reads, commit/rollback and final
reads without normalization. Cases cover ten variants, NULL/Delete, integer
boundaries, f64 bits/nonfinite errors, Unicode/NUL/long text, typed selection,
owner/namespace failures, 400/401/805-ID batches and deliberately corrupt storage.
The Rust observer uses the existing locked rusqlite 0.40.2 bundled SQLite only as
a development comparator; the C++ runtime gains no additional dependency.

Native contracts additionally verify clone-versus-new executor identities,
pending dynamic work blocking commit, queue wakeup on cancellation, safe driver
errors, and failed deferred-constraint commit rollback/release. Their ownership/
cleanup scenarios are separate from the recorded public source observations.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin sqlite_dynamic_storage_5_0_6 --locked --offline
cmake --build build_native_current \
  --target teaql_sqlite_dynamic_storage_tests teaql_sqlite_dynamic_storage_probe
python3 tests/parity/generate_sqlite_dynamic_storage.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_dynamic_storage_5_0_6 \
  --cpp build_native_current/teaql_sqlite_dynamic_storage_probe
ctest --test-dir build_native_current -R '^sqlite_dynamic_storage_' --output-on-failure
```

The runtime ledger's missing-bound-storage guard remains until actual Context
provider preparation, graph writes, authoritative dynamic readback and audits are
connected. Implementing the SQLite SPI does not qualify generated durable graph
save, cursor enhancement or the full G16/G17/P5 responsibility matrix. Historical
5.0.5 evidence retains its original baseline.


Final gates: original real SQLite/C++ 123/123, native 6/6 (transport, stream,
transaction, dynamic contracts/comparison and generated API), targeted ASan/UBSan
5/5 with leak scanning disabled, module import 1/1, 1,390 catalog exports and seven
facade archives. Formatting checks 778 handwritten files. Selected old request
fixtures now supply explicit intent. Old native assertions permitting a foreign
cursor inside a transaction and retaining a failed consumed transaction are
replaced by the pinned source exclusion/immediate rollback behavior. No existing
historical corpus baseline is rewritten, and no physical-router/MIPS result is
claimed. Full G16/G17/P0-P7 remains unqualified.
