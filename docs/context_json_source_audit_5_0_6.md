# Context JSON entry source review: Rust 5.0.6

Status: accepted after original14, native2/module2/focused-sanitizer2, 7072
actual-source semantic observations per profile, canonical bodies, no-work
builds and final captured-input/reference guards. This covers the complete 243-line
`teaql-runtime/src/context/json.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`46854ba65946d60c0937a3d29ac4593a4318902c5bd11aee206d18eeac4190b5`.
The separately inventoried `context/json/graph.rs` and `context/json/tests.rs`
remain separate acceptance units.

## Entry points and metadata

`UserContext::decode_json_entity` and `decode_json_entities` in the C++ Context
header preserve the single/list entry responsibilities. Metadata lookup precedes
field-layout validation; list cardinality is checked afterward. Missing installed
entities, mismatched properties or relations, absent field indexes and layout
errors remain distinct. Descriptor equality compares the full property and
relation values with order-independent membership and exact lengths. Table-name
changes alone do not fail this boundary, matching the source.

The list entry creates one operation-local read session, propagates the first
conversion/model error and keeps each result's payload independent. Graph decoding
is delegated to the shared graph implementation; scalar-only input follows the
ordinary model construction path. Presentation hydration grants no save authority.
C++ Context checks and allocator-aware results implement the agreed lifetime
adaptation; the session introduces no new ownership domain.

## Scalar rows and input trust

`JsonReadSession::scalar_row` requires an object and rejects the seven runtime
state names before ordinary dynamic-field handling. Persistent `#` fields require
trusted definitions and storage provenance. Readonly `_` values require a carrier
and a nonempty name; JSON null becomes Value Null, while other values remain JSON.
Relation object/array/null inputs are reserved for graph hydration. Ordinary
fields resolve model names and presentation column aliases; generated C++ metadata
can carry the presentation alias separately from its SQL column spelling.

Conversion occurs before canonical-name sorting and duplicate-alias validation,
which preserves original error precedence. Unknown fields and type errors report
field names without echoing sensitive values. Actual NULL and omission remain
distinct, and sorted present columns define the shared layout.

The scalar switch retains all ten DataTypes. Nullable null is handled first.
Bool requires a boolean; I64/U64 preserve sign/range checks; F64 accepts finite
numbers. Decimal accepts strings or number renderings and rejects invalid parses.
Text and LargeText share the string-only branch. JSON rejects nonnullable null.
Date uses the existing source-qualified date parser. Timestamp accepts signed
milliseconds or RFC3339 input; the C++ convenience parser's zone-less form is
excluded at this boundary. These are typed conversions, not general coercions.

## Shape ownership and allocation repair

Rust's `NativeJsonShapes` uses borrowed field-name lookup keys and copies names
only when installing new immutable geometry. The previous C++ map used owning
strings for lookup and storage: every row copied its field names, even on hits.
The initial public batch regression retains this mismatch in
`parity/context_json_shape_before_5_0_6.json.gz` and its line-buffered diagnostic
receipt. Increasing a name from 32 to 2048 characters added 4032 requested arena
bytes for one row, but 203616 for 100 rows.

The repair uses string-view vectors for lookup. A cache miss copies names into a
Context-owned ColumnLayout, then points the cached views at that layout's immutable
names. Cached keys do not borrow incoming JSON. Cache hits create no owning field
name strings. Payloads remain independent and layouts remain Context borrows.

`teaql_runtime/tests/json_shape_allocation_tests.cpp` exercises the public batch
entry for 1, 100 and 10000 rows using both 32- and 2048-character readonly field
names. Input construction is outside measurement. Arena-entry linker wrapping
counts allocation requests before bump allocation, and a positive byte delta
proves the long-name control is observed. The long-minus-short byte delta must
remain constant as row count grows. Every row must resolve its field and share
its actual geometry with the batch's first row. This is a field-name allocation
invariant, not a claim that whole-batch allocation is constant or zero.

## Original and semantic evidence

`parity/context_json_original_5_0_6.json.gz` runs all fourteen unchanged original
Context JSON unit tests successfully. Its source-preserving reference view and
exact command are retained. Passing these Rust tests does not itself qualify
all corresponding C++ test responsibilities.

`parity/context_json_source_installed_v3_5_0_6.json.gz` rebuilds the original
observer after verifying every original archive file plus its explicit appended
access bridge. It reproduces all 7072 immutable corpus outcomes against native
and imported-module consumers before the allocation repair. Both lifecycle
self-tests, canonical bodies, no-work builds and final source/reference guards
pass. This is semantic evidence for the prior implementation, not allocation
repair acceptance. The first attempt lacked a prepared derived archive; the
second used the wrong module executable path. Those failed receipts are retained.

The corpus covers descriptor/layout precedence, numeric kinds and limits,
nullable projections, dynamic carriers, aliases and error precedence, forbidden
state, model construction errors, array shape-sharing matrices and graph calls.
The lifecycle self-test rejects expired User references and reads an explicit
Global deep copy, including nested view-specific relationships, from a new
Context. Default PMR allocation is disabled.

Final repair verification reruns the corpus and both lifecycle/allocation
checks under native, modules and focused ASan/UBSan. All pass in
`parity/context_json_shape_installed_v2_5_0_6.json.gz`, with leak scanning off.
The receipt verifies canonical module bodies, no-work rebuilds, all original
archive/bridge bytes and captured source/reference identities before acceptance.

The first repaired installed attempt passes the lifecycle check and shows a
constant 2016-byte long-name delta at 1 and 100 rows. Preparing the 10000-row
long-name JSON fixture exceeds the ordinary Context byte budget before the
measurement completes. That failed receipt is retained as
`parity/context_json_shape_installed_5_0_6.json.gz`. The test now explicitly grants
its input fixture a 256 MiB User/application budget; production defaults and
measured allocation assertions are unchanged. All six samples remain required.

At the repaired native checkpoint, measured requested bytes are 12920/14936
(short/long) for one row, 69152/71168 for 100 rows and 5692352/5694368 for
10000 rows. Every long-minus-short delta is exactly 2016 bytes. Both native and
imported-module lifecycle/allocation tests and all 7072 semantic observations
pass. The focused sanitizer repeats the same checks successfully, and the final
receipt records successful source/reference guards.


Acceptance covers this complete entry/scalar/shape source and its C++ ownership
adaptation. The separately inventoried graph implementation, fourteen-test source,
other pending helpers and remaining generator capabilities are not promoted by
this review. This checkpoint changes no generator artifact or public API.
