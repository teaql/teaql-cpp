# Embedded per-owner relation facets

Complete original `enhance_query_relations_internal`, `relation_facets` and
`execute_facets` methods execute through the existing direct access bridge.
Prepared child/facet fetches reach a recording executor. This checkpoint
qualifies embedded relation facet responsibility, not the full flat graph,
dynamic loading, asynchronous failure/cancel or model/configuration matrix.

## Actual source evidence

The 48 complete observations vary plain/faceted queries, include-all versus
restricted facets, child slice/order, to-one versus to-many relations and
valid/typed-NULL/missing owner keys. Full parent value maps, loaded facet value
maps and request filter diagnostics match and reproduce **48/48**.
The recording executor supplies fixed rows; predicate scope is qualified by
complete request snapshots, separately from physical SQLite integration.
See the [driver review](parity/hydration_owner_facets_5_0_6_driver_review.json) and
[source provenance](parity/hydration_owner_facets_5_0_6_source_provenance.json),
which records a completed 27-observer build and byte-preserved original archive
prefixes and registry versions/checksums.

The first 24 plain controls already matched. All 24 faceted observations lacked
per-owner results in C++; their
[initial outputs](parity/hydration_owner_facets_5_0_6_initial_gaps.json) are retained.
After facet dispatch was implemented, 12 request diagnostics differed because
the Rust fixture assigned the purpose field while the C++ fixture used a builder
that added a Purpose trace. Both fixtures now use the original field assignment;
production trace behavior is unchanged. The
[post-dispatch differences](parity/hydration_owner_facets_5_0_6_after_dispatch_gaps.json)
remain separate from final observations.

## Shared runtime responsibility

The embedded hydration node now runs per-owner facet operations after publishing
the relation's child rows. It uses captured local keys, so relation aliasing cannot
change membership. Outer membership queries remove the materialized child page's
slice, partition and ordering; valid owner keys add equality, while NULL/missing
keys use a false predicate. Existing validated intent and relation trace are
preserved before shared facet execution.

Facet results attach to the relation's loaded SmartList, including empty
relations. To-one loaded results preserve the single-row view. A thin runtime
adapter delegates typed facet list/aggregate fetching to the existing entity
service and maps its errors to the existing RuntimeError contract. Provider,
executor, transport and data-service responsibilities remain separate.

Existing TeaqlRuntime and facet declarations move to a dependency-safe shared
`facet_runtime.hpp`; public names and existing function bodies are unchanged.
The [relocation record](parity/owner_facets_interface_relocation_5_0_6.json)
retains original declaration hashes, and module declaration locators follow the
new header. Generation and generated artifacts are unchanged.

Context-owned operations forward wake/cancel to their active children and retain
no extra ownership domain. Scope descriptors, query options and nested facet
results use the same Context resource. Flat/compact graph facet installation is
still unqualified and is not implied by this embedded implementation.

## SQLite and lifetime integration

The native SQLite regression limits each owner's materialized child page to one
row but checks facets against the full relation: owner 1 gets both tag values,
owner 2 gets its own value and the missing-key owner gets an empty facet. Values
never cross owners, and nested facet results explicitly copied to Global memory
survive actual UserContext/database destruction. Default PMR allocation fails
throughout these contracts.

Fixture include/construction and scoped trace-vector compile failures remain
recorded. Trace nodes are appended with explicit allocator-aware construction;
no implicit copy assignment or alternate allocation domain is introduced.

## Reproduction and remaining acceptance

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_hydration_owner_facets_probe
python3 tests/parity/replay_hydration_owner_facets_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/hydration_owner_facets_5_0_6 \
  --cpp build_native_current/teaql_hydration_owner_facets_probe --check
```

Relevant native checks pass **8/8**, targeted ASan/UBSan **2/2** (leak scan
disabled), and import-only module checks **2/2** after terminal builds. All **112**
canonical consumer/fixture bodies and **1,477** catalog exports verify. Module
paths verify; pinned format/check covers **998** handwritten files. Native,
sanitizer and imported probes run the same complete source corpus. See the
[terminal record](parity/hydration_owner_facets_5_0_6.json).

The [binding inventory](parity/active_corpus_baseline_after_owner_facets_5_0_6.json)
counts 138 active, 64 historical and 21 custom selections. These static counts
are not behavioral acceptance.

All 17 full capabilities and P7 remain partial. Additional flat graph facet
installation, dynamic readback, asynchronous failure/cancel and model/metadata/
privacy/source-SQL matrices remain required.
