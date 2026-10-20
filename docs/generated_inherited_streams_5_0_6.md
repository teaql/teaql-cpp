# Inherited stream and parent metadata qualification: Rust 5.0.6

Current status: G04/P2 is qualified with documented language/ownership adaptations;
normal inheritance generation is enabled. See [the final acceptance](inheritance_acceptance_5_0_6.md).
The checkpoint status and guard statements below are retained historical evidence.

This checkpoint uses the exact runtime and generator commits in the active
locks. It supplements the inheritance qualification matrix; G04/P2 remains
partial because the frozen producer's inherited Business-ID combination still
does not compile. No frozen generated source is modified.

## Parent metadata API

`scripts/verify_inheritance_metadata.py --check` compares 36 constructed metadata
inputs through the original `ObjectDescriptor.getPredefinedFields()` and
`getPredefinedFieldLayoutRevision()` APIs and the C++ generator's private
`CppFieldLayoutMetadata` adapter. Observations retain ordered fields, canonical
names, declaring owners, types, entity types, layout revision, exception class
and complete message. JSON object-key ordering is not a semantic observation;
array ordering and diagnostic values are compared exactly.

The cases cover compatible canonical-name redeclarations, type/entity-type
incompatibility, unresolved parents, self/two-node/three-node cycles, empty and
duplicate names, skipped computed/dynamic fields, and ancestor chains with empty
intermediate models at depths 2, 8 and 32. This qualifies the fixed-layout metadata
API, not malformed KSML frontend diagnostics or every generator failure path.

## Public generated stream boundary

The stream matrix contains 432 cases: three source models, three result models,
normal/minimal entries, chunk sizes 1/2/4, stops after 0/1/2/all items, and valid
data or an invalid rank in the second row. Both probes execute the public
generated requests against real in-memory SQLite and compare native entity
values and complete errors without rewriting them. After explicit C++ cancellation
or Rust stream destruction, both execute a real query through the same executor.

The original generated `request_support.rs` decodes each complete chunk through
`decode_compact_rows_with_read_metadata` before yielding any entity. Consequently,
an invalid second row with chunk size 2 or 4 rejects that entire chunk; with size
1, the first successful chunk remains visible. The shared C++ typed stream now
uses the existing batch decoder at the same bounded chunk boundary. It preserves
shared read metadata and cancels the source on decode failure. There is no full
result materialization, new dependency or generated-model special case.

Connection availability has different concrete instrumentation: Rust uses
`try_lock` on its pool; C++ requires an immediate nonblocking poll of a real query.
Successful query reuse is checked separately. Counting SQLite statements is
incorrect because released prepared statements may remain in the connection cache.
The observation qualifies cleanup outcomes, not identical startup or asynchronous
error-delivery timing.

## C++ ownership adaptation

The native inherited-model contract also destroys a UserContext with an active
stream and a queued query, using an explicitly Global-owned SQLite connection.
Before destruction, the queued query must remain pending. Afterwards, checked
stream and E borrows must reject access, an explicit Global model copy must retain
its inherited fields and destination allocator, and a new UserContext must read
through the same connection immediately. Default PMR allocation is disabled.

Rust's borrow rules do not permit destroying its context while retaining such
borrows. This is a C++ lifecycle adaptation checked by native and targeted
ASan/UBSan tests; it is not presented as a Rust destruction-timing comparison.

## Reproduction

Native regression passes 5/5 (inherited stream corpus, inheritance lifecycle,
entity stream protocol, returned-model decoding and optional-field integration).
Targeted ASan/UBSan passes 3/3 with `detect_leaks=0`; the module-based inheritance
integration passes 1/1. Both live stream executables
reproduce all 432 retained observations, including 72 decode failures; every case
releases its connection lease and successfully reuses its executor. The 36 parent
metadata observations pass live reproduction. Formatting and active module-path
integrity checks pass. These are scoped gates, not full-suite release acceptance.

```sh
python3 scripts/verify_upstream.py
python3 scripts/verify_inheritance_metadata.py --check
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_generated_inherited_stream_probe teaql_generated_inheritance_tests teaql_entity_stream_tests
ctest --test-dir build_native_current --output-on-failure -R '^(generated_inherited_stream_rust_parity|generated_inheritance_contracts|entity_stream_contracts)$'
```

The Rust oracle is `rust_field_layout/src/bin/generated_inherited_stream_5_0_6.rs`.
Build it against the locked reference and pass its executable plus the completed
C++ probe to `tests/parity/generate_generated_inherited_stream.py` to reproduce
the retained corpus. Run `compare_generated_inherited_stream.py --rust ... --cpp ...`
to verify both live executables against that corpus.
