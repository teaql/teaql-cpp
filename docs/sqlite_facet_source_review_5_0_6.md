# SQLite facet integration source review (Rust 5.0.6)

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-provider-sqlite/tests/facet_trace.rs`, 1,158 lines, SHA256
`78efae6409e460bf8e85afe1c9d24b1ba82f72d8f08f247904135fe39d536e82`.
Status: **accepted** for the complete locked test-source responsibilities SF01-SF10.
The final assertion reconciliation and native/imported receipt below supersede
historical pending statements. The containing provider and runtime source files
retain independent acceptance obligations.

## Original execution

The [complete original receipt](parity/sqlite_facet_complete_source_5_0_6.json.gz)
runs all ten unchanged tests successfully. The test source is copied byte for
byte into a disposable package using actual `futures-executor`, not a substitute
executor. Its lock starts from the reference lock; every resolved registry
identity must be a member of that reference. Both dependency resolution and test
execution are offline, with test execution locked. The package uses reference
`futures-executor`/`futures-util` 0.3.32 and `rusqlite` 0.40.1. The older
`scripts/verify_original_facet_diagnostics.py` selects different dependency
versions, so its result is not substituted for this exact-dependency evidence.

Test discovery must equal the ten original names, and execution must report all
ten passing with no ignored tests. Before/after source hashes and the reference
checkout are checked. The owned source/build tree (207,162,427 bytes) is removed;
commands, resolved lock, complete output and source identity remain in the receipt.

```sh
source scripts/toolchains/activate.sh
python3 scripts/replay_sqlite_facet_source_5_0_6.py \
    --record /tmp/sqlite_facet_source_fresh.json.gz
```

## Whole-file responsibilities

`Schema` and the local Runtime adapter connect actual SQLite execution to the
runtime facet SPI. `setup` creates three entity descriptors with forward/reverse
edges and protected fields, initializes schema and seeds rows transactionally.
`options`, `counts` and `verify` retain root/nested facet projections, relation
aggregates, counts, routes and request-local privacy. `SchoolRow` is a raw typed
carrier; `PlatformView` and `SchoolTypeView` retain runtime graph state.
`loaded_setup` adds empty and second-owner controls and installs their decoders.
`loaded_query` combines a filtered TopN relation, two relationship aggregates and
per-owner facet metadata. None of these helpers is replaceable by fabricated
result rows or manually supplied trace frames in the C++ integration counterpart.

| ID | Complete original test | Required composed assertions |
| --- | --- | --- |
| SF01 | `root_facets_keep_original_root_and_field_edges` | Three root rows; include-all zero members; three actual SQL logs; exact operation/request/relation/provider/SQL ancestry; protected inherited intent; unchanged source query; unrelated follow-up isolation. |
| SF02 | `nested_facets_preserve_counts_routes_privacy_and_log_off` | All include-all/logging combinations; nested counts and zero members; five or zero logs; complete nested routes without duplicate ancestors; follow-up isolation. |
| SF03 | `future_facet_bindings_are_safe_before_root_rows_entities_and_stream` | Future facet binding protection at the first physical root statement across raw, typed and stream entry points; declaration does not execute the future facet prematurely. |
| SF04 | `repeated_facet_diagnostics_merge_with_removed_count_sources` | Repeated diagnostic attachment retains removed count-query sources and protects current/future bindings without changing execution intent or leaking secrets. |
| SF05 | `loaded_relation_facets_keep_per_parent_counts_and_empty_typed_lists` | All eight include-all/logging/threshold combinations; three parents; TopN length at most one but full filtered facet membership; both aggregate counts; selected empty typed list; no sidecar columns; exact bounded SQL counts and routes. |
| SF06 | `loaded_relation_future_binding_masks_first_root_sql` | A binding used only by a future loaded-relation facet is protected before the first root SQL log. |
| SF07 | `loaded_relation_compact_sidecars_never_enter_persistent_values` | Ordinary compact entry retains per-parent facet sidecars and counts; JSON and persistent maps omit the sidecar aliases. |
| SF08 | `loaded_relation_facets_survive_slow_typed_hydration_with_root_aggregate` | Explicit root aggregate forces the slow typed path; selected-relation facet counts remain distinct from whole-root counts; twelve physical statements; protected future binding. |
| SF09 | `loaded_to_one_facets_remain_query_metadata_for_shared_and_filtered_targets` | Shared and filtered target relations retain owner-specific facets without contaminating another edge or persistent data. |
| SF10 | `loaded_to_one_null_and_missing_keys_retain_empty_facets_without_orphan_counts` | All eight missing-key/include-all/logging combinations; actual raw SQL omits the local key; nullable extra owner; five root rows; empty relation facets, zero orphan counts and bounded statement work. |

## Existing C++ evidence and remaining integration work

The current `public_facet_logs` corpus has 36 actual SQLite diagnostic cases;
`derived_query_diagnostics` has 576 future-source/count/entry observations.
They provide relevant SF01-SF04/SF06 mechanics, but their different model/input
matrices must be reconciled against each complete assertion above before claiming
those tests are covered. A matching corpus count alone is not such a review.

The 2,304 `hydration_flat_facets` observations cover flat graph hydration across
cardinality, paging, missing IDs, pending work and errors. The 72
`graph_relation_facets` observations cover graph storage, sharing, facets and
snapshot separation, with a separate Context-lifetime self-test. These are
helper-level evidence, not the real SQLite composition required by SF05-SF10.
In particular, they do not establish the combined three-parent TopN/full-membership
SQL counts, the slow typed root-aggregate path or raw SQL missing-FK behavior.

The next implementation/verification work is to supply those complete SQLite
caller counterparts and reconcile the public diagnostic assertions, preserving
all ten original tests. Any differences must be retained and resolved at the
responsible runtime layer. The pending quoted-column aggregate decision is a
separate boundary; it does not authorize changing this source's raw metadata,
expected counts or original tests. No source or capability promotion is made.

The [current native regression receipt](parity/sqlite_facet_review_native_5_0_6.json.gz)
passes all five selected checks (four differential corpora and the graph-lifetime
self-test) after building their four consumers. This verifies the related
existing mechanics only. No runtime or generated code changes, module rerun,
sanitizer run, MIPS or complete integration verdict is implied. Source totals
remain 112 accepted / 10 pending / 2 external exclusions.

## Installed loaded-relation counterparts (SF05-SF08)

`teaql_runtime/tests/sqlite_facet_source_tests.cpp` now executes the actual runtime
and SQLite provider using the original three entity/relationship roles. The
[installed receipt](parity/sqlite_facet_loaded_paths_5_0_6.json.gz) records passing
native and canonical imported executions. The initial harness compile failure
is retained [separately](parity/sqlite_facet_candidate_compile_failure_5_0_6.json.gz);
it was corrected without changing runtime behavior or expected source results.

Eight include-all/logging/threshold combinations preserve all SF05 count,
member-identity, selected-empty-list, sidecar, route, privacy and statement-count
assertions. The log checks include the first root SQL required by SF06. The
ordinary compact path checks SF07's full per-parent counts and excludes facet
sidecars from JSON and persistent maps. The explicit root aggregate exercises
SF08's slow typed path: filtered facet counts remain 2/1/0 while whole-root
counts are 3/1/0, with twelve statements and protected future bindings.

Two adaptations are explicit. Handwritten allocator-aware carriers replace the
Rust row/state structs without supplying relation results. Synthetic fixture
rows are inserted using a raw SQLite transaction after runtime schema creation;
setup finishes before observation and does not claim governed mutation parity.
No result rows, facet counts or trace frames are fabricated by the fixture.

The native CTest and imported CTest each run the complete new program.
Canonical verification covers 1,135 bodies; format/check covers 1,799 handwritten
files. Runtime and generated implementation bodies are unchanged. Captured build
logs are compressed in the receipt and removed; stable preset outputs remain.
No sanitizer, MIPS or complete-suite claim is added for this test-only work.

The previous gap analysis is narrowed by these concrete counterparts: SF01-SF04
still need complete diagnostic assertion reconciliation. SF09-SF10 are covered
by the additional execution below. The complete
file remains pending_review and no capability or phase is promoted.

## Shared, filtered, NULL and missing-key to-one paths (SF09-SF10)

The [extended receipt](parity/sqlite_facet_to_one_paths_5_0_6.json.gz) preserves
passing native and imported executions of the complete expanded program. SF09
checks shared targets, filtered identity-only targets, exact per-owner facet
membership/counts, and exclusion from persistent maps and compact JSON. SF10
covers eight missing-key/include-all/logging combinations using an actual NULL
foreign key and raw SQL that omits the foreign-key column. Empty relations retain
empty or zero-count facets as selected, without borrowing another owner's counts.

Both CTests pass; canonical verification covers 1,135 bodies and formatting
covers 1,799 files. The recorded runtime inputs are unchanged from the SF05-SF08
receipt. Build logs are retained compressed and their disposable originals
removed. Stable preset outputs remain. SF01-SF04 reconciliation, whole-source
acceptance and the active broader goal remain incomplete; no source or
capability is promoted.

## Repeated count diagnostics (SF04)

The [count diagnostic receipt](parity/sqlite_facet_count_diagnostics_5_0_6.json.gz)
records the complete native and imported programs passing after adding the
original SF04 caller. A School count removes its unused executable relation,
then attaches two separate future facet diagnostic sources and an empty source.
The actual provider returns count 4 in one row and emits exactly one SQL log;
all three secrets from the removed relation and future sources remain masked.
The fixture retains the four original School rows; additional loaded-path
Platform/SchoolType seeds cannot affect this School-only count and do not run
as queries. The original source assertions are preserved.

Runtime input hashes remain unchanged. Both CTests, 1,135 canonical bodies and
1,799-file format/check pass. Captured build logs are compressed in the receipt
and removed; stable preset outputs remain. The preceding gap statements are
historical: **SF01-SF03 remain pending**, while SF04-SF10 have installed live
SQLite counterparts. Whole-source and capability statuses are unchanged.

## Future bindings at every root entry (SF03)

The [nine-entry receipt](parity/sqlite_facet_future_entries_5_0_6.json.gz) records
passing native and imported executions of the complete expanded program. The
original nine raw/owned/SmartList/typed/enhanced/aggregate/stream entry points
each return the four original School rows. Exactly nine SQL logs are emitted: no
future facet executes. Every log has four trace frames, no bound parameters and
no future secret in its diagnostic rendering. The original query comment still
contains the secret, and executable child enhancements remain empty.

An allocator-aware raw SchoolRow carrier mirrors the original raw row without
adding graph state or generated-model behavior. Fixture setup retains the same
four Schools; additional loaded-path Platform/SchoolType rows are irrelevant
to this unfiltered School-only read, and no future selection executes. Runtime
input hashes are unchanged from the preceding receipt. Both CTests, 1,135
canonical bodies, 1,799-file format/check, upstream-lock verification and active
module-path verification pass. Build logs are compressed in the receipt and
their disposable originals removed; stable preset outputs remain.

The current remaining gap is **SF01-SF02 root/nested facet diagnostic and route
reconciliation**. SF03-SF10 now have installed live SQLite counterparts. This
source remains pending_review, with no capability or phase promotion.

## Final assertion reconciliation

The candidate now contains a counterpart for every original test. Qualification
requires the final native/imported receipt; this mapping alone is not a passing
verdict. All counts and selected identities are checked against the original
expectations, not against candidate-derived expected results.

| Source test | C++ caller and retained assertions |
| --- | --- |
| SF01 | `root_and_nested_facets(false, true, true)`: three filtered roots, full type choices/counts, three SQL logs, exact operation/request/relation/provider/SQL frames, inherited masked intent, unchanged source comment and independent follow-up isolation. |
| SF02 | Four nested include-all/logging combinations: nested identity/count sets, five or zero statements, exact nested routes and the same follow-up isolation. |
| SF03 | `future_bindings_at_all_entries`: all nine original entry shapes, four rows each, exactly nine logs with four frames/no parameters/no secret, no executable future enhancement and unchanged source comment. |
| SF04 | `repeated_count_diagnostics`: removed executable relation, two distinct future sources plus empty attachment, count 4, one statement and all three secrets masked. |
| SF05 | Eight `per_parent_facets` combinations: original TopN versus full membership counts, matching aggregate, zero members, selected empty typed list, sidecars, batch/probe bounds and ancestry. |
| SF06 | The SF05 include-all/threshold-zero case checks every log, including the first root statement, for the future secret. |
| SF07 | `compact_and_slow_paths`: compact selected-list cardinality, complete facet count/identity set, exclusion from JSON and persistent maps. |
| SF08 | The same caller clears observed logs and executes explicit root aggregates: filtered facet versus whole-root counts, selected relation retention, twelve statements and masked future predicate. |
| SF09 | `shared_and_filtered_to_one`: typed and compact shared-target facets, filtered identity-only target retaining the actual FK, exact counts and exclusion from persistent representations. |
| SF10 | Eight `null_and_missing_to_one` combinations: actual NULL insertion and actual raw SQL FK omission, five roots, exact per-owner full or empty choice sets/counts, at most twelve statements, no logs when disabled. |

The fixture Runtime adapter performs Context service lookup, installs only the
trace context supplied by the runtime, calls the real aggregate SmartList
service, and converts its error to RuntimeError. It never creates result rows,
relation frames or counts. Schema delegates to the registered SQLite provider.
The base fixture now excludes the two loaded-path additions; the loaded fixture
adds them before observation. Allocation-aware row carriers and asynchronous
polling replace Rust ownership/async syntax without changing these assertions.

## Final source verdict

The [final receipt](parity/sqlite_facet_source_acceptance_5_0_6.json.gz) records
both complete C++ executions passing after installation of SF01-SF02. The
original ten-test receipt remains byte-preserved and its source hash matches
the locked checkout. The reconciliation above covers every original test,
helper role and assertion. Runtime input hashes remain unchanged, canonical
verification covers 1,135 bodies, and format/check covers 1,799 handwritten files.

The [initial compile failure](parity/sqlite_facet_roots_compile_failure_5_0_6.json.gz)
is retained: the fixture's async adapter initially used the transport error type
instead of Executor::Error, and a temporary log collection needed a named owner.
Both were corrected before successful execution, without changing runtime
behavior or expected outcomes. Fixture seeding and C++ ownership adaptations
remain explicit above. No sanitizer, MIPS or full-suite run is claimed.

Accept only `teaql-provider-sqlite/tests/facet_trace.rs`. Active source totals
become **113 accepted / 9 pending / 2 excluded_external**. Capability totals stay
**11 qualified / 6 partial**, and P0-P7 remain incomplete. Compressed receipts
retain build logs; their disposable originals are removed. Stable native and
modules preset outputs are retained for reuse.
