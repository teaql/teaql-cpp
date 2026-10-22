# SQLite dynamic query integration source review (Rust 5.0.6)

Status: **pending_review**. All 23 unchanged original tests pass; complete C++
caller/assertion reconciliation remains open. This is the provider test file,
not the separately accepted runtime test file with the same basename.

## Locked original execution

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-provider-sqlite/tests/dynamic_field_query.rs`.
Length: 1740 lines; SHA256 `c9c4f49da606d40c42062617f7eea6f91e8a6dae8ead0786092e9ede4e613d69`.

The [isolated original receipt](parity/sqlite_dynamic_query_isolated_source_5_0_6.json.gz)
runs all 23 original tests with no skipped tests. Test source is copied byte for
byte; actual reference macros generate the Rust entity carrier. The manifest
uses reference package paths and exact futures-executor/futures-util 0.3.32 and
rusqlite 0.40.1. Registry identities are a subset of the reference Cargo.lock;
test discovery must equal every source test name. Cargo runs offline and locked.
The source and reference guards are checked again after execution.

```sh
source scripts/toolchains/activate.sh
python3 scripts/replay_sqlite_dynamic_query_source_5_0_6.py \
    --record /tmp/sqlite_dynamic_query_fresh.json.gz
```

The controller sets both CARGO_TARGET_DIR and TMPDIR inside its owned temporary
directory, so the original reopened-database fixture also belongs to the probe.
It removes 218,521,726 bytes after retaining compact source/lock/command/output
records. The [initial receipt](parity/sqlite_dynamic_query_complete_source_5_0_6.json.gz)
also passes all 23 tests but predates TMPDIR isolation. Its 218,493,054-byte build
tree was removed; it does not prove removal of the original test's external
28,672-byte temporary database. No broad /tmp cleanup is authorized or performed.
The isolated replay supersedes that cleanup limitation for subsequent runs.

## Complete test inventory and reconciliation obligations

The table identifies all original test boundaries; it is not an acceptance
matrix yet. Each row still requires direct mapping of every assertion and helper
responsibility to installed C++ execution evidence.

| ID | Original test and line | Required responsibility |
| --- | --- | --- |
| SDQ01 | `dynamic_stream_uses_native_cursor_and_keeps_value_null_missing_and_tail` (131) | Native cursor chunks 2+1, Value/NULL/NotLoaded, selected masks, shared snapshots, connection lease and post-EOF query. |
| SDQ02 | `dynamic_stream_drop_releases_native_cursor_and_allows_audited_save` (212) | Early cursor drop releases native lease and permits audited Save. |
| SDQ03 | `dynamic_stream_invalid_stored_type_fails_the_batch_and_releases_locks` (253) | Invalid stored type rejects the entire batch and releases native locks. |
| SDQ04 | `dynamic_stream_covers_empty_exact_full_and_large_batch_limits` (291) | Empty, exact-full and large chunk-size boundaries. |
| SDQ05 | `dynamic_stream_plan_cannot_cross_executor_instances_even_on_the_same_connection` (351) | Plan bound to original executor: a second executor on the same connection must reject before cursor acquisition. |
| SDQ06 | `dynamic_stream_plan_validates_complete_batches_before_exposing_any_sidecar` (393) | Complete batch validation: owner mismatch, omitted owner, unrequested owner, unselected value and type mismatch; no partial sidecars; duplicate-owner sharing. |
| SDQ07 | `held_dynamic_view_cannot_follow_a_changed_storage_profile` (484) | Held entity cannot follow a changed namespace/profile; no native write or audit; restoring the profile permits retry. |
| SDQ08 | `held_dynamic_view_cannot_follow_a_different_executor_instance` (568) | Held entity cannot follow a different executor instance. |
| SDQ09 | `sqlite_child_enhancement_preserves_dynamic_union_through_native_save_and_readback` (628) | Child enhancement merges two dynamic selections, preserves Value/NULL/missing and snapshot sharing; native-only Save retains both extension values. |
| SDQ10 | `fluent_save_uses_the_same_atomic_ledger_path_and_retains_selected_extensions` (729) | Fluent Save uses atomic ledger execution and retains selected extensions. |
| SDQ11 | `configured_provider_on_separate_executor_cannot_borrow_native_transaction` (839) | Separate configured executor cannot borrow the native transaction. |
| SDQ12 | `invalid_dynamic_readback_rolls_back_both_sides_and_retry_retains_original_intent` (875) | Invalid readback rolls back native and extension writes; original intent remains retryable. |
| SDQ13 | `changed_definitions_and_nonfinite_values_reject_before_native_dml` (922) | Changed definitions and nonfinite values reject before native DML. |
| SDQ14 | `explicitly_composed_dynamic_graph_rolls_back_together_without_adopting_unrelated_rows` (965) | Explicit graph composition rolls back together without adopting unrelated rows. |
| SDQ15 | `incomplete_native_projection_cannot_bypass_full_checker_via_dynamic_only_save` (1083) | Incomplete native projection cannot evade complete Checker through dynamic-only Save. |
| SDQ16 | `incompatible_dynamic_definition_composition_rejects_without_partial_import` (1124) | Incompatible definition composition rejects without partial import. |
| SDQ17 | `ordinary_ledger_save_persists_dynamic_null_delete_and_version_with_masked_audit` (1167) | Ordinary ledger Save persists dynamic NULL/Delete/version with masked audit. |
| SDQ18 | `failed_dynamic_dml_rolls_back_native_and_preserves_intent_for_retry` (1262) | Dynamic DML failure rolls back native writes and preserves retry intent. |
| SDQ19 | `dynamic_only_update_rejects_stale_version_and_preserves_unrelated_context_rows` (1333) | Dynamic-only stale-version rejection preserves unrelated Context rows. |
| SDQ20 | `unbound_new_entity_definitions_cannot_be_silently_assigned_to_current_storage` (1387) | Unbound new-entity definitions cannot silently bind to current storage. |
| SDQ21 | `new_entity_dynamic_intent_survives_id_allocation_and_readback_is_authoritative` (1422) | New-entity dynamic intent survives ID allocation; readback is authoritative. |
| SDQ22 | `durable_extensions_survive_reopening_and_decode_into_shared_typed_views` (1466) | Database reopening retains Value/NULL/missing, shared typed views and schema idempotence; post-Context ownership needs explicit C++ adaptation. |
| SDQ23 | `sqlite_audited_roots_load_row_owned_extensions_with_shared_masks` (1637) | Audited roots load row-owned extension values with shared masks. |

## Existing C++ evidence to reconcile

`teaql_runtime/tests/dynamic_relation_tests.cpp` contains shared selection,
stream and batch validation controls. `dynamic_graph_save_tests.cpp` exercises
actual SQLite atomic native/dynamic Save, rollback, readback, allocation,
retention and policy denial. `dynamic_provider_tests.cpp` validates provider
binding and batch rejection. SQLite storage and public stream test programs
cover lower-level persistence/cursor behavior. These are candidate delegates,
not proof that all 23 composed source callers and assertions are preserved.
In particular, an ordinary cursor test without selected dynamic fields cannot
stand in for SDQ01-SDQ06, and the accepted runtime dynamic query source cannot
stand in for this real-provider integration.

The original DynamicSchool fixture includes indexed load state, fixed layout
mappings, native-name mutation tracking, generated dynamic methods, audited
initial inserts, a shared audit sink and database-backed definitions. Its
atomic_fixture/atomic_rows and stream seeding are part of the required review.
Do not replace audited Save assertions with raw INSERT/UPDATE success.

SDQ22 intentionally reads an Arc-owned Rust entity after dropping its Context.
C++ borrows must instead fail after Context close; retained values require an
explicit deep copy into an independently live destination. Qualification must
record and exercise that approved ownership adaptation, rather than extend the
UserContext lifetime or silently omit the retained-value assertion.

Next: reconcile complete streaming/batch/provenance callers, then graph Save,
rollback/retry and durable reopen responsibilities. No source or capability is
promoted by this original-only replay. Totals remain 113 accepted / 9 pending /
2 excluded_external; capabilities remain 11 qualified / 6 partial.

## Installed complete-batch validation counterpart (SDQ06)

The [batch-merge receipt](parity/sqlite_dynamic_batch_merge_5_0_6.json.gz) records
native and imported `dynamic_query_contracts` passing after extending
`database_stream_and_cleanup`. The actual DatabaseDynamicFieldsProvider and
SQLite executor store create the trusted selected-field stream plan. All five
original malformed inputs reject with their exact error codes; errors omit the
private canary and neither row gains a partial sidecar. The valid duplicate-owner
case preserves both payloads and checks shared definition and selected-code
identities directly.

Fixture names map DynamicSchool/note/untouched and owner 1 to School/title/count
and owner 7. The unselected count field uses the existing I64 fixture declaration;
its private Text value must still fail selection before type validation. This
retains and strengthens that error-precedence control without claiming identical
model metadata. Direct input rows are intentional here, matching the original
plan.merge test; they are not substituted for provider-executed stream rows.

The whole existing test program passes in both frontends. Canonical verification
covers 1,135 bodies; format/check covers 1,799 handwritten files. No runtime or
generator body changed. Build logs are retained compressed and their disposable
originals removed; stable preset outputs remain. No sanitizer or MIPS run was
required for these synchronous test-only assertions. SDQ06 has an installed
counterpart, but the remaining original callers and complete source verdict
remain pending; source/capability totals do not change.

## Same-connection executor provenance (SDQ05)

The [stream-binding receipt](parity/sqlite_dynamic_stream_binding_5_0_6.json.gz)
records the complete dynamic-query program passing native and imported execution.
A second SqliteMutationExecutor is constructed on the original SQLite connection;
it receives the original provider's trusted plan through the actual
`stream_sql_with_dynamic_fields` entry. It must reject synchronously with
`DYNAMIC_FIELD_STORAGE_PROVENANCE_MISMATCH`. An immediate poll of an actual
ordinary query on the original transport then returns all three rows, proving
that rejection leaves the shared connection lease available.

This preserves the original same-connection/different-executor control. The
original Rust try_lock assertion maps to C++'s immediate native lease acquisition
through the real query operation, rather than a raw SQLite call that could bypass
the runtime lease. Fixture entity/table names map to School/school_data; the
original compiled-query comment and chunk size 2 are retained. The original
provider-created plan selects all dynamic fields.

No runtime or generator implementation changed. Both CTests, 1,135 canonical
bodies, 1,799-file format/check and module-path verification pass. Captured build
logs are compressed and their disposable originals removed. SDQ05 and SDQ06 now
have installed counterparts; the other source obligations and whole-source
verdict remain pending, with unchanged source and capability totals.

## Native cursor boundaries and storage failure (SDQ03-SDQ04)

The [cursor receipt](parity/sqlite_dynamic_cursor_paths_5_0_6.json.gz) records
passing native/imported executions of the complete dynamic-query program.
The actual database provider now stores a selected NULL on the second owner,
while retaining the earlier unselected count value. Five original batch sizes
(1, 2, 3, 73, 8192) drain exactly three roots through the runtime stream and
indexed decoder. Checks include contiguous chunk indexes, bounded chunks, the
2+1 full/short-tail markers, native IDs/names, Value/NULL/NotLoaded distinctions,
unselected-field absence, clean entities and shared Value/NULL availability.

While the first two-row chunk holds the native lease, a second actual query
must remain pending. EOF must then allow immediate native query completion.
An actual empty selected query yields no chunk, public zero-sized streaming
rejects with the original error code, and neither retains the connection lease.
These are real cursor executions, not synthetic stream chunks.

The failure case corrupts persisted metadata on the second row using the
original data_type/is_null/payload mutation. The first two-row batch returns no
partial success, reports DYNAMIC_FIELD_DEFINITION_MISMATCH without the private
canary, and is terminal. An immediate native query and a typed query without
dynamic selection both succeed afterward, proving lease release on failure.

Fixture values map original IDs 1/2/3 and names school-1/2/3 to 7/8/9 and
alpha/beta/gamma; note maps to title. The selected payload is stored rather than
stream-value. Native roots and extensions are seeded by the existing fixture's
raw SQL/transaction-owned storage SPI. This qualifies the observed cursor
contracts, not original audited seeding or Save. In-cursor SDQ01 assertions are
now present; its post-close retained entity inspection and SDQ02's held streamed
entity audited Save remain open. Do not infer those from ordinary query success.

Runtime input hashes remain unchanged. Native and imported CTests, 1,135
canonical bodies, 1,799-file format/check and module-path verification pass.
Captured build logs are compressed and their disposable originals removed;
stable preset outputs remain. No source or capability promotion is made.

## Retained entities and early-close audited Save (SDQ01-SDQ02)

The [retained-save receipt](parity/sqlite_dynamic_retained_save_5_0_6.json.gz)
records native, imported and focused ASan/UBSan executions passing. Every batch
size now retains all decoded School entities, closes the completed cursor and
checks Value/NULL/NotLoaded again, including unchanged selected payload, absence
of the unselected count and clean mutation state. Their UserContext remains live;
no borrow is extended beyond its owning Context.

The early-close path reads and decodes one row from a one-row-chunk cursor. An
actual competing query is pending before close and immediately succeeds after
explicit stream cancellation. The held entity then stages title=after-drop and
enters the public AuditedSaveExt<School>::save path with the original audit
reason. An ordinary enhanced-entity query reads after-drop from the database.
This is actual audited persistence, not an ordinary query substituted for Save.
C++ explicit cancellation is the existing resource-close equivalent of the
original Rust lexical stream drop; Context teardown remains independent.

The preceding SDQ01 retained-view and SDQ02 Save gaps are therefore closed at
the installed caller level. The documented seed/model adaptations still apply:
the fixture does not yet reproduce original audited initial seeding, and the
remaining SDQ07-SDQ23 composition/persistence/source obligations stay pending.
No whole-source or capability promotion follows from these six caller groups.

Runtime input hashes remain unchanged. Both frontend tests and the targeted
sanitizer test pass, with leak scanning disabled by the committed preset.
Canonical verification covers 1,135 bodies, format/check covers 1,799 files and
module-path verification passes. Build logs are compressed in the receipt and
their disposable originals removed; reusable native/module/sanitizer outputs
remain in the committed preset directories. No MIPS or full-suite run is claimed.

## Held-view profile and executor changes (SDQ07-SDQ08)

The [held provenance receipt](parity/sqlite_dynamic_held_provenance_5_0_6.json.gz)
records native, imported and focused ASan/UBSan executions of the complete
`dynamic_graph_save_contracts` program. `held_view_storage_provenance` loads an
actual enhanced typed entity, stages both native-name and dynamic-title changes,
then exercises a profile switch and a different physical executor/database.
Both public AuditedSaveExt calls reject with the exact provenance mismatch.

After rejection, actual typed queries confirm both databases' native names are
unchanged; versions stay 3, audit sinks remain empty, original extension storage
still contains old, and the held entity retains its pending dynamic intent.
Restoring the original profile or using the original Context permits retry:
only the original database changes, its version becomes 4, one audit is delivered,
and the original dynamic operand is persisted. The other database remains intact.
The original test's audit reasons and pending operand strings are preserved.

C++ cross-Context execution uses an explicit allocator-aware School copy into
the destination Context. That copy preserves storage provenance and does not
implicitly share ownership or authorize a different executor. Definitions are
also copied explicitly before installing the destination provider. Both Contexts
remain live during the checks. The same-Context profile test needs no ownership
adaptation; its rejected Save and successful retry use the original held intent.

Fixture names map DynamicSchool/note to School/title and atomic to profile.
The retained fixture uses one owner 7 at version 3 instead of three seeded owners;
these source cases inspect the held first owner and the second database, which
are asserted directly. Its initial seed uses raw native SQL and transaction-owned
extension storage, so original audited initialization remains a separate gap.
The fixture's existing raw audit sink proves absence/count of delivery here;
this is not a new safe-audit redaction qualification.

No runtime or generator body changed. Native/imported/targeted sanitizer tests,
1,135 canonical bodies, 1,799-file format/check and module-path verification pass.
Leak scanning is disabled by the committed preset. Captured build logs are
compressed and their disposable originals removed; stable preset outputs remain.
SDQ01-SDQ08 now have installed caller counterparts. SDQ09-SDQ23 and complete
source/setup reconciliation remain pending; no source or capability is promoted.

## Child-selection union through native-only Save (SDQ09)

The [union-save receipt](parity/sqlite_dynamic_union_save_5_0_6.json.gz) records
native and imported execution of the complete graph-save program passing.
`child_dynamic_union_save` uses three actual SQLite owners. It seeds Value and
NULL controls for two dynamic Text fields through public AuditedSaveExt calls,
then selects extra at the root and title through the child enhancement. Both
fields retain Value/NULL/NotLoaded across the three owners, with no pending
dynamic mutation intent. Value and NULL owners share indexed availability;
the missing owner has distinct availability.

The first entity stages only its native name. Its actual audited Save increments
the version and emits one audit, returns enhanced-native-save, and serializes
both #title=enhanced-private and #extra=root-private. A fresh provider query
confirms that the second owner's native name is unchanged. No local fixture
code merges selected fields or manufactures returned rows.

Names map original note/untouched to title/extra. The retained fixture starts
native versions at 3, so the seed and native-only changes end at version 5
instead of the original 1-to-3 progression. Both increments are observed through
real Save. An existing count field remains unrelated storage. Initial native
roots still use fixture SQL, but the dynamic merge controls themselves are now
prepared through the original audited mutation path and original intent text.
This does not close the complete original startup/seeding reconciliation.

Runtime input hashes remain unchanged. Native/imported CTests, 1,135 canonical
bodies, 1,799-file format/check and module-path verification pass. This functional
test extension did not rerun sanitizer or MIPS profiles; earlier sanitizer
receipts retain their earlier scope. Build logs are compressed in the receipt
and their disposable originals removed. SDQ01-SDQ09 have installed caller
counterparts; SDQ10-SDQ23 and the complete source verdict remain pending.

## Selected fluent Save and durable unselected siblings (SDQ10)

The [fluent-save receipt](parity/sqlite_dynamic_fluent_save_5_0_6.json.gz) records
passing native/imported execution of the whole graph-save program after adding
`selected_dynamic_fluent_save`. Public AuditedSaveExt first saves title=fluent
and extra=keep and returns the newly changed sibling. A fresh bounded typed
query selects only title; extra is NotLoaded. Saving title=changed retains that
NotLoaded state while advancing the native version. A subsequent unchanged Save
retains title=changed in its returned view.

An independent bounded typed query selects all dynamic fields and confirms
extra=keep still exists in durable storage. It does not use raw SQL as a
substitute for the original typed read. An explicit Context-owned copy of the
returned DynamicFieldValues then erases title locally; the original entity
still returns changed. All original audit/comment/purpose text is retained.

The existing fixture mappings apply: note/untouched map to title/extra, and the
two mutations advance versions 3-to-4-to-5 rather than 1-to-2-to-3. The unrelated
count fixture field is not used to stand in for the Text sibling. Initial native
seeding is still the separately documented source reconciliation gap.

Runtime input hashes remain unchanged. Native and imported CTests, 1,135
canonical bodies, 1,799-file format/check and module-path verification pass.
Build logs are compressed and their disposable originals removed. This
functional-only extension did not rerun sanitizer or MIPS profiles.
SDQ01-SDQ10 have installed caller counterparts; SDQ11-SDQ23 and complete source
acceptance remain pending, with no source/capability status promotion.

## Separate transaction binding and invalid readback (SDQ11-SDQ12)

The [atomic-failure receipt](parity/sqlite_dynamic_atomic_failures_5_0_6.json.gz)
records native/imported executions of the complete graph-save program passing.
For SDQ11, an alternate executor resource owns a separately constructed SQLite
transport on the same connection. A provider bound to that resource loads the
held entity, but the registered native graph saver still uses the original
executor. Public Save must report DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED,
leave native name/version unchanged, emit no additional audit and retain intent.
The fixture-only AlternateExecutor type preserves the Rust AlternateSchema
instantiation's distinct resource role; it does not change executor behavior or
introduce another runtime/provider abstraction.

For SDQ12, the selected extension starts absent. An actual AFTER INSERT trigger
replaces its payload with not-json, causing authoritative readback to fail with
DYNAMIC_FIELD_INVALID_STORAGE. Fresh typed reads confirm native name/version
rollback and a NotLoaded extension; the audit sink stays empty and the held
entity keeps its pending mutation. After dropping the trigger, the same held
intent retries successfully. Fresh reads prove both native and dynamic operands
committed, one version increment occurred and exactly one audit was delivered.

The previously documented fixture mappings remain: School/title/owner 7/version
3 replace DynamicSchool/note/owner 1/version 1. Audit assertions measure no
additional delivery from a zero-event observation baseline instead of retaining
the source's three initial-seed events. Initial fixture seeding remains an
independent reconciliation gap. SQL removes the old selected extension before
observation so the failure test exercises the original INSERT-trigger boundary;
it does not fake rollback or repair the held entity before retry.

Runtime input hashes remain unchanged. Native/imported CTests, 1,135 canonical
bodies, 1,799-file format/check and module-path verification pass. Captured build
logs are compressed and their disposable originals removed. No sanitizer/MIPS
rerun is claimed for this functional test-only extension. SDQ01-SDQ12 now have
installed callers; SDQ13-SDQ23 and complete source acceptance remain pending.

## Nonfinite/revision preflight and incomplete projections (SDQ13, SDQ15)

The [preflight receipt](parity/sqlite_dynamic_preflight_5_0_6.json.gz) records
native7 and imported3 plus four imported differential gates passing. SDQ13
stages a native name and NaN amount, checks the exact nonfinite-number error,
then replaces v1 definitions with v2 while keeping the original pending NaN.
The second Save must report the definition mismatch. Native names/versions and
audit counts remain unchanged; a SQLite UPDATE trigger counter remains zero,
proving that even rolled-back native UPDATE attempts did not occur.

SDQ15 loads only id/version with selected dynamic fields, verifies name is
NotLoaded, and stages a dynamic-only edit. With the original RequireLoadedName
logic installed through TypedEntityChecker, Save returns RuntimeError::Check,
retains pending intent and changes neither native version nor audit delivery.
The prior fixture naming/version/seed adaptations remain explicit. SDQ13 clears
initial extension rows before observation so replacing the definition revision
does not introduce unrelated persisted-v1 read failures.

### Fixture protocol correction and preserved failure

The [initial failed receipt](parity/sqlite_dynamic_preflight_fixture_failure_5_0_6.json.gz)
retains both native/imported failures at the partial-projection assertion.
The handwritten School fixture inherited EntityDefaults' no-op
set_checker_loaded_fields. The reference derive macro's indexed-entity branch
in teaql-macros/src/derive_impl.rs restores an indexed projection from those
fields; existing generated C++ support already implements that responsibility.
The fixture now performs the same partial-to-indexed conversion through its
Context snapshot pool. Neither runtime nor generated implementation changed,
and the failure assertion was preserved.

All five consumers of the shared fixture were rebuilt. Native CTest passes
7/7, covering three contract programs and four original comparison gates.
Imported CTest passes 3/3; separate imported comparisons match dynamic query
250/250, relation loading 138/138, governance 24/24 and attachment states 24/24.
Canonical verification covers 1,135 bodies; format/check covers 1,799 files;
module-path verification passes. Runtime hashes are unchanged. No new sanitizer
or MIPS result is claimed. Build logs are compressed and their disposable
originals removed; stable preset outputs remain.

Installed caller coverage is now SDQ01-SDQ13 and SDQ15. **SDQ14 and SDQ16-SDQ23**,
original setup reconciliation and the complete source verdict remain pending.
No source or capability status is promoted by these focused gates.

## Typed definition-composition rejection (SDQ16)

The counterpart `incompatible_dynamic_composition` in
`teaql_runtime/tests/dynamic_graph_save_tests.cpp` loads target and source through
two actual SQLite typed queries. It stages the target title, installs an empty
source payload with `other-revision` definitions and its composed loaded snapshot,
then stages both native name and dynamic title on the source. The public
`LedgerEntityDefaults<School>::compose` protocol must return
`ConflictingDynamicDefinitions`, and the complete receiving change set must
remain equal to its pre-composition copy. A subsequent typed query, version and
audit checks additionally verify that this in-memory rejection performed no
persistence. School/title/id7/version3 remain the previously documented fixture
adaptations of DynamicSchool/note/id1/version1; this case does not qualify the
original audited bootstrap or multi-owner transaction scenario.

The initial failure is preserved in
[the fixture receipt](parity/sqlite_dynamic_composition_fixture_failure_5_0_6.json.gz).
A locally constructed definition was passed to a checked managed borrow. The
corrected test explicitly registers that definition with `user_context.make`.
No runtime or generator behavior was changed to accommodate the test.

The [focused receipt](parity/sqlite_dynamic_composition_5_0_6.json.gz) records
native 1/1 and imported 1/1 contract programs passing after the fixture fix.
Both programs also retain all previously installed graph-save assertions.
Pinned format/check covers 1,799 files and module-path verification passes.
No sanitizer or MIPS result is claimed for this test-only extension. Stable
preset directories were reused, with no disposable build tree created.
Installed callers now cover SDQ01-SDQ13, SDQ15 and SDQ16. SDQ14, SDQ17-SDQ23,
original setup reconciliation and complete source acceptance remain pending.

Canonical module verification passes for 1,135 consumer/fixture bodies.

## Explicit composed graph rollback and retry (SDQ14)

`composed_dynamic_graph_rollback` loads three independent typed School rows
through the registered SQLite data service. Parent and child stage native names
and dynamic titles; an unrelated third row stages only a dynamic title. The
public ledger composition protocol imports only the child into the parent.
An actual SQLite BEFORE INSERT trigger rejects the child's extension write.
The failed public audited Save is followed by a fresh typed query checking all
three original versions, absent extensions, unchanged parent/child names and
zero delivered audits. Parent and unrelated pending intent must remain present.
After dropping only the rejecting trigger, the unchanged parent is saved again.
Fresh typed reads must show both composed names and extensions, two incremented
versions and two audits, while the unrelated row remains unchanged in storage
and retains its independent pending intent.

The fixture maps original owner IDs 1/2/3 to 7/8/9, note to title, and initial
version 1 to 3. Existing seeded extension rows are cleared before observation.
The original initial three audited inserts are represented by raw native setup
and a zero audit baseline; audit deltas are checked, but audited bootstrap and
safe-sink masking are not claimed by this counterpart. The original untouched
field is not selected or mutated in this scenario. No runtime or generated
implementation changes are introduced.

The [initial compile receipt](parity/sqlite_dynamic_composed_graph_compile_failure_5_0_6.json.gz)
records a test adaptation error in both frontends: this handwritten carrier does
not expose a version scalar member. The corrected assertions read the actual
loaded version through `EntityRuntimeState::get_original_version(entity_key)`;
no version assertion is removed and the fixture implementation is unchanged.

The [composed graph receipt](parity/sqlite_dynamic_composed_graph_5_0_6.json.gz)
records native 1/1 and imported 1/1 contract programs passing, including all
previous graph-save assertions. Canonical verification covers 1,135 bodies;
pinned format/check covers 1,799 files, and module-path verification passes.
No sanitizer or MIPS rerun is claimed for this test-only addition. Stable
preset outputs were reused; no disposable build directory was created.

Installed caller coverage now includes SDQ01-SDQ16. SDQ17-SDQ23, original
fixture/bootstrap reconciliation and the complete source verdict remain open.
Source and capability totals are unchanged: 113 accepted / 9 pending / 2
excluded_external; 11 qualified / 6 partial. P0-P7 remain incomplete.

## Ledger DML failure and stale extension-only writes (SDQ18-SDQ19)

`dynamic_dml_failure_retry` stages native and dynamic operands on a freshly
loaded typed SQLite row and invokes `save_audited_ledger_entity`, matching the
original test's public ledger entry. A real BEFORE INSERT trigger raises
`PRIVATE-DRIVER-CANARY`. The error must contain `DYNAMIC_FIELD_STORAGE_FAILED`
and must not contain the driver canary; the original change set remains nonempty.
A fresh query must show the original native name/version and absent selected
extension, with no audit delivered. Dropping only the trigger and retrying the
same row must return the staged name/extension, increment the version once and
deliver exactly one audit.

`dynamic_stale_version_isolation` loads current and stale views of owner7 and an
independent owner8 in the same Context. It stages unrelated owner8 intent, saves
an extension-only current owner7 change, then rejects an extension-only stale
owner7 Save. Fresh typed reads must preserve the current extension/version and
the unrelated owner's original version/absent extension. Both stale and unrelated
pending intents remain present. The additional audit-count assertion excludes
post-rejection delivery. The read helper's optional owner filter uses a typed
Expr predicate; existing callers without that option retain their earlier query.

Both scenarios preserve the previously documented fixture adaptations: School,
title, IDs 7/8 and baseline version3 replace DynamicSchool, note, IDs 1/2 and
version1. Raw bootstrap and raw audit-count observations do not establish the
original audited bootstrap or safe-sink masking. SDQ18 validates returned error
privacy, not all diagnostic destinations. Runtime and generated code are unchanged.

The [DML/stale receipt](parity/sqlite_dynamic_dml_stale_5_0_6.json.gz) records
native 1/1 and imported 1/1 contract programs passing, including earlier graph
Save callers. Canonical verification covers 1,135 bodies; pinned format/check
covers 1,799 files and module paths pass. No sanitizer or MIPS run is claimed.
Stable preset directories were reused; only a compact receipt was added, with
no disposable build tree to remove.

Installed callers cover SDQ01-SDQ16 and SDQ18-SDQ19. SDQ17, SDQ20-SDQ23,
original setup/bootstrap reconciliation and the complete source verdict remain
pending. Source/capability counts and P0-P7 incomplete status are unchanged.

## New entity storage provenance and authoritative readback (SDQ20-SDQ21)

`new_dynamic_entity_provenance` runs two independent empty-database fixtures.
Both create an id0 typed School, explicitly mark it new, stage its native name,
install fully loaded indexed state and empty dynamic values, and stage a title.
The unbound case uses the original unbound definitions and the public audited
Save facade. It requires `DYNAMIC_FIELD_STORAGE_PROVENANCE_MISMATCH`, an empty
native table, retained dynamic intent and no audit delivery.

The bound case constructs the same namespace/provider and obtains definitions
through `definitions_for_context`, binding them to the actual registered storage.
An actual SQLite AFTER INSERT trigger uppercases the persisted JSON payload.
The public `save_audited_ledger_entity` call must allocate an ID, return version1
and return `CANONICALIZE`, rather than the staged lowercase operand. A fresh typed
query by the allocated ID additionally verifies the stored extension and one
audit delivery. The deterministic allocator starts at100; this strengthens the
original positive-ID check without changing allocation ownership.

The existing fixture is cleared before either observation, so this case needs
no seeded-row version adaptation. School/title/profile replace the original
DynamicSchool/note/atomic names. The carrier stores the loaded version in its
runtime state; the assertion uses `get_original_version`. Fully loaded indexed
construction and explicit new/native intent mirror the original fixture helper.
Runtime and generated implementation are unchanged.

The [new-entity receipt](parity/sqlite_dynamic_new_entity_5_0_6.json.gz) records
native 1/1 and imported 1/1 graph-save programs passing, including prior callers.
Canonical verification covers 1,135 bodies, pinned format/check covers 1,799
files, and module paths pass. No sanitizer or MIPS result is claimed. Stable
preset directories were reused; the compact receipt is the only new verification
artifact, with no disposable build tree created.

Installed caller coverage includes SDQ01-SDQ16 and SDQ18-SDQ21. SDQ17,
SDQ22-SDQ23, original fixture/bootstrap reconciliation and the complete source
verdict remain pending. P0-P7 and aggregate capability/source totals are unchanged.

## Value, NULL, Delete and safe audit delivery (SDQ17)

`dynamic_null_delete_safe_audit` installs a Context-owned SafeAuditEventSink and
calls the public `save_audited_ledger_entity` path three times. The first Save
stages both a native name and a dynamic private canary, with the same canary in
the audit reason. Assertions inspect returned name/value/version, the cleared
change set, one safe event, a masked `#title` field and complete safe-event Debug
output with no canary. The second Save stages NULL, verifies unchanged selected
snapshot identity before persistence, then checks the incremented version,
explicit Null state and absence of the canary from all safe events. The third
Save stages Delete and checks the next version, NotLoaded in both the returned
entity and a fresh typed SQLite read, three safe deliveries and no canary.

This closes the safe-sink observation omitted by the preceding raw audit-count
fixtures. It does not assert that the raw privileged sink is masked. Source
name/field/version mappings remain School/title/versions4-6 versus
DynamicSchool/note/versions2-4; safe event counts are deltas1-3 after the existing
raw bootstrap instead of counts including three original audited inserts.
The original bootstrap responsibility still needs final reconciliation. Runtime
and generated code remain unchanged.

The [initial compile receipt](parity/sqlite_dynamic_safe_audit_compile_failure_5_0_6.json.gz)
preserves the failed whole-container Debug call in both frontends. The available
public formatter accepts individual SafeAuditEvent objects. The corrected test
formats every captured event and rejects the canary in any result; none of the
event payload or trace fields are omitted from the privacy observation.

The [safe-audit receipt](parity/sqlite_dynamic_safe_audit_5_0_6.json.gz) records
native 1/1 and imported 1/1 graph-save programs passing after the test correction,
including all preceding callers. Canonical verification covers 1,135 bodies,
pinned format/check covers 1,799 files and module paths pass. No sanitizer or
MIPS result is claimed for this test-only addition. Stable preset directories
were reused; compact evidence is retained without disposable build trees.

Installed callers now cover SDQ01-SDQ21. SDQ22-SDQ23, original setup/bootstrap
reconciliation and complete source acceptance remain open. Source totals stay
113 accepted / 9 pending / 2 excluded_external, capability totals stay
11 qualified / 6 partial, and P0-P7 remain incomplete.

## Audited SQLite roots with row-owned dynamic masks (SDQ23)

`audited_roots_shared_dynamic_masks` starts from a fresh UserContext and SQLite
connection, registers modeled metadata through the existing setup helper, calls
the real SQLite schema provider via `ensure_schema`, and registers the executor.
It creates IDs1-3 through public audited Save, not raw INSERT, asserting version1
for every returned root and three safe audit deliveries. Only after native
seeding does it install the source-shaped in-memory dynamic provider with owner1
Value, owner2 NULL and owner3 absent. A bounded typed SQLite query checks all
three rows, the first native name, extension values/states, identical selection
snapshot identities for Value/NULL and a different identity for the missing
extension. Every enhanced row must remain at version1 with no dirty fields.

This follows the original provider ordering and actual audited bootstrap. The
only naming adaptation is School/title versus DynamicSchool/note; IDs1-3,
version1, native names, private-note operand, bounded ordering and all original
assertions are retained. The Context-owned snapshot identity replaces Rust Arc
pointer identity. This focused caller supplies audited bootstrap evidence but
does not automatically reconcile every earlier raw-seeded scenario. Runtime and
generated implementations are unchanged.

The [audited-roots receipt](parity/sqlite_dynamic_audited_roots_5_0_6.json.gz)
records native 1/1 and imported 1/1 graph-save programs passing, including all
prior callers. Canonical verification covers 1,135 bodies, pinned format/check
covers 1,799 files and module paths pass. No sanitizer or MIPS result is claimed
for this functional test addition. Stable preset directories were reused;
compact evidence was retained without a disposable build tree.

Installed callers cover SDQ01-SDQ21 and SDQ23. SDQ22 durable reopen/explicit
retention and whole-source helper/assertion reconciliation remain pending.
SDQ23 supplies actual audited bootstrap evidence; this is not a relabeling of
the earlier raw-seeded fixtures. Source and capability status remain unchanged.

## Durable reopen and explicit destination retention (SDQ22)

`durable_dynamic_reopen` creates an exclusively owned temporary directory and
places the SQLite database inside it. A local RAII owner removes that directory
after both Contexts and their connection resources have been destroyed. The
first Context installs real metadata, SQLite schema/runtime/executor/provider
layers, repeats ensure_schema, and seeds IDs1-3 through public audited Save.
The transaction-owned dynamic-field SPI then writes owner1 Value and owner2
NULL and commits. This SPI call is fixture initialization, not an alternative
application-facing Save API.

After destruction of the first Context, an independently assembled Context
opens the same file, reconciles schema and performs a bounded typed query. The
assertions retain all original values, NULL/missing states, native names and
version1, shared Value/NULL masks, distinct missing mask and shared definition
identity. The C++ ownership adaptation explicitly deep-copies the first typed
row into GlobalContext before destroying the reopened UserContext, checks nested
payload/definition allocation ownership and reads the retained value afterwards.
It does not access the invalid original borrow or implicitly extend its lifetime.
School/title replace DynamicSchool/note; namespace school-demo, IDs, seed names,
versions, stored-value and operation ordering remain source-shaped.

The [initial compile receipt](parity/sqlite_dynamic_reopen_compile_failure_5_0_6.json.gz)
retains the test's mismatched-allocator string comparison failure in both
frontends. The corrected comparison uses string views for the same exact
expected name; it does not change carrier allocation or weaken the assertion.

The [durable-reopen receipt](parity/sqlite_dynamic_reopen_5_0_6.json.gz) records
native 1/1, imported 1/1 and targeted ASan/UBSan 1/1 graph-save programs passing,
including preceding graph-save callers. Leak scanning remains disabled.
Canonical verification covers 1,135 bodies; pinned format/check covers 1,799
files and module paths pass. No MIPS or physical-device execution is claimed.
Stable preset directories were reused. Temporary database directories are
removed by the fixture; the post-test scan found no matching leftover directory.
Only compact success/initial-compile evidence is retained.

All SDQ01-SDQ23 now have installed C++ callers with recorded adaptations.
This is not yet complete source acceptance: the final helper/assertion mapping,
original fixture versus adapted setup reconciliation and joined integration
review remain pending. Source totals stay 113 accepted / 9 pending / 2
excluded_external; capabilities stay 11 qualified / 6 partial and P0-P7 incomplete.

## Whole-source helper review: stream bootstrap correction

Reviewing `atomic_fixture`, `seed_stream_extensions` and `dynamic_stream_request`
against the installed SDQ01-SDQ06 caller found a material setup gap: the C++
stream corpus initialized native rows with raw SQL and extensions through the
transaction SPI. Passing those cursor assertions alone did not exercise the
original audited initialization-to-cursor transition.

`database_stream_and_cleanup` now invokes the SQLite schema provider, registers
the executor, creates all three native roots through public audited Save and
asserts version1. It then queries the typed roots, stages selected Value/NULL
and an unselected private Text field on the first two, saves through audited
Save and asserts version2 before opening any stream. The existing unselected
numeric count control is retained separately to preserve the stronger
selection-before-type error-precedence stimulus in SDQ06. Both per-chunk and
post-close retained-view checks require the private Text field to stay NotLoaded.
All previous cursor, invalid-batch, lease, early-close Save and exact error
assertions remain installed. Names/IDs map to alpha/beta/gamma and7/8/9; title
and extra map to note and untouched, and stored maps to stream-value.

The original Schema wrapper maps to the runtime metadata-backed executor;
AuditProbe maps to the Context-owned safe sink in SDQ17/SDQ23;
RequireLoadedName maps to the restored typed checker projection protocol in
SDQ15; AlternateSchema maps to the separate executor resource type in SDQ11.
The original atomic_fixture's three-field definition profile and all earlier
one-owner/raw-seeded graph cases still need a final whole-source reconciliation.
These mappings are review findings, not a source acceptance verdict.

The [stream-bootstrap receipt](parity/sqlite_dynamic_stream_bootstrap_5_0_6.json.gz)
records native 1/1 and imported 1/1 dynamic-query programs passing with the
corrected initialization. All earlier program controls, including asynchronous
provider paths, remain installed. Canonical verification covers 1,135 bodies;
pinned format/check covers 1,799 files and module paths pass. No new sanitizer
or MIPS result is claimed for this functional fixture change. Stable preset
directories were reused, with only compact evidence added and no disposable
build tree created. Source/capability status remains unchanged.

## Final reconciliation inventory and concrete closure work

The [machine-readable review inventory](parity/sqlite_dynamic_query_reconciliation_5_0_6.json)
maps every one of the 23 exact original test names/lines to its installed C++
caller and records 120 reviewed obligation groups. The inventory binds the
reference source and both current candidate programs plus their shared carrier
by SHA256. An obligation group may contain several original assertions; this
count is neither an assertion count nor a semantic acceptance score. Focused
receipts remain evidence for their own source checkpoints, not automatically
for later changed inputs.

Three finite closure requirements remain:

1. **DQ_CLOSE_1 — shared atomic fixture.** The reference creates three roots via
   audited Save at version1, starts with empty extensions, configures Text note,
   Text untouched and F64 amount together, and captures safe audit events from
   initialization onward. Earlier graph counterparts often use the one-owner
   version3/raw-sink/prepopulated-extension Fixture. Per-case mapping must address
   every material setup difference or replace it with the original-shaped setup.
   SDQ23's audited bootstrap proves that path exists; it does not by itself prove
   its composition with every earlier rollback, checker or provenance case.
2. **DQ_CLOSE_2 — typed carrier/helper contract.** The original indexed derive
   has id/version/name; the handwritten School carrier retains eleven frozen
   layout slots. Reconcile projection, new state, runtime ledger and JSON helper
   behavior explicitly. The prior missing checker-projection hook is a concrete
   example of why a passing low-level delegate is insufficient.
3. **DQ_CLOSE_3 — final joined checkpoint.** Once fixture/carrier reconciliation
   is resolved, run native/imported integration at one exact candidate checkpoint,
   bind original replay evidence, and inspect all case/helper dispositions before
   promoting the source. Memory/retention changes require the relevant sanitizer
   gate; no routine MIPS or physical-device gate is introduced here.

No C++ code changed during this inventory review, so runtime tests were not
repeated. Inventory validation checks exact source test discovery, caller
presence, input hashes and obligation coverage structure. It cannot prove the
semantics of the listed obligations. All three closure requirements remain open;
source status, capability totals and P0-P7 status are unchanged.

## Source-layout carrier reconciliation work

The shared handwritten carrier now has separate template instantiations for the
frozen eleven-slot example layout and the original SQLite test's three-slot
layout. The graph-save counterpart program selects the source-layout type:
id0, version1, name2, revision dynamic-school-sqlite-v1 and no relations. Existing
example-oriented callers retain the frozen layout. Both types share decoding,
loaded snapshot, dynamic-field attachment, checker projection restoration and
allocator-aware copy implementations; no generated artifact is hand-edited.

This removes the extra-slot geometry from graph-save qualification while keeping
historical example tests meaningful. The source-shaped graph suite must rerun
with all direct shared-fixture consumers and the relevant original comparisons.
DQ_CLOSE_2 remains open until those gates and remaining helper/JSON/version
contract reconciliation are complete; this change alone is not a promotion.

An explicit layout control checks source field_count3, id0/version1/name2,
exact source revision and absence of platform, alongside frozen field_count11,
name3/version10 and separate Context registrations. The
[initial compile receipt](parity/sqlite_dynamic_source_layout_compile_failure_5_0_6.json.gz)
retains the dependent-base lookup error introduced by the shared template;
qualifying inherited members fixes that test implementation issue without
changing runtime behavior.

The [source-layout receipt](parity/sqlite_dynamic_source_layout_5_0_6.json.gz)
records native7/7, imported3/3, targeted ASan/UBSan1/1 and four imported Rust
comparisons (250,138,24,24 matching observations). All five direct fixture
consumers were rebuilt. Canonical verification remains1,135 and pinned
format/check covers1,799 files. Paths and diff checks pass. Leak scanning stays
disabled; no MIPS result is claimed. Stable preset outputs were reused and only
compact success/initial-failure evidence was added.

DQ_CLOSE_2 is partially addressed: graph-save cases now use the exact indexed
geometry. Stream-specific cases still select the frozen carrier, and complete
scalar version/JSON helper reconciliation remains open. DQ_CLOSE_1 and the final
DQ_CLOSE_3 verdict remain open as well. Earlier inventory hashes identify their
historical checkpoint; this receipt identifies the new graph/fixture inputs.
No source or capability promotion is made.

## Stream layout and scalar/JSON helper correction

The SQLite stream integration caller now locally selects SourceSchool, so all
provider-source SDQ01-SDQ23 counterparts use the original three-slot geometry.
Other example-oriented callers retain the frozen carrier. A direct stream layout
assertion checks id0/version1/name2 and the exact revision before bootstrap.

Review of locked teaql-macros/src/derive_impl.rs found a further carrier gap:
the original has an independent version scalar and only serializes indexed
fields whose load state is loaded. The source carrier now decodes/copies its
version scalar, exposes the version protocol, includes loaded version in mutation
values/JSON and omits NotLoaded id/name/version. The frozen carrier retains its
prior serialization behavior. This is a handwritten test-carrier correction,
not a generated-artifact patch or runtime API change.

Five direct controls cover full scalar rows, missing name, missing version,
explicit NULL version (default zero but loaded) and invalid present version type.
They inspect scalar values, mutation projection, exact JSON membership/value and
explicit destination copies. Existing actual SQLite Save/readback, partial
Checker rejection and retention tests must pass with these corrected helpers.
The controls derive from the locked macro field conversion/loaded-guard source;
they are not reported as a new independently executed Rust oracle corpus.

The [scalar-carrier receipt](parity/sqlite_dynamic_scalar_carrier_5_0_6.json.gz)
records native7/7, imported3/3 and targeted ASan/UBSan2/2 passing. Four imported
Rust comparisons retain250/138/24/24 matches; canonical verification remains
1,135, pinned format/check covers1,799 files, and paths/diff checks pass. All five
shared-fixture consumers were rebuilt. The receipt binds both candidate programs,
the shared carrier and the locked macro mapping/derive sources. No MIPS or new
independent Rust scalar corpus is claimed. Leak scanning stays disabled.
Stable preset outputs were reused; only compact evidence was added.

All provider-source stream and graph counterparts now select the three-field
carrier. The version scalar and loaded-only mutation/JSON surfaces are corrected
and tested. DQ_CLOSE_2 retains final helper-disposition review; DQ_CLOSE_1's shared
atomic fixture and DQ_CLOSE_3's final joined source verdict remain open. This
checkpoint does not promote source or capability status.

## Shared atomic fixture migration: first governed failure callers

Fixture now has an explicit source-setup mode separate from its retained legacy
mode. Source setup skips raw native CREATE/INSERT and extension seeding. It
configures Text title, Text extra and F64 amount together (note/untouched/amount
mapping), uses the real schema provider, clears schema audit events, and creates
three roots by public audited Save. Every returned version is1 and the retained
safe audit baseline is exactly3. No extension values are seeded. IDs7/8/9 and
names original/second/third remain the documented local naming adaptation.

SDQ11, SDQ12, SDQ15 and SDQ16 now select this shared setup. Their typed root read
expects all three bounded rows before choosing the first, instead of assuming
one persisted owner. Transaction binding, invalid authoritative readback,
incomplete projection Checker and incompatible composition assertions use native
version1 and the original safe audit baseline3. SDQ12 retry checks version2 and
safe audit count4. Its artificial pre-test extension deletion is removed.
SDQ16's conflicting revision now retains the same three field definitions as
the source setup, so the conflict changes the revision rather than also changing
the field set. Other legacy and not-yet-migrated source cases retain their prior
setup pending migration; DQ_CLOSE_1 is not closed by this first group.

The [atomic-fixture receipt](parity/sqlite_dynamic_atomic_fixture_5_0_6.json.gz)
records native1/1 and imported1/1 graph-save programs passing, including the
four migrated scenarios and all retained cases. Canonical verification remains
1,135; pinned format/check covers1,799 files and paths/diff checks pass. No new
sanitizer or MIPS result is claimed for this setup-only change. Stable preset
outputs were reused and only compact evidence retained. DQ_CLOSE_1 remains open
for the other source counterparts; no source/capability promotion is made.

## Shared atomic fixture migration: provenance, union and preflight

SDQ07-SDQ10 and SDQ13 now select source-mode Fixture as well. Both held-view
provenance scenarios create three audited version1 owners in each database and
start with empty extension storage. Safe audits are explicitly cleared before
observing rejection/retry, matching the zero-baseline observation in the original
provenance tests. Rejection preserves empty extensions and pending intent;
original-owner retry reaches version2 and delivers one safe event.

The child-union and fluent selected-field cases use the common Text/Text/F64
profile directly, removing their raw extra-owner INSERT and bespoke definition
replacement. Their version assertions now use the original version progression:
child union's native-only Save reaches3, and fluent updates reach2 then3.
SDQ09's additional audit-delta check uses the safe sink. SDQ13 no longer deletes
artificially seeded extensions or replaces the starting profile. It stages NaN
against the common amount definition, then changes only the definition revision,
requiring native version1, unchanged safe audit count3 and zero observed UPDATE
attempts. All earlier exact errors, selected masks, JSON and typed readbacks
remain required. No runtime or generated implementation changed.

The [atomic provenance/union receipt](parity/sqlite_dynamic_atomic_provenance_5_0_6.json.gz)
records native1/1 and imported1/1 graph-save programs passing with these five
additional source-fixture callers and all retained cases. Canonical verification
covers1,135 bodies; pinned format/check covers1,799 files, and paths/diff checks
pass. No new sanitizer/MIPS result is claimed for this fixture-only migration.
Stable preset outputs were reused and only compact evidence retained.

Source-mode atomic fixture now covers SDQ07-SDQ13, SDQ15 and SDQ16. SDQ14 and
SDQ17-SDQ21 still require their shared fixture migration (including a no-seed
source mode for new-entity cases). DQ_CLOSE_1 and full-source acceptance remain
open; source/capability counts and P0-P7 status are unchanged.

## Remaining shared atomic fixture callers

SDQ14 and SDQ17-SDQ21 now use source-mode Fixture. Its new explicit seed switch
allows SDQ20/21 to start with modeled empty storage, without deleting previously
seeded records. Other migrated callers begin with three audited version1 owners,
empty extension storage and three safe seed events. No production or generated
implementation changed.

The composed graph rejection keeps all versions at1 and the audit count at3;
retry changes only the composed two owners to version2 and produces five total
safe events. The third owner retains its seeded name and pending unrelated intent.
DML failure/retry and stale-version isolation now assert this same source-shaped
version and safe-audit baseline. Value/NULL/Delete advances versions2/3/4 with
safe event counts4/5/6; the dynamic masking assertion examines the first mutation
event after the three seed events, and privacy checks inspect every event.
Unbound new definitions leave storage and safe audits empty. Bound creation
allocates an ID, returns version1 and authoritative normalized payload, and
delivers one safe audit.

The [remaining-fixture receipt](parity/sqlite_dynamic_atomic_remaining_5_0_6.json.gz)
binds the current candidate and shared fixture hashes to native1/1 and imported1/1
passing graph-save contracts. Canonical verification covers1,135 bodies; pinned
format/check covers1,799 files. Module path and diff checks pass. No sanitizer or
MIPS rerun is claimed for this fixture-only change. Stable preset directories
were reused; no disposable build tree was created.

Every SDQ07-SDQ21 atomic caller now uses the common source-mode setup. This
completes their migration coverage, not the final helper/assertion disposition
audit or joined source acceptance gates. DQ_CLOSE_1 remains open until that audit
records the remaining identifier/helper adaptations. Source and capability
statuses remain unchanged.

## Native update helper reconciliation

Review of the original DynamicSchool::update_native_name found that it updates
both the typed name member and the runtime ledger. Several C++ source callers
previously staged only the ledger value. SourceSchool now exposes the matching
application-style updateNativeName helper, and those source callers use it.
SDQ17 explicitly checks the typed member and nonempty change set before Save.
This repairs the input state rather than relying only on authoritative readback.
The stream fixture also includes the original amount/F64 definition; its extra
count/I64 field remains an additional unselected-value validation control.

The derive helper review found an outstanding guard: the original
teaql-macros/src/derive_impl.rs installation method rejects an incompatible
snapshot layout identity or dynamic definition owner before installing values.
The handwritten carrier currently installs both without that guard. This is
recorded explicitly in DQ_CLOSE_2. The generated carrier already delegates to
GeneratedDynamicFields::install, but its tests do not substitute for repairing
and testing this handwritten source carrier. Whole-source acceptance remains
pending regardless of the existing successful positive installation paths.

The [native-helper receipt](parity/sqlite_dynamic_native_helper_5_0_6.json.gz)
binds both changed test programs and the shared carrier to native7/7 and
module3/3 passing gates. Separate imported comparisons match250 query,
138 relation,24 governance and24 loaded-state observations. Canonical1,135,
format/check1,799 and source/module path guards pass. No new sanitizer/MIPS
claim is made: allocation and ownership mechanics were not changed. Stable
preset directories were reused and no disposable build tree was created.
The explicit missing installation guard keeps DQ_CLOSE_2 and source acceptance
open; this checkpoint does not promote any source or capability.

## Dynamic installation guard repair

SourceSchool now delegates installation to the existing shared
GeneratedDynamicFields::install runtime primitive. This preserves the original
derive's layout identity and definition-owner validation before changing either
the runtime payload or loaded snapshot. The frozen eleven-slot carrier retains
its historical behavior; this change qualifies the three-slot source counterpart,
not every handwritten carrier in the repository.

A new source_dynamic_installation_contract installs a valid original payload,
stages independent pending intent, then attempts an owner mismatch and a layout
mismatch. The latter uses the frozen School layout, which has the same entity
name but a different layout identity, with a changed candidate payload. Both
must return the original incompatible-owner/layout diagnostic and preserve the
original snapshot identity, loaded payload, owner and entire pending change set.
The test exercises the carrier entry point rather than calling the runtime
primitive directly. Production runtime bodies and generated artifacts are unchanged.

The [initial failed guard control](parity/sqlite_dynamic_install_guard_initial_failure_5_0_6.json.gz)
retains native6/7 and module2/3 results with exact candidate sources. Its expected
loaded value was incorrect: EntityRuntimeState::update_dynamic_field updates the
visible payload to pending while recording the mutation. Rejection must preserve
that current pending value, not the earlier original payload. The corrected
assertion keeps the snapshot and full-ledger checks. No runtime behavior was
changed to satisfy the test.

Further assertion review found a distinct unfinished responsibility: SDQ09/10
use database version checks where the source asserts the returned scalar version,
and several other callers inspect the runtime original-version record. These
checks are useful but do not directly prove the same returned member assertion.
DQ_CLOSE_3 now records the required direct SourceSchool::version checks; the
installation repair cannot close this separate gap.

The [successful guard receipt](parity/sqlite_dynamic_install_guard_5_0_6.json.gz)
records native7/7, module3/3 and436 imported differential observations passing
after the test expectation correction. Canonical1,135, pinned format/check1,799,
module/source path and diff guards pass. All five consumers were initially
rebuilt; only the graph-save programs needed rebuilding for the assertion fix.
No new sanitizer/MIPS evidence is claimed for this adapter/test change. Stable
build directories were reused, no disposable build tree or local core file was
retained, and compressed success/failure evidence records the useful diagnostics.
The identified installation guard gap is resolved; the overall source remains
pending until the distinct remaining assertion/helper audit is complete.

## Direct typed version assertion reconciliation

The source has24 version-scalar checks, including all-row predicates. The
reconciliation ledger now lists each exact original line and C++ caller. These
callers check SourceSchool::version() directly on Save results or fresh typed
queries; existing SQLite and runtime original-version checks remain additional
controls. This covers native-only Save, fluent selected updates, rollback/retry,
composed owners, NULL/Delete, stale isolation, new allocated entities, reopened
storage and audited roots. Redundant direct name assignments preceding the
new updateNativeName helper were removed.

The continuing derive review identified another distinct metadata gap. Rust
derives not_null for all non-Option id/version/name fields, while the handwritten
descriptor marks only id non-null and setup registers the frozen School
descriptor. DQ_CLOSE_2 records source-specific descriptor selection, original
field order/nullability and modeled-schema validation as required follow-up.
Typed scalar readback success does not resolve that metadata discrepancy.

The [returned-version receipt](parity/sqlite_dynamic_returned_version_5_0_6.json.gz)
records native1/1 and imported1/1 graph-save programs passing with these direct
scalar checks. Canonical1,135, pinned format/check1,799 and module/source path
guards pass. No runtime or generator implementation changed, and no new sanitizer
or MIPS evidence is claimed. Stable preset directories were reused and only the
compact receipt was added. The version-assertion findings are resolved; source
metadata reconciliation and the full final audit remain open.

## Source descriptor and modeled schema reconciliation

SourceSchool::entity_descriptor now follows the original derive declaration
order id/version/name and marks all three non-Option fields non-null. The
setup helper explicitly selects source metadata for source-mode atomic and
stream callers and for the audited-root example. The file-reopen configuration
already requests SourceSchool's descriptor directly and therefore receives the
same repaired metadata. Legacy callers retain the frozen descriptor; the older
allocated-graph fixture explicitly selects it instead of inheriting the source
alias's changed metadata.

source_descriptor_contract checks all three properties' order, names, SQL
column names, types, nullability and ID/version flags. It then inspects
PRAGMA table_info on the empty schema created by the actual registered provider,
requiring exactly three expected columns, the ID primary key and non-null
name/version constraints. The prepared statement is finalized before the
Context-owned SQLite connection is destroyed. A frozen-descriptor control
preserves its historical nullable name/version behavior separately. This
repair changes test metadata and routing only; production provider/runtime and
generated artifacts remain unchanged.

The [source-descriptor receipt](parity/sqlite_dynamic_source_descriptor_5_0_6.json.gz)
records native7/7, module3/3 and436 imported differential observations passing.
All five shared-fixture consumers were rebuilt. Canonical1,135, pinned
format/check1,799, module/source path and diff guards pass. No sanitizer or MIPS
rerun is claimed for this metadata/test routing change. Stable preset build
directories were reused; only compact evidence was added.

DQ_CLOSE_1 is now qualified for the shared atomic fixture: all SDQ07-SDQ21
callers use source metadata and modeled schema, the three audited version1 roots
or deliberately empty new-entity setup, empty initial extensions and safe-audit
baselines. The helper ledger documents renamed identifiers and explicit Context
copies. This is a scoped exit verdict. DQ_CLOSE_2/3 and whole-source acceptance
remain open for the final complete helper/assertion disposition audit; source
and capability counts do not change.

## Exact assertion inventory and typed postconditions

The [original assertion inventory](parity/sqlite_dynamic_query_assertions_5_0_6.json)
retains all163 original assertion macro occurrences across23 tests, with exact
expressions, lines, source hash and mapped candidate callers. Looped assertions
are counted once; this is review bookkeeping rather than semantic proof or an
execution count. The isolated original23/23 receipt still matches the locked
source hash.

Review found two postconditions checked at the wrong API layer. SDQ01 now
executes a bounded typed enhanced query after the two-plus-one stream finishes,
in addition to checking immediate native lease availability. SDQ20 now performs
the ordinary bounded typed enhanced root query after rejecting unbound new
definitions, requiring an empty result alongside the existing raw storage,
pending-intent and safe-audit checks.

The complete derive review also identified a remaining dynamic-mutator gap.
Original update/delete methods validate indexed state and layout identity and
refresh the loaded snapshot if availability changes. Source callers currently
invoke the ledger directly, bypassing that carrier behavior. Existing
GeneratedDynamicFields::update/erase provides the shared implementation used
by generated C++; the source carrier must delegate to it and exercise the
original entry semantics. DQ_CLOSE_2 explicitly records this responsibility.
No whole-source promotion is justified by the assertion inventory alone.

The [typed-postcondition receipt](parity/sqlite_dynamic_typed_postconditions_5_0_6.json.gz)
records native2/2 and module2/2 passing with exact current input hashes and
the still-matching isolated original23/23 receipt. Canonical1,135, pinned
format/check1,799, module/source paths and diff checks pass. No new sanitizer or
MIPS evidence is claimed for these test additions. Stable preset directories
were reused and only compact receipts/inventory retained. The new typed
postcondition gaps are resolved; dynamic carrier mutation entry semantics remain
explicitly open, with no source/capability promotion.

## Indexed dynamic mutation entry reconciliation

SourceSchool now exposes updateDynamicField/deleteDynamicField adapters that
delegate to existing GeneratedDynamicFields::update/erase. All28 direct ledger
mutation calls in the source graph and stream counterpart ranges now use these
carrier entries. Earlier legacy ledger-focused tests keep their original SPI
calls. No generated artifacts or shared runtime implementation were changed.

source_dynamic_mutator_contract verifies first selection refreshes the indexed
snapshot, a subsequent NULL update preserves its identity, deletion removes
availability and refreshes the snapshot, and repeated deletion preserves it.
It also exercises update and delete against non-indexed and wrong-layout states.
Both operations must return the original exact error and preserve the visible
pending value, complete ledger change set, and prior snapshot presence/identity.
These controls invoke the same carrier methods as the source counterparts.

The [source-mutator receipt](parity/sqlite_dynamic_source_mutator_5_0_6.json.gz)
records native7/7, module3/3 and436 imported differential observations passing.
Canonical1,135, pinned format/check1,799 and module/source path guards pass.
No sanitizer/MIPS rerun is claimed: this adapter uses existing Context-owned
runtime primitives without changing ownership or cleanup. Stable build
directories were reused; only compact evidence was added.

The mutation entry gap is resolved. Continued helper review notes that the
original derive validates CompactRow dynamic-property metadata before scalar
extraction, while the source handwritten decoder currently begins decoding
directly. The generic hydration path does not add that guard. DQ_CLOSE_2 records
this exact remaining decoder prevalidation responsibility; no whole-source or
capability promotion is made at this checkpoint.

## Decoder prevalidation and original unselected-field control

SourceSchool now calls CompactRow::validate_dynamic_properties before scalar
extraction or carrier construction, wrapping its error as the original derive
does. The frozen carrier is unchanged. Five controls exercise direct decoding
and the actual read-hydration helper: valid I64 metadata value, NULL, an invalid
declared value, simultaneous metadata/native errors, and a native-only error.
The expected metadata error is exact and excludes the private payload; it takes
precedence over an invalid native ID. Valid metadata still allows the ordinary
native conversion error to surface.

The SDQ06 audit also distinguishes the original type-correct but unselected
Text field from the retained extra wrong-type/unselected I64 control. The latter
tests error precedence but cannot substitute for the former: a faulty
implementation could reject only wrong-typed unselected values. Both inputs
must be retained with the same unselected-value error, privacy and no-sidecar
postconditions.

## Final source acceptance

Status: accepted for the complete locked provider test source. DQ_CLOSE_1,
DQ_CLOSE_2 and DQ_CLOSE_3 are qualified. The [final acceptance receipt](parity/sqlite_dynamic_query_source_acceptance_5_0_6.json.gz)
binds current carrier/test inputs, original unchanged23/23 replay and163 original
assertion sites to native7/7, module3/3 and436 imported differential observations.
All23 candidate case entries are invoked by their executable main functions.
The original source hash and test-name sets match the retained isolated replay.

The final SDQ06 loop includes the original type-correct unselected Text input
and the additional wrong-typed unselected I64 input, both requiring the exact
unselected-value error, no private payload and no partial sidecar publication.
Decoder prevalidation passes all five direct/read-hydration controls. The
reconciliation ledger records every case, all24 original version checks, helper
dispositions and the repaired fixture, carrier, schema and API-layer gaps.

Identifier/table/namespace renaming, ordered fixture values, owner-thread lease
checks and explicit destination Context copies are documented adaptations.
Original objects retained after Rust Context destruction correspond to explicit
GlobalContext copies in C++; Context borrows are not kept alive implicitly.
The combined stream program preserves each required state/error and uses fresh
rows for batch-negative controls; no byte-identical SQL or universal macro
acceptance is claimed.

Canonical1,135, pinned format/check1,799, module/source provenance and diff guards
pass. No new sanitizer, MIPS, physical-device, released-package or generator
reproduction result is claimed by this source checkpoint. The previous scoped
sanitizer evidence remains historical. Stable native/module directories were
reused; no disposable build tree was created. Compact final evidence and the
original assertion inventory are retained, including earlier failed controls.

Only teaql-provider-sqlite/tests/dynamic_field_query.rs advances. Source totals
are now114 accepted /8 pending /2 excluded_external. The separate runtime test
source with the same basename retains its own acceptance. Provider lib.rs,
remaining runtime graph/save/resolved sources, full capabilities and P0-P7 are
not promoted by this result; capability totals remain11 qualified /6 partial.
