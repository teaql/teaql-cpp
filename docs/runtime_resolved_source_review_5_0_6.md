# Resolved entity service: Rust 5.0.6 reconciliation draft

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: all 2,261 lines of `teaql-runtime/src/data_service/resolved.rs`, SHA256
`f15e11d112c32c3a076d35d7b548a2afe33f4a49236e61b454ea0fe57433f6d8`.
Disposition: **pending_review**. This draft separates complete-source obligations
from focused witnesses; neither the table nor an accepted delegate proves this
whole source. It introduces no additional migration scope or dependency.

## Responsibility map

| Review | Original responsibility | C++ mapping and remaining reconciliation |
| --- | --- | --- |
| RQ01 | Dynamic-aware typed decode | `dynamic_entities.hpp`, `dynamic_loading.hpp`, `read_hydration.hpp`: supported carriers, validated intent, empty rows and a fresh decoding root remain separate concerns. The entry/cursor matrix checks empty/nonempty unsupported-carrier guards; supported indexed carriers retain their own storage/stream witnesses. |
| RQ02 | Load and merge dynamic rows | Collect every loaded u64 owner ID before calling Context, validate the returned batch and merge through shared shapes. Reconcile error order, duplicates, partial merge and cancellation against actual source. |
| RQ03 | Recursive loaded-row flattening | `relation_graph.cpp`: preserve loaded-cache precedence, NULL, owner/child errors, forward-key recovery, reverse edges, identity-only filtered references, first table installation and recursive facets. The exact-dependency 3,568-case replay is focused evidence. |
| RQ04 | Attach/freeze a flat identity graph | Share one root, builder and installed-identity set across all rows; propagate partial row changes on error and freeze once. Do not substitute JSON projection-view semantics. |
| RQ05 | Bootstrap-aware behavior lookup | `entity_service.hpp`: query behavior uses the query entity; mutation behavior uses the repository entity. Generated schema bootstrap disables both. Preserve callback/resource lookup responsibility. |
| RQ06 | Query preparation and identity projection | Validate intent, join service/query lineage, run before-select behavior then RequestPolicy, and retain relation local keys. Typed reduced projections add metadata-defined identity/version; aggregate/DTO paths remain distinct. Reconcile callback mutations and error timing. |
| RQ07 | Insert preparation | Preserve before-insert behavior, ID/version/timestamp preparation and RequestPolicy order, with distinct values/commands/generated values. Existing lower mutation qualification does not accept this higher preparation path automatically. |
| RQ08 | Update/delete/recover preparation | Preserve each command factory, version requirement, before-behavior and RequestPolicy role. Distinguish repository entity from physical command entity and retained old values. |
| RQ09 | Ordinary row/compact/owned entries | `entity_query.hpp`: preserve each original shortcut guard and query ownership/trace path. In particular, ordinary direct rows and compact relation probes do not share identical Query-owned aggregate guards. The entry matrix preserves those differences. |
| RQ10 | Continuous-page preparation/publication | Validate slice/shape/order, build the scoped key, load and validate a cursor, apply seek conditions, then publish only after all enhancements succeed. Preserve observations and fail-open store errors. See the separate G09 reopening/supplement. |
| RQ11 | Optimization keys | `query_keys.cpp` and pagination adapters: preserve normalized query shape, namespace, user/domain-root scope and original hashing/width adaptations. Reuse existing key/model source evidence only with its exact dependency/input guards. |
| RQ12 | ID-set pagination | `id_set_page.hpp`: shape fallback, stable ID tie-break, store read/build lock/recheck, bounded ID fetch, bad-ID/limit/write fallback, TTL and page query. Generic store acceptance is not acceptance of every caller. |
| RQ13 | Stream execution | `entity_stream.hpp`: retain diagnostic source and validated intent, reject unsupported enhancement before opening, select observed/unobserved/dynamic provider contracts, attach dynamic property definitions and retain lazy errors/cleanup. |
| RQ14 | Prepared provider query | Preserve enabled-cache delegation, separate capture switches, executor observer, final metadata recording and dynamic property definitions. The owned variant moves/clears query lineage into the physical request; it is not interchangeable with a borrowed query clone. |
| RQ15 | Aggregate cache | Preserve custom-backend precedence over concrete memory cache, canonical keys, TTL, cached empty rows, hit/miss/provider telemetry and propagation of provider errors. A hit reclassifies privacy from the current query/compiler without database I/O or a fabricated SQL-success log. Do not infer fail-open backend-panic behavior from the telemetry wrapper. Reconcile combined behavior/policy/privacy cases rather than relying on cache models alone. |
| RQ16 | Enhancement membership snapshots | Original `fetch_prepared_all_with_aggregates` snapshots decoded scalar membership immediately after root fetch, before object-group, child-query or relation mutations. The independent [membership witness](aggregate_membership_snapshot_5_0_6.md) reproduces 864 late-snapshot/error-order differences per frontend. The correction prepares explicit Context-owned keys before enhancement and reuses the assembler; installed/sanitizer qualification is tracked separately. |
| RQ17 | Enhancement order | Preserve groups, child queries, relation loading, relationship aggregates and finally cursor registration. Do not publish a cursor after a failed aggregate or use a pre-aggregate boundary. Current query/cursor witnesses address that specific correction. |
| RQ18 | SmartList and ordinary typed entries | Preserve loaded/total metadata, raw versus typed rows and dynamic guards. Ordinary typed/SmartList methods do not implicitly acquire all enhanced-entry behavior. |
| RQ19 | Enhanced typed branch selection | `enhanced_entities.hpp`: distinguish ordinary, ID-set, compact-flat, flat and embedded graph paths, explicit versus Query-owned aggregates, fresh roots, trace scopes and final decoding. Shortcut coverage does not qualify every graph caller. |
| RQ20 | Public PurposedSelectQuery wrappers | Preserve diagnostic-source selection, owned/borrowed adapters, typed bounds and the single runtime/context execution domain. Existing public signatures remain unchanged by the current row-operation correction. |
| RQ21 | Direct mutation orchestration | `entity_mutation.hpp`: preserve preparation, physical command execution, scoped delete/recover and mutation policies. These methods are not whole graph Save. |
| RQ22 | Event delivery/privacy | `emit_event` binds the service's mutation privacy before Context delivery. Preserve commit/rollback delivery and post-commit failure contracts through their separate event/transaction responsibilities. |
| RQ23 | Prepared insert and batch insert | Execute before emitting created events; preserve per-item trace and empty-batch return. Do not emit successful events after failed persistence. |
| RQ24 | Prepared update and batch update | Preserve supplied old snapshots, updated fields, expected-version increment, new-value overlay and per-item trace. The current source's audit readback helper deliberately returns None without querying; no extra audit SELECT may be invented. |
| RQ25 | Scoped repository construction | Preserve the same Context/executor and independent intent/privacy snapshots, reset only the intended lineage, and prevent a child/sibling from changing its parent's diagnostic ancestry. C++ uses explicit destination copies and checked Context borrows. |
| RQ26 | Query-scope privacy classification | `diagnostic_query.hpp` and entity-service adapters: validate intent, expand facet diagnostics and classify every relation/child/aggregate/group query before parent SQL. Separate aggregate arguments contribute diagnostics without fabricated executable relations. |
| RQ27 | Ordinary future-size regression | The unchanged original test checks lazy ID-set observation and a bounded ordinary Rust future. Its passing replay is retained by loaded-graph qualification; a Rust byte size is not a C++ frame-size guarantee. |

## Current focused evidence

- [Loaded graph reconstruction](loaded_graph_protocol_5_0_6.md): exact original
  dependency replay, 2,032 retained plus 1,536 new observations, original1/caller1
  and complete Context destruction/Global-copy counterparts.
- [Query-owned enhanced aggregates](enhanced_aggregate_execution_5_0_6.md):
  96 reproduced shortcut differences, repaired source/native/imported320, with
  native11/module5 and preserved earlier failures.
- [Query entry/cursor protocols](query_entry_cursor_5_0_6.md): distinct entry
  semantics, original source8,000, cursor side effects, existing G09 regression
  selection and explicit pending-put ownership controls. Its current verification
  status and the fixture sanitizer correction are recorded there.

The current RQ16 witness observes the original scalar attachment key separately
from an object-valued alias, the exact child request and final attached aggregate.
Its explicit snapshot overload preserves the existing assembler, standalone
helper behavior and Context-domain validation. This focused correction does not
close every child-query or relation-hydration composition in the whole source.

Whole-source acceptance still requires the remaining callback/cache/stream/
mutation/public-wrapper responsibilities and their composed callers, with
unchanged-source reuse identified explicitly. Source counts remain 84 accepted,
38 pending and two external exclusions. This draft advances no source,
capability, generator phase, full-profile or device verdict.
