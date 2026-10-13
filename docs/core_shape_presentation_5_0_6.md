# Core result layouts and presentation protocols: Rust 5.0.6

Status: scoped core protocol checkpoint qualified; P0-P7 remains open.
Runtime source authority is the exact commit in `upstream.lock.json`.

## Result layout responsibility

The existing Context-owned `ColumnLayout` already stores shared column and
relation names, readonly property schema, a primary loaded snapshot and typed
decoder caches. `CompactRowLayout` is now its authentic source-name alias; this
preserves the existing C++ API and gives both spellings the same actual behavior.
No new owner or duplicate layout implementation is introduced.

The public `with_dynamic_property_definitions()` now reuses the exact layout
when schema definitions compare equal, or creates a new immutable layout with
the original names and relation availability. An absent schema differs from an
explicit empty schema. Equality compares names, relation names and schema values;
cached loaded snapshots do not participate. Constructors and retention still
copy nested metadata into the destination Context when the source cannot lend to
it. Source `Arc`/`Weak` ownership is adapted to Context borrows and explicit
copies, as in the preceding loaded-state qualification.

## Presentation responsibility

`EntityJsonTraversal` is a Context-allocated path stack. `enter(entity, address)`
checks the pair, rather than a persisted entity ID or an application-wide visited
set, and permits expansion only below depth 128. Every call pushes its pair,
including rejected entries; callers must balance it with `leave()`. Leaving an
empty path is a no-op. Completed sibling paths can expand the same object again.
C++ copies entity-name values into the bound allocator because its string views
cannot enforce Rust's static-string lifetime. This does not mutate entity ledgers
or perform I/O.

The core Entity defaults expose `borrowed_json(destination, traversal)` with an
empty optional result by default. `into_json(destination)` first offers that hook
a fresh traversal; any supplied JSON value is returned without consuming model
mutation values. Otherwise it calls the distinct `into_json_values()` bridge,
whose default delegates to `into_values()`, and retains the existing framework-key
removal and dynamic-extension behavior. Thus adapters can implement presentation
without changing mutation collection. These are kernel snake_case protocols.

The original locked generator does not emit this borrowed presentation hook.
The direct custom-model comparison exercises the frozen runtime Entity defaults;
it is not evidence of complete generated graph JSON or generated relation-cycle
presentation. Those integration responsibilities remain separate.

## Evidence and remaining API audit

The direct corpus has 144 layout, 42 path-traversal and 14 presentation cases.
It exercises repeated/replaced/empty schemas, duplicate columns and relations,
type/address distinctions, zero/large addresses, depth 127/128/129/255, rejected
entry unwind, sibling revisits and null/false/scalar/nested JSON hook returns.
The custom model records which consumption bridge was called. Context tests
disable default PMR and destroy source/destination Contexts after foreign schema
copying and explicit Global retention. Module consumers cover the new root names.
All 200 observations match the original and replay from the committed corpus.
Native checks pass 5/5, targeted ASan/UBSan checks pass 2/2 (`detect_leaks=0`),
and module Context/import checks pass 2/2. The 1,416-entry export catalog and
896-file formatter check pass. No MIPS or physical-device claim is made.

The root API verifier now reports all missing responsibilities instead of stopping
at the first crate. At this checkpoint, core/data-service/SQL root names passed;
runtime lacked
13 exports from `document_round_trip.rs`, including `ContextBoundDocumentService`
and its distinct snapshot, verified/submitted/accepted states and request/error
types. That gap required an independent runtime protocol implementation and
original comparisons, rather than placeholder aliases or an audit exclusion.
Full source-group acceptance remains pending in the active source map.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin core_shape_presentation_5_0_6
cmake --build build_native_current --target teaql_core_shape_presentation_probe teaql_core_shape_presentation_tests teaql_compact_row_api_tests teaql_loaded_snapshot_probe
python3 tests/parity/generate_core_shape_presentation.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/core_shape_presentation_5_0_6 --cpp build_native_current/teaql_core_shape_presentation_probe
ctest --test-dir build_native_current --output-on-failure -R '^(core_shape_presentation_rust_parity|core_shape_presentation_contracts|compact_row_api_contracts|loaded_snapshot_rust_parity)$'
python3 scripts/verify_module_exports.py
python3 scripts/verify_public_exports.py
```

The later [document runtime checkpoint](document_round_trip_5_0_6.md) supplies
the missing runtime declarations and implementation. The last command now passes
its declaration audit; complete document/source semantics remain separate.
