# Context dynamic-field providers in Rust 5.0.6

The current [reference-dependency replay](parity/dynamic_provider_reference_5_0_6.json.gz)
compiles the unchanged observer through an independent manifest with the exact
original registry versions/checksums. All 72 original/native results reproduce
the retained corpus without differences; earlier evidence below keeps its scope.

Current G16 status: qualified by the separate
[complete acceptance audit](dynamic_storage_acceptance_5_0_6.md). The provider
checkpoint below retains its original narrower evidence and pending boundaries.
Reference: `teaql-runtime/src/dynamic_fields.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Responsibilities

`DynamicFieldsProvider` preserves the distinct Context-level responsibilities of
schema initialization, batch loading, trusted stream planning and transaction
write preparation. Its defaults permit schema initialization as a no-op, reject
stream preparation with `DYNAMIC_FIELD_STREAM_UNSUPPORTED`, and reject graph
preparation with `DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED: save`.

`UserContext::set_dynamic_fields_provider(origin, provider)` registers a checked
resource borrow. The provider must already belong to the explicit origin through
`make<T>()`. Context load/schema operations lazily resolve that resource when
polled, snapshot their inputs into User memory, forward wakeups, and cancel nested
work before arena reclamation. A missing provider permits schema initialization
but rejects loading or stream preparation. `dynamic_graph_provider()` additionally
requires the provider's explicit graph-save support.

`DatabaseDynamicFieldsProvider<E>::create(memory, namespace, definitions)` accepts
trusted configuration, rejects Unicode-whitespace-only namespaces and duplicate
owner types, and obtains storage through the registered executor type `E`.
`definitions_for_context()` does no SQL: it attaches the configured profile and
executor source identity to immutable definitions. Query loading validates owner
and selection before storage access, then attaches authority after storage has
completed, preserving the source's error order. Stream preparation validates both
opaque binding key and source identity against the target cursor store.

Graph preparation checks configured schema equality, transaction-bound storage,
its association with the registered ambient executor, and the expected
metadata's profile/source provenance before constructing a `DynamicFieldWrite`.
It performs no native DML. Store validation and transaction execution remain
separate responsibilities. The low-level store validates values and owner IDs;
preparation does not move those checks ahead of the source-defined boundary.

`InMemoryDynamicFieldsProvider::from_owners()` validates each fixture against its
definitions and copies its values. It supplies an empty map for every missing
requested owner and filters selected codes. It cannot prepare durable writes or
stream storage plans. It adds no alternative save API.

## Batch and audit contracts

`DynamicFieldBatch::into_values()` consumes a scoped batch, validates owner type,
selection, unsolicited/omitted owners and unselected payloads, then invokes the
shared dynamic-value batch validator. Duplicate IDs produce independent payloads
with shared immutable selected-code geometry. Stored NULL remains distinct from
missing extensions. Consuming a failing batch does not expose a partially loaded
entity result.

The private `detail::mutation_audit_value()` preserves the original audit shape:
SET contains `operation` and actual persisted `value` when available, otherwise
its proposed value; DELETE contains only `operation`. It does not emit events.
Connecting this value to graph-save audit/readback remains separate work.

## C++ ownership adaptations

Provider configuration, batches, queued inputs and nested maps/strings propagate
the explicit allocator. Immutable definitions borrow a compatible registered
scope; copying a provider or batch into another lifetime explicitly copies its
metadata and values. There is no `Arc`, shared owner, TLS or ambient allocation
that extends User memory lifetime. Configuration spans contain non-null pointers
to registered immutable definitions; null is C++ API misuse and throws.

The small private database implementation erases only executor resource lookup
through a function pointer. Storage behavior remains in data-service/provider
layers, and public `DatabaseDynamicFieldsProvider<E>` retains the typed executor
configuration. A plain SQLite transport can be registered where the C++ transport
also implements the query SPI; the source comparator uses the full SQL executor
adapter in both languages.

Operations returned by a provider/store must belong to the invoking UserContext.
A foreign operation is cancelled and rejected before installing callbacks. This
is an explicit C++ lifetime check. Operation handles and results remain Context
borrows and must not be accessed after close. Stack values containing PMR
containers must also be destroyed before the supplying Context is deleted; a
checked metadata borrow does not make an independent PMR value safe to destroy
after its allocator has gone away. The close tests end those local scopes first
and leave the Context-owned asynchronous operations pending.

## Evidence and reproduction

The recorded corpus contains 72 direct calls to the unchanged frozen database
and fixture providers, backed by real SQLite storage and a registered SQL
executor. It compares configuration errors, selected/missing owner values,
namespace authority, schema initialization, stream binding, graph preparation
and error precedence. Private batch conversion and audit helpers cannot be called
from an external Rust crate; their source-reviewed contracts have native tests
and are not described as direct Rust observations.

Native tests additionally cover sibling-view independence, explicit Global
copies surviving actual User deletion, Context-mediated committed reads,
transaction exclusion, cancellation, and User destruction rolling back pending
writes before a subsequent Context reuses Global storage. Named-module coverage
constructs a provider through `teaql_runtime` and resolves real SQLite metadata.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin dynamic_provider_5_0_6 --locked --offline
cmake --build --preset native \
  --target teaql_dynamic_provider_tests teaql_dynamic_provider_probe
python3 tests/parity/generate_dynamic_provider.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_provider_5_0_6 \
  --cpp build_native_current/teaql_dynamic_provider_probe
ctest --test-dir build_native_current -R '^dynamic_provider_' --output-on-failure
```

The original Rust dev oracle links with the host C driver explicitly: selecting
the packaged C++ toolchain's `cc` caused an `fcntl64`/host-libc link mismatch.
This affects oracle development linkage, not the locked production C++ presets.
No C++ dependency or original reference source changed.

Verified checkpoint: native provider/ledger/SQLite selection 6/6; targeted
ASan/UBSan selection 4/4 with `detect_leaks=0`; named-module import 1/1.
The export verifier covers 1,402 catalog entries across seven packages, and
facade-symbol inspection finds no duplicate out-of-line implementation. The
path/source verifier records 114 production source units and keeps semantic
dispositions pending. Formatting checks cover 782 handwritten C++ files. These
counts qualify only the selected contracts, not the complete migration.

## Historical remaining integration at the initial checkpoint

This list records the initial provider checkpoint. Later storage, graph Save,
generated carrier and hydration implementations are reconciled in the current
[G16 requirements](dynamic_storage_requirements_5_0_6.md); they are not missing
implementations merely because they appear in this preserved historical list.

The ledger's temporary pre-I/O save guard remains. Actual native/dynamic graph
transaction preflight, writes, persisted-value readback and audit emission still
need integration and source qualification. Query/root/relation hydration and
cursor-local dynamic loading must consume these providers. Generated typed
carriers, dynamic JSON decoding, complete diagnostics/configurations and shared
SQLite dynamic statement caching remain open. This checkpoint does not qualify
full G16, full 5.0.6 parity, or phases P0-P7.


## Provider canonical module checkpoint

The complete provider observer and native lifecycle/audit contract now have
canonical imported consumers. The first build found an existing helper omitted
from the runtime facade: `teaql::detail::mutation_audit_value` was declared in
its public header but unavailable to import-only consumers. The
[initial failure](parity/dynamic_provider_module_missing_export_5_0_6.json.gz)
retains every diagnostic and the pre-repair source hashes.

The repair adds its existing declaration to the module facade, export catalog
and isolated lookup. Provider algorithms, header declarations and the complete
canonical contract body remain byte-identical. The reviewed catalog now contains
1,499 exports. Native2/module2 and the rebuilt module import contract pass; 806
canonical bodies verify. Full pinned formatting/check passes 1,649 files without
timestamp changes. The [terminal provider record](parity/dynamic_provider_reference_terminal_5_0_6.json.gz)
and [exact export-only proof](parity/dynamic_provider_export_only_proof_5_0_6.json.gz)
retain the completed gates and original failure. That checkpoint did not qualify
G16 by itself; the later complete acceptance above does. Its current catalog
contains 1,503 exports and 820 canonical bodies.
