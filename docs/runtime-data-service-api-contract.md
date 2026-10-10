# Runtime data-service API review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete six-file runtime service source group is accepted after the final
reconciliation and validation below. Earlier partial/running paragraphs retain
the implementation history. This review is independent
of the accepted generic data-service executor/model layer and SQLite provider.

## Base, Context and shared model reconciliation

The complete base.rs (348 lines), context.rs (353), types.rs (166) and mod.rs
(15) were reviewed against the installed separate RuntimeDataService,
ContextDataService, EntityDataService and UserContextMetadata adapters. Their
responsibilities remain distinct despite allocator-aware C++ implementation.
Complete resolved.rs execution and helpers.rs canonicalization still require
reconciliation before accepting the combined source group.

Base queries preserve trace/comment and the two independent capture flags,
observed execution, concrete executor errors, plain rows versus SmartList and
entity decoding. Plain fetch_all does not record successful metadata locally;
SmartList/typed reads record before decoding. Base enhanced reads delegate to
typed reads. SQL mutation tracing adds an Entity frame only if the same entity
has no such frame; success metadata precedes optimistic-lock classification.
Update/Delete conflicts require an expected version and zero rows; Recover
always conflicts on zero rows and retains Debug-only ID spelling. Batch inserts
and updates execute individual ordered requests, preserve optional per-row
traces/version/old values, filter requested fields, stop on first failure, retain
prior successful metadata and validate the versioned final row total. The four
many methods retain sequential accumulation and Debug-overflow behavior.

Context metadata remains a live delegation rather than a registry snapshot.
Metadata absence is checked before executor lookup; entity service absence is
checked before Context service construction. register_executor retains its
separate type-erased graph saver installation responsibility. Explicit
for_executor supports transaction-owned execution; public reusable services
start with empty trace and no private query intent. Trace replacement uses a
Context copy, and RelationLoadPlan keeps all nine fields, recursive children,
optional query, Clone/PartialEq and complete structural Debug.

Private query-tree intent never becomes User/ledger/wire state. Leaf statement
provenance accumulates sequentially; statement-local binding policies are never
applied to another statement's positional parameters. Safe logging runs after
accumulation. Query observer presence follows select capture; mutation observers
follow the independent mutation option. Nested Context query/provider telemetry
retains operation attributes, Noop short circuit, scope activation during work,
success/failure classification and failure-isolated callbacks. Concrete Error
and output types need no blanket Clone assumption.

Context mutation cache invalidation occurs only after successful completion and
uses both separately registered cache interfaces. Final comment chooses the
complete trace chain over an explicit comment, preserves pending versus numeric
entity identity, and prefixes only a nonempty user identifier. Empty choices
produce None. These comments remain diagnostic intent, not authorization.

Returned foreign query/mutation work is cancelled before rejection; pending
child wake forwarding detaches before cancellation. Stable Context-owned
metadata views prevent asynchronous adapters from retaining temporary wrapper
addresses. Checked resource borrows cannot outlive their explicit origins, and
cross-domain payload retention requires independent destination storage.

This reviewed subset does not accept complete entity preparation, pagination,
query caching/hydration, stream execution, mutation/event composition, graph
saving or the complete six-file source group. Source-test disposition counts
and exported names remain locators, not proof of those behaviors. Context source responsibility is accepted separately after its bootstrap-root
correction and complete 375-check integration.


## Resolved preparation subset

Resolved source lines227–408 were compared with entity_service.hpp preparation
and the query adapter entry points. Repository entity/select/command factories
remain separate from behavior and request-policy enforcement. Select trace
prepends repository trace, then behavior runs before policy. Relation local-key
projection is added only for requested mapped relations. Typed hydration restores
id/version only for reduced entity projections, preserving aggregate/DTO shape.

Insert preparation runs behavior then policy, requires the actual command
entity descriptor, assigns an ID only for missing/Null/I64zero/U64zero, sets the
initial managed version, temporarily marks Create status, runs checker/fix and
removes the temporary status before propagating its error. Update preparation
runs behavior then policy without inventing a second check/fix or ID allocation.
Internal list preparation preserves its fast path only when continuous paging,
object grouping, child enhancements and relations are absent. The remaining
resolved execution body and complete helper composition stay under review;
this subset does not close the source group.


## Complete resolved execution reconciliation

The complete resolved.rs production body (1–1823) and helpers.rs production
ranges1–191 and225–363 were reviewed against the separate row, stream, enhanced-
entity, ID-page, relation-graph, query-key and mutation adapters. The two embedded
helper tests retain their accepted original body/input/assertion dispositions.
The combined six-file source group remains partial pending final integration.

Continuous paging preserves all fallback diagnostics and validation order:
options/slice/limit, supported shape, sole plain id order, first page, unavailable
store, missing cursor, and entity/direction/page-size/offset/expiry mismatch.
A valid cursor adds its boundary predicate and resets only the SQL offset.
Only a full nonempty page with a final id registers the next cursor. Registration
happens after object groups, child queries and query relations, and before outer
relation aggregates. Store failure changes observation rather than returned rows.
Query keys retain the exact canonical query Debug, namespace and principal
responsibility; the ID-set key independently includes active-root identity.

ID-set paging rejects the original unsupported shapes, adds an id tie-breaker,
reads the store before and after its build lock, bounds the id-only probe using
saturating max_ids+1, rejects non-u64 IDs, and retains exact totals only for a
successful retained set. Limit/store failures retain ordinary fallback and their
original optional diagnostic count. Page slicing converts offset/limit to native
size_t with source saturation, clears the pagination option, resets offset and
adds selected IDs to the existing filter. The build guard releases on completion,
error and cancellation. Retained payloads and query/result objects own nested
storage in their explicit Context domains.

Prepared queries retain final-comment resolution, independent debug/metadata
capture and owned-query trace transfer. Aggregation cache preference is the
registered backend before the concrete fallback. A hit reconstructs query intent
from the current compiler without database work or a synthetic successful SQL
log. A miss records actual query metadata before caching rows. Cache/provider
telemetry scopes retain operation attributes, hit/miss outcomes and distinct
failure categories; cancellation closes child work and scopes before reclamation.
Cache rows never own raw inherited intent. Private query scopes snapshot ancestry
for descendants without mutating an unrelated sibling's intent.

Basic SmartList and typed reads retain their direct Context compatibility path;
they do not acquire enhanced-query paging or relation behavior accidentally.
Enhanced reads restore reduced identity/version projections, perform ID paging,
choose direct/compact/flat/embedded hydration in source order, apply query plans
before behavior plans, freeze one graph before typed decoding and retain optional
ID-set totals including zero. Missing versus loaded-null to-one relations remain
distinct. Embedded flattening preserves scalar/null behavior, decoder availability,
reverse owner IDs, target-type/id deduplication and exact decoding errors. Fresh
roots retain separate mutable ledgers and only immutable graph anchors are shared.

Streaming prepares and validates a root query, rejects relation/object-group/
child enhancement with the exact source message, defaults chunk size to1000,
selects observed versus plain provider streaming from capture options and maps
concrete errors without requiring Clone. The returned stream is a Context borrow;
foreign streams cancel before rejection and cancellation propagates to the source.
Public PurposedSelectQuery entry points retain distinct owned and borrowed APIs.

Mutation preparation/publication is reconciled independently: behavior precedes
policy, Create assigns missing identity/version before checker/fix, Update retains
its narrower responsibility, and Delete/Recover retain their distinct lineage
selection. Prepared successful writes precede raw event publication. The current
old-row helper intentionally performs no database query; a complete supplied old
snapshot is preserved, while incomplete snapshots yield the source absent value.
Updated events merge written fields with old values and include managed id/version
and explicit changed fields. Empty batches emit nothing; successful batch writes
finish before per-row events, stopping on the first sink failure. Aggregation
cache invalidation follows successful Context mutation completion.

Helper reconciliation preserves all Value-specific relation bucket spellings,
nonnegative signed/unsigned identity merging, U64zero versus empty-list aggregate
defaults, single-cell versus object results and removal of the grouping foreign
key. Canonical aggregation queries recursively normalize predicate/function/
subquery/group/relation/child trees, stable-sort and deduplicate commutative and
IN/NOT-IN lists, and clear only original runtime/observability fields. Initial
version/time defaults affect only missing/Null/integer-zero values; source
version increment retains Debug overflow versus Release wrapping. Memory quota
failures and native-time range failures remain separate contracts.

## Reproduced cursor-profile mismatch

The full source review finds unconditional C++ rejection when a full page
registers next_offset beyond u64. Actual pinned public calls preserve Debug
panic and Release wrapping. Six of16 explicit boundary observations disagree
on MIPS MinSizeRel before repair; native Debug matches all16. The existing165
public query observations continue to match live original Rust on both profiles.

The C++ overflow check is now Debug-only; unsigned addition retains original
Release wrapping. The probe accepts explicitly supplied offsets and observes
execution overflow separately from purpose validation. New profile-specific
observations are recorded from actual original Debug and Release executables;
no arithmetic or runtime orchestration is duplicated in the oracle. Rows,
owned rows, outer relation aggregates, cache-miss/seek registration and plain
SmartList controls retain their separate results. CTest registers the boundary
comparison with its actual build profile. Full native and selected ASan/UBSan/
MIPS integration must finish before complete service-source acceptance.


## Remaining private typed-aggregate surface

The source also retains fetch_entities_with_relation_aggregates_internal<T>
(lines1256–1277), independently of enhanced entity fetch. It restores reduced
identity projection, uses row aggregate execution and decodes with one fresh
EntityRuntimeState, without ID-set preparation or behavior relation hydration.
The C++ internal surface has no corresponding entry. This source method has no
call sites elsewhere in the pinned runtime, but it still describes a distinct
retained capability; enhanced fetch is not an equivalent replacement because
it adds paging and relation responsibilities. Complete service acceptance stays
partial until the internal typed-aggregate path is retained and verified.


## Typed-aggregate implementation and verification

The missing internal entry is now installed through EntityDataServiceAccess and
the runtime data_service umbrella. TypedAggregateOperation preserves source
identity projection before row execution, creates one fresh decoding root,
reuses the existing row relation-aggregate operation and decodes every row through
from_compact_row_with_context. It does not enter enhanced-entity ID-set preparation
or behavior relation hydration. Concrete executor errors move through existing
completion helpers; pending child wake/cancel follows the scoped async protocol.
The public business-query surface and generator remain unchanged.

The independent original private-body oracle supplies48 twice-agreeing projection,
empty-list, provider-failure and model-failure observations. Its row input is
controlled; broader row execution has its independent public source comparisons.
The concrete native probe uses the actual row runtime and compares the same
projection, error and rooted decoding results. A separate default-PMR-disabled
contract seeds pagination observations, proves the helper leaves them intact,
requires actual User reclamation and checked state expiry, and verifies explicit
Global row/state copies through a new User's rebound Global reference.

Selected native/sanitizer/MIPS builds and these two checks are running. The earlier
376/376 plus51/51 and separate3/3 cross results cover the cursor correction, not
this new header/interface. Complete source acceptance remains partial until
current focused checks, stable consumer integration and wider validation finish.


The changed typed-helper interface consumers finish62 tasks on each cross profile;
no-work refreshes precede ASan/UBSan56/56 in38.96 seconds (leak scanning disabled)
and MIPS/QEMU56/56 in30.28 seconds. Post-test builds report no work. These include
the original51 service selection, both new helper/retention checks and all three
noncopyable-completion/foreign-child checks. Additional base/stream consumers
are being refreshed independently. Complete native338 tasks and378 regression
remain active. The source group remains partial until that integration and live
public/private source comparisons are final; the full goal is unchanged.


## Complete runtime service source acceptance

All six complete files (3068 lines including two embedded helper tests) and their
public/private surfaces, inherited/default behavior, model diagnostics, concrete
error transfers, execution ordering, provenance and ownership are accepted.
The missing private typed-aggregate responsibility is retained separately from
enhanced paging/hydration, and the cursor counter preserves source build-profile
semantics. No source wire/hash/serialization capability is invented.

Current full native378/378 passes in77.97 seconds after338 consumer tasks, with
420 nonempty compiled dependency records and no-work before/after. Selected
ASan/UBSan56/56 in38.96 seconds (leak scanning disabled), MIPS/QEMU56/56 in30.28
seconds and additional base/stream3/3 selections finish after stable builds.
Thirteen fresh source matrices match on native and MIPS:256/468/112/165/193/154/
432/266/356/48/451/1502/162. The165 public entity-query count corrects the earlier
prose inference293; it is the actual corpus/live-run count. The 32-bit key fixture
and controlled private-helper row input retain their explicit evidence limits.

Evidence is retained in measurements/parity/2026-10-09-runtime-service-source-acceptance.
This closes the runtime service responsibility and Stage4 composition within
explicit Global/User allocation, checked Context borrows, deep destination copies
and owner-thread dispatch. Graph/save execution, mutation-governance/audit saving
integration, runtime-root and final Stage6 requirements remain separate. Physical-
router execution remains deferred; no generator or runtime dependency changes.
