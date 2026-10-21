# Context JSON graph source review: Rust 5.0.6

Status: accepted after native3/module3/focused-sanitizer3, both actual-source
corpora and final source/reference guards. The complete 352-line
`teaql-runtime/src/context/json/graph.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` has SHA-256
`7bdb1c0d3b9b513c5b796ba97df5bb18e79b63a01743acdb87509312a2c5629f`.
Its C++ counterpart is the graph portion of `teaql_runtime/src/context/json.cpp`.
The separately inventoried JSON entry and JSON test sources retain their own
acceptance status.

## Detection, planning and ownership

`has_relation_input` detects modeled relation object/array/null payloads. Scalar
input uses ordinary model construction without graph state. Graph input seeds the
root capability, parses all views, installs typed edges, freezes the graph and
constructs the root with its own view coordinate. A failed parse does not install
partial typed edges. The existing decoder registry supplies model capabilities;
input cannot replace it.

`GraphJsonPlan` maps to the operation-local C++ Plan: monotonically assigned view
IDs, node count, per-entity capability cache, native views keyed by entity and
identity, and relation shape cache. Parsed nodes retain their descriptor, row,
view coordinate and pending edges. Each edge retains its relation descriptor,
owner identity and ordered children. C++ references and containers remain within
the caller's Context; the graph does not acquire shared lifetime ownership.

Every parsed node increments the count before checking depth greater than 64 or
count greater than 100000. Target installation and decoder capability are checked
even for empty relations. Owners require a modeled, explicitly loaded nonnegative
identity. Many-relations require arrays, one-relations require object or null,
and every child must be an object. The immediate child count is checked against
the remaining node budget before reserving or decoding child views.

## Joins and repeated identities

Local and foreign keys resolve modeled names or mapped columns. A forward
relation whose local key is not the identity infers that key from its target or
NULL, validates a supplied value, and converts inferred values through the scalar
field contract. Missing target keys and incompatible key values retain their
separate errors. Reverse relations and identity-local relations compare loaded
child foreign keys with the loaded parent key; omitted keys do not invent a
conflict. Relation availability is recorded only after successful validation.

Repeated identities preserve every projection. Only their overlapping native
properties are compared. I64/U64 values with the same nonnegative integer compare
as the same join key; other values retain ordinary typed equality. Reconstructed
prior views infer forward scalar keys without reparsing nested graphs or copying
relation payloads. Missing/already-loaded/identity local keys are skipped; null
and object payloads follow their distinct inference paths. Conflicting repeated
values fail rather than widening a view or choosing one payload silently.

The source's NativeViews stores its first input directly and allocates its
additional vector only for repeats. The prior C++ implementation allocated a
vector for every identity. The repair gives the C++ Plan the same first/additional
representation. On repeats it checks the first input and every additional input
before appending the new view. Input pointers live only in the synchronous parse
plan; installed models contain Context-owned rows and graph borrows.

## Installation

Edges install depth first. Each parent gets a fresh state sharing the root graph
borrow and carrying the parent's JSON view coordinate. Child installation returns
(view, row) pairs to the registered target decoder together with exact owner,
identity, relation name and cardinality. The graph freezes before the root is
returned. Decoder and freeze errors propagate through the EntityError boundary.
The existing lifetime test checks explicit Global retention and rejected expired
User references, including nested view-specific edges.

## Evidence and allocation controls

The unchanged fourteen original Context JSON tests pass in
`parity/context_json_original_5_0_6.json.gz`. The existing 7072-outcome observer
covers scalar/graph dispatch, registration, cardinality, join conflicts, repeated
identity projections, depth boundaries and layout sharing. Its original archive
and explicit appended access bridges are verified before rebuilding the observer.
This matrix is not a claim to enumerate every possible model configuration.

`tests/parity/replay_json_graph_limits_5_0_6.py` adds eleven exact original
observations. Seven distinguish oversized relations from malformed child input,
including the reduced budget inside a nested parent. Four use three repeated
views: both first-view and additional-view conflicts must fail; consistent and
sparse views must retain their separate projection results. No error or output
normalization is applied. The C++ probe's `--large-fixture` explicitly budgets
512 MiB for these inputs; ordinary probe and production defaults are unchanged.
The retained corpus also has a native CTest comparator.

`parity/context_json_graph_unique_before_5_0_6.json.gz` records the old allocation
baseline for 1/100/10000 unique children: 96/791/70253 arena requests. The installed
public-entry regression requires at most 94/690/60252, removing one additional-view
vector allocation for each child and the root. Input construction is excluded
from measurement; typed child count, order, identities and loaded list state are
checked afterward. Counters observe actual arena requests before bump allocation.
This is not a zero-allocation graph claim or a wall-clock performance claim.

Final qualification passes native3, imported-module3 and focused sanitizer3
lifecycle, shape-allocation and unique-graph tests, plus both original-source
observation corpora (7072 existing and 11 additional cases per profile).
`parity/context_json_graph_installed_5_0_6.json.gz` records canonical bodies,
no-work rebuilds and captured-input/reference guards before source acceptance.
Leak scanning remains disabled. The native retained-corpus CTest entry also
passes in `parity/context_json_graph_limits_ctest_5_0_6.json.gz`. The full JSON test-source review
and remaining generator phases remain separate work.


Measured unique-graph requests are exactly 94/690/60252 in native and modules,
matching removal of 2/101/10001 pointer-vector requests from the retained baseline.
Requested bytes remain 15031/112143/9938887: the direct first pointer increases the
map value size while eliminating the separate one-pointer allocation. This is an
allocation-request improvement, not a reduction in total requested bytes.
No public runtime API, generated artifact or lifetime domain changes here.
