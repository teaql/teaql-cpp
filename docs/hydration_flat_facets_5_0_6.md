# Flat graph owner facet dispatch

Flat and compact-flat relation hydration now call the existing shared facet
runtime and install query-only results in the identity graph. Provider, executor,
transport, service and runtime responsibilities remain separate. No generator
or generated artifact changes belong to this checkpoint.

## Original execution contract

The observer builds original relation plans and calls the complete original
`hydrate_flat_plans_internal` or `hydrate_compact_flat_plans_internal` methods
through a direct append-only access bridge. It uses the actual original Context
graph decoder registry and prepared query executor boundaries. The bridge maps
planner errors explicitly; it contains no reconstructed hydration algorithm.
The [completed source provenance](parity/hydration_flat_facets_5_0_6_source_provenance.json)
records all 29 observers, unchanged original archive prefixes and locked registry
versions/checksums. The [driver review](parity/hydration_flat_facets_5_0_6_driver_review.json)
records the complete stimulus and observation scope.

The 2,304 stimuli vary flat/compact-flat, to-one/to-many, forward/reverse keys,
valid/typed-null/missing local values, valid/missing second owner ID,
include-all/restricted facets, pending delay, poll budget and first/second owner
failure. All child queries carry a slice; facets use complete owner membership.
Full parent maps, availability markers, facet maps, typed relation rows, identity
tables, graph counts, request filters, errors and stop counts are compared.

Original outcomes are 192 success, 1,044 cancellation and 1,068 errors. Errors
include 840 facet fetch failures, 132 facet owner ID failures and 96 reverse
relation owner ID failures. Cancellation drops the actual original future before
observing its partially installed graph. These observations qualify the
recording executor boundary, not physical original SQLite or a threaded scheduler.

Initial C++ outputs differed in 1,728 observations because facets were not
dispatched and graph installation could precede the source's failure/cancel
boundary. Their [complete initial outputs](parity/hydration_flat_facets_5_0_6_initial_gaps.json)
are retained. Shape-only availability markers were added to the final source
matrix after this first comparison. Initial bridge error conversion and shared
model fixture include compile failures remain separate records.

## Shared runtime behavior

Ordinary flat hydration completes an owner's facet operation before publishing
that owner's relation and inverse views. Successful earlier owners remain in
the partial graph when a later owner fails or is cancelled. To-one facet maps
are published before relation decoding; to-many facets are published together
with successful typed SmartList decoding.
The existing relation installation signatures remain available; facet-aware
overloads carry the extra metadata. Header and import-only consumers compile
checks for the original function-pointer types. Missing owner IDs fail at the original
boundary and retain the original error category and text.

Compact-flat forward to-one hydration validates each owner ID and executes all
owner facets before installing the target batch. For a non-leaf forward ancestor,
this happens before descendant loading. Existing final edge-view reconstruction
still preserves descendant projections. Other compact-flat relations publish
their availability marker before awaiting the owner's facet operation.

Context-owned per-owner buckets retain duplicate non-ID local-key membership;
ID-key buckets are consumed once, as in the original. Waiting states forward
wake/cancel to their active operation. Registered cleanup stops pending children
before graph, host buffers and arena blocks are reclaimed. No implicit lifetime
extension or new allocation domain is introduced.

## SmartList decoder responsibility

The graph decoder registry gains the source's metadata-preserving
`decode_compact_smart_list` counterpart. The existing vector list decoder delegates
through it. Typed row conversion preserves total count, aggregations, summary,
facets and load state. The Context decoder adapter keeps this registry boundary
distinct from query execution. Relation installation passes facets through this
SmartList boundary so both typed lists and independent facet lookup retain them.

Compact reverse installation no longer skips a missing local key. It decodes an
empty relation and validates the owner ID, matching the original empty-result
semantics. A missing relation key remains distinct from an invalid owner ID.

## SQLite and Context integration

The native SQLite fixture now runs embedded, flat and compact-flat modes. A one-row
materialized child page retains facets from complete relation membership: two
tags for the first owner, one for the second and an empty facet for a missing-key
owner. Typed graph lists preserve their facet metadata. Explicit Global graph
copies remain readable after actual UserContext and database destruction.

Separate flat and compact-flat cleanup contracts suspend a facet executor owning
a 32 KiB Context host buffer. Closing the Context stops the executor exactly once
while that storage remains readable, then reclaims every arena block. These
contracts disable the default PMR allocator.

## Terminal verification

Actual original/native outputs match and reproduce **2,304/2,304**. Final native
checks pass **8/8**, targeted ASan/UBSan **3/3** (leak scan disabled), and import-only
module checks **3/3** after successful terminal compiler/linker processes. Header
and imported probes execute the same source corpus and retain the same assertions.
All **115** canonical consumer/fixture bodies, **1,477** catalog exports and module
paths verify; pinned formatting/check covers **1,001** handwritten files.
See the [terminal record](parity/hydration_flat_facets_5_0_6.json).

The [static binding inventory](parity/active_corpus_baseline_after_flat_facets_5_0_6.json)
counts 141 active, 64 historical and 21 custom selections. This proves comparator
bindings only, not semantic acceptance. An edit-search failure before mutation
is retained separately and is not counted as proof of the compatibility change;
the final overloads and function-pointer compilation checks are verified.

## Reproduction and remaining acceptance

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_hydration_flat_facets_probe
python3 tests/parity/replay_hydration_flat_facets_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/hydration_flat_facets_5_0_6 \
  --cpp build_native_current/teaql_hydration_flat_facets_probe --check
```

The complete historical hydration family, nested/dynamic relation configurations,
loaded-row graph reconstruction, JSON graph view coordinates, original physical
SQL/privacy matrices and generated model facet APIs remain open. This checkpoint
does not qualify every SmartList metadata combination or full decoder behavior.
The [loaded-row follow-up audit](parity/loaded_row_graph_facets_followup_5_0_6.json)
identifies the separate `relation_graph.cpp` reconstruction path, which currently
extracts only loaded row data and does not forward SmartList metadata. This is
a source audit and still requires actual behavioral comparison.
All 17 full capabilities and P7 remain partial.
