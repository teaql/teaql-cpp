# Resolved entity service: Rust 5.0.6 reconciliation draft

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: all 2,261 lines of `teaql-runtime/src/data_service/resolved.rs`, SHA256
`f15e11d112c32c3a076d35d7b548a2afe33f4a49236e61b454ea0fe57433f6d8`.
Disposition: **pending_review**. This draft separates complete-source obligations
from focused witnesses; neither the table nor an accepted delegate proves this
whole source. It introduces no additional migration scope or dependency.

## Definition coverage

The [locked-source inventory](parity/resolved_responsibility_inventory_5_0_6.json)
maps all 75 function definitions to 26 primary responsibility groups. RQ27 is
implemented in the separately included `query_future_tests.rs`; it is not counted
as a definition in this file. `ContinuousPageExecution` is the RQ10 state carrier.
The inventory includes the thin public wrappers, policy forwarding methods and
command factories, rather than counting only asynchronous execution bodies.

Run `python3 scripts/verify_resolved_inventory_5_0_6.py` to verify the clean locked
reference, exact source hash, definition order/lines, one primary assignment per
function, and local candidate/evidence entry paths. The recognizer is specific to
this pinned source and does not claim to parse arbitrary Rust. A listed evidence
entry is a review starting point; it does not prove the complete function or its
composed callers. All groups remain subject to whole-source reconciliation.

## Responsibility map

| Review | Original responsibility | C++ mapping and remaining reconciliation |
| --- | --- | --- |
| RQ01 | Dynamic-aware typed decode | `dynamic_entities.hpp`, `dynamic_loading.hpp`, `read_hydration.hpp`: supported carriers, validated intent, empty rows and a fresh decoding root remain separate concerns. The [prepared-selection replay](resolved_dynamic_typed_5_0_6.md) directly qualifies incoming-scope selection, post-callback dynamic selection, empty results and unsupported-carrier guard ordering; the [intent-guard continuation](resolved_dynamic_guard_5_0_6.md) additionally repairs four later guard differences with source/native/imported72 and native4/module4; the [ordinary indexed caller replay](resolved_dynamic_loaded_5_0_6.md) now covers successful ready-provider hydration (source/native/imported288, native5/module5). Multirow sharing, suspended indexed loading and enhanced callers remain separate. The entry/cursor matrix checks empty/nonempty unsupported-carrier guards; supported indexed carriers retain their own storage/stream witnesses. |
| RQ02 | Load and merge dynamic rows | The [2,724-input direct helper replay](resolved_dynamic_rows_5_0_6.md) compares actual owner-ID collection, Context/provider ordering, complete batch validation, duplicates, selected-shape identities and 16 partial-merge errors. A shared internal service entry delegates to the unchanged operation at all five callers. Ready-provider source observations and existing complete native pending-provider cleanup programs retain distinct scopes. |
| RQ03 | Recursive loaded-row flattening | `relation_graph.cpp`: preserve loaded-cache precedence, NULL, owner/child errors, forward-key recovery, reverse edges, identity-only filtered references, first table installation and recursive facets. The exact-dependency 3,568-case replay is focused evidence. |
| RQ04 | Attach/freeze a flat identity graph | Share one root, builder and installed-identity set across all rows; propagate partial row changes on error and freeze once. Do not substitute JSON projection-view semantics. |
| RQ05 | Bootstrap-aware behavior lookup | `entity_service.hpp`: query behavior uses the query entity; mutation behavior uses the repository entity. Generated schema bootstrap disables both. The [select preparation continuation](resolved_select_preparation_5_0_6.md) observes query-entity selection and actual bootstrap suppression; mutation lookup retains its separate callers. |
| RQ06 | Query preparation and identity projection | Validate intent, join service/query lineage, run before-select behavior then RequestPolicy, and retain relation local keys. The [1,036-input continuation](resolved_select_preparation_5_0_6.md) exercises actual borrowed/owned preparation, callback rewrites/errors, final-entity projection and input immutability. Typed reduced projections add metadata-defined identity/version; aggregate/DTO paths remain distinct and require their own callers. |
| RQ07 | Insert preparation | The [1,690-input preparation continuation](resolved_mutation_preparation_5_0_6.md) directly exercises before-insert behavior, then RequestPolicy, final-entity metadata, ID/version initialization and Create-marked Checker/Fix. These methods do not call timestamp preparation. Distinct values/commands/generated values and later persistence remain separate responsibilities. |
| RQ08 | Update/delete/recover preparation | The [direct update preparation controls](resolved_mutation_preparation_5_0_6.md) retain repository-entity behavior selection, policy rewrites, old snapshots and optional expected versions without insert initialization or Checker execution. The four mutation factories transparently delegate to accepted core constructors with the repository entity and supplied key/version; C++ adds its explicit allocator. Delete/recover orchestration and execution-time version requirements retain separate callers; preparation must not invent those checks. |
| RQ09 | Ordinary row/compact/owned entries | `entity_query.hpp`: preserve each original shortcut guard and query ownership/trace path. In particular, ordinary direct rows and compact relation probes do not share identical Query-owned aggregate guards. The entry matrix preserves those differences. |
| RQ10 | Continuous-page preparation/publication | Validate slice/shape/order, build the scoped key, load and validate a cursor, apply seek conditions, then publish only after all enhancements succeed. Preserve observations and fail-open store errors. See the separate G09 reopening/supplement. |
| RQ11 | Optimization keys | `query_keys.cpp` and pagination adapters: preserve normalized query shape, namespace, user/domain-root scope and original hashing/width adaptations. Reuse existing key/model source evidence only with its exact dependency/input guards. |
| RQ12 | ID-set pagination | `id_set_page.hpp`: shape fallback, stable ID tie-break, store read/build lock/recheck, bounded ID fetch, bad-ID/limit/write fallback, TTL and page query. The [explicit-intent continuation](resolved_id_set_intent_5_0_6.md) preserves233 historical outcomes and adds233 explicit-purpose controls: source/native/module466, native4 and canonical1132. It observes153 query calls and61 attempted puts, closing the earlier missing-purpose early-exit gap. Generic store acceptance is not acceptance of every caller. |
| RQ13 | Stream execution | `entity_stream.hpp`: retain diagnostic source and validated intent, reject unsupported enhancement before opening, select observed/unobserved/dynamic provider contracts, attach dynamic property definitions and retain lazy errors/cleanup. The [readonly stream continuation](resolved_stream_schema_5_0_6.md) repairs 80/769 differences with a checked borrow of the registered Context schema: source/native/imported769, native10/module8 and scoped SAN4 pass. This closes RQS01 for G09 without whole-source acceptance. The [generated readonly-stream continuation](generated_readonly_stream_5_0_6.md) additionally checks the complete 1003-row/14-chunk original scenario, absent schema through list/stream calls and retained generated entities after cursor close (original3/native1/module1, SDK reproduction). |
| RQ14 | Prepared provider query | Preserve enabled-cache delegation, separate capture switches, executor observer, final metadata recording and dynamic property definitions. The owned variant moves/clears query lineage into the physical request; it is not interchangeable with a borrowed query clone. The [schema continuation](resolved_query_schema_5_0_6.md) repairs readonly definition attachment across compact/owned/cache returns; its current gates are recorded separately. |
| RQ15 | Aggregate cache | Preserve custom-backend precedence over concrete memory cache, canonical keys, TTL, cached empty rows, hit/miss/provider telemetry and propagation of provider errors. A hit reclassifies privacy from the current query/compiler without database I/O or a fabricated SQL-success log. Do not infer fail-open backend-panic behavior from the telemetry wrapper. Reconcile combined behavior/policy/privacy cases rather than relying on cache models alone. The [cache execution continuation](resolved_cache_execution_5_0_6.md) adds288 original/native/module outcomes for custom-versus-concrete precedence, empty hits, cache bypass, classifier counts and exact ordered cache/provider telemetry; native5 and canonical1133 pass. The [schema/cache witness](resolved_query_schema_5_0_6.md) covers raw-before-schema cache storage and logging-disabled hit classification, without whole-cache acceptance. The [generated readonly-cache continuation](generated_readonly_stream_5_0_6.md) adds two enhanced typed reads with shared snapshot/schema identity and exactly one physical SQLite statement, still without accepting every cache composition. |
| RQ16 | Enhancement membership snapshots | Original `fetch_prepared_all_with_aggregates` snapshots decoded scalar membership immediately after root fetch, before object-group, child-query or relation mutations. The independent [membership witness](aggregate_membership_snapshot_5_0_6.md) reproduces 864 late-snapshot/error-order differences per frontend. The correction prepares explicit Context-owned keys before enhancement and reuses the assembler; installed/sanitizer qualification is tracked separately. |
| RQ17 | Enhancement order | Preserve groups, child queries, relation loading, relationship aggregates and finally cursor registration. Do not publish a cursor after a failed aggregate or use a pre-aggregate boundary. Current query/cursor witnesses address that specific correction. |
| RQ18 | SmartList and ordinary typed entries | Preserve loaded/total metadata, raw versus typed rows and dynamic guards. Ordinary typed/SmartList methods do not implicitly acquire all enhanced-entry behavior. The [192-input typed continuation](resolved_query_schema_5_0_6.md) restores compact result metadata and fresh runtime state. The [288-input preparation continuation](resolved_dynamic_typed_5_0_6.md) unifies ordinary/dynamic typed decoding, scopes from incoming selection and uses post-Behavior/Policy selection while bypassing aggregation cache. Source/native/imported288, native10/module10 and SAN4 pass; full supported indexed/enhanced callers remain separate. |
| RQ19 | Enhanced typed branch selection | `enhanced_entities.hpp`: distinguish ordinary, ID-set, compact-flat, flat and embedded graph paths, explicit versus Query-owned aggregates, fresh roots, trace scopes and final decoding. The [enhanced scope continuation](resolved_enhanced_typed_5_0_6.md) repairs 704 differences in ordinary-entry preparation/scope timing; the explicit aggregate and owned entries retain earlier binding. Source/native/imported3456 and native12/module12 include retained graph/cursor/membership/cleanup regressions. The [generated child/root continuation](dynamic_child_selection_5_0_6.md) adds original2/generated7 with native1/module1 and SDK reproduction, covering dynamic value/NULL/missing merge, shared availability, JSON and five malformed provider faults through the actual enhanced entry. These selected callers do not qualify every graph composition. |
| RQ20 | Public PurposedSelectQuery wrappers | Preserve diagnostic-source selection, owned/borrowed adapters, typed bounds and the single runtime/context execution domain. Existing public signatures remain unchanged by the current row-operation correction. |
| RQ21 | Direct mutation orchestration | `entity_mutation.hpp`: the [bound-intent execution continuation](resolved_mutation_execution_5_0_6.md) compares actual insert/update/delete/recover and prepared/batch calls, including 1,472 new provider-reaching inputs. It preserves preparation, trace selection, physical command execution, scoped delete/recover and request policies. These methods are not whole graph Save or full Mutation Policy governance. |
| RQ22 | Event delivery/privacy | The [5,486-input privacy continuation](resolved_mutation_privacy_5_0_6.md) observes original `emit_event` forwarding through actual mutation calls: absent versus empty provenance, masked/credential/unknown/target-ID data, intent replacement and safe-event fields/trace. Canary assertions supplement equality. Commit/rollback delivery and protected external publication retain their event/transaction and G17 responsibilities; raw sink access is not a protected log endpoint. |
| RQ23 | Prepared insert and batch insert | The [3,518-case execution continuation](resolved_mutation_execution_5_0_6.md) observes writes before created events, per-item trace, empty-batch return and provider/event failure boundaries. Controlled provider success is not live transaction/commit evidence. |
| RQ24 | Prepared update and batch update | The [execution continuation](resolved_mutation_execution_5_0_6.md) compares supplied old snapshots, updated fields, expected-version increment, new-value overlay and per-item trace. The [privacy/boundary continuation](resolved_mutation_privacy_5_0_6.md) adds 60 maximum-version Debug failures after provider execution, including first-event delivery before second-event overflow. It makes no new Release claim. The source audit readback helper returns None without querying; no extra audit SELECT is invented. Invalid short batch-ID inputs retain their caught source panic/native checked-exception boundary. |
| RQ25 | Scoped repository construction | Preserve the same Context/executor and independent intent/privacy snapshots, reset only the intended lineage, and prevent a child/sibling from changing its parent's diagnostic ancestry. C++ uses explicit destination copies and checked Context borrows. |
| RQ26 | Query-scope privacy classification | `diagnostic_query.hpp` and entity-service adapters: validate intent, expand facet diagnostics and classify every relation/child/aggregate/group query before parent SQL. Separate aggregate arguments contribute diagnostics without fabricated executable relations. |
| RQ27 | Ordinary future-size regression | The [complete future-test source review](runtime_service_surface_source_review_5_0_6.md) now records original1/observer1 and native1/module1: unpolled observation/provider laziness, completion and the8192-byte ceiling. Rust5504 and native/imported2064 are distinct ABI measurements, not a total allocation bound. This accepts the separate test file, not resolved.rs. |

## Current focused evidence

- [Prepared dynamic selections](resolved_dynamic_typed_5_0_6.md): source/native/
  imported288, native10/module10, canonical/no-work and SAN4. Two reads and four
  ordered callbacks per case prove cache bypass and final-selection guards;
  eight native pending/deletion paths preserve fresh roots and cleanup.
- [Readonly streamed chunks](resolved_stream_schema_5_0_6.md): original769,
  native10/module8, canonical/no-work and scoped SAN4. Registered schema borrows
  retain lazy errors/empty chunks, exact-once cleanup and explicit Global copies;
  only G09 restores its prior scope through the separate joined verdict.
- [Readonly results and ordinary typed lists](resolved_query_schema_5_0_6.md):
  actual-source/native/imported2,304+192, native45/module18, canonical/no-work
  and scoped ASan/UBSan4. Prepared schemas, cache-hit classification and fresh
  typed decoding roots are repaired. The later readonly stream continuation closes
  the streamed-schema portion of RQS01 within its recorded scope.
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
- [Insert/update preparation](resolved_mutation_preparation_5_0_6.md): actual
  source/native/imported1,690, native5/module4 and no-work. Prepared values and
  scalar metadata/Checker/ID responsibility are distinct from database writes.
- [Resolved mutation execution](resolved_mutation_execution_5_0_6.md): retained
  1,502 plus 2,016 trusted-intent controls, native6/module5 and no-work. Actual
  source observations include 1,648 provider calls and 982 attempted events;
  physical SQLite commits and full governance/privacy remain separate.
- [Mutation provenance and version boundaries](resolved_mutation_privacy_5_0_6.md):
  retained 3,518 plus 1,968 new controls, actual-source/native/imported5,486,
  native7/module6 and permanent safe-output assertions. Raw sinks, real commits,
  Release overflow and whole protected-I/O/governance remain separate.
- [Dynamic row loading](resolved_dynamic_rows_5_0_6.md): actual complete-source
  helper and both frontends match 2,724 validation/merge/sharing inputs. Whole
  typed decoding, provider storage and source acceptance remain separate.

The current RQ16 witness observes the original scalar attachment key separately
from an object-valued alias, the exact child request and final attached aggregate.
Its explicit snapshot overload preserves the existing assembler, standalone
helper behavior and Context-domain validation. This focused correction does not
close every child-query or relation-hydration composition in the whole source.

Whole-source acceptance still requires the remaining callback/cache/stream/
mutation/public-wrapper responsibilities and their composed callers, with
unchanged-source reuse identified explicitly. Active source counts are 112 accepted,
10 pending and two external exclusions. This draft advances no source,
capability, generator phase, full-profile or device verdict.

## Prepared cache intent ordering follow-up

For RQ14/RQ15, [prepared-cache intent ordering](resolved_cache_intent_order_5_0_6.md)
records eight reproduced differences across 18 original-source observations.
The shared prepared-query operation now consults the cache before constructing
request intent, matching the private Rust helper. Public entries retain mandatory
intent validation. Native and module comparisons match 18/18; public query,
schema/cache and complete enhancement regressions also pass. This focused repair
does not promote the complete source to accepted.

## Scoped repository and privacy traversal readback

The source's `scoped_data_service_internal` keeps the same Context/executor,
clones mutation intent/privacy and request intent, clears repository trace and
snapshots query privacy independently. `EntityDataService::scoped_data_service`
retains these responsibilities through Context-bound allocator copies;
`query_scoped_service` restores the parent trace for query execution. There is
no shared ownership extension of the Context.

For RQ26, the original `query_scoped_service` validates intent, expands diagnostic
facets, and traverses a LIFO list containing relation queries, child enhancements,
relation aggregates and object-group queries. `EntityDataServiceAccess::query_scoped`
uses the same insertion/traversal order. Separate aggregate arguments are appended
only to a diagnostic clone, conditioned on execution-metadata capture. The C++
callers in `entity_query.hpp`, `enhanced_entities.hpp` and `diagnostic_query.hpp`
retain that distinction from the actual executable query. Source and C++ differ
in when the temporary scoped copy is allocated relative to intent validation;
this readback does not assert allocation-count equivalence.

This is implementation readback, not new execution evidence. The [direct scoped-privacy continuation](resolved_scope_privacy_5_0_6.md) now
adds eight original/native/module observations of parent/child/sibling privacy
independence, inherited intent and diagnostic child traversal. Complete trace,
mutation privacy and other diagnostic tree compositions still require their
separate evidence before closing RQ25/RQ26.

## Diagnostic tree continuation

For RQ26, the [direct tree replay](resolved_scope_tree_5_0_6.md) now compares
28 actual-source/native/module observations across all five query-child carriers,
their combined order, disabled capture, invalid intent and unchanged source query.
It includes the existing facet-expansion behavior rather than replacing it with a
generic recursive walk. Combined with the preceding eight snapshot observations,
it supplies direct caller evidence beyond the accepted redaction/expansion models.
Separate aggregate arguments retain their generated enhanced-entry witness in
`dynamic_json_privacy_5_0_6.md`; physical protected-log publication remains the
Context/provider responsibility. This continuation does not promote the whole
source or rerun unrelated historical provider gates.

## Public wrapper reconciliation

Readback of the nine original public wrappers distinguishes optional diagnostic
scoping for ordinary rows/SmartList/typed/enhanced reads, mandatory source scoping
for streams, and mandatory aggregate-aware scoping for explicit aggregate list
entries. Owned forms retain their corresponding request semantics; C++ stores
Context-owned operation snapshots rather than extending a stack query lifetime.

The [SmartList aggregate continuation](resolved_public_aggregate_scope_5_0_6.md)
reproduces a wrapper gap with three differences: bound intent or an existing
diagnostic source could suppress classification of separate aggregate arguments.
The SmartList entry now delegates through the existing complete-scope operation
before the unchanged row/list mechanics. This repair is necessary for RQ20/RQ26;
ordinary wrapper reuse alone was not sufficient proof of parity.

## Current joined regression gate

The [31-test union](resolved_joined_regression_5_0_6.md) exposed an ordinary-row
inline-aggregate shortcut regression despite the previous focused checkpoints.
Conditional normalization is restored to the source enhancement-entry guards.
The repaired native union passes all 31 tests, with current input hashes held
stable across build and execution. The module union also passes all 31 equivalent commands. Complete source
reconciliation remains outstanding. G09 was subsequently restored by its
[separate requalification](resolved_g09_requalification_5_0_6.md); this source-level
union does not replace capability acceptance.

## Private typed-aggregate scope readback correction

The earlier draft incorrectly stated that
`fetch_entities_with_relation_aggregates_internal` calls
`query_scoped_service(query)` directly. Readback of the clean, hash-verified
locked source shows that line 1509 calls
`query_scoped_service_with_aggregates(query, relation_aggregates)`. The helper at
line 2203 appends separate aggregate queries as diagnostic child enhancements
when execution metadata capture is enabled. It delegates unchanged when the
aggregate list is empty or capture is disabled.

`TypedAggregateOperation` follows this same initial dynamic-selection and
unbound-intent guard and diagnostic-clone construction. The proposed source
mismatch was a documentation error, not grounds for a runtime change. The [direct private-entry observer](resolved_private_aggregate_scope_5_0_6.md)
now matches all eight original/native/module outcomes for dynamic selection,
metadata capture and bound intent; native regression passes 3/3. Its missing-
descriptor fixture isolates classification before query preparation; it does
not prove successful dynamic aggregate hydration.
Whole-source reconciliation remains pending.

## Flat-plan representation boundary

The [flat predecode review](resolved_flat_predecode_5_0_6.md) identifies an RQ19
caller mismatch beyond the earlier preparation/scope corpus: unrequested inline
children were decoded before ordinary Entity hydration. Four of twenty actual
original-runtime observations differ. Removing the adaptation globally fixes
those four but loses 36 generated nested-relation observations, so that candidate
is explicitly rejected. The current repair makes graph-backed model hydration
an explicit optional capability, defaulting to false and enabled by the generated
model base. Installed native, module, lifecycle and complete active-SDK
reproduction evidence is tracked in that review; this finding does not promote
the whole source or replace the remaining P0-P7 acceptance requirements.

## Joined repair and pending typed-work checkpoint

The [37-check native/imported union](resolved_joined_regression_5_0_6.md#joined-regression-after-the-flat-graph-repair)
passes after the explicit model representation repair, with frozen inputs and
all previous 31 checks retained. RQ01/RQ19 now also have eight executed C++
pending dynamic-work cleanup cases: ordinary, private aggregate, enhanced and
continuous enhanced entries, each with explicit cancellation or actual Context
deletion. Native/imported/focused sanitizer executions preserve the existing
multirow query program and validate one join/destruction before arena reclamation.
This closes those cancellation-path coverage gaps without replacing original
ready-provider comparisons. The subsequent [sixteen-scenario continuation](resolved_dynamic_loaded_5_0_6.md#suspended-typed-caller-completion)
adds controlled successful/error resumption at the same four entries, wake
forwarding, single delivery, multirow loaded-state sharing and actual ID reads
during cleanup. Native/imported/focused sanitizer each pass the complete program;
threaded-provider stress and whole-source acceptance remain separate.
The complete source remains pending_review.

## Mutation Release-policy continuation

The [release arithmetic replay](resolved_mutation_privacy_5_0_6.md#release-arithmetic-policy-continuation)
resolves the missing maximum-version policy observation for RQ21/RQ22 without
changing runtime bodies. Sixty retained Debug overflow inputs plus sixty adjacent
controls match all120 actual original Release-policy/C++ NDEBUG observations,
including144 provider calls and144 attempted events. C++ mutation templates are
compiled in the probe with a support-archive symbol audit. This is an isolated
policy qualification, not a complete Release build or live SQLite commit gate.
Whole-source acceptance remains pending.

## Current complete-method and composed-consumer readback

The current readback retains the 75-definition/26-group inventory and verifies
it again against the unchanged locked source. The two top-level helpers, the
continuous-page state carrier, all thin wrappers and the private mutation/event
methods remain in scope. The separately included future-test source keeps its
independent acceptance; it is not counted as another function in this file.

Several distinctions must survive the complete-method reconciliation:

- Ordinary and compact list entry points have different relation-aggregate fast
  paths. Prepared owned/borrowed reads, raw SmartList reads and enhanced typed
  reads also retain their distinct normalization, cache and hydration routes.
- Query preparation selects behavior from the incoming query entity, applies
  behavior before Policy and adds relation local keys using the final metadata.
  Typed entity paths restore identity/version fields without changing DTO or
  aggregate projection semantics. Dynamic selection capture and empty-row
  guards remain specific to each caller rather than one universal entry guard.
- Cached data contains rows rather than a retained request intent. A cache hit
  reconstructs diagnostic classification from the current compiler without a
  provider call or fabricated SQL-success record. Readonly schema attachment and
  fresh typed roots remain separate from the shared cached payload.
- Stream observation owns its scoped service through delayed consumption.
  Dynamic stream storage/selection admission precedes opening the provider;
  unsupported relation/group enhancement retains its explicit failure boundary.
  Continuous cursors publish only after the relevant successful execution path.
- Mutation preparation, provider calls and event emission retain source order.
  The private `fetch_current_event_row` intentionally performs no database read;
  complete original snapshots avoid replacement, while missing/partial snapshots
  retain the source's empty fallback. Insert/update/delete/recover and batch
  events retain their different trace handling, earlier effects before failures,
  and checked-Debug versus Release version arithmetic.
- A repository scope preserves its receiver entity and takes an independent
  snapshot of diagnostic ancestry. Explicit aggregate arguments participate in
  privacy traversal without adding fabricated execution relation frames. The
  complete request tree is classified before the first protected SQL record.

These are source/caller dispositions, not additional features inferred from
delegate acceptance. Their original observations remain in the responsibility
table and its linked reviews, including the retained initial mismatches and
rejected alternatives.

The [current imported union](parity/resolved_source_current_module_union_5_0_6.json.gz)
passes **37/37 equivalent native command bodies**, with only each executable path
replaced by its canonical module consumer. It does not claim 37 newly registered
module CTests. Exact native selection, argument/environment preservation,
individual terminal commands, compiler outputs, no-work builds before/after,
1,163 canonical consumer/fixture checks and immutable input/reference guards are
retained. All 11,412 execution inputs agree with the full native epoch. The owned
controller directory (3,814 bytes) was removed after its children exited.

No source function, generated SDK or original corpus changes in this readback.
The current full native build and compiler contracts passed. Its terminal CTest
result is **876/877**, with the obsolete stream comparator as the only failure;
the [strict-source readback](protected_stream_shapes_5_0_6.md) records that issue
and the comparator repair separately. No source disposition or capability is
promoted by this checkpoint; complete integration, source acceptance and all
remaining P0–P7 exits remain separate decisions.
