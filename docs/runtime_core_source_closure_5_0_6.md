# Final runtime core source cohort (Rust 5.0.6)

This review qualifies the four remaining runtime/provider source files at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. It closes their complete source
responsibilities, including model defaults, delegated contracts, private helpers,
original tests and C++ ownership adaptations. It does not qualify all generated
model configurations or complete G13/P0–P7 merely by reaching a source count.

## Complete source responsibilities

| Source | Complete review | Acceptance scope |
| --- | --- | --- |
| `teaql-runtime/src/graph.rs` (563 lines) | [GM01–GM12](runtime_graph_source_review_5_0_6.md), [body map](runtime_graph_responsibilities_5_0_6.json) | Ordered EntityValues and conversions; operation/mutation alternatives; scope tokens and recovery; plan/item/batch defaults, comparisons, Debug, adjacent batching and independent counters; update-field selection; complete GraphNode construction/builders/dynamic state; borrowed hierarchical comments; both original tests. Graph-save/service algorithms retain their separate accepted source owners. |
| `teaql-runtime/src/data_service/resolved.rs` (2,261 lines) | [RQ01–RQ26](runtime_resolved_source_review_5_0_6.md), [75-definition inventory](parity/resolved_responsibility_inventory_5_0_6.json) | Every helper/wrapper/factory and execution body: behavior/Policy ordering and bootstrap suppression, owned/borrowed normalization, projection restoration, cache/key and two pagination protocols, dynamic loading/merging and guard order, row-graph attachment, typed roots/schemas, streams, mutation preparation/execution and events. The separately included future-test file retains its own acceptance. |
| `teaql-runtime/src/lib.rs` (4,513 lines) | [Complete root review](runtime_root_source_review_5_0_6.md) | Production declarations/reexports at lines 1–129 and 4508–4513, all five published namespaces, private and generated-support exports, and all 75 original root assertions with their supporting fixtures. The root adds no independent persistence, wire or ownership algorithm; implementing leaves remain distinct. |
| `teaql-provider-sqlite/src/lib.rs` (3,970 lines) | [SP01–SP30](sqlite_provider_source_review_5_0_6.md) | Public dialect/error/executor/provider contracts and all private implementations: physical schema/index/foreign-key validation and events, prepared/layout caches, bindings/decoding/inference, buffered/native/dynamic streams, typed bootstrap admission, IDs/business IDs and retries, transactions, connection/clone identity and cleanup; all 42 original root tests. |

Resolved-service distinctions are preserved rather than folded into one generic
path. Ordinary/compact/enhanced entries keep their different shortcuts. Behavior
precedes Policy; final metadata supplies local keys. Cache hits reconstruct the
current diagnostic classification without fabricated SQL. Dynamic selection and
empty-row guards remain caller-specific; successful enhancement precedes cursor
publication. Mutation preparation precedes provider work, then source-defined
events. `fetch_current_event_row` intentionally returns no row; partial old
snapshots retain the source fallback. Debug/Release version policies and error
effects remain explicit in their linked gates.

The provider review likewise preserves independent statement/layout caches,
pre-commit schema events, structured-error versus sink-exception recovery,
transaction versus business-ID commit failures, storage namespaces and cursor
quiescence. User-owned resources cannot escape into another Context; same-domain
clones borrow state and Global resources can serve a bound User operation.
Synchronous owner-thread/leased-connection behavior is the documented C++
scheduler/ownership boundary, not a claim of identical Rust scheduling.

## Original and current evidence

The graph [fresh original closure](parity/runtime_graph_source_original_closure_5_0_6.json.gz)
preserves all 67 original runtime source/test files. It matches 606 complete
observations and executes both unchanged graph root tests, one supplemental
dynamic-component test and the separately reported service fallback test.
The 464-case operation/plan/module corpus includes 32 module-graph controls
outside this source file; they are not invented graph.rs responsibilities.
Nonempty dynamic state and complete model formatting/equality remain covered.

Resolved-service original observations remain in its individual body/caller
reviews. The current union executes all 37 selected command bodies through
canonical imported binaries; it does not claim fresh Rust execution or 37 newly
registered module CTests. Provider original evidence retains all 42 source tests
and exact matrix/driver/source identities. Root execution retains all 75 source
tests and the separately scoped 272-test original library run; its ignored
subprocess fixture is exercised by its parent.

The [four-source current module closure](parity/core_sources_current_module_checkpoint_5_0_6.json.gz)
uses the same **11,413** frozen inputs and references as the full native epoch:

- Graph: 14/14 registered CTests and the retained model/helper observations.
- Resolved service: 37/37 equivalent native command bodies with only canonical
  module executable paths substituted; arguments/environments/timeouts unchanged.
- Runtime root: 11/11 registered CTests, all 75 mappings and three actual
  old-baseline/changed-body/missing-registration rejection controls.
- SQLite provider: 15,676/15,676 complete retained source outcomes and 27/27
  registered CTests, including stream/binding/lease controls.

Terminal builds, exact registries/JUnit, zero skips, no-work checks and 1,163
canonical consumer/fixture checks pass. Root export checks cover 1,524 names
across seven modules. Each controller directory is removed after its child exits.

Provider timestamp Release behavior reuses its independently qualified
[unchecked source/frontend gate](parity/sqlite_baselines_unchecked_5_0_6.json)
and [terminal profile record](parity/sqlite_baselines_5_0_6_terminal.json): 14,355
observations plus 64 controls match; Release CTests pass 9/9. The complete
`teaql_provider_sqlite/src/decoding.cpp` hash is still
`7fc38477e23803c8915dff6e1599d4bccf602e2ede8223dcdeee47cac036d27f`, identical to
that receipt. This is explicit unchanged-decoder reuse, not a fresh complete
Release build or a claim that checked expectations apply to unchecked code.

## Full native closure and boundaries

The [full native terminal receipt](parity/audit_expression_full_native_terminal_5_0_6.json.gz)
passes **877/877**, with zero failures/errors/skips. It includes all 196 generated
API compiler contracts. Complete builds, no-work checks before/after, exact
registry/JUnit and all 11,413 frozen input/reference checks pass. The
[owning wrapper](parity/audit_expression_full_native_owner_5_0_6.json.gz) exits
successfully and removes 1,115,108 bytes of temporary state/log/JUnit files.
The earlier 876/877 stream-comparator failure remains immutable historical
evidence; it is not relabeled as this successful epoch.

Subsequent syntax-parser installation changes Java, permanent metadata tests and
provenance. Its source/compiler/regeneration gates are separate. No C++ function,
module export or ordinary generated implementation changes; the source-cohort
decision remains tied to the completed frozen C++ checkpoint. Historical 5.0.5
responsibilities and target paths are preserved before replacing stale summaries.

After the recorded disposition update, totals are **122 accepted / 0 pending /
2 excluded_external**. The two concrete external telemetry integrations remain
excluded; their core SPI responsibilities stay in scope. This source decision
does not approve the unanswered quoted-FK or invalid-naming behavior decisions,
waive complete dispatcher/provenance or generated grammar/caller review, or
accept every governed/privacy configuration. Capabilities remain **11 qualified /
6 partial**, and P0–P7 remains open. No new sanitizer, MIPS or physical-router
execution is claimed by this cohort.
