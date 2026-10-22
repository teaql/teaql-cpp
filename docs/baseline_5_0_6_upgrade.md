# Rust 5.0.6 baseline upgrade

Current source status: **118 accepted / 4 pending_review / 2 excluded_external**.
The latest [complete graph-service acceptance](service_graph_source_acceptance_5_0_6.md)
qualifies SG01-SG26 with fresh original1+14 observers/9694 outputs and joined
native30/module30.
Capabilities remain **11 qualified / 6 partial**, and P0-P7 remain incomplete.
The earlier full-native877 checkpoint is separate and is not relabeled as the
current source-closure gate. Earlier counts below retain their historical scope.

Current SQLite facet source closure: [complete source review](sqlite_facet_source_review_5_0_6.md)
qualifies all SF01-SF10 responsibilities with original ten tests, complete native
and imported callers, and canonical1,135. Active source totals are **113 accepted
/ 9 pending / 2 excluded_external**; capabilities stay **11 qualified / 6 partial**
and P0-P7 remain incomplete. Source counts in preceding-work checkpoints below
retain their historical scope.

Current G09 status: **qualified** after [resolved-service requalification](resolved_g09_requalification_5_0_6.md): native37/module10, service31/31, compiler1+24, Java79 and strict25-workspace/1650-file reproduction. Active capabilities are **11 qualified / 6 partial**; source totals remain **112 accepted / 10 pending / 2 excluded_external**. Reopening notes below are retained historical checkpoints; whole resolved.rs and P0-P7 remain incomplete.

Current capability override: G09 is reopened after the [joined resolved-service regression](resolved_joined_regression_5_0_6.md); active capabilities are 10 qualified / 7 partial pending requalification. Source totals remain 112 accepted / 10 pending / 2 excluded_external. Earlier checkpoint counts retain their historical scope.

Status: references locked; behavioral migration and qualification are active.
Current G15/G16 acceptance is recorded separately in the capability ledger and
their final audits. Checkpoint notes below preserve their original narrower
scope and pending statements; they are not the current capability summary.
Authorized by the user on 2026-10-11 while generator parity work was active.
This steering supersedes the initial goal's 5.0.5 runtime and earlier generator
reference without replacing its alignment objective or discarding ongoing edits.

Current relation source closure: [final acceptance](relation_source_acceptance_5_0_6.md)
qualifies the complete named relation source with original3 and native26/module26.
Active source totals are **112 accepted / 10 pending / 2 excluded_external**.
Capability and generator phase status remain unchanged.

Historical private memory source closure: [final acceptance](memory_source_acceptance_5_0_6.md)
qualifies the complete test-oracle responsibility with fresh source/candidate
replay. Source totals at that checkpoint were **111 accepted / 11 pending / 2 excluded_external**.
This does not promote unrelated capabilities or complete the generator phases.

## Exact references

- Runtime tag: v5.0.6, commit `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
  The remote annotated tag and local peeled commit agree. The workspace package
  version is 5.0.6. The clean detached reference is .reference/teaql-rs.
- Generator source reference: `45e1572c4956ca963410d8ea436ead3c8857087a`,
  recorded in that release's retained minimal generated fixture PROVENANCE.json.
  Inspect the clean detached .reference/teaql_code_gen worktree. The independent
  cpp_support implementation workspace remains based on `77b301e9`; changing a
  reference does not rewrite its ancestry or discard its C++ implementation.
- Previous runtime source is retained at .reference/teaql_rs_5_0_5. The historical
  lock is docs/parity/baselines/v5_0_5.lock.json. Original user checkouts remain
  untouched. The independent generator's existing uncommitted work is preserved.

## Behavioral delta and migration responsibilities

The release changes 328 repository paths. Fifty-eight changed Rust source files
belong to the five in-scope core/data-service/SQL/runtime/SQLite crates. The
[machine-readable delta](parity/baseline_5_0_6_delta.json) records old/new hashes
and pending dispositions. Counts and unchanged declarations are not acceptance.

| Contract family | Required review and qualification |
| --- | --- |
| Shared loaded state | Fixed property indexes, inline/overflow availability, immutable shared snapshots, sparse versus complete views, zero/false/Null/NotLoaded, view-local copy-on-write and independent mutations. Adapt sharing to explicit C++ Context lifetimes. |
| Typed checker reconstruction | Preserve projection indexes, snapshots, dynamic metadata and sparse-update rejection; Context/time fixes and their evidence must precede governed writes. |
| Request-owned intent and trace | Preserve root intent, canonical nested routes, branch/ledger attribution, late allocated IDs, SQL diagnostics and audit privacy. Invalid intent/provenance must reject before protected I/O according to source. |
| Dynamic fields and properties | Separate durable storage extensions from readonly derived properties; use trusted storage/profile provenance, Context metadata, explicit NULL/delete and transaction-backed SQLite persistence. |
| Hydration, JSON and streams | Shared projection layouts, alias validation, isolated same-identity views, bounded cursor loading, early close/lease cleanup and governed presentation-versus-storage authority. |
| SQL relations, aggregation and transactions | Membership, grouped partition/HAVING rebinding, forward filtered views, nested facets, logical trace paths and transaction lease isolation. |

Use the release's retained generated narrow fixture and shared-load-state/
trace-chain examples as additional fixed reference evidence. They supplement
source review and do not replace native C++ behavior comparisons.

## Evidence transition

Existing source inventories, module acceptance ledgers, committed oracle corpora,
measurement reports and earlier stage/Q/E checks were produced against 5.0.5.
Keep their provenance intact. Do not change their commit labels to manufacture
5.0.6 evidence. Baseline-bound comparison scripts correctly reject historical
corpora against the new active lock until the original calls are replayed and
requalified. Rust oracle dependency lockfiles may require a reviewed upgrade.

The active generator capability ledger is rebuilt from the new frozen producer.
Core source-map/disposition and module-root evidence refresh is part of P0/P7;
new or changed contracts stay pending until qualified. The historical six-stage
acceptance remains historical. Neither changing upstream.lock.json nor generating
fresh C++ headers constitutes completed 5.0.6 migration.

Continue under [cpp_generator_parity_plan.md](cpp_generator_parity_plan.md):
first dependency/source audit, then runtime contracts, then thin generated adapters
and actual application/source comparisons. SQLite-only scope, Context ownership,
camelCase application naming and targeted verification policy remain unchanged.

## Active source inventory and pending map

The separate [5.0.6 source inventory](parity/v5_0_6/upstream_inventory.json)
contains 124 files, 378 source declarations and 693 source tests.
[source_dispositions.json](parity/v5_0_6/source_dispositions.json) maps every
file to its existing or proposed package responsibility, retaining previous
status and unchanged/changed/new hashes separately from pending qualification.
The five complete primary G16 files have accepted active-baseline
dispositions, backed by the [source disposition receipt](parity/g16_primary_source_dispositions_5_0_6.json.gz)
and [full G16 audit](dynamic_storage_acceptance_5_0_6.md). Their previous entries
are preserved. The [document/reference acceptance](document_reference_acceptance_5_0_6.md)
adds three complete protocol files and qualifies SQ08. The subsequent
[request-intent review](request_intent_acceptance_5_0_6.md) and
[five-file core review](core_primitives_acceptance_5_0_6.md) brought the total
to 14 accepted / 110 pending. The later [complete indexed-state source
review](shared_load_state_acceptance_5_0_6.md#whole-load_staters-source-disposition)
accepts `load_state.rs`, bringing that checkpoint to 15 accepted / 109 pending.
The subsequent [complete allocation-test review](load_state_allocations_5_0_6.md)
fixes repeated same-shape merge metadata allocation and accepts the original four-test
file. That checkpoint has 16 accepted / 108 pending. The subsequent
[whole shared-state test review](shared_state_source_tests_5_0_6.md) accepts
`dynamic_field_state.rs` and `dynamic_property_metadata.rs`, repairing immutable
dictionary/selection sharing and large-batch arena lookup. That checkpoint had 18 accepted / 106 pending. The later
[complete typed-batch/source review](generated_load_state_bulk_5_0_6.md) accepts
all nineteen `shared_load_state.rs` responsibilities after the alias repair and
actual 12-batch/40,404-value, cold metadata and lifetime gates. That typed-batch checkpoint had **19 accepted / 105 pending**. Wider quoted-FK differences remain
unaccepted and G15 stays partial.
The core review includes exact-dependency
source replay and a real system-clock range correction; unchanged hashes alone
are not its acceptance proof. Previous entries and source-change
classification are retained in the receipts; accepted
capability subcontracts do not automatically accept an entire overlapping file.
Regenerate and
check the inventory with:

```sh
python3 scripts/inventory_upstream.py --inventory-only --output-dir docs/parity/v5_0_6
python3 scripts/inventory_upstream.py --inventory-only --output-dir docs/parity/v5_0_6 --check
python3 scripts/verify_generator_parity.py
```

The generator verifier checks the release's producer provenance, frozen source
hashes/anchors and capability record consistency. Its successful normal run
is an integrity check. `--require-complete` requires qualified capabilities and
rejects any remaining unqualified capability. It does not infer semantic coverage.

## Current paths and historical move evidence

workspace_paths_5_0_6.json now records active source/structure authority separately
from the immutable historical move and retirement ledgers. The default path
verifier checks seven package/target/module names, current source placement and
underscore spelling, all 124 fixed source/disposition links and 769 retained
historical destinations. It verifies the pinned source and fresh inventory; pending
semantic dispositions remain pending. Fifteen positive/negative controls exercise
baseline/hash/coverage/owner/name/path rejection without changing workspace files.

--historical-checkpoint also archives C++ 0.0.3 at
42e688ee59ea9a7671a0d0ea6a6549fd850e993f and runs that snapshot's original verifier:
769 retained files, 309 unchanged canonical production bodies, one relocation and
seven reviewed retirements pass. Current historical ledger bytes must equal the
committed checkpoint. This supersedes earlier reports that the old default check
rejects the upgraded lock, while preserving the underlying 5.0.5 evidence intact.
No active body digest or parity status is manufactured from current implementation.


## Durable dynamic core checkpoint

Distinct definitions/selection/value/mutation carriers now supplement readonly
properties. 296 original/C++ observations match frozen dynamic_fields.rs, with
native 3/3, targeted sanitizer 2/2 and module import 1/1 passing. The core source
map retains pending qualification and links this limited evidence. Runtime
entity/load-state/ledger integration and SQLite/JSON/stream storage remain open.
See [the core contract](dynamic_fields_5_0_6.md); this is not full G16 acceptance.


The subsequent loaded-state/row checkpoint records 176 direct snapshot/read-row
observations, distinct from the earlier 296 value-carrier cases. Native regression
passes 10/10 and module import 1/1. Read metadata now carries durable values and
readonly schemas separately from persistence columns; typed runtime entity and
storage integration remain pending. Historical 5.0.5 observations are unchanged.


Runtime dynamic state/ledger checkpoint records 147 unchanged public original/C++
observations and native 6/6, targeted sanitizer 4/4 and module import 1/1 gates.
Definitions/originals remain separate from native changes; composition and view
copying preserve their source responsibilities. Dynamic privacy joins governed
collection. Pending extensions reject before reads/writes until bound storage
exists, with a typed DynamicField runtime error. G16/G17 full qualification remains
open; see [the ledger contract](dynamic_ledger_5_0_6.md).


The data-service storage SPI checkpoint records 49 unchanged pure plan/write Debug
observations. Native 5/5, targeted sanitizer 4/4 and module import 1/1 pass. Executor
fixture query entry now supplies explicit comment/purpose while preserving its
existing delegation and transaction assertions. Source evidence is not relabeled
from 5.0.5. Actual SQLite/provider/cursor/transaction execution remains pending;
see [the SPI contract](dynamic_storage_spi_5_0_6.md).


Concrete SQLite storage checkpoint compares 123 real original/C++ store calls;
native 6/6, targeted sanitizer 5/5 and module import 1/1 pass. Transaction ownership
excludes ordinary work until terminal completion; failed consumed commit rolls
back immediately as source Drop does. Selected 5.0.5 fixtures gain mandatory
request intent and their changed isolation/cleanup assertions are requalified
natively without relabeling old corpora. See [the store contract](sqlite_dynamic_storage_5_0_6.md).
Runtime provider/graph/audit/readback and cursor enhancement remain open.


Context dynamic provider checkpoint adds the database/immutable fixture provider
SPI and UserContext resources, trusted stream/write preparation, consuming batch
conversion and source-shaped private audit values. The 72 direct frozen-source
observations use real SQLite storage and a registered SQL executor. Scope and
reproduction are recorded in
[dynamic_provider_5_0_6.md](dynamic_provider_5_0_6.md).
Actual query/graph hydration, transaction write/readback/audit integration and
thin generated adapters remain open; the ledger pre-I/O guard stays in place.
G16 and full 5.0.6 acceptance remain unqualified.


Dynamic query checkpoint adds scoped query selection, typed root/enhanced loading,
metadata-preserving decoding and real SQLite enhancement inside the cursor lease.
The 250 direct original/C++ cases match values, load bits, sharing and failure
contracts; see [dynamic_query_hydration_5_0_6.md](dynamic_query_hydration_5_0_6.md).
The handwritten carrier fixture does not establish generated School parity.
Relation/child fetching and embedded traversal, graph write/readback/audit,
complete hooks/trace/config/diagnostics and thin generated adapters remain open.
The ledger pre-I/O guard and all pending qualification labels are retained.


Dynamic relation checkpoint adds Context-driven relation/child loading, checked
child overlays, whole-batch embedded traversal and fetch-time attachment keys.
The separate 138-case original/C++ corpus preserves errors and original metadata
retention behavior; see [dynamic_relation_hydration_5_0_6.md](dynamic_relation_hydration_5_0_6.md).
Typed compact-flat graph hydration has native evidence. Source direct embedded
dynamic-only map retention, facets, complete typed/generated configurations,
graph write/readback/audit and full hook/trace/config/diagnostics remain open.
Historical 5.0.5 records are unchanged; G16 and complete 5.0.6 parity stay pending.


Transaction-local commit audit checkpoint implements operation-owned queues,
poll-only owner-thread activation, post-commit delivery and first-error wrapping.
Automatic ledger and manual scope tests preserve database commit status when a
sink fails; rollback, failed commit and cancellation discard facts. Native 9/9,
targeted sanitizer 6/6 (leak scanning disabled), module import 1/1, original private
unit tests 5/5 and 20 direct public error observations pass. See
[commit_audit_5_0_6.md](commit_audit_5_0_6.md) for scope and reproduction.
Dynamic native-fact enrichment is present, but the complete dynamic graph write/
readback path and generated adapters remain open. The ledger guard stays in place;
G16/G17 and all unqualified capabilities keep their pending status.


Dynamic ledger persistence checkpoint replaces the temporary blanket rejection
with transaction-backed preflight, native optimistic updates, extension apply/
authoritative readback and queued native-fact enrichment. Automatic and manual
save tests cover failure/rollback, NULL/delete, trigger values, assigned IDs,
foreign-key rebinding and explicit Global retention after UserContext deletion.
Fourteen direct public graph protocol observations match; the original SQLite
dynamic integration suite passes 23/23 separately. See
[dynamic_graph_save_5_0_6.md](dynamic_graph_save_5_0_6.md) for the exact evidence
and ownership adaptations. Complete non-root Checker/Fix/governance and all
hook/trace/privacy/configuration/generated-carrier matrices remain open.
G16/G17 and phases P0-P7 remain unqualified.

Final dynamic ledger gates: native 7/7, targeted sanitizer 7/7 (leak scanning
disabled), module import 1/1, formatting 798 files, 1,404 exports and seven
facade archives pass. All 17 generator capabilities remain unqualified.


Pending dynamic preflight checkpoint discovers native/dynamic/new/deleted related
keys using original values and normalized identities. Unrepresented dynamic keys
run the shared Checker/Fix pass before policy review; Fix output rejoins the
original ledger. Policy plans include extension-only operations and #code intent
values. Twenty-four direct public original/C++ denying-save observations match;
the separate original entity_save library selection passes 17/17. See
[pending_dynamic_preflight_5_0_6.md](pending_dynamic_preflight_5_0_6.md).
Generated Checker/Fix, readonly authority and complete hook/trace/privacy/config
matrices remain pending; no capability or complete phase is newly qualified.

Final pending-dynamic gates pass: native 7/7, targeted sanitizer 7/7 (leak
scanning disabled), module import 1/1, 800 formatted files, 1,404 exports and
seven facade archives. All 17 generator capabilities remain unqualified.


Generated dynamic-carrier checkpoint adds the shared GeneratedDynamicFields
helper, thin indexed Model install/mutation adapters, camelCase application
methods and ref-qualified Q selectDynamicFields. All five fixtures regenerate
and reproduce (55 files), selected Java tests pass 11/11, and 80 direct frozen
entity-macro observations match. Native 5/5, targeted sanitizer 5/5 (leak scanning
disabled), modules 3/3, formatting 804 files, 1,406 exports and seven facade
archives pass. See [generated_dynamic_fields_5_0_6.md](generated_dynamic_fields_5_0_6.md).
The original generator checkout remains untouched. Full readonly, generated
configuration and remaining G01-G17/P0-P7 acceptance stay open.

The [complete evaluation-source review](eval_source_acceptance_5_0_6.md)
accepts all EV01-EV08 responsibilities in `eval.rs`, bringing the current source
dispositions to **20 accepted / 104 pending**. A reproduced Indexed Debug gap
affects all 18 new scenarios; the six-line repair restores the full snapshot
formatter and checked expired-borrow access. Native 6/module 6/SAN 4 pass, with
1,136 retained and 18 complete actual-source scenarios/180 mutations per profile
and all three original eval tests. The initial fixture compile and native/module
5/6 failures remain separate from the successful retries. All other eval header
bytes, producer and all 78 SDKs remain unchanged. Native registry is 730 and
canonical imported bodies 860; neither count establishes complete semantic
acceptance. G15 quoted-FK differences, other source responsibilities and full
current SQ05/G13/P7 and P0-P7 remain open; no historical full run is relabeled.

The [complete core Entity review](entity_source_acceptance_5_0_6.md) accepts
EN01-EN12 for `entity.rs`, bringing dispositions to **21 accepted / 103 pending**.
Its 230 new actual-source cases reproduce 111 differences; independent metadata
defaults, typed MutationIntent auditing and distinct boxed JSON dispatch repair
those paths. All 2,344 actual-source outcomes match in native 16/module 16/SAN 8.
The original non-Clone program, pure intent-header body preservation, explicit
Global retention and actual User deletion also pass. Initial compile/fixture/
include-cycle/combined-lookup failures remain preserved. Generator source and all
78 SDKs are unchanged. New core erased-state defaults do not qualify nonempty
generated adapters or every runtime caller; those remain explicit G13 work.
Native registry is 732; complete current SQ05/P7 and the whole P0-P7 objective
remain open, without relabeling the historical 718/721 full-suite result.

The [borrowed runtime-state dispatch checkpoint](runtime_state_bridge_5_0_6.md)
repairs the ordinary automatic dynamic-save carrier boundary before generation.
Explicit empty/wrong core hooks are authoritative, a valid non-ledger borrow is
accepted, and absent state fails before a hook can bypass the guard by clearing
its pending flag. Native7/module7/SAN4 and24 full actual-source plus944 retained
automatic-save observations per profile pass. Actual User deletion, checked
lifetime rejection and explicit Global retention preserve the ownership model;
independent typed-ledger Save and default-only C++ compatibility remain distinct.
Disk-full compiler failures and reversible oracle relocation are retained.
Generator/SDKs are unchanged at this runtime checkpoint. Nonempty generated
adapters, complete entity_save.rs and G13/G17/P0-P7 remain open; source dispositions
stay21 accepted/103 pending. Registry734/consumer874/export1506 are structural
counts, not a full-suite or complete semantic qualification claim.

The [generated borrowed-root checkpoint](generated_runtime_state_bridge_5_0_6.md)
emits the missing macro-derived core hook over the existing Model root after the
shared runtime gate. Six complete actual original generated-model cases match;
native9/module6/SAN4 and81 Java tests pass. All78 SDKs regenerate and strictly
reproduce, with only64 exact support-header insertions and2201 protected artifacts
unchanged. The previous template/test bodies and current runtime mechanics are
preserved. The full compiler-contract process is still running: this checkpoint
is not complete adapter, G13/G17 or P0-P7 acceptance. Source dispositions remain
21 accepted/103 pending; registry736/consumer875/export1506 are structural counts.

The [standing external telemetry scope review](external_telemetry_scope_5_0_6.md)
revalidates the two previously excluded concrete OpenTelemetry/OTLP files using
complete old/current source equality and explicit feature/exporter boundaries.
Current source dispositions are **21 accepted / 101 pending / 2 excluded_external**.
The two exclusions are not accepted implementations or executed external tests.
Core telemetry, privacy, propagation, lifecycle and all other pending source and
P0-P7 requirements remain in scope. No C++/SDK/compiler input changes or runtime
tests accompany this documentation-only review. The generated adapter's existing
compiler-contract run continues with its original frozen inputs.

The [complete trimming-helper source review](serde_source_acceptance_5_0_6.md)
accepts SU01-SU06 for all118 lines of serde_utils.rs. Native3/module1 replay all675
complete retained presentation observations, including145 trimming cases, and
all four actual original unit tests pass with exact reference dependencies.
Required/optional/NULL/Some(empty), Unicode boundaries, immutable input and
explicit destination ownership retain the existing C++ JSON adapter contract.
No C++ body, SDK, dependency or compiler input changes. Other presentation
sources and complete G13/P7 remain open; source status is now **22 accepted /
100 pending / 2 excluded_external**. The full generated compiler gate is still
running with its original inputs. No new sanitizer, MIPS or full-suite result.

The [complete presentation data review](presentation_sources_acceptance_5_0_6.md)
accepts WP01-WP10 for all508 lines of web.rs and214 lines of xls.rs. Native3/module1
and785 fresh original protocol observations pass;675 existing complete API
observations are reused after checking current input/executable hashes. All
seven models, constants/factories, binding override distinctions, non-Clone
owning iterators, wire/default/NULL semantics, navigation and explicit Global
retention after actual User destruction remain. Existing JSON/naming/checked
arithmetic adaptations are explicit; no Web server or spreadsheet engine is
added. Current source status is **24 accepted / 98 pending / 2 excluded_external**.
No C++/SDK/compiler input change, new sanitizer/MIPS/full-suite result or complete
G13/P7 acceptance. The original generated compiler-contract run remains active.

The [generated borrowed-root adapter](generated_runtime_state_bridge_5_0_6.md)
is now qualified in its bounded scope. The original compiler process terminated
successfully with **55 positive / 141 negative** contracts, including all three
new lvalue/temporary/moved-expression controls. This completes the earlier
pending gate over native9/module6/SAN4, six complete original generated-model
observations,81 Java tests and78-SDK restricted/strict reproduction. Runtime
mechanics were qualified before emission; all existing bodies/typed Save paths
and2201 protected artifacts remain. Historical compiler-pending checkpoints keep
their original scope. Source status stays **24 accepted / 98 pending /
2 excluded_external**; this accepts no additional source file. Complete
entity_save.rs, broader model/caller integration, G13/G17 and P0-P7 remain open.

## Complete core descriptor metadata source acceptance

The [metadata acceptance](core_metadata_source_review_5_0_6.md) qualifies all
MD01-MD12 responsibilities in `teaql-core/src/meta.rs`, including its three
unchanged original tests and 424 actual-source outcomes per frontend. Installed
native 5/imported 5 and complete canonical module-body/source/SDK preservation
pass. Source dispositions advance to **40 accepted / 82 pending / 2 external
exclusions**, while all seventeen capability statuses remain unchanged. Native
discovery is 742 after the native64 boundary comparison; no new whole-suite,
sanitizer/MIPS/device or complete P0-P7 acceptance is claimed.

## Complete public data-service and original request source acceptance

The [data-service source review](data_service_source_review_5_0_6.md) accepts
three complete source files under DS01-DS18/RV01-RV07. Its permanent request
vector/privacy contracts preserve their earlier bodies and authentic fixture.
Ten actual original tests, 1,308 exact source/native/imported outcomes per
frontend, native 12/imported 12 and 930 canonical bodies pass. The reference
registry guard rejects the historical divergent lock; an independent exact-lock
observer supplies the final source evidence. All 78 SDKs and existing production
bodies remain unchanged. Source status is 43 accepted/79 pending/2 external;
capability statuses remain eleven qualified/six partial. Current discovery 744
is not a full-suite pass. Complete P0-P7 and the protected runtime/provider
responsibilities retain their original scope and outstanding decisions.

## Complete core Expr source qualification

The [EX01-EX12 review](core_expression_source_review_5_0_6.md) accepts all 420
lines and fifty methods in the current core Expr source, including 5.0.6's
separate typed-LIKE pattern/original leaf. Actual original calls retain twelve
variants, twelve operators, fourteen functions, ordered NaN-aware composition,
the strict twenty-value threshold and all-operator subquery preparation. All
69 original core unit tests and 1,403 actual-source outcomes per frontend pass.
Native 6/imported 6, four new complete canonical consumers, exact reference
registry identities, stable builds and 1,682-file formatting pass. All 934
canonical bodies, existing C++ bodies, six native archives and 78 SDKs retain
their required identities.

Source status is now 44 accepted/78 pending/2 external and native discovery 746.
All capability statuses remain unchanged. This accepts one complete source, not
SafeExpression, complete SelectQuery, protected SQL/runtime callers or whole
G13/G14/G17/P7. Historical 735/736 and 718/721 failed full epochs and unanswered
source-behavior decisions remain. No runtime/generator implementation or SDK
body changes, new Java/generation, full-suite, sanitizer/MIPS or device result
is claimed.

## Complete SafeExpression source qualification

The [SE01-SE16 review](safe_expression_source_review_5_0_6.md) accepts all 308
lines, twenty-four methods, six built-in TeaqlEmpty implementations, public
custom predicates and the four unchanged original tests. All 580 actual
original/native/imported outcomes per frontend agree. One positive original
compiler example executes; eight negative examples preserve exact Clone/type
bounds, including each list adapter independently. Native 5/imported 5 and all
938 canonical bodies pass, with stable builds and pinned formatting of 1,683
files. Existing production/test/observer/helper bodies, six native archives and
all 78 SDKs retain their hashes.

The initial 132 mismatches were an observer JSON-macro token defect, preserved
separately; only the observer boolean identifier changed. No runtime or generator
algorithm was changed. Source status is 45 accepted/77 pending/2 external and
discovery is 747. All seventeen capability statuses remain unchanged. Historical
735/736 and 718/721 full epochs remain failed; this is not complete current
integration. Whole generated E, protected runtime/query/save callers,
G13/G14/G17/P7 and unanswered source-behavior decisions remain open. No new
Java/generation, full-suite, sanitizer/MIPS or physical-device result is inferred.

## Complete core mutation model source qualification

The [MU01-MU14 review](core_mutation_source_review_5_0_6.md) accepts the complete
291-line source: three separate map roles, six command types, MutationKind, all
fourteen methods and derived/map protocols. Actual original/native/imported
outcomes agree for all 2,048 inputs per frontend, including 240 new real-NUL,
NaN, builder and independent-batch boundaries. One compiler positive executes;
fourteen exact negatives preserve role/iteration/Default boundaries. All 69
original core unit tests and native 5/imported 5 pass. Native assertions now
cover conversion and assignment in all six role directions; production and
existing function bodies remain unchanged.

All 942 canonical bodies and six native archives/78 SDK identities verify.
Source status is 46 accepted/76 pending/2 external and native discovery 748.
All capability statuses remain unchanged. Disk pressure after testing was
resolved by removing only three task-owned incremental caches; qualified
inputs and binaries were preserved. Historical 735/736 and 718/721 full epochs
remain failed. Whole mutation/provider/governance/protected execution,
G13/G14/G17/P7 and unanswered source-behavior decisions retain their complete
requirements. No new generator/runtime algorithm, Java/generation, full-suite,
sanitizer/MIPS or device result is claimed.

## Complete SQL binding provenance source qualification

The [SB01-SB14/BT01-BT11 review](sql_bindings_source_review_5_0_6.md) accepts
the complete 137-line binding source and 342-line original test file. All eleven
original tests and 1,209 exact source/native/imported observations per frontend
pass. The full semantic port now includes typed-LIKE original privacy. Native
6/imported 6, focused ASan/UBSan 1 with leak scanning disabled, immutable corpus
replay, 944 canonical bodies and 1,684-file formatting pass.

The new matrix reproduced 120 scenarios/720 pretty tuple differences while all
semantic policy/value/secret/rebind outputs agreed. The production repair uses
the existing structural formatter for the original-operand pair only. New
nonempty-original retention and allocation-failure contracts preserve explicit
Global/User ownership. Other production/test function bodies, six native
archives and all 78 SDKs remain unchanged.

Source status is 48 accepted/74 pending/2 external and native discovery 749;
all capability statuses remain unchanged. Historical 735/736 and 718/721 full
epochs remain failed. Complete SQL/executor/provider/protected runtime callers,
G13/G14/G17/P7 and unanswered behavior decisions retain their original scope.
No Java/generation, full-suite, MIPS or physical-device result is inferred.

## Complete SQL type and SQLite renderer source qualification

The [ST01-ST15/SR01-SR06 review](sql_types_source_review_5_0_6.md) accepts
the complete 332-line SQL type source and 118-line rendering test responsibility
within the standing SQLite profile. All six original tests pass; native coverage
retains all SQLite branches in five tests and the explicit PostgreSQL-only/other
backend exclusions. All 2,210 original/native/imported observations per frontend
agree, including 632 new NUL/callback/finiteness/error/model/width boundaries.
Native 6/imported 6, exact corpus replay and 946 canonical bodies pass with stable
builds. C++ production/test/observer bytes, six native archives and all 78 SDKs
remain unchanged. No handwritten C++ or runtime/generator algorithm changed.

Source status is 50 accepted/72 pending/2 external and native discovery 751.
All capability statuses stay unchanged. New maximum-usize comparisons are
native64-only. Historical 735/736 and 718/721 full epochs remain failed; current
whole integration, full SQL/provider/protected callers, G13/G14/G17/P7 and
unanswered behavior decisions retain their full scope. No new Java/generation,
full-suite, sanitizer/MIPS or physical-device result is inferred.

## Query intent storage and runtime lineage

The [query source continuation](core_query_source_review_5_0_6.md) qualifies the
48-case purpose repair, native 28/imported 26 and 2,602 per-frontend source inputs
with their explicit cache/lifetime boundaries. The active query has 31 fields;
the historical 26-field review remains historical. Its missing schema builder
and complete source acceptance remain open. Sources stay 50/72/2 and all
capability statuses remain unchanged; native discovery advances to 753.

## Readonly query schema builder

The [qualified schema builder](query_schema_builder_5_0_6.md) restores the missing
source entry and same-Context immutable sharing. Original core 69, source 480
per frontend, native 15/imported 15 and selected sanitizer 3 pass. Its closing
regression and repair preserve precharged diagnostics without new public IDs.
Source status stays 50/72/2; whole query/source and baseline qualification remain
open. Native discovery is 755, without relabeling historical full-suite results.

## Complete query state and row error diagnostics

The [complete-state continuation](query_full_state_5_0_6.md) verifies every current
SelectQuery field, full Clone/equality/Debug and actual recursive list preparation.
All 124 destination copies survive actual source UserContext deletion. A separate
300-input original panic witness exposed 270 row-extension error text differences;
the repaired boundary and 48 direct DynamicFieldError diagnostic observations now
match exactly. Original/native/imported each replay all 472 inputs. Native 21 and
imported 21 pass after terminal builds and finish with no work. Source status
remains 50/72/2, capabilities remain unchanged and native discovery is 759.
Whole query/source and baseline acceptance remain open; no new sanitizer, MIPS,
Java/generation or physical-device result is inferred.

## Read-result sidecar presence

The [carrier-presence qualification](row_read_sidecar_5_0_6.md) reproduces and
repairs 24 absent-versus-empty merge differences among 972 exact source inputs.
The fix uses the existing Context-owned carrier allocator; native/imported each
pass 22 selected checks and finish with no work. All 472 query/error inputs
reproduce again with original core 69/69. Complete rows survive actual source
UserContext destruction after explicit Global copies. Native discovery is 760;
source status remains 50/72/2 and all capability statuses remain unchanged.
This closes the named sidecar-presence responsibility without promoting the
whole query source, active baseline or historical failed full-suite epochs.

## Complete query and row model source

The [CQ01-CQ17 review](core_query_acceptance_5_0_6.md) accepts the complete current
`query.rs` responsibility group. The final gaps were direct Record Debug and the
always-checked row constructor diagnostic. Current native 25/imported 25 pass;
original core 69/shared 19 and a fresh 2,602-input source replay join 1,405 new
exact inputs and 1,924 explicitly reused original observations. Source status is
51 accepted / 71 pending / 2 external. All SDK bytes and existing query/row
algorithms remain preserved. Native discovery is 762, not full current baseline
qualification; historical full failures and all open capability exits remain.

## Complete memory engine and evaluator source

The [IM01-IM14 review](inmemory_source_acceptance_5_0_6.md) accepts all production
methods/helpers and nineteen original tests. A reference-locked source observer
replays 6,532 unchanged and 2,200 new outcomes through native and imported
frontends. Current 4/4 selections pass in each frontend, including the complete
original test body newly added to canonical imports. Source status is 52/70/2;
native discovery is 763. Production, existing contracts and all SDK bytes remain
unchanged. This is native64 source qualification with explicit clock/sort
dispositions, not a new full baseline, MIPS or physical-device result.

The subsequent [complete generated-support review](generated_support_source_review_5_0_6.md)
qualifies GS01-GS19 and only that whole source disposition. New 822 exact
private-state observations per original/native/imported frontend, both original
compiler/observer tests, native3/imported4 and unchanged reuse of 2,196 prior
observations and nineteen original SQLite tests support the decision. Production
function/header and SDK bytes stay unchanged; three existing helpers gain module
aliases, catalog and lookup. Source status is 53/69/2, capability status remains
11 qualified / 6 partial, and native/imported discovery is 765/521. Historical
failed whole-suite epochs, complete capability/phase exits and physical-device
limitations retain their separate outcomes.

## Complete UserContext source checkpoint

The [UC01-UC25 review](context_source_review_5_0_6.md) accepts only the complete
1,631-line context.rs and its six embedded tests. Current native17/imported17
core checks and native23/imported23 delegation checks pass, with 3,362 exact
core outcomes and 1,088 freshly compiled original audit-dispatch outcomes per
frontend. The original runtime view preserves all 67 source/test files and its
273 original test names. A source-copy timestamp repair prevents Cargo from
reusing an appended observer from another disposable view.

Only this source disposition advances: **54 accepted / 68 pending /
2 excluded_external**. Existing schema, logging, language, telemetry,
commit-audit and document/reference evidence is reused at its checked unchanged
body and original scope. Ten canonical imports reuse existing test/probe bodies;
no production C++ function/header, generator or SDK changes. Full baseline,
capability, child-source, protected-publication and P0-P7 integration remain
active. The historical failed native epochs retain their outcomes.

## Complete registry and module assembly source checkpoint

The [RG01-RG24 review](registry_source_review_5_0_6.md) accepts the complete
837-line registry.rs. All 27,257 exact actual-source/native/imported observations
per frontend agree, including 1,296 new SmartList metadata/failure/replacement
cases. Current native11/imported11 pass with final no-work builds. Five canonical
module entries close previously missing decoder/graph execution; all existing
contract/probe bodies and production functions/headers remain unchanged.

The byte-checked complete original view retains 67 source/test files and its
273-test registry, with only the existing JSON access bridges appended. There
are no embedded registry tests. Existing hydration/state/Context evidence is
reused only for its explicit scope and checked unchanged bodies; changed
historical dependencies are recorded rather than silently relabeled.

Only this source disposition advances: **55 accepted / 67 pending /
2 excluded_external**. All seventeen capability verdicts stay unchanged. Current
source qualification does not accept the remaining underlying/caller files,
behavior decisions or full P0-P7 integration. The historical failed whole-suite
epochs retain their outcomes; SDKs and protected artifacts remain byte-identical.

## Complete Checker source and constructor boundary checkpoint

The [CK01-CK22 review](checker_source_review_5_0_6.md) accepts the complete
745-line checker.rs and all five original tests. A Rust-positive/C++-negative
compiler witness reproduces early generic constraints. The runtime now permits
metadata carriers while requiring Entity, allocator-aware copy and matching
typed logic before Checker erasure/registration. Its valid materialization,
Fix, required findings and sparse merge body is byte-identical.

All 2,716 exact observations per frontend, native11/imported11 and four paired
negative compiler programs pass. The paired positive program exercises five
construction combinations. Targeted native5/imported5/sanitizer5 ownership and
dispatch checks pass, with leak scanning disabled; the native/imported five
checks overlap their eleven-check selections. Existing generated compiler
1-positive4-negative and native flow1,620/governed SQLite Save540 regressions
also pass. These latter comparisons reuse their retained source corpora.

Only checker.rs advances: **56 accepted / 66 pending / 2 excluded_external**.
All capability verdicts stay unchanged. Four canonical imports reuse existing
model/original bodies and add the new constructor contract. Generator, SDK and
protected reference artifacts remain unchanged. Earlier constructor/source-fixture
failures and failed full native epochs retain their outcomes. No new whole-suite,
MIPS, external-provider, GPU or physical-device result follows from these gates.


## Complete entity-runtime source checkpoint

The [ER01-ER32 review](entity_runtime_source_review_5_0_6.md) accepts the complete
entity_runtime.rs source with explicit Context/owner-thread/exception adaptations.
All 21 unchanged original tests and 4,226 exact original/native/imported outcomes
pass, including 770 new equality/dynamic-diagnostic observations. Value-first
NaN equality and delayed LedgerEntity receiver bounds repair actual source
mismatches; the two default method bodies remain byte-identical.

Native17/imported17, paired compiler1-positive3-negative and existing generated
native2/imported1 Context-retention/SQLite Save/Q/E callers pass. Final selected
builds report no work. The initial metadata collector and duplicate module-test
configuration failures remain preserved, together with historical full-suite
failures. Only entity_runtime.rs advances, to **57 accepted / 65 pending /
2 excluded_external**. All seventeen capability verdicts and complete P0-P7 exits
remain unchanged. All 78 existing SDKs and 5,741 protected artifacts retain their
starting bytes. No new full-suite, sanitizer, MIPS, physical-device or generation
reproduction result is claimed.


## Complete SQL terminal-diagnostic checkpoint

The [DX01-DX16 review](sql_diagnostics_source_review_5_0_6.md) accepts complete
diagnostic_execution.rs and diagnostic_stream.rs after exact-reference replay.
The historical SQL protocol workspace has 44 registry identity differences;
an independent source view preserves all 12 original files, runs the seven
original stream tests and reproduces 7,035 outcomes per native/imported frontend.
All retained query/mutation/transaction/stream/kind outputs stay unchanged.

Native11/imported11 and selected ASan/UBSan2 pass, including 352 new guard/journal
cases with actual User deletion, normal/unwinding close, late recorder lifetime
and throwing sinks. Leak scanning is disabled. Production functions/headers,
78 SDKs / 3,457 files, 5,741 protected artifacts and the old observer remain
unchanged; final selected builds report no work. Source dispositions advance only
the two diagnostic files, to **59 accepted / 63 pending / 2 excluded_external**.
All capability verdicts and full P0-P7 requirements remain unchanged. Executor,
mutation, runtime/governance/privacy and complete current integration stay open;
no historical failed epoch is relabeled as passing.


## Complete SQL execution diagnostic test-source checkpoint

The [EQ01-EQ06/EM01-EM21 review](sql_execution_tests_source_review_5_0_6.md)
qualifies both complete source test files. Rust 5.0.6 has twenty-one mutation
tests, six more than the historical port. New coverage retains repeated-root
identity, whole-batch old-value privacy, successful transaction/batch/guarded
readbacks and the absence of invented readbacks for zero-match/physical-delete
writes. The target-ID fixture now puts its canary in owned intent as well as trace.

All 27 unchanged original tests and native2/imported2/selected ASan-UBSan2 pass,
with final no-work rebuilds and leak scanning disabled. Production C++ sources
and headers, all 78 SDKs / 3,457 files and 6,091 protected artifacts are unchanged.
Canonical module coverage is 1,002 bodies / 1,516 exports. Native discovery stays
775 and the module preset discovers 561 imported tests; neither is a full run.
Pinned format/check passes 1,704 files. The failed nonexistent-verifier invocation
is preserved with its successful corrected follow-up.

Only execution_tests.rs and mutation_execution_tests.rs advance, to **61 accepted /
61 pending / 2 excluded_external**. All seventeen capability verdicts and complete
P0-P7 requirements remain unchanged. Whole executor/mutation/provider/governance/
privacy qualification and complete current integration remain open. Historical
735/736 and 718/721 full-suite epochs remain failed. No generator, Java, full-suite,
MIPS or physical-device acceptance is newly claimed.


## Complete SQL mutation orchestration checkpoint

The [ME01-ME20 review](sql_mutation_execution_source_review_5_0_6.md) accepts
the complete 523-line mutation_execution.rs source. New original-source inputs
reproduce 276 guarded-sidecar and 16 descriptor-cache differences. The runtime
now preserves the guarded entry's absent inherited sidecars and uses the same
executor descriptor cache for ordinary batch classification and compilation.

All 6,191 exact original/native/imported/sanitizer outcomes, 27 unchanged original
tests and native9/imported9/selected ASan-UBSan9 pass. Actual generated SQLite
Q/E and ledger composition pass native2/imported1; selected final builds report
no work. Leak scanning is disabled. New fixture/import, corpus-baseline metadata
and JUnit-collector failures remain preserved with explicit successful follow-ups;
no original expected outcome was edited.

The complete production diff covers two files; the other 6,089 protected artifacts
and all 78 SDKs / 3,457 files retain their bytes. Module verification covers 1,003
canonical bodies and 1,516 exports. Native discovery is 777 and the module preset
discovers 563 imported tests; these are inventories, not full runs. Pinned format/
check passes 1,705 files. Only this source file advances, to **62 accepted /
60 pending / 2 excluded_external**. All capability statuses and full P0-P7 exits
remain unchanged. Whole executor/provider/runtime/governance/privacy, complete
current integration and standing behavior decisions remain required. Historical
735/736 and 718/721 full-suite epochs stay failed. No new Java/generation, MIPS
or physical-device acceptance is claimed.


## Complete SQL executor source checkpoint

The [SX01-SX27/XT01-XT16 review](sql_executor_source_review_5_0_6.md) qualifies
complete executor.rs: independent transport, conditional stream/transaction
interfaces, typed errors, descriptor/shape/FIFO caches, binding recollection,
partition probes, mutation forwarding and native/buffered/dynamic stream timing.
Four missing native ports complete all sixteen embedded source tests. A 720-case
dynamic stream fixture reproduces 24 actual metadata/optional-observer ordering
differences; one shared header now captures on the flag and wraps only when
metadata and observer both exist. Initial fixture errors/output correction remain
preserved without changing any original expected outcome.

All 8,243 exact original/native/imported outcomes, 50 original tests and native19/
imported19 pass. Targeted ASan/UBSan3 covers 720 dynamic outcomes and two existing
stream/move-only lifetime checks; leak scanning is disabled. Actual SQLite dynamic
stream and generated Q/E checks pass native2/imported1, with final no-work builds.
Pinned format/check passes 1,706 files. The other 6,090 protected artifacts and
all 78 SDKs / 3,457 files are unchanged; the production diff is one header.

Module verification covers 1,007 canonical bodies and 1,516 exports. Native
discovery is 778 and the module preset discovers 567 imported tests; these are
inventories, not complete runs. Only executor.rs advances, to **63 accepted /
59 pending / 2 excluded_external**. All seventeen capability statuses and full
P0-P7 exits remain unchanged. Pending provider/runtime/governance/privacy callers,
complete current integration and standing behavior decisions remain required.
Historical 735/736 and 718/721 epochs stay failed. No new Java/generation, MIPS
or physical-device result is claimed.


## Complete SQL dialect/root source checkpoint

The [DC01-DC31/root review](sql_compiler_source_review_5_0_6.md) accepts complete
dialect.rs and lib.rs: seven helpers, SchemaIndexSpec, all 43 trait methods,
private validation/identifier bodies, 22 direct aliases and all 33 original
root/dialect test dispositions. The historical root port lacked the new grouped
partition/HAVING regression; its complete literal SQL and binding assertion is
now retained. Three non-SQLite renderer-only tests remain explicitly excluded.

A new standalone observer uses 129 exact reference registry identities; 44 old
observer differences remain historical. All 6,728 source/native/imported outcomes,
33 unchanged original tests and native13/imported13 checks pass, including actual
grouped SQLite execution. Final selected builds report no work. Production and
6,091 protected artifacts, including all 78 SDKs / 3,457 files, remain unchanged.
Pinned format/check passes 1,706 files. Module checks cover 1,009 canonical bodies
and 1,516 exports; native discovery stays 778 and the module preset discovers
569 imported tests. Inventories are not full integration acceptance.

Only the two compiler/root source files advance, to **65 accepted / 57 pending /
2 excluded_external**. All seventeen capability verdicts and complete P0-P7 exits
remain unchanged. Provider/runtime/caller qualification, current full integration
and standing behavior decisions retain their full requirements. Historical
735/736 and 718/721 full-suite epochs remain failed. No new sanitizer/MIPS/device
or Java/generation-reproduction result is claimed.


## Shared SQLite lease/binding checkpoint

The [provider review](sqlite_provider_lease_binding_review_5_0_6.md) reproduces
12/15 actual imported-provider/original differences before repair. Root async
execute/fetch/stream now waits for the lease before converting bindings, while
synchronous and repeated-query native-lock/prepare precedence stays distinct.
A separate native-guard marker preserves poisoning only inside guarded work.

All 87 new exact source outcomes plus retained 144 agree per frontend; native 9/9,
imported 4/4 plus its separate lease comparator, and selected ASan/UBSan 4/4 pass.
Leak scanning is disabled. The unchanged actual original provider root passes 42/42;
the native gate runs all 42 mapped root responsibilities. Selected final builds
report no work. No public API, generator, SDK or ownership domain changes.

The frozen 778-test epoch is explicitly superseded after the actual counterexample,
before changing inputs: build 130/controller 1, no CTest execution, all original
logs and 10,633 frozen input identities preserved. It is not a suite pass.
The [SP01-SP30 review](sqlite_provider_source_review_5_0_6.md) keeps the whole
provider pending, including SP17's explicit native chunk-capacity bound. Full
current integration will follow the necessary source repairs; no fresh whole
native epoch has started. Source status stays 65 accepted/57 pending/2 external,
all six partial capabilities and every P0-P7 exit remain open. Historical failed
full suites retain their outcomes; no new MIPS/device or Java/reproduction gate
is claimed.


## Native SQLite chunk-capacity checkpoint

The [SC01-SC06 review](sqlite_chunk_capacity_source_review_5_0_6.md) qualifies
the complete three-test stream allocation source after 250/858 actual native
capacity differences. One shared provider loop adds nine lines: short prefixes,
positive capacity bounds and the zero-size single-tail path retain original
behavior. All 858 live original observations match each frontend; retained 87
binding/poison controls remain intact. Original 3/3, native 7/7, imported 6/6 and
selected ASan/UBSan 6/6 pass, including native transaction/dynamic integration
and actual UserContext deletion. Leak scanning is disabled. Final selected builds
have no work. Source informational process-allocation counters map explicitly to
C++ arena-upstream measurements; no numerical equivalence or speedup is claimed.

Before the ledger update, 10,643 existing execution identities retain their bytes
except one provider loop and four build registrations; seven new execution files
install the observer/project/corpus/verifier. Producer, SDKs, previous corpora
and both references are unchanged. Format/check covers 1,708 C++ files; module
checks cover 1,011 canonical bodies and 1,516 exports. Discovery is 782 native/
573 imported, not a full run. Only the allocation test source advances, to
66 accepted/56 pending/2 external. All capability verdicts, containing-provider/
caller responsibilities, current complete integration and every P0-P7 exit remain
unchanged. No new whole native epoch, MIPS/device, Java or generation gate is claimed.
