# Dynamic query hydration in Rust 5.0.6

Status: typed root loading, shared decoding and SQLite cursor enhancement have
scoped original-source evidence. G16 and the complete migration remain partial.
Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Query and service contracts

`SelectQuery::select_dynamic_fields()` snapshots a typed selection into the
query's explicit allocator. An unused query has no dynamic-selection allocation.
Allocator-aware query copies include this selection, and query equality compares
its contents. The application facade exposes `selectDynamicFields()`; it delegates
to the kernel method and preserves the existing checked Q binding and one-Context
execution boundary. Generation is unchanged at this checkpoint.

Persistent extensions are never physical SQL projections. Typed root and
enhanced entity reads execute their native query, validate carrier support for a
nonempty result, ask the Context provider for the selected extensions, validate
its complete batch, and merge read sidecars before decoding. Empty typed results
skip both carrier rejection and provider loading. Invalid or missing owner IDs
produce the original entity error; loading checks `try_u64()` without adding the
cursor plan's positive-ID restriction. Providers preserve missing versus NULL.

The rare typed path captures the validated root intent before query preparation.
It uses the existing data-service/executor layers, rather than implementing SQL
inside examples or generated code. The asynchronous row loader snapshots owner,
IDs, selection and intent into User memory; it borrows the enclosing operation's
rows and cancels its nested provider operation before arena reclamation. Ordinary
typed queries retain their existing execution path.

## Shared decoder

`decode_compact_rows_with_read_metadata()` retains the source's public adapter
bridge. It interns native row layouts for the bounded pass, consumes each row's
dynamic sidecar, rejects unsupported entity carriers, decodes with the runtime
root, requires an indexed load snapshot, and installs augmented availability
alongside that entity's private dynamic values. Hydration creates no mutations
and does not put extensions into native original/persisted column snapshots.

A pass-local cache keys augmented states by non-reused base snapshot and selected
code identities. It contains immutable geometry only. The direct source corpus
checks the resulting sharing matrix as well as values and load bits. Different
row payloads remain independent. The graph decoder registry uses this bridge for
single, batch, list and optional relation carriers, so an already hydrated
sidecar survives type erasure. Actual dynamic relation fetching is separate work.

The typed C++ stream adapter uses the same bridge and keeps its optional shape
cache only for one chunk. It allocates that cache only when a dynamic sidecar is
present. Results and snapshots remain Context borrows; copying to another scope
requires the existing explicit destination allocation.

## Cursor enhancement and separation of layers

The data-service `query_stream_with_dynamic_fields()` SPI remains distinct from
SQL `stream_sql_with_dynamic_fields()`. Defaults reject unsupported enhancement
before opening a cursor. The SQL executor checks owner and selection agreement,
compiles through its existing descriptor/plan cache, and maps transport failures
and execution diagnostics through the existing stream adapters. Compile errors
remain deferred error streams, as in the original implementation.

The Context service obtains a trusted provider plan against the registered
executor's store. SQLite validates its binding and positive chunk size, then uses
a rare cursor state whose base native cursor and cleanup remain unchanged. Each
chunk reads extensions using the same connection while its cursor lease is held;
it must not invoke an ambient asynchronous store and wait behind its own lease.
The plan validates IDs and merges values before the chunk is published. Missing
IDs and corrupt extension values map to privacy-safe native Bind errors containing
stable field/code information. Driver payloads are not included.

Raw streaming requires identity in the projection when persistent extensions are
requested. Its positive-ID rule belongs to the cursor plan; it is deliberately
different from ordinary root batch loading. The corpus compares explicit identity
projections and the missing-ID failure path. Cancellation closes the cursor and
wakes queued operations before releasing host memory. No transaction or external
resource is kept alive by shared ownership.

## Source observations and native checks

`dynamic_query_5_0_6_cases.json` records 250 calls against the unchanged original:
actual typed `School` root/enhanced queries, the public central decoder, and real
SQLite cursor enhancement. Its C++ fixture is a handwritten indexed carrier with
the retained School layout/revision and a deliberately narrow native query fixture
using only name/id/version. It is not generated School parity or full model parity.
The frozen generated Rust fixture is used directly; no reference sources are patched.

The cases cover sparse native/dynamic availability, same-identity views, exact
snapshot sharing, NULL/absent/zero values, empty results, selected codes and type
errors, provider absence, fixture stream rejection, database streaming, missing
identity, chunk boundaries and zero chunk rejection. The corpus is committed only
after live original/C++ agreement. It does not normalize errors or sharing results.

Native checks additionally verify that hydration leaves the ledger empty and
native originals separate, unsupported carriers fail on nonempty results, explicit
Global query copies own their selection metadata, and the camelCase runtime
facade executes through the shared service. Real database checks exercise cursor
lease exclusion, early cancellation waking an ordinary SQL query, typed streaming
availability, and selection/plan mismatch rejection. Existing provider close
checks continue to cover nested async quiescence during actual User deletion.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin dynamic_query_5_0_6 --locked --offline
cmake --build --preset native \
  --target teaql_dynamic_query_tests teaql_dynamic_query_probe
python3 tests/parity/generate_dynamic_query.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_query_5_0_6 \
  --cpp build_native_current/teaql_dynamic_query_probe
ctest --test-dir build_native_current -R '^dynamic_query_' --output-on-failure
```

Verified checkpoint: selected native regression 10/10; targeted ASan/UBSan
4/4 with `detect_leaks=0`; named-module import 1/1. The native selection includes
query/model, entity/SQL stream, relation hydration/enhancement, provider and ledger
contracts. Two historical SQLite relation fixtures now supply an explicit root
comment/purpose and propagate it through the existing internal intent binding;
their relation assertions remain intact.

The export catalog verifies 1,404 entries across seven packages, and facade archive
inspection finds no duplicated out-of-line implementation. Active path/source
checks retain 114 production units, 124 source links and pending semantic labels.
Formatting checks cover 788 handwritten C++ files. The original-source corpus has
also been compared live after the final cursor cleanup changes. No MIPS or
physical-device execution was performed for this runtime checkpoint.

## Remaining integration

Relation and child-query fetch stages still need to load dynamic values through
the provider, preserve embedded sidecars throughout nested traversal, and use
checked merges before graph installation. The shared graph decoder now preserves
already loaded metadata; that alone does not qualify those fetch stages. Hook and
trace edge cases, readonly query-schema propagation, complete query Debug/config
surfaces, dynamic JSON decoding and generated indexed carriers remain open.

Graph-save transaction preflight, extension writes, persisted-value readback and
audit emission still need integration. The ledger's temporary pre-I/O guard stays
in place. SQLite dynamic statements still use short-lived prepared statements
rather than the native shared cache. This checkpoint does not establish full G16,
G15/G17, generated model parity, full 5.0.6 acceptance or phases P0-P7 completion.
