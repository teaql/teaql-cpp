# Shared loaded-state kernel checkpoint for Rust 5.0.6

Status: this scoped kernel checkpoint is qualified in native, sanitizer and
named-module gates. This checkpoint covers the kernel geometry and
readonly property schema, not complete G15/G16 or generator parity.

## Representation and ownership

The source reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, specifically
teaql-core/src/load_state.rs, eval.rs, query.rs and dynamic_properties.rs.

FieldLayout remains immutable generated type geometry. LoadedSnapshot contains
64 fixed bits, optional overflow slots, optional named relation/`#` selections
and optional readonly result-schema definitions. Overflow/named sets allocate
only when needed. Derived `_` properties never acquire fixed slots or imply
presence merely because a schema declares a type.

SnapshotPool is an explicit result-Context object. Its per-layout hint admission
is capped at 512. Live hits check their hash bucket without scanning all historical
shapes; misses remove destroyed-object hints before admission. Saturation affects
admission only. Values and mutation ledgers never enter this geometry cache.
A Global FieldLayout may lend immutable metadata into a User pool, but the Global
layout does not retain User snapshots. The pool has no singleton/TLS/default owner.

Publication registers immutable snapshots in the owner arena. ArenaBorrow checks
both destination and source ContextState before accessing arenas, then validates
the registered object ID/type. It rejects unmanaged objects and foreign User
borrows. Ordinary wrapper destruction does not access deleted User storage; Global
diagnostics must remain alive. Generic arenas retain their explicit owner-lifetime
contract. The borrow header also compiles as a standalone memory-layer include. Arena exposes the existing memory-layer ContextState through a virtual
query; runtime supplies its existing Global-owned diagnostic record.

Snapshot updates preserve identity on no-op, otherwise copy geometry and publish
an independent immutable state. Published overflow and named sets occupy separate
Context-managed immutable nodes. Fixed-bit or schema-only changes borrow both
nodes without copying; wide-slot changes detach only overflow and named-selection
changes detach only the named set. Pointer-sharing and managed-allocation assertions
cover these component boundaries. Earlier row views remain unchanged. Explicit
SnapshotPool.copy deep-copies foreign metadata, optional sets and schema into the
destination, while compatible Global metadata may be borrowed. Mutable load-state
copies share only immutable geometry, then detach through the specified pool.
Context-owned published states remain charged until close, as required by the
unified C++ lifetime. Rust weak-hint reclamation does not introduce implicit C++
shared ownership or extend User lifetimes.

## Kernel API scope

- LoadedSnapshot: fully_loaded, projection, checked layout, fixed/overflow/named
  accessors, is_loaded, equality, immutable with_loaded and readonly schema updates.
- SnapshotPool: explicit publication, destination copying and bounded per-layout
  hints. Fully loaded means all fixed fields, not selected relations/durable fields.
- LoadState: Indexed, legacy-to-indexed conversion with exact layout identity,
  mark_loaded/mark_unloaded and scoped copy behavior. Client serialization of
  Indexed is rejected; deserialization cannot install internal masks.
- DynamicPropertyDefinitions: immutable validated `_` declarations, lookup,
  typed-value validation, logical equality/fingerprint and conflicting/subset/union
  merges. Explicit merge_copy owns its destination data; reusable_merge only
  reports an existing owner-bound borrow for synchronous kernel reuse.

The fingerprint is an internal C++ lookup key, not Rust's hash algorithm or a wire
format. Equal schema data produces equal C++ fingerprints. Canonical names and
validation messages retain the source contract.

## Actual source comparisons and tests

The focused Cargo oracle directly calls unchanged Rust 5.0.6 APIs. Its 351 input
sets cover fixed/overflow boundaries 2/31/32/63/64/65/66/129/130/141, named relations
and durable selections, ignored derived/unknown projections, no-op/changed/reverted
geometry, exact invalid-field messages, readonly empty/present/invalid schemas,
all ten DataTypes against native and JSON value families, merge reuse/conflicts,
six legacy/indexed LoadState variants, mutation before conversion, layout mismatch
and runtime-only serialization errors. Source published states are held explicitly
to match the C++ result-Context lifetime. These comparisons do not claim complete
source weak-cache/drop behavior or dynamic storage integration.

Final native scoped integration passes 9/9: new snapshot/FieldLayout contracts and source
corpora, existing entity expression/core collection/Context resource/provider
borrow contracts. Direct Context tests disable the default PMR resource and cover
copy-on-write, lazy auxiliary storage, 512-hint saturation, retained views, explicit
foreign rejection/deep copy, destroyed layout diagnosis and access/destruction of
borrow wrappers after actual User deletion while Global diagnostics remain alive.
The final corpus includes 1,920 source action observations comparing auxiliary
set pointer sharing. Native assertions also require fixed-bit changes to register
only a snapshot and verify destination string allocators for long named selections.
A further explicit-destruction fixture verifies that an earlier snapshot root can
be destroyed while a copied view retains independent auxiliary nodes; dead hints
are removed on misses and cannot resurrect destroyed object identities.
That final assertion refresh passes native 1/1 and sanitizer 1/1 without
changing production behavior.
Targeted ASan/UBSan passes 6/6 with leak scanning disabled. Module import passes
1/1; all 1,312 catalog exports and seven canonical facade archives verify.
These are focused gates, not full-suite or complete generator qualification.

```sh
source scripts/toolchains/activate.sh
cargo build --offline --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml \
    --target-dir /tmp/teaql_cpp_field_layout_rust_506
cmake --build --preset native --target teaql_loaded_snapshot_tests teaql_loaded_snapshot_probe
python3 tests/parity/generate_loaded_snapshot.py \
    --rust /tmp/teaql_cpp_field_layout_rust_506/debug/teaql_cpp_field_layout_oracle \
    --cpp build_native_current/teaql_loaded_snapshot_probe
ctest --preset native -R '^loaded_snapshot_(contracts|rust_parity)$' --output-on-failure
```

## Remaining integration

Generated fixed-index/type metadata installation, entity/CompactRow shared
hydration, original snapshot sharing, complete Checker reconstruction and model
changed/eval helpers remain separate P1 work. LoadedSnapshot.with_dynamic_fields
awaits the complete trusted DynamicFieldDefinitions/Values protocol; it must
replace only persistent selections without disturbing fixed/relation state.
Readonly/durable value carriers, JSON authority, SQLite persistence/streams and
request-owned trace/privacy remain open. No generator feature guard is removed
by this kernel checkpoint, and no 5.0.5 corpus is relabelled as 5.0.6 evidence.

## Fixed result-row integration

ColumnLayout now caches a primary typed immutable snapshot and additional model
layout identities within the row's explicit Context. CompactRow.indexed_load_state
accepts the explicit SnapshotPool and managed FieldLayout; it rejects a foreign
result pool before publication. CompactRow.share_layouts uses a temporary batch
set to share equal ordered projections without retaining row values. Different
projection orders and sparse shapes remain distinct. Shape-changing insertion
creates a new ColumnLayout, so an earlier row and its cached availability remain
unchanged. Updating a value leaves the immutable shape shared and the value local.

Copies into another Context copy column geometry and values, omit source cache
bindings, and rebuild indexed state against destination metadata. A native fixture
checks actual source User deletion after copying. All nested cache allocations use
the row arena; no global cache or default resource is introduced. Cache bindings
check managed snapshot/layout identities, including explicitly destroyed entries.

Sixty additional direct Rust 5.0.6 CompactRow calls compare ordered shape sharing,
primary/additional typed snapshot identities, fixed/overflow projections, missing
fields, view-local unloaded changes and row value isolation. The combined corpus
now contains 411 inputs, retaining the previous 351 snapshot/schema observations.
These cases cover fixed column shapes only. Relation hydration metadata, readonly
schema attachment, durable dynamic fields, generated batch decoders and original
snapshot/checker assembly remain pending. Focused native contracts and the updated
source corpus pass 3/3. Targeted ASan/UBSan passes 2/2 with leak scanning disabled;
named-module import passes 1/1, including alias-based row availability. The 1,312
export catalog and seven canonical facade archives verify. This does not qualify
the entire G15 capability.
