# Loaded row graph reconstruction

The shared loaded-row reconstruction path now follows the complete Rust 5.0.6
`attach_flat_relation_graph` and recursive `flatten_relation_graph` behavior.
This path is separate from flat query dispatch: it consumes already loaded
compact rows, builds typed graph views and performs no protected I/O.

## Actual original evidence

An append-only direct bridge calls the original attachment method. The original
Context graph registry, model row conversion and read metadata helpers execute
unchanged. The source observer's executor panics on query, mutation or capability
access. The [completed source provenance](parity/loaded_row_graph_5_0_6_source_provenance.json)
records 30 compiled observers and original archive/registry preservation. The
[driver review](parity/loaded_row_graph_5_0_6_driver_review.json) states the matrix
and its remaining boundaries.

The 2,032 observations cover cardinality, forward/reverse keys, plain object/list/
null/typed-null/scalar/absent/identity payloads, absent/populated/empty loaded
caches, facets, nested loaded rows, decoder presence and valid/missing/invalid
owner IDs. Extra cases compare shared identity projections, edge-owned filtered
views, model decode failures and missing/invalid child IDs. Observations include
complete final rows, relationship shape markers, remaining caches, table/edge
projections, full recursive facet metadata, list metadata and exact errors.

The original produces 1,594 successful attachments and 438 errors: 96 facet owner
errors, 8 forward owner errors, 328 reverse owner errors, 4 model decode failures
and 2 missing child-ID errors. Attachment errors do not return the private
partial graph; the observer does not invent access to it. The first C++ comparison
had **856** differences. Their [complete outputs](parity/loaded_row_graph_5_0_6_initial_gaps.json)
retain row, marker, facet, projection and error discrepancies.

## Metadata contract correction

The earlier [follow-up audit](parity/loaded_row_graph_facets_followup_5_0_6.json)
overstated metadata preservation. Actual source reconstruction keeps the loaded
relation's **facets**, then creates a fresh relation SmartList. Its outer total
count, summary and aggregations reset; load state becomes the newly constructed
list's state. Facet SmartLists themselves retain all their metadata recursively.
The paired matrix deliberately provides nonempty outer metadata and verifies
these resets alongside complete facet metadata. Copying every outer field would
change the original contract.

Nonempty to-one facets install independently before null handling. A null to-one
therefore retains facet lookup without manufacturing an optional entity view.
Missing owner IDs fail at this original boundary. To-many facets pass through the
metadata-aware typed graph list decoder, retaining both list and independent
facet lookup. Missing decoder registrations leave the relation untouched.

## Other reconstruction responsibilities

Forward to-one reconstruction copies the first child's foreign key back into
the parent's local key. A single child containing only that identity key becomes
an edge-owned optional view. It never publishes a table entry that could hide a
fully fetched sibling or expose that sibling through the filtered view.

Full forward children enter the identity table once, preserving the first
projection for duplicate identities. Relation availability markers are written
at the original full-forward boundary, rather than prematurely for null, scalar,
reverse or filtered identity branches. Existing loaded shape information remains
intact. The shared runtime keeps Context allocators throughout recursive child,
facet, row and graph storage; no new ownership domain or dependency is introduced.

## SQLite and lifetime integration

The existing SQLite owner fixture loads a one-row child page with facets from
the full relation membership. The embedded result is now also reconstructed into
a typed graph; owner facet sizes remain 2/1/0. Both original loaded rows and the
reconstructed graph are explicitly copied into Global and remain readable after
actual database and UserContext destruction.

A separate lifetime contract supplies intentionally stale outer relation metadata
and complete long-string facet metadata. It verifies the source-shaped rebuild,
independent Global string storage, pristine mutation state, retained typed list
facets and rejection of a User relation handle after close and destruction. The
default PMR resource is disabled. Query-only graph metadata never creates a
mutation ledger or implicitly extends Context lifetime.

## Terminal verification

Actual source/native outputs match and reproduce **2,032/2,032**. Relevant native
checks pass **10/10**, including generated facets, relation analysis, enhanced
entity and application query contracts. Targeted ASan/UBSan checks pass **3/3**
(leak scan disabled), and import-only module checks **3/3** after terminal builds.
The same source corpus and Context lifetime body execute through header and
module consumers. All **116** canonical consumer/fixture bodies, **1,477** exports
and module paths verify; pinned format/check covers **1,002** handwritten files.
See the [terminal record](parity/loaded_row_graph_5_0_6.json).

The [static corpus inventory](parity/active_corpus_baseline_after_loaded_graph_5_0_6.json)
counts 142 active, 64 historical and 21 custom selections. These binding counts
are not semantic acceptance.

## Reproduction and remaining qualification

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_loaded_row_graph_probe
python3 tests/parity/replay_loaded_row_graph_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/loaded_row_graph_5_0_6 \
  --cpp build_native_current/teaql_loaded_row_graph_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^loaded_row_graph_(rust_parity|context_contracts)$'
```

This matrix does not qualify every original historical hydration input, dynamic
fields/JSON view coordinates, protected original SQL/privacy behavior or complete
generated model/configuration surfaces. Additional typed decoder metadata and
integration matrices remain separate. All 17 complete capabilities and P7 remain
partial.
