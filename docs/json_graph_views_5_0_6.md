# JSON graph view coordinates and option identity index

The runtime now retains the Rust 5.0.6 JSON graph coordinate and optional identity
index concepts. Relation views are keyed by view ID, canonical owner entity,
owner ID and relation name. Different JSON projections of the same identity can
therefore share a frozen graph without widening each other's relationship views.
This checkpoint implements graph storage/readback prerequisites; it does not
claim complete JSON parsing or generated JSON graph decoding.

## Original source qualification

Thin append-only bridges expose the original private graph builder and runtime
view methods. Original storage, lookup, ambiguity handling, graph copying and
diagnostics execute unchanged. The [completed source provenance](parity/json_graph_views_5_0_6_source_provenance.json)
records 32 observers, original archive prefixes and locked registry checksums.
The [driver review](parity/json_graph_views_5_0_6_driver_review.json) records the
complete scope and retained original edge replacement behavior.

All 2,304 actual original observations compare complete graph lookups, model
projections, facet visibility, graph counts, view-copy results, ledger behavior
and runtime Debug text. Controls cover ordinary/JSON graphs; equal/different/max
u64 views; zero/nonzero identities; selected/unselected IDs; invalid IDs;
repeated, replaced and cleared edges; lists and options; table priority; and
shared versus independent mutable ledgers. Owner spellings are canonicalized
consistently. No output normalization hides a storage or diagnostic difference.

## Storage and lookup responsibility

Ordinary tables retain priority regardless of the caller's view. If the table
does not contain the requested identity, JSON read graphs can resolve a unique
typed to-one projection through their view/type/ID index. Ordinary graph builders
do not materialize that index, and ordinary optional relation installation never
indexes identities. JSON to-many lists also remain unindexed.

Only explicitly loaded, valid IDs enter the JSON option index. A repeated selected
identity at the same view/type marks that entry ambiguous; neither projection
becomes an implicit identity fallback. Other views remain independent. Null
options remain loaded empty edges. These are read-result coordinates, not new
ownership domains or mutable ledger keys.

The original index stores edge coordinates rather than pointers to payloads.
Repeated/replaced edge behavior is retained exactly, including cases where an
older index key follows the subsequently replaced edge. This matrix does not
silently repair that original behavior or claim a stricter identity contract.

JSON list installation preserves the list's own facets but does not populate the
separate ordinary relation facet index. Ordinary list installation still does.
Facet lookup uses the current view coordinate, preserving the original distinction.

## Runtime views and Context ownership

EntityRuntimeState retains its coordinate through clones, fresh graph views,
shared graph composition and loaded-view transfer. Mutable ledger sharing and
independence remain unchanged. Debug prints the actual coordinate rather than
a constant zero. Existing ordinary behavior uses coordinate zero.

The optional JSON index lives in a Context-owned sidecar and materializes only
for a JSON read graph. Keys, optional edge references, strings, relation payloads
and the sidecar inherit the containing graph's allocator. Explicit destination
copies deep-copy them with the graph; no shared owner extends Context lifetime.
The new fields change state/key layout, so dependent consumers must rebuild.
Existing public names and ordinary constructors remain available.

A lifecycle contract creates ambiguous projections in view 1 and a unique
projection in view 2. It explicitly copies view 2 and its graph to Global, verifies
independent nested string/index storage, destroys UserContext and then verifies
both retained projection isolation and rejection of a User relation handle. The
default PMR resource is disabled throughout these contracts.

## Terminal verification

Actual complete original/native outputs match and reproduce **2,304/2,304**.
Relevant native checks pass **11/11**, including ordinary runtime diagnostics,
facets, historical hydration and loaded-row reconstruction. Targeted ASan/UBSan
checks pass **3/3** (leak scan disabled); import-only module checks pass **2/2**
after terminal compiler/linker processes. All **119** canonical consumer/fixture
bodies, **1,477** catalog exports and module paths verify; pinned formatting/check
covers **1,004** handwritten files. See the [terminal record](parity/json_graph_views_5_0_6.json).

The [static binding inventory](parity/active_corpus_baseline_after_json_graph_views_5_0_6.json)
counts 145 active, 63 historical and 21 custom selections. These counts prove
corpus binding only, not complete semantic acceptance.

## Reproduction and remaining acceptance

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_json_graph_views_probe
python3 tests/parity/replay_json_graph_views_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/json_graph_views_5_0_6 \
  --cpp build_native_current/teaql_json_graph_views_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^json_graph_views_(rust_parity|context_contracts)$'
```

Actual JSON parsing, metadata/capability validation, typed JSON graph registry
decoding, dynamic field/JSON/SQL/privacy and complete generated model/configuration
matrices remain open. All 17 full capabilities and P7 remain partial.
