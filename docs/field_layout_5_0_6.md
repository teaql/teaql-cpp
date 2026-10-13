# Rust 5.0.6 FieldLayout implementation checkpoint

Status: FieldLayout construction/getter scope qualified in native, sanitizer and
named-module checks. This is one part of G15.
LoadedSnapshot, indexed LoadState, shared hydration, generated field-index metadata
and checker reconstruction are not implemented or accepted by this checkpoint.

## Source and representation

Reference: teaql-core/src/load_state.rs at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. FieldLayout::from_generated validates
immutable entity/revision, fixed indexes, canonical/member/column mappings,
relation names and expected typed members. The C++ class uses Context-owned
strings and allocator-aware vector/map/set storage; indexes never depend on
lookup order. Derived `_` properties and durable `#` fields cannot receive fixed
slots. Field and relation metadata remain separate.

The public getters preserve source meanings. shared_entity_name shares immutable
owner bytes through a scope-bound string_view; it does not introduce Arc/shared_ptr
ownership. FieldLayout is an owning kernel value: its allocator must outlive its
C++ destruction, and returned views are borrows. Ordinary Context values are not
promised to be safely usable after owner close. Use managed Context construction
and a deliberate allocator-aware destination copy for retention.

from_generated returns Result<FieldLayout, String> in the supplied domain. Source
validation failures retain their exact messages. Allocation/Context limit failures
remain actual allocation failures; they are not reported as invalid model metadata.
An explicit delegated UserContext.make_global copy snapshots all metadata and is
accessed through UserContext.get/rebind_global after the original User is deleted.

## Focused semantic and ownership evidence

A separate locked Cargo oracle calls the original 5.0.6 FieldLayout directly;
it does not copy the Rust algorithm. Forty-four constructor/getter input sets
match C++ output exactly, including empty identity/revision, invalid/reserved/out-of-
range/duplicate indexes, missing id/version, mapping conflicts, unknown or alias
canonical mappings, incomplete mappings, invalid typed members, reordered inputs,
Unicode/long identity and field counts 2/31/32/63/64/65/66/129/130/141.

The recorded corpus keeps the 5.0.6 commit and excludes LoadedSnapshot evidence.
Native CTest field_layout_contracts and field_layout_rust_parity pass 2/2.
Targeted ASan/UBSan passes 2/2 with leak scanning disabled. Module import
contracts pass 1/1, the catalog covers 1,303 exports and seven facade archives
contain no duplicated out-of-line implementations. A fresh direct source replay
also verifies all 44 committed observations. Native ownership tests
run with PMR's default resource disabled and verify input-text copies, immutable
identity borrowing, Context-owned errors and complete Global retention after
actual User deletion. Existing historical 5.0.5 corpora are not relabelled.

```sh
source scripts/toolchains/activate.sh
cargo build --offline --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml \
    --target-dir /tmp/teaql_cpp_field_layout_rust_506
cmake --build --preset native --target teaql_field_layout_tests teaql_field_layout_probe
python3 tests/parity/generate_field_layout.py \
    --rust /tmp/teaql_cpp_field_layout_rust_506/debug/teaql_cpp_field_layout_oracle \
    --cpp build_native_current/teaql_field_layout_probe
ctest --preset native -R '^field_layout_(contracts|rust_parity)$' --output-on-failure
```

Module consumers import FieldLayout and core::FieldLayout, construct a real layout
and call the canonical out-of-line implementation. Exports/combined lookup cover
both names. Per-layout shared-state hints now live in the explicit result-Context SnapshotPool,
qualified separately in [loaded_snapshot_5_0_6.md](loaded_snapshot_5_0_6.md).
FieldLayout itself never retains User snapshot pointers in Global metadata.
Complete generated hydration and source LoadedSnapshot integration remain open.
