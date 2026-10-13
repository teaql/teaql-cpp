# Durable dynamic field core contracts

Status: core carriers implemented; runtime/SQLite integration and complete G16
qualification remain open. The reference is Rust v5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, specifically
`teaql-core/src/dynamic_fields.rs`. Readonly computed `_properties` remain a
different namespace and keep their existing definitions contract.

## Definitions, selection and values

`DynamicFieldDefinitions` retains owner type, revision, declared names/types and
an optional provider profile/executor binding. Unicode-blank names and names
starting with `#` or `_` are invalid; names are validated without normalization.
Duplicate names and blank owner/revision are rejected with the original error
code and field. Storage identity participates in definition equality, while
`same_schema` compares owner/revision/types only. Binding is trusted provider
metadata, never a tenant permission claim or client-supplied wire field.

`DynamicFieldSelection` distinguishes All from a typed selected-name map. Explicit
selections validate every name and type; All is an unrestricted selection shape.
Runtime names never acquire fixed model field indexes or SQL column authority.

`DynamicFieldValues` owns independent row payloads and borrows immutable managed
definitions and actual selected-name geometry. `from_batch` reuses identical
geometry across rows even when their payloads differ, including loaded NULL.
Requested-but-absent fields remain NotLoaded and do not enter actual geometry.

Validation preserves the source Value variants. Untyped NULL is accepted for any
declared type; typed NULL must match exactly. Text serves Text/LargeText. Json
requires Json; Value Object/List are rejected even for a Json declaration.
Numeric variants are not implicitly coerced. Zero and false remain loaded values.

`DynamicFieldMutation::set` carries an owned Value, including NULL.
`DynamicFieldMutation::erase` is explicit Delete. Assigning NULL never deletes;
deleting a known field removes its row payload and selected name. Failed type or
name validation leaves that row unchanged.

## Context ownership and view composition

Publish definitions and readable rows with `user_context.make<T>()` or
`global.make<T>()`. Factory results are allocator-aware construction values;
calling `field` requires its receiver to be a registered managed object. A field
wrapper checks its Context/object identity before looking up a payload and uses
stable definition bytes rather than retaining the caller's name buffer. Reading
a known missing field allocates no payload or name and performs no I/O.

Same-Context copies share immutable definitions and geometry, while payloads are
copied. Shape changes publish replacement geometry and leave other views intact;
updating an already selected name keeps its geometry identity. The operation-local
`DynamicFieldMergeShapes` cache contains no row values. Its allocation identities
are not reused within the owning Context. Subset unions reuse existing geometry;
repeated non-subset unions reuse their cached Context object.

Merge first requires full definition equality, including storage binding. Only
actual incoming values overwrite the receiver; incoming missing fields never
clear loaded values. The C++ merge takes a borrowed source and copies operands
into the receiver rather than consuming a Rust carrier. Cross-domain construction
deep-copies User definitions, geometry and nested values into the destination.
Global metadata may be borrowed by that application's User Contexts.

`with_storage_binding` produces an explicit construction copy.
`with_storage_binding_shared` publishes managed metadata and preserves a matching
binding's object identity when it can safely be borrowed into the destination.
These APIs do not grant SQL execution authority by themselves.

After actual User deletion, field-wrapper operations fail through the Global-owned
Context diagnostics before accessing reclaimed arena bytes. Explicit Global
copies remain valid. Returned raw payload pointers are ordinary scoped C++ borrows;
retaining such a pointer bypasses the checked wrapper and does not extend lifetime.
The wrapper observes its registered row's current state after an intervening
mutation; C++ does not enforce Rust's exclusive borrow rule at compile time.

## Source observations and verification

The dedicated Rust observer calls the unchanged public definitions, selection,
value-validation, batch, assign/delete and merge APIs. Its dependency is the frozen
reference, and the recorded corpus retains that source's SHA-256. The C++ observer
uses the same input variants and reports errors, declared types, loaded states,
payloads and initial/final geometry-sharing matrices without normalization.
The corpus excludes provider SQL, runtime ledgers and generated APIs.

Reproduce with the existing locked Rust observer crate and native CMake targets:

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --locked --offline --bin dynamic_fields_5_0_6
cmake --build build_native_current \
  --target teaql_dynamic_fields_tests teaql_dynamic_fields_probe
python3 tests/parity/generate_dynamic_fields.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_fields_5_0_6 \
  --cpp build_native_current/teaql_dynamic_fields_probe
ctest --test-dir build_native_current -R '^dynamic_fields_' --output-on-failure
```

Native contracts additionally exercise provenance mismatch, independent values,
unchanged geometry for existing assignment, scoped NULL/Delete mutations and
explicit Global copies surviving actual User deletion. Sanitizer and module
results are recorded in the generator parity plan after their processes finish.

## Loaded state and row read metadata

`LoadedSnapshot::with_dynamic_fields` validates the extension owner against the
fixed entity layout and replaces only `#code` availability. Fixed bits, overflow,
relation markers and readonly property definitions survive. Equal selected-code
sets return the original snapshot; value-only edits require no new availability.
`with_property_schema` supports explicit result-schema replacement/clearing without
changing any availability. Old published snapshots remain immutable.

`ColumnLayout` now retains optional readonly property definitions, and cached
typed load states carry them. Schema attachment batches by actual result layout;
column insert/remove, relation marking, cross-Context copies and layout interning
preserve schema identity or semantic equality as appropriate. Interning never
merges otherwise equal projections with different readonly schemas.

`CompactRow` has one optional read-metadata owner, containing separate relation
lists and durable field values. Ordinary rows allocate no sidecar. Native keys,
values, map/JSON conversion and persistence payloads exclude this metadata.
Set/merge/take methods and `clear_loaded_relations` follow the source distinction
between metadata payloads and relation availability markers. Taking durable values
does not remove lists; clearing all payloads does not change projection geometry.

Checked `try_extend` validates the combined readonly schema, final surviving native
values and both durable definitions before changing native or read payloads.
Incoming actual values overwrite existing values; incoming absence does not clear
already loaded durable fields. Relation markers/lists and readonly schemas merge
alongside native projections. The existing `extend` delegates to the checked path
and throws for an incompatible merge, matching the source's infallible adapter.
Its C++ source parameter is borrowed; the destination receives scoped copies.

Durable row payloads are independently registered Context objects so returned
field wrappers retain their Context/object checks. Sidecars borrow those private
objects; replacing or clearing a sidecar detaches it, while its previous payload
is reclaimed by the owning Context. This is arena ownership, with no reference
count or implicit Context lifetime extension. Cross-domain row copies duplicate
the complete native, relation, durable and schema payload before retention.

`EntityDefaults` exposes the source's unsupported-carrier defaults and separate
readonly property queries. Hydration and mutation reject a missing carrier instead
of fabricating intent. Default entity presentation removes the five framework
mutation keys, preserves readonly properties and includes only actual loaded
durable fields under `#code`. Generated/runtime carriers still need their own
qualified implementations; exposing a default hook is not storage support.

The second dedicated observer records 176 direct original/C++ calls covering
dynamic snapshot replacement across fixed/overflow boundaries, owner rejection,
row metadata, schema/provenance merge rejection, payload clearing/taking and held
view isolation. It preserves hashes of load_state.rs, query.rs, dynamic_fields.rs
and dynamic_properties.rs. Additional native contracts test property-schema
clearing, unsupported entity defaults/presentation and explicit Global row copies
surviving actual User deletion. The latter native C++ ownership adaptations are
not represented as extra Rust observations.

```sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --locked --offline --bin dynamic_carrier_5_0_6
cmake --build build_native_current \
  --target teaql_dynamic_carrier_tests teaql_dynamic_carrier_probe
python3 tests/parity/generate_dynamic_carrier.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_carrier_5_0_6 \
  --cpp build_native_current/teaql_dynamic_carrier_probe
ctest --test-dir build_native_current -R '^dynamic_carrier_' --output-on-failure
```

## Remaining integration

Install durable values/selections into typed runtime entity carriers using the
new loaded-state/row protocol without mixing readonly property authority. Add Context metadata/storage
service protocols, separate dynamic mutation/original ledgers and full governed
privacy collection. Implement SQLite definition/value reads, trusted storage
binding validation, transaction-backed writes, NULL/delete behavior, JSON views,
stream hydration and cancellation cleanup. Thin generated adapters follow runtime
qualification. No generated marker or readonly property currently supplies this
persistence protocol, and this core checkpoint does not qualify G16 or P5.


Final ownership/regression evidence for the row checkpoint: native 10/10,
targeted ASan/UBSan 6/6 with leak scanning disabled, module import 1/1, 1,368
catalog exports and seven distinct facade archives. Both dynamic corpora verify
against live source (176 row/snapshot and 296 value-carrier observations).
Formatting checks all 768 handwritten C++ files. Full G16/P5 remains unqualified.
