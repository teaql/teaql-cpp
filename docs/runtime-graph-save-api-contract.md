# Runtime graph/save API review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete four-file graph/save source group is accepted at the final
2026-10-10 snapshot below. Earlier partial checkpoints describe the preceding
review; original tests/examples alone did not replace full source responsibility.

## Graph model reconciliation

The complete graph.rs production body 1–369 and both embedded trace/batch tests
were reviewed against graph.hpp and graph.cpp. Mutable EntityValues stays distinct
from MutationValues, CompactRow and EntitySnapshot. GraphOperation and ordered
GraphMutationKind remain distinct, and for_update preserves its boolean mapping.
GraphNode retains every scalar/relation/operation/comment/dirty/snapshot field,
Upsert construction, reference/remove/value/relation builders, optional identity
and comments. None versus a present empty dirty set retains different meaning.
Allocator-aware destination copies preserve every nested value and child scope.

Persistent TraceScopeToken and synchronous ScopedCommentNode retain independent
roles. Both recover only nonempty comments in parent-to-child order. Token Clone
retains all fields and parent lineage; native same-domain links borrow owned
arena objects, while cross-domain copies duplicate the parent chain explicitly.
No shared owner extends a User lifetime.

Plans retain optional root, ordered items/batches, monotonically assigned item
indices and visited identities. Push checks Debug overflow and wraps unsigned
Release values. Rebuilding batches groups only adjacent matching entity/kind/
Update-field sequences, clears old batches and uses empty non-Update fields.
Counts derive from actual batches, distinct from item len and emptiness. Sorted
update fields preserve source key order and exclude exactly the supplied set.
GraphTransactionBoundary retains Started/AlreadyActive/Unsupported with its
source model traits; execution responsibilities remain separate.

The source also derives raw structural Debug for these graph models. The initial
C++ runtime umbrella lacked corresponding writers. The recorded pre-repair
public debug_string compile check fails; the implementation below restores them.
Evidence is retained in measurements/parity/2026-10-09-runtime-graph-model-review.
Raw model diagnostics must remain separate from privacy-filtered audit/log output;
no graph serde/Hash or structured wire protocol may be invented to fill this gap.
Complete graph execution, ledger/preflight/transaction/governance ordering,
cleanup, model protocols and installed integration remain under review.


## Graph diagnostic implementation

The independent graph_debug.hpp now restores raw source-derived diagnostics for
EntityValues, GraphOperation, GraphMutationKind, GraphTransactionBoundary,
GraphNode, TraceScopeToken, ScopedCommentNode, GraphMutationPlanItem,
GraphMutationBatch and GraphMutationPlan. All source fields/order, option/tuple/
map/set shape, parent scopes and empty defaults use the existing DebugWriter;
no dependency, serde/Hash protocol or safe-log routing is added.

An actual pinned graph-model oracle observes 128 inputs twice with complete
Clone/PartialEq/default diagnostics in nine modes, including Unicode/control/
long-width numeric fields, NaNs, nested list/object/JSON/decimal/date/timestamp
values, empty versus absent fields, nested children and persistent/borrowed
trace parents. Native standalone output matches all 128. Full plan formatting
uses empty/singleton visited HashSets because source iteration order is not
prescribed; a separate native lifetime contract requires all multi-set members.
No full-model field or formatted output is normalized away.

The public umbrella includes the new writers. Focused native/sanitizer/MIPS
consumer builds and all four graph-model/existing-model checks passed. The
new lifetime contract disables default PMR, actually deletes User and compares
retained raw diagnostics through explicitly rebound Global copies of nodes,
plans, nested snapshots and token ancestry. Complete saving/execution review
remains pending; the native integration snapshot below includes these consumers.
This does not accept the
four-file graph/save group or Stage 5.


The graph model focus finishes before all four checks on each profile:
native 4/4 in 9.05 seconds, ASan/UBSan 4/4 in 13.52 seconds (leak scanning disabled)
and MIPS/QEMU 4/4 in 8.41 seconds. Post-test selected builds report no work.
This retains 128 model observations and existing graph behavior, plus actual User
reclamation and complete Global parent-token/snapshot/diagnostic storage. The later complete
381-check native snapshot includes the public umbrella consumers.

## Repository extraction surface

The original EntityDataService exposes a public graph_node_from_entity<T>
(lines 324–351), with repository/entity-descriptor mismatch rejection before
extracting model fields. It captures dirty fields, original values, deletion and
comment before consuming into_values, and delegates its own graph value parsing.
The pre-repair native public repository had no corresponding member; a concrete
executor/Entity compile check failed. The existing free graph_node_from_entity(Context,...)
implements the separately broader Audited save extraction, including loaded-field
and explicit fresh status. Those roles remain separate. The public repository member now preserves the
name check, getter ordering, metadata-driven recursive extraction, getter
overrides of reserved dirty/snapshot fields, deletion relation clearing and
comment precedence. It never calls is_new or is_field_loaded. Each captured
field and nested result is copied into the repository allocation domain.

The original public method produced 141 observations in two identical runs.
The C++ fixture uses a move-only model from another Context, deletes that
Context before observing complete diagnostics, forbids the two extra getters
and disables default PMR allocation. Inputs include 32 getter-state combinations
with four reserved-field combinations, name/metadata errors, malformed relation
values, empty relations and typed null. Complete graph diagnostics use nine
formatting modes; explicit Global copies preserve those diagnostics. Native, ASan/UBSan (leak scanning disabled) and MIPS live comparisons match all
141. Sanitizer and MIPS CTest selections each pass 1/1, with post-test no-work
builds. Complete native consumer validation passes 381/381 after all 340 build tasks
finish successfully; 423 compiled object dependency records are nonempty and
pre/post-test builds report no work.

## Retained graph execution responsibilities

Production review of data_service/graph.rs lines 548–1100 and 1187–1286
identifies independent recursive insert/upsert, reference/remove validation,
checker-driven update-command construction and scoped deletion responsibilities.
The source insert route rejects Remove, resolves one-child relations before
the parent write, handles many-child relations afterwards and rereads persisted
values. The upsert route retains forced create, missing-ID insertion, duplicate
child rejection, attachment rules, dirty-field updates and optimistic versions.
Reference validation permits only identity/version fields, rejects children and
checks existence, deletion and expected versions. Remove validation checks
identity/existence/deletion before scoped deletion. These retained private
paths cannot be assumed equivalent to the batch graph planner. Their native
responsibility reconciliation remains open; the internal entity/core graph
save adapters are restored in the follow-up below. This group and Stage 5 remain partial.


The original file explicitly retains recursive graph futures as stable internal
boundaries for generator migration (`data_service/graph.rs:1`). Their current
lack of ordinary external calls therefore does not justify deleting the roles.
The next implementation review must preserve the following entry boundaries:

| Original entry | Required behavior | Current review |
| --- | --- | --- |
| `save_entity_graph_from_internal` | Convert core EntityGraph recursively; group duplicate child relation names; preserve comments; map Save/Delete to Upsert/Remove; invoke governed graph save | Restored; 541-observation adapter focus |
| `save_entity_graph_internal` | Repository-specific typed extraction followed by graph save | Restored; 541-observation adapter focus |
| `save_entity_internal` | Skip persistence before extraction when status does not need it; deletion clears children before graph save | Restored; 541-observation adapter focus |
| `save_entity_with_comment_internal` | Apply comment and deletion state; preserve the source's absence of the no-persist shortcut | Restored; 541-observation adapter focus |
| `save_entity_graph_with_comment_internal` | Repository extraction, comment override, graph save | Restored; 541-observation adapter focus |
| `create_entity_graph_with_comment_internal` | Repository extraction, explicit Create, comment override, graph save | Restored; 541-observation adapter focus |
| `insert_graph_node_scoped` / `upsert_graph_node_scoped` | Retain independent recursive write order, reference/removal behavior, relation cardinality/attachment and authoritative row reads | Restored; 792-observation recursive-write focus |
| `validate_reference_node` / `validate_remove_node` | Preserve exact pre-I/O checks, existence/deletion/version outcomes and reference snapshot reconstruction | Restored; 1620-observation node-command focus |
| `graph_update_command` / `delete_graph_node` | Preserve checker marker cleanup, dirty projection, original snapshot/version and audit lineage before scoped writes | Restored; 1620-observation node-command focus |

Existing planner, batch executor, ledger and DynGraphSaver implementations have
separate recorded observations. Their passing fixtures do not automatically
prove equivalence for these retained entry boundaries. No generator change is
needed to restore the kernel responsibilities.


Trace timing also distinguishes the retained routes. Recursive insert handles a
Reference before constructing that node's own comment scope, so validation sees
only the parent lineage. Recursive upsert constructs the node scope first and
passes the active lineage to Reference/Remove validation. Upsert-to-insert
fallback strips the comment after retaining its scope, avoiding a duplicate
annotation. Scoped deletion receives the parent scope and reconstructs the
node comment once. These ordering rules must be observed through the original
private methods; adding validation to public plan_graph would change its own
source behavior, where Reference/Remove items are collected early.


The entity_save.rs production sections are interleaved with four test modules.
Review covers both dynamic saver methods and their transaction boundaries,
standalone extraction/value parsing, relation mutation merge, ledger discovery,
preflight cleanup and object locations, both audited ledger entry points and
inner paths, runtime error conversion, cancellation and relation-snapshot reuse.
Current implementations retain separate caller-owned and saver-owned transaction
roles. Caller-owned saving does not clear/commit/rollback its returned pending
ledger; saver-owned ledger completion clears committed changes only after a
successful transaction commit. Authoritative readback occurs through the write
executor before transaction completion. Fix-time/evidence scope and safe audit/
log publication remain separate responsibilities with their existing evidence.
Complete source acceptance still depends on restoring and observing the retained
repository paths listed above and on the final composition/cleanup snapshot.


The final native snapshot for this review passes 381/381 in 77.70 seconds.
It follows the terminal-successful 340-task consumer rebuild and includes both
new model checks and repository extraction parity. All 423 compiled object
dependency records are nonempty, with no-work before/after testing. The ledger
remains 53 accepted / 2 partial; this snapshot does not accept the retained
recursive save routes or finish stages 5–6. Physical execution remains deferred.


## Retained save adapter implementation, 2026-10-10

All six typed/core-graph save entry boundaries listed above are restored in
entity_save_adapters.hpp. A Context-owned lazy operation captures the model,
status and comment, preserves the status-only shortcut and forwards pending
work/wakes to the independent governed graph saver. Getter extraction occurs
only on first poll. Terminal/cancellation hooks detach forwarding and cancel
children before reclaiming model memory. Core conversion has its own operation
and retains every ordered child, comment and unparsed mutation field.

541 actual unchanged source-method inputs agree across two original runs and
match native, ASan/UBSan and MIPS/QEMU live execution. The full GraphNode raw
result, all getters, provider/query/event requests and errors are compared.
Three selected checks pass on each profile, including the existing 520 graph
execution observations. Lifecycle contracts disable default PMR, check pre-poll
cancellation, pending wake forwarding and exactly-once provider cancellation,
actually destroy User and compare explicitly rebound Global graph copies.
Post-test selected builds report no work. ASan leak scanning is disabled.

Evidence: measurements/parity/2026-10-10-graph-save-adapter-review. These edits
follow the earlier complete 381-check snapshot; 383 checks are registered, but
whole-project integration at this newer state remains pending. The recursive
insert/upsert/reference/remove/update/deletion entries remain open, as do the
complete graph/save group and stages 5–6. The source generator and production
dependencies remain unchanged; physical-device validation stays deferred.


## Node validation/update/deletion implementation, 2026-10-10

The four original private boundaries now have separate EntityDataServiceAccess
entries backed by graph_node_commands.hpp. Reference/remove pre-I/O checks,
strict I64 existence/deletion/version handling, authoritative Reference reset,
checker marker cleanup, metadata/dirty update projection, original snapshots
and scoped deletion trace provenance retain their source responsibilities.
The synchronous update call guards both invoking and foreign node arenas.
Asynchronous deletion explicitly copies the filtered parent trace into its User
allocation domain; it does not introduce Clone or ownership for ScopedCommentNode.

Two original runs agree on 1620 complete observations. Native, ASan/UBSan and
MIPS/QEMU live comparisons match 1620/1620; four selected checks pass on each,
including 541 earlier save-adapter and 520 execution observations. Null-default-
PMR lifecycle checks close a foreign parent-scope Context before deletion, retain
signed trace IDs/order, forward pending wakes, cancel reads exactly once on actual
User close and rebind explicit Global authoritative Reference copies. Post-test
selected builds report no work. Sanitizer leak scanning is disabled.

Evidence: measurements/parity/2026-10-10-graph-node-command-review. Only retained
recursive insert_graph_node_scoped/upsert_graph_node_scoped remain unimplemented
among the listed entries. Complete graph/save responsibility and composition,
current whole-project integration and stages 5–6 still require acceptance.
385 tests are registered; the earlier native 381-check snapshot predates these
follow-ups. Physical hardware testing remains deferred.


## Retained recursive write implementation, 2026-10-10

Both recursive insert/upsert boundaries now use recursive_graph_write.hpp.
Context-owned lazy states preserve source operation/scoping distinctions,
existence/forced-create fallback, comment stripping, one-child-parent-many order,
cardinality/target/attachment checks, canonical duplicate detection, checker/dirty
updates, strict optimistic versions and authoritative per-node reads. Upsert
checks target metadata and its ID property even for an empty many relation;
insert does not add that check. Parent local values are required even for empty
or detached many relations. No delete-missing behavior or governed batch-planner
semantics is invented for these retained methods.

Parent trace prefixes are explicitly copied into User, relation frames have stable
managed storage, pending child/read/mutation work forwards wakes and is cancelled
before arena reclamation. The synchronous ScopedCommentNode model remains distinct.
792 complete actual original-method observations agree across two source runs and
match live native, ASan/UBSan and MIPS/QEMU output. Existing 1620/541/520 matrices
remain intact. Real SQLite tests enable foreign keys and triggers to prove actual
one-child-parent-many execution, authoritative rereads, dirty/version/reference
upsert behavior and caller-owned rollback of all rows. They delete User and compare
explicitly rebound Global recursive snapshots, with default PMR disabled.

ASan/QEMU exposed a new test-fixture lifetime violation: unmanaged stack nodes
used User allocation and destructed after User reset. Those inputs now use
user.make<GraphNode>(); no production behavior or observation is weakened.
The before logs and corrected results are retained in
measurements/parity/2026-10-10-recursive-graph-write-review. All five scoped checks
pass on each profile after correction, with post-test no-work; ASan leak scanning
is disabled. All previously listed missing entry boundaries are implemented.
Full consumer regression and complete graph/source composition remain running;
the group and Stage 5 stay partial until that final acceptance evidence completes.


## Complete graph/save source acceptance, 2026-10-10

All production responsibilities in graph.rs, data_service/graph.rs,
data_service/executor.rs and entity_save.rs are reconciled. This includes every
model field/default/helper/derived diagnostic, public extraction/planning,
retained internal save/recursive/validation/update/deletion entry, batch and
ledger executor, transaction boundary, fix/evidence scope, preflight/discovery/
merge/location helpers and all audited/saver entry points. Node operations,
mutation kinds, value/snapshot/generated results and persistent/borrowed trace
models remain distinct. None versus empty metadata, strict versus converting
identities/versions, source ordering, failure and cancellation outcomes remain
observable. No unsupported wire/Hash or deletion behavior is added.

The final consumer build finishes 344 tasks plus a two-task ownership-fixture
refresh. Native passes 387/387 in 79.85 seconds; all 426 compiled dependency
records are nonempty and pre/post-test builds report no work. Fresh 61-task
sanitizer/MIPS selections plus fixture refresh each pass 44/44, respectively
26.98/24.58 seconds. Post-test selected targets report no work; ASan leak scanning
is disabled. Evidence and the complete selected test list are retained in
measurements/parity/2026-10-10-runtime-graph-source-acceptance.

The new model/extraction/adapter/node/recursive matrices retain 128/141/541/
1620/792 complete original observations. Existing plan, execution, ledger,
saver, audited, fix, governance and privacy source matrices remain covered.
Real SQLite paths assert triggers, foreign keys, versions, authoritative
transaction reads and full rollback; ownership paths assert actual User close,
scoped native quiescence and explicit Global deep copies. Unmanaged PMR stack
fixtures must destruct before User close or use Context factories; the corrected
fixtures and their before-ASan/QEMU failures are recorded without weakening output.

This accepts the complete source group and Stage 5 within the explicit C++
lifetime/owner-thread and supported-protocol contract. It does not accept Stage 6
runtime-root/final example/target requirements. Generator and dependencies remain
unchanged. Hardware execution remains explicitly deferred.


Stage 5 has six assigned source groups in the authoritative ledger: checker,
mutation policy, Context transaction scopes, events, logging/privacy/formatters
and this graph/save group. All six are accepted. The four-file count above
refers only to this graph/save source group, not to the whole stage.
