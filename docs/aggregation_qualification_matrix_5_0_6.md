# G08 aggregation, relationship analysis and facets

G08 remains partial. The current source-result blocker is AG07's quoted-column
metadata behavior; a user clarification is pending. The strict relationship gate
still rejects all 546 affected results. Neither a passing partial comparator nor
the controlled metadata experiment below establishes unchanged-source parity.

References remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. The
[complete source review](parity/aggregation_complete_source_review_5_0_6.json.gz)
retains twenty complete generic aggregate/cache methods, six complete field and
relation templates, centralized relation naming and the relevant runtime bodies.
The [previous ledger](parity/aggregation_ledger_before_review_5_0_6.json.gz)
preserves every historical compound question.

The table separates implementation responsibilities rather than merging their
layers. Evidence qualifies each named mechanism at its stated boundary; AG07
remains applicable wherever a quoted-column aggregate result is attached.

| ID | Complete responsibility | Evidence and current disposition |
| --- | --- | --- |
| AG01 | Generic grouping, all twelve aggregate functions, aliases and source SQLite errors | Twenty generic bodies are retained. [Scalar aggregation](generated_aggregation_5_0_6.md), [text/projection execution](predicate_projection_requirements_5_0_6.md), [grouped execution](generated_execution_shapes_5_0_6.md) and [alias identity](query_alias_contracts_5_0_6.md) preserve expression/query state, actual rows and unsupported-function failures. The current reference-dependency replay reproduces all 1,668 scalar observations. |
| AG02 | Per-field scalar/object-storage methods and source eligibility | Complete `fieldMethods` and `objectStorageFilterMethods` templates retain ordinary, ID/version, numeric and comparable branches. G07's field/configuration qualification and the restored float-function SQLite cases retain their independent contracts. Application methods use camelCase; storage fields and result aliases keep their authentic identity. |
| AG03 | Forward/reverse presence, absence and matching | Complete object/reverse templates preserve local/foreign keys, minimal live children and negative membership. [Matching/statistics inputs](matching_statistics_inputs_5_0_6.md) qualify public request/selection/core inputs, unrelated source-compatible children, checked bindings and real success/error outcomes. |
| AG04 | Object grouping, details and roll-up | [Relationship execution](generated_relation_analysis_5_0_6.md), [analysis inputs](relation_analysis_inputs_5_0_6.md) and [generated adapters](relation_analysis_adapters_5_0_6.md) retain grouping metadata, matching-plus-grouping roll-up, NULL/NotLoaded and scoped detail views. They use existing graph/query services rather than local example implementations. |
| AG05 | Reverse counts, statistics, numeric/comparable field aggregates and default aliases | Complete `relationAggregateMethods`/`relationFieldAggregateMethod` and `RustRelationAggregateMethod` preserve scalar/list roles, unlimited full children, alias vocabulary and the core-conversion boundary. [Matching/statistics](matching_statistics_inputs_5_0_6.md) retains 2,808 actual outcomes. Quoted-column attachment results remain subject to AG07. |
| AG06 | Bare versus materialized query inputs and metadata conversion | [Bare query inspection](generated_core_query_view_5_0_6.md), [analysis adapters](relation_analysis_adapters_5_0_6.md) and [matching/statistics](matching_statistics_inputs_5_0_6.md) preserve the actual different source conversions. Membership consumes bare core state; grouping/statistics materialize options; field conveniences rewrap the converted core in an empty selection. Context/source identities and explicit destination copies remain enforced. |
| AG07 | Aggregate result key normalization and attachment | **Pending behavior decision:** 6,576 original relationship results match and 546 quoted-Line cases differ. Current SQL logs are identical; original Rust attaches zero/empty results while native raw identifiers attach actual data. The strict gate rejects completion. A raw-metadata source control matches all 7,122 native outcomes but remains candidate evidence, not accepted parity. |
| AG08 | Generated facet sequence, restrictions, metadata, errors and attachment | [Generated facets](generated_facets_5_0_6.md), [async facets](generated_facets_async_5_0_6.md), [analysis adapters](relation_analysis_adapters_5_0_6.md) and [deferred facet/page](deferred_facet_page_5_0_6.md) cover materialization, outer-filter merging, include-all/restriction, duplicate names, actual errors, totals and compact attachment. The frozen generated helper retains its nonrecursive results and separate repository trace prefix. |
| AG09 | Public runtime recursive facets and protected diagnostics | [Public facets](public_facets_5_0_6.md) qualify 1,178 complete public outcomes and 36 actual private-binding SQL cases. Public root-intent validation, recursive options, query-owned trace and replacement of nested facet maps remain distinct from AG08's generated helper. No common name is used to merge their different source semantics. |
| AG10 | Graph storage and embedded/flat/compact/loaded-row facet views | [Graph facet storage](graph_relation_facets_5_0_6.md), [flat/compact dispatch](hydration_flat_facets_5_0_6.md), [owner dispatch](hydration_owner_facets_5_0_6.md), [owner async](hydration_owner_facets_async_5_0_6.md) and the [later flat-boundary repair](parity/flat_graph_boundary_repair_5_0_6.json) cover canonical owners, NULL/cardinality independence, query-only metadata, isolated views and actual deletion. Historical statements that those exact paths remain unimplemented are superseded by their later evidence. General graph/model qualification remains G13. |
| AG11 | Relation planning, grouped per-parent Top-N, stable order, HAVING bindings and copied facet clearing | [Planning extensions](relation_planning_extensions_5_0_6.md) retain 1,152 actual original planning outcomes and native SQLite/lifetime controls. [SQL protocol](sql_protocol_5_0_6.md) retains ranked GROUP BY/HAVING parameter order and actual cached SQLite execution. The current native selection rebuilds their exact tests. |
| AG12 | Cache configuration, propagation, canonical keys and readonly schema identity | [Generated cache](generated_aggregation_cache_5_0_6.md), [query optimization](query_optimization_5_0_6.md), [readonly hash](readonly_schema_hash_5_0_6.md) and [hot execution](execution_hot_cache_and_lookup_5_0_6.md) retain default/explicit hints, parent-to-child propagation, purpose/user/root/readonly keys and actual hit/miss events. A hint alone is not a physical-optimization claim. |
| AG13 | Real aggregation cache storage, expiry, independent results, clearing and governed invalidation | All 204 actual generated scenarios reproduce with exact reference dependencies. Recording wrappers delegate real get/put/invalidation. Wide-separation real-clock controls, stale database values, copied rows, twenty governed saves and literal key/event/SQL records remain unchanged. Whole mutation audit/governance responsibility remains G17; quoted-key attached values remain AG07. |
| AG14 | Retained metadata, nonempty readonly Debug/hash and initialized row/snapshot coordination | [Readonly schema hash](readonly_schema_hash_5_0_6.md) qualifies 69 observations; [initialized cache](initialized_snapshot_cache_5_0_6.md) qualifies 80 semantic cases plus the explicit ownership observation. Schema propagation and canonical query keys compose with those reviewed mechanisms. Rust weak ownership and native checked arena borrows retain their explicit difference; whole G15 remains independent. |
| AG15 | Typed/compact/page/stream execution boundaries, Policy, intent and privacy | Qualified G09 owns execution sequencing, source/result decoding and supported stream restrictions. [Composed execution](composed_general_queries_5_0_6.md), [private JSON](dynamic_json_privacy_5_0_6.md), [simultaneous dynamic facets](dynamic_json_facets_5_0_6.md), hot caches and deferred facets preserve protected-I/O boundaries and exact source failures. Rows/stream are not silently routed through typed-list enhancement. |
| AG16 | Thin generated adapters, compiler constraints, source-selected names and reproduction | Existing adapter/body-preservation records retain the complete `Into<QuerySelection>` family and literal aliases. G06/G07/G09/G10 qualification and current generator/SDK fingerprints preserve emitted behavior. No producer or generated C++ implementation changes belong to this review. Complete application/sample/Assist workflows remain G12. |
| AG17 | Supported field/model configurations and explicit source failures | Existing nullable/scalar, inherited, constant/candidate, grouped, readonly/dynamic and protected matrices cover their source-selected branches through the same runtime. Field/helper inventories alone do not qualify behavior. Unsupported SQLite functions, original aggregate/subquery errors and the AG07 discrepancy remain explicit; broader G13-G17 responsibilities are not waived. |
| AG18 | Nested allocation, checked bindings, Global retention, pending resources and Context destruction | [Resource replacement](deferred_resource_replacement_5_0_6.md), deferred facet/page, typed aggregate and graph/cache ownership controls preserve exactly two allocation domains, nested copies, actual cancellation/join/delete and expiry. Ordinary wrappers do not extend Context lifetime. No request-private arena, shared owner, singleton or third-party runtime dependency is introduced. |

## Current original-dependency replay

Three additional bin targets reuse the byte-unchanged historical Rust observers
inside the existing reference-compatible workspace. No dependency, lock, original
driver, frozen generated domain or runtime algorithm is changed.

| Observer | Actual original records reproduced | Current source matches | Unqualified result differences |
| --- | ---: | ---: | ---: |
| Scalar aggregation | 1,668 | 1,668 | 0 |
| Relationship analysis | 7,122 | 6,576 | 546 |
| Generated aggregation cache | 204 | 204 | 0 |

The [replay record](parity/aggregation_current_dependencies_5_0_6.json.gz) verifies
registry versions/checksums, unchanged corpora and every original result before
executing C++. Original outputs are not rewritten. The existing exact descriptor
Debug spelling adapter remains separate from business results. Pending native
snapshots are labeled review evidence and never counted as source matches.

## Quoted-key control and required decision

The [fresh witness](parity/aggregation_quoted_key_current_witness_5_0_6.json.gz)
reproduces orders 10/11/12: original counts `0, 0, 0`, native counts `2, 1, 0`.
Both execute the same SQL with the same parameters. Frozen Rust's
`enhance_relation_aggregate` looks up `Line.order_id`'s `column_name`, which is
the quoted SQL token `"order"`; SQLite returns the raw column key `order`.
The remapping misses and the attachment layer publishes defaults.

An isolated source control installs the same generated module's metadata through
the public Context `with_metadata` API, changing only that property to the raw
identifier. SQL/provider/query/attachment algorithms and original generated models
remain unchanged. All other observer bodies are byte-identical after removing
the assembly hook and relative fixture-path adjustment. Its
[source proof](parity/aggregation_raw_metadata_bridge_source_5_0_6.json) and
[complete paired outputs](parity/aggregation_raw_metadata_bridge_results_5_0_6.json.gz)
retain **7,122/7,122** native matches under the already declared descriptor Debug
adaptation. This isolates the representation cause; it is not a replacement for
the unmodified original observations.

Two outcomes remain reviewable: retain native raw-identifier correctness with an
explicit source-defect/metadata disposition, or reproduce the original default
results. The user clarification is still pending. Native production behavior,
the old corpus, qualification tags and strict rejection have not been changed.

## Current integration and remaining exit

The [current native record](parity/aggregation_current_integration_5_0_6.json.gz)
rebuilds 28 exact targets and passes **33/33** selected checks, including real
cache/facet/graph/planning and Context contracts. These are integration checks;
the partial relationship comparator's zero exit still excludes AG07. Its
`--require-complete` negative control correctly refuses the pending 546 cases.

The whole G08 capability remains partial until the behavior decision and its
corresponding implementation/acceptance disposition and gates are complete.
G12-G17 and remaining P0-P7 requirements retain their own obligations. Previous
module/sanitizer evidence keeps its stated checkpoint; no new sanitizer, MIPS,
GPU, external provider or physical-device execution is claimed.
