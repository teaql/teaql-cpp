# Runtime entry and handwritten example intent migration

Status: scoped native/lifetime and example-source checkpoint; G17/P7 remain
partial. Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Execute the intended error and cleanup paths

The full native diagnostic run exposed old fixtures without request intent.
They rejected before the behavior/policy callback, foreign operation-domain check
or native failure-producing child. Such rejection did not establish their
intended completion or cleanup assertions.

Query fixtures now supply comment and purpose at their entry. Base/Context
mutation fixtures explicitly bind `MutationIntent`; private entity mutation
fixtures use the existing intent binder. Graph and ledger inputs supply their
own comments. Intent is copied into the existing Context-owned service state;
temporary validation results leave scope before actual Context destruction.
No runtime default intent or bypass was added.

The restored checks cover callback close protection, cancellation before rejecting
a foreign operation domain, move-only native error propagation, pending/wake
forwarding, error destruction, terminal hooks and actual UserContext deletion.
The generic completion fixture continues to exercise Base, Context and Entity
query/mutation entry families, graph/ledger work, scoped completions and
transaction terminal-error precedence. Existing failure and lifecycle assertions
remain intact.

## Preserve handwritten example business operations

The two handwritten examples retain their models, graph saves, projections,
relationships, IDs, audit reasons and version-transition assertions. Their query
entry points add the comments required by the active request contract. The
rollback test supplies explicit comments for its verification queries.

The unchanged stock Rust example binaries omit those comments. This checkpoint
does not claim they execute successfully with missing intent. A separate 5.0.6
driver imports the unchanged original example library directly, invokes its real
SQLite context and public runtime operations, and supplies the same explicit
caller comments as C++. It does not reimplement their runtime behavior or edit
the frozen source.

Fresh observations contain two business scenarios and **34 ordered SQL entries**:
11 for schema/CRUD and 23 for relations/graph. They retain operation, purpose and
audit reason, including derived state-read and persisted-result readback intent.
The C++ ownership fixture compares the complete ordered entries with these actual
Rust results, then verifies explicit Global copies after User deletion and failed
graph-save rollback of both the root and earlier sibling writes.

`runtime_examples_5_0_6_cases.json` and
`runtime_examples_provenance_5_0_6.json` are separate from the original 5.0.5
artifacts. Their comparator enforces matching baseline and source hashes.
The old corpus, provenance and Rust oracle remain intact. The shared old
comparator accepts optional corpus names so the separately named current wrapper
can reuse its mechanics without changing historical evidence.

The oracle adds a direct reference to the original derive crate already locked
transitively. No registry package version or C++ dependency was added. Reproduce:

```sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
    CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
    cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml \
    --bin runtime_examples_5_0_6
python3 tests/parity/generate_runtime_examples_5_0_6.py --check \
    --rust /tmp/teaql_cpp_field_layout_rust_506/debug/runtime_examples_5_0_6
python3 tests/parity/compare_runtime_examples_5_0_6.py \
    --schema-crud build_native_current/teaql_sqlite_schema_crud \
    --relations-graph build_native_current/teaql_sqlite_relations_graph \
    --rust /tmp/teaql_cpp_field_layout_rust_506/debug/runtime_examples_5_0_6
```

## Checkpoint gates and limits

Native CTest passes **10/10** for the selected entry/ownership/example contracts.
Targeted ASan/UBSan passes **7/7**, with leak scanning disabled. The source driver
matches both business scenarios and all ordered provenance entries; a fresh
source replay/reproduction check passes. No MIPS or physical-device run is
claimed. No generator output changed.

This restores scoped C++ ownership/protocol checks and requalifies these two
example scenarios with explicit intent. Complete source-based runtime/private
entry, protected-I/O, trace/privacy/model configuration and remaining historical
oracle migrations are still required. The initial full native 292/489 result is
preserved; no later whole-suite pass is inferred.

A subsequent native query-fixture selection passes **3/3** for enhanced entities, ID-set pages and typed aggregates. Explicit entry intent restores real decoder/hydration paths, coordinated cache builds, pending wake/cancel behavior, retained Global data and actual Context deletion; existing assertions remain. The historical Rust corpora for these families are not relabeled by this C++ maintenance. Targeted ASan/UBSan also passes **3/3**, with leak scanning disabled. The combined checkpoint therefore has 13 native checks and 10 sanitizer checks; it does not requalify the corresponding old Rust corpora.
