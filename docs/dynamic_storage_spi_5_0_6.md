# Dynamic extension storage SPI and cursor plans

Current G16 qualification is recorded in the [complete acceptance audit](dynamic_storage_acceptance_5_0_6.md). The text below preserves this earlier checkpoint and its original remaining-work boundaries.

Status: data-service interfaces and pure plans implemented; actual SQLite storage,
runtime providers, cursor enhancement and transaction write execution remain open.
Reference: Rust v5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
`teaql-data-service/src/dynamic_fields.rs` and the executor defaults in `lib.rs`.

## Responsibilities and ownership

`DynamicFieldStore` exposes transaction-bound status, an opaque operation binding
key, optional non-reused source identity, schema initialization, value reads,
whole-batch write validation and write execution. QueryExecutor's optional store
accessor defaults to None/null. A stream executor's optional dynamic enhancement
entry returns DYNAMIC_FIELD_STREAM_UNSUPPORTED before opening a native cursor.
Existing query/mutation/transaction abstractions keep their responsibilities.

`DynamicStorageFuture<T>` adapts the source's owned future to the existing
Context-owned `AsyncOperation<Result<T, DynamicFieldError>>` protocol. Its return
is a borrow and never extends Context lifetime. Concrete adapters must snapshot
short-lived arguments into the operation's allocation domain or keep documented
borrows alive until completion, and must stop/wait before referenced memory is
reclaimed. This is a low-level resource SPI; it receives explicit Memory plus
validated request intent, while runtime configuration belongs to UserContext.
No new async library or runtime dependency is introduced.

`DynamicStorageRows` keeps owner-ID entries separate from native projections.
`DynamicFieldWrite` contains a trusted namespace, immutable definitions, owner ID
and typed Set/Delete mutations. Its C++ `namespace_name` member adapts Rust's
`namespace`, a reserved C++ keyword. The write carrier does not independently
authorize persistence: the actual transaction store validates complete batches
before native graph DML. Application tenancy, permission decisions, arbitrary SQL
and an independent entity-save API are outside this SPI.

All nested strings, mutations, selections and intents propagate allocators.
Cross-domain construction copies definitions/payloads into the destination;
eligible Global metadata may be borrowed by its User Context. Opaque binding keys
and source IDs are comparison metadata, never serialized or logged as data.

## Stream plan contract

`DynamicFieldStreamPlan::create` validates selection names/types, a nonblank
trusted namespace and a present source identity in source order. Namespace bytes
are not normalized. It retains checked request intent, a definitions snapshot
bound to namespace/source identity, and both opaque store comparison coordinates.
The C++ boundary additionally rejects a moved-from invalid QueryIntent before
capturing it. Source valid-intent observations remain unchanged.

`validate_store` compares both binding key and source identity. Reusing a connection
or allocator address does not substitute for executor provenance; real adapters
must satisfy the source's non-reuse/clone identity contract. Missing identity
fails closed. The pure plan holds no store pointer or alternate connection.

`owner_ids` reads the actual native `id` projection and applies the core Value
conversion, rejecting missing, invalid and zero IDs. `merge` checks row/ID lengths,
exact owner sequence, unrequested/omitted owners, selection membership and all
payload types before attaching the batch's new values. Duplicate owners get
independent row payloads and shared immutable selected-name geometry; explicit
empty entries are required for requested owners with no stored values.

Existing per-row durable provenance is checked by the normal row merge path.
As in the source, a later existing-row incompatibility can fail after earlier
private rows were merged: a cursor adapter must not publish any part of a failed
chunk. This pure helper is not a rollback or transaction boundary. C++ inputs
are borrowed and copied into destination scopes instead of consuming Rust maps;
an error returned from a foreign row is copied into the plan's allocation domain.
Native columns, readonly schema and relation metadata preserve their separate
contracts.

## Privacy-safe write diagnostics

`DynamicFieldWrite` Debug contains only owner type and changed field names, ending
with a non-exhaustive marker. Namespace, operation binding, owner ID and mutation
operands are omitted. DebugWriter adds an optional non-exhaustive struct mode;
existing two-argument calls retain their formatting. Both ordinary and pretty
outputs are compared directly against the unchanged source Debug implementation.
Stream plans intentionally have no public serializer or Debug formatter.

## Evidence and reproduction

The dedicated original/C++ observers record 49 pure calls: namespace/selection
validation order, missing/source identities, store mismatch, owner conversions,
batch owner and value failures, duplicate-owner payloads, and privacy-safe ordinary/
pretty write Debug. A fake store supplies comparison identities and throws on I/O;
these observations are not actual storage, executor clone or SQLite behavior.

Native contracts additionally verify zero cursor calls for an unsupported executor,
shared geometry with independent values and explicit Global plan/write copies
surviving actual User deletion. Concrete provider read/write, lease, cancellation
and transaction identity behavior require their own subsequent qualification.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin dynamic_storage_5_0_6 --locked --offline
cmake --build build_native_current \
  --target teaql_dynamic_storage_tests teaql_dynamic_storage_probe
python3 tests/parity/generate_dynamic_storage.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_storage_5_0_6 \
  --cpp build_native_current/teaql_dynamic_storage_probe
ctest --test-dir build_native_current -R '^dynamic_storage_' --output-on-failure
```

G16 remains partial. Its next dependencies are a concrete SQLite store, Context
provider configuration, actual cursor/transaction binding, dynamic audit/readback
and typed generated carrier adapters. The ledger's missing-transaction-storage
rejection remains in place until that execution path is implemented and qualified.


Final gates: native 5/5 (storage contracts/comparison, executor SPI, dynamic ledger
and existing model Debug), targeted ASan/UBSan 4/4 with leak scanning disabled,
module import 1/1, 1,390 catalog exports and seven facade archives. The live source
comparator matches all 49 observations. Formatting checks 774 handwritten files.
The executor fixture's old intent omission is corrected with explicit entry
comment/purpose; no existing provider/transaction assertion is weakened. No
SQLite I/O, generator emission, MIPS or physical-device result is claimed here.
