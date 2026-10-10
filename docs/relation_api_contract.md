# Relation runtime responsibility review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The public RelationLoadPlan is declared in data_service/types.rs; execution and
private helpers are in data_service/relation.rs production lines 1–1142, before
embedded tests. This review maps the responsibilities and model gap and records the complete
execution-body comparison. Installed native/target validation and surrounding
runtime/privacy composition retain their independent gates; the complete
relation source acceptance is recorded below.

## Complete source acceptance, 2026-10-09

The complete relation.rs production body 1–1142 and public/default/trait/protocol/
ownership responsibilities below are accepted. Current native 369/369 passes in
80.91 seconds with no-work before/after testing and 418 nonempty dependencies.
Fresh sanitizer/MIPS selections each pass 51/51, including all relation execution,
hydration, enhancement and original cache/flat/event contracts. Current MIPS
models match 162/162 live original observations. The reproduced returned-work,
registry/load and metadata/telemetry lifetime gates are repaired and verified.

The model is imported from data-service types, which retains its independent
broader source acceptance scope. Surrounding Context/service/privacy, graph
saving and complete Stage 4 remain open. Evidence is retained under
measurements/parity/2026-10-09-relation-source-acceptance. Earlier partial/staged
statements describe historical checkpoints. Hardware testing is user-deferred.

| Responsibility | Native mapping and source boundary |
| --- | --- |
| RelationLoadPlan | All six strings, many flag, optional full SelectQuery and recursive children; explicit-domain Clone/PartialEq and complete derived Debug. No new serde/hash capability is invented. |
| Behavior/string planning | Resolve the entity first; group dot-separated loads by ordered relation name, then recursively validate child metadata. Missing entity/relation and child errors preserve source order. |
| Query/load planning | Preserve supplied load order and independent query clone; nested query relations recursively produce child plans. No behavior relation replaces an explicit query plan. |
| Identity collection | Numeric nonnegative signed/unsigned IDs normalize to u64 identity; other values use exact graph bucket keys. Sort/deduplicate identities and omit missing fields. Equal-key unstable-sort representative choice is not a source guarantee. |
| Flat eligibility | Both query and behavior plan trees require registered decoders recursively. Select the flat path only when every referenced target is supported. |
| Query construction | Copy supplied query or create target query, enforce target entity, foreign/child-local projection and stable sliced ID order; attach normalized parent IN identities and semantic partition only when a slice exists. |
| Compact query construction | Own the relation tree in the flat hydrator and clear nested query relations to avoid duplicate subtree fetching. Preserve other query settings and identity projection. |
| Top-N plan selection | No many/query/slice means Window. Explicit threshold zero disables probes; positive threshold compares parent count; absent threshold uses provider capability. |
| Probe dispatch | Empty parent IDs return before capability/fetch. Provider-owned batching requires capability, absent explicit threshold and metadata disabled. Otherwise probes run sequentially through the recursive child fetch entry point, clear partition and preserve per-parent slices. |
| Relation telemetry | Preserve operation name and entity/relation/count/limit/threshold/selected-plan/probe-count attributes. Parent result cardinality and failure classification remain separate from query execution. |
| Ordinary enhancement | Fetch rows, remove partition-rank marker, attach many/list or optional/null values, then recursively enhance embedded child records under their own plans. |
| Object/group/child enhancement | Skip empty parent IDs, ensure identity projection, preserve query and trace boundary, and merge by exact graph identity keys. Child-query fields extend existing parent rows only for matching identities. |
| Relation aggregates | Build the relation load first, attach empty results when no IDs, propagate cache options only under source conditions, reset ordinary projection/order/slice/relations, ensure count/group/filter, preserve trace and rename foreign-key columns after fetch. |
| Full flat hydration | Descendants hydrate before their parent relation is installed in one identity graph. Match inverse metadata exactly; decode inverse list/option and owner relations, rejecting missing u64 reverse owner IDs. Loaded-null forward optional markers remain distinct from unrequested relations. |
| Compact flat hydration | Forward to-one installs fetched targets in one batch. Reverse/many relations bucket by normalized identity and consume each bucket once, preserving original duplicate-parent behavior. Leaf and recursive paths remain separate. |
| Ordinary inverse attachment | Copy parent snapshot without the current relation; append inverse-many only to an actual list, overwrite inverse-to-one, preserve child order and choose first child for to-one. |
| Child repository scope | Keep the same checked executor/Context and extend source relation trace; do not bypass entity query behavior, metadata capture or query privacy intent. |

## Ownership and scheduling

Plan/query/parent/child state uses explicit Context resources. Async work
snapshots inputs, borrows the registered executor and joins before host-memory
reclamation. Native owner-thread dispatch replaces source Send/Sync scheduling;
this is not arbitrary concurrent Context support. Generic provider errors use
the existing copy-or-take ownership path, preserving noncopyable associated
errors. Hydration publishes into a Context-owned graph and never implicitly
extends entity lifetime beyond close. Cross-domain retention explicitly copies
all recursive plans, full queries and nested values into the destination scope.

Query intent, trace, top-N telemetry, cache propagation and graph installation
remain independent responsibilities. Source/private execution comparisons and
complete body-to-body review must prove these mappings before source acceptance;
a model writer and source inventory alone do not do so.

## Complete execution-body comparison, 2026-10-08

The complete pinned production body, lines 1–1142, has now been inspected
without treating its embedded tests as the end of API discovery. Planning and
identity/query construction map to `teaql_runtime/src/data_service/relation.cpp`; top-N decisions,
telemetry attributes and row fetching map to `detail/relation_execution.hpp`;
recursive execution maps to `detail/relation_hydration.hpp`; graph installation
maps to `teaql_runtime/src/data_service/relation_installation.cpp`; aggregate/object/child work maps
to `detail/relation_enhancement.hpp` and `teaql_runtime/src/data_service/relation_enhancement.cpp`.
The public and scoped repository entries remain in `detail/entity_service.hpp`,
with query and enhanced-entity orchestration in their independent layers.

The comparison checks more than successful attachment: empty identities return
before capability/fetch; explicit threshold zero disables probes; implicit
provider batching requires disabled metadata; explicit probes retain slices
and clear partition. Embedded relations remove rank markers and recursively
process child records in source order. A failed child leaves the taken record
empty, matching source `mem::take` behavior. Flat descendants install before
parents; inverse metadata compares all three identifying fields; reverse owner
IDs are required; forward loaded-null markers remain distinct. Compact forward
to-one installation uses the batch decoder, and reverse buckets are consumed
once, preserving duplicate-parent behavior.

Aggregate planning preserves metadata resolution before empty-input handling,
cache propagation, query resets, default count/group/filter, parent trace and
post-fetch physical-key renaming. Object and child enhancement skip empty IDs,
retain full queries and use last-row-wins identity maps. Child repositories
retain the executor, Context and inherited query intent rather than dispatch
directly to SQL. Embedded telemetry encloses recursive work and reports parent
cardinality or the exact failure category.

This completes the execution-body comparison. Installed model/integration
regression and fresh wider execution/hydration/enhancement selections are still
required before source acceptance. Full surrounding service/query/privacy and
Stage 4 composition retain their independent gates.

## Reproduced model gap and original proof

A public umbrella compile reproduces missing RelationLoadPlan Debug. The staged
writer preserves exact field order and full optional query/recursive children,
without using count summaries or translating into a new wire shape.

The actual original public oracle records 162 twice-agreeing plan models in
nine modes. It uses original SelectQuery construction/filter/comment/order/
limit/offset/threshold methods, recursive children, Clone/PartialEq and a change
to each of the nine fields. Query Value NaNs preserve source nonreflexive equality;
other full-width/Unicode query data and native-width thresholds remain exact.
The staged native probe matches all 162 live original cases.

A default-PMR-disabled ownership executable explicitly retains nested plans,
query filters/comments and children in Global, destroys the original User,
checks destination-owned query storage, formats/copies through a new User, and
rejects expired plan references before traversal. Sanitizer/MIPS and installed
CMake validation remain independent; physical-router execution is unverified.

## Current staged target evidence

Native, ASan/UBSan and MIPS/QEMU finish successfully. Each model probe matches
162/162 live original cases and each recursive ownership test passes with
default PMR disabled. Existing separate relation execution/hydration/enhancement
corpora contain 432/356/266 inputs; fresh execution-body/composition and installed
full target acceptance remain required independently of the model repair.

## Installed wider execution evidence

The rebuilt execution/hydration/enhancement and original cache/flat/event
relation selection passes 9/9 on MIPS/QEMU in 5.81 seconds and 9/9 under
ASan/UBSan in 7.31 seconds, after no-work refreshes. Leak scanning is disabled
for this sanitizer selection. Installed models additionally pass the combined
12-check native and 20-check sanitizer/MIPS selections. All 162 relation
model observations match live original Rust on installed MIPS. The full
native 343-test snapshot remains pending, as do surrounding composition gates.

## Current synchronous callback lifetime gate, 2026-10-09

The installed cleanup integration passes complete native 353/353 and fresh
ID/relation sanitizer/MIPS selections of 26/26. Relation planning models match
all 162 live original cases on native and MIPS. The final source/API review
checks public relation-load/plan/query entries, identity ordering, query
projection/slicing and complete behavior/query plan construction against the
original bodies and their recursive native implementation.

The review reproduces an independent C++ lifetime gap: relation_loads invokes
the behavior synchronously without an active quiescence boundary. A custom
behavior can close its User while that call still holds User service/borrow
state. Registry lookup has the same gap. Controlled close callbacks return
and immediately exit without accessing reclaimed memory. Public async select
behavior/policy controls already fail fast through operation quiescence.

The installed repair guards the full relation-load call and registry dispatch.
Normal planning, empty defaults, source errors and result scopes stay intact.
Ordinary exceptions release the guards before a later close. Eight new callback
contracts and complete 361-consumer integration are pending; complete relation,
Context and Stage 4 acceptance remain open. See
measurements/parity/2026-10-09-runtime-callback-scope-review.

## Installed dispatch integration reconciliation

The metadata/checker/telemetry follow-up is installed after native361, preserving
all public relation planning/execution interfaces and complete model fields.
The current new-layout selections pass 51/51 on sanitizer and MIPS, including
execution/hydration/enhancement, ordinary cache/flat/event relation source ports,
metadata callbacks and scoped service/query/examples. Current MIPS relation
models match 162/162 live original observations. Guard exception release and
trusted shutdown reads also pass independently. No source behavior is replaced
by a locally emulated query or identity-graph save.

Complete native 369 consumers are rebuilding; source acceptance still requires
that finished integration snapshot. Context and surrounding service/privacy
contracts retain their independent gates. Physical-router tests are deferred
at the user's request, so target acceptance uses cross-compilation and QEMU.
