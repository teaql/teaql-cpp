# Core descriptor metadata source acceptance

Status: MD01-MD12 and the complete `meta.rs` source disposition are accepted
by the [terminal receipt](parity/core_metadata_source_acceptance_5_0_6.json.gz).
Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

This review covers all 296 lines of `teaql-core/src/meta.rs`, including its three
tests. The three descriptor types remain distinct. Their use by generated
queries, schema providers, SQL binding policy and graph Save has separate
consumer responsibilities. The [container-attribute review](container_metadata_5_0_6.md)
covers the generated adapters over this metadata.

## Complete source responsibilities

| ID | Source responsibility | Native mapping and evidence |
| --- | --- | --- |
| MD01 | Complete PropertyDescriptor fields and defaults | Preserve name, DataType, nullable, column name, independent ID/version flags and three optional u32 constraints. New properties copy their name into the column, start nullable, have no roles and have absent constraints. All ten source DataTypes are observed. |
| MD02 | Every property builder | Column replacement, not-null, ID, version, length, precision and scale update their own field without validation or normalization. Zero and maximum u32 values remain present values. Native `column` and `with_*` names avoid collisions with public fields; logical results retain the source values. |
| MD03 | Complete RelationDescriptor model and builders | Keep name/target, two `id` defaults, false many, true attach and true delete-missing. Local/foreign replacement, to-many, attached/detached and remove/keep-missing preserve source order and repeated toggle behavior. No relation target or key is inferred from another field. |
| MD04 | EntityDescriptor construction and independent identities | Preserve name, default-table delegation, absent service, empty ordered collections, false audit declaration and absent audit maximum. Changing a name does not recompute an already stored table. Naming delegates to the separately qualified source naming contract. |
| MD05 | Table/service replacement and collection builders | Store exact strings, including empty, NUL and Unicode. Service None and Some(empty) are distinct. Properties and relations append in order, retaining duplicates. Their constraints do not become schema validation in this layer. |
| MD06 | Declared audit masks and optional maximum | Replacing masks sets declaration true even for an empty vector; order, duplicates and exact bytes remain. Maximum None, Some(0) and the machine-sized maximum remain distinct. C++ range replacement snapshots aliased input before publication; the existing ownership test exercises self-replacement. |
| MD07 | Every lookup and writable iterator | Property/relation and ID/version lookups return the first match or absence. Returned values borrow the stored descriptor. Writable iteration preserves order and excludes every ID field, including one also marked version; a separate version field remains writable. Empty iteration and missing roles are covered. |
| MD08 | Clone, PartialEq and Eq | Compare every source field, ordered nested collection, optional presence and declaration flag. Independent clones retain explicit-empty mappings. The 32 single-field/order/equality controls and all builder outputs retain complete before/after values. |
| MD09 | Complete derived Debug | Preserve field order and all fields in compact/pretty, lower/upper hexadecimal, pretty hexadecimal, width/precision, custom fill/sign and zero-padding modes. Raw descriptor diagnostics remain separate from safe SQL/audit publication. |
| MD10 | Explicit allocation and retention | The unchanged default-PMR-disabled test copies descriptors and raw diagnostic strings into Global, actually destroys UserContext and checks every nested allocator. It verifies mutable snapshot independence, 1,024-byte payloads, original-object borrows and self-aliased mask replacement. The complete C++ test body executes through headers and imports. |
| MD11 | Original tests and complete APIs | All 28 source constructor/builder/query methods and all three models' Debug/Clone/PartialEq/Eq protocols are reviewed. All three unchanged original tests execute in the actual frozen core crate; their exact builder/lookup assertions already exist in the unchanged C++ contract body. No Default, Copy, Hash, Ord or Serde protocol is invented for these source structs. |
| MD12 | Current provenance and frontend qualification | Replay all 344 retained inputs against actual frozen source with exact registry identities, then compare both C++ frontends. Eighty additional inputs exercise all types/flag modes with empty/NUL/Unicode/long strings and maximum constraints. Both frontends pass the installed five-test selection and preserve canonical source bodies. |

## Source, type and ownership adaptations

The Rust consuming builders map to native mutable fluent references. Explicit
allocator-aware destination copies replace implicit Rust Clone ownership; no
shared owner extends a Context lifetime. Raw pointers and writable ranges borrow
their descriptor and remain subject to descriptor destruction/moves, container invalidation and Context
close.
The existing owner-thread and explicit lifetime contract remains applicable.

Native metadata includes the separately documented `json_column_name` adapter.
It has no source field in this file. Its native copy/equality behavior does not
add a Rust field or alter the source Debug layout. The source comparison uses
the source-representable descriptor state; quoted-FK generation/decoding remains
under its unresolved G08/G15 disposition. Required lookups and the extra native
constructor conveniences likewise remain native extensions. Empty source column
replacement maps to `column("")`; the native constructor's default-column
convenience does not replace that setter.

The new boundary corpus includes the native64 `usize` maximum. Its installed
comparison is registered only on 64-bit profiles. The unchanged 344-input corpus
retains its existing 32-bit-compatible limits and header consumer registration.
This review makes no new MIPS/QEMU or physical-device execution claim.

## Actual source observations

The [source verifier](../scripts/verify_core_metadata_source_5_0_6.py) invokes the
three `meta::tests` tests in the complete unchanged original core crate. The
reference observer compiles the unchanged existing Rust driver against that same
frozen core. Registry versions/checksums, complete source text, actual commands,
executable hashes and every output are retained in the
[source replay](parity/core_metadata_source_initial_5_0_6.json.gz).

All **344/344** prior inputs reproduce unchanged on original/native/imported
frontends, including the 160 builder combinations, 32 field/order equality
controls and 152 naming controls. All **80/80** new boundary inputs also agree:
**424 exact outcomes per frontend**, without value, identity, format or error
normalization. The historical corpus and original driver are preserved.

The new corpus is created only after the actual source outputs and both native
frontends agree. Subsequent runs require complete reproduction of its bytes and
values. Its installed comparator uses the existing strict shared comparison
contract; no failed cases are skipped or replaced with native expectations.

## Installed gates and final disposition

The complete existing metadata C++ contract and probe are now permanent
canonical module consumers. The module preset includes both targets. Native
and imported selections each pass **5/5**: metadata ownership/original-builder
contracts, the retained 344-case comparison, the new 80-case comparison, and the
existing core API contract/comparison. Configuration, compiler/linker completion,
JUnit registration and stable final no-work checks are retained in the terminal
receipt, together with source preservation and status reconciliation. All **927**
canonical consumer/fixture bodies verify. The existing C++ implementation/test
bodies, both historical/current metadata corpora, all **3,457** active SDK files,
**2,201** protected artifacts and six native archives retain their hashes.

Only this source disposition advances: **40 accepted / 82 pending / 2 external
exclusions**. All seventeen capability statuses remain **11 qualified / 6 partial**.
The native64 boundary comparator brings discovery to **742** tests; the focused
five-test result is not a complete 742-test run. Both failed historical full
epochs retain their original results.

The [initial acceptance-writer failure](parity/core_metadata_source_acceptance_initial_failure_5_0_6.json)
records a JUnit path error after both test processes had already completed
successfully. CTest resolves its relative output path against each build
directory. The completed XML files were copied byte-for-byte into the evidence
directory before the successful acceptance writer; no test was restarted or
reported as passing from an incomplete compiler output.

No C++ implementation, test body, generated SDK or runtime dependency changes
for this review. No sanitizer or full-suite result is inferred from these
focused gates. Source acceptance remains separate from complete G13/G14/G17,
the unresolved naming/quoted-FK decisions and P0-P7.

## Reproduction

Use a fresh record path; retain the existing boundary corpus:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_meta_api_tests teaql_meta_api_probe
cmake --build --preset modules_native --target teaql_module_meta_api_tests teaql_module_meta_api_probe
python3 scripts/verify_core_metadata_source_5_0_6.py \
    --module-build-dir build_modules_native \
    --record /tmp/teaql_core_metadata_source_fresh_5_0_6.json.gz
ctest --preset native --output-on-failure \
    -R '^(meta_api_contracts|meta_api_rust_parity|core_metadata_source_rust_parity)$'
ctest --preset modules_native --output-on-failure \
    -R '^module_(meta_api_contracts|meta_api_rust_parity|core_metadata_source_rust_parity)$'
```
