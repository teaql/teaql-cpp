# Request intent and ledger trace scopes for Rust 5.0.6

Status: implementation and focused qualification in progress. G17/P1 remain open.
Core query/mutation envelope ownership is implemented with focused evidence.
Complete SQL batch lineage and every privacy/provenance boundary remain required
work; this checkpoint does not establish complete G17 qualification.

## Core intent contracts

RequestKind, RequestIntentError, QueryIntent and MutationIntent preserve the
original core responsibilities. Validation checks comment before purpose and
recognizes source Unicode whitespace. Validated values preserve supplied text
without trimming it. Intent fields are private, with no default constructor;
allocator-aware copies explicitly retain them in their destination Context.
RuntimeError has a distinct RequestIntent variant instead of reducing the error
to an ordinary Graph message. Display, Debug and error codes retain the source
shape, including the public error model's field-based code selection.

## Query request ownership

SelectQuery retains allocator-owned purpose separately from diagnostic frames.
QueryRequest requires QueryIntent, exposes comment/purpose getters and supports
checked from_query construction. Explicit intent remains independent of query
fields, diagnostic ancestry and capture flags. Canonical execution traces replace
stale Comment/Purpose frames with owned intent and retain the originating scope
name and real nested route. SQL diagnostic metadata uses that canonical chain.

Base query execution returns a typed RuntimeError::RequestIntent before executor
dispatch. Context query execution no longer derives intent from user identifiers
or diagnostic frames. Entity service preparation validates before behavior/policy;
private operation and child-service copies inherit root QueryIntent independently
of SQL privacy accumulators. Captured and uncaptured request paths retain intent;
uncaptured entity requests may omit diagnostic trace frames according to source.

Audited graph planning, execution and ledger readback bind their explicit mutation
reason to the source-defined internal query purpose on a private service. This
supplies legitimate intent for state-loading queries; it is not a trace fallback.
The following mutation-envelope checkpoint migrates the independent carrier;
complete mutation privacy boundaries still require separate qualification. Historical 5.0.5 corpora remain unchanged and their
baseline checks deliberately reject them under the active 5.0.6 lock.

## Immutable scope storage and lifecycle adaptation

EntityMutationLedger stores complete TraceScopeToken pointers and a separate
unassigned_trace_scopes map. Pointers borrow Context-owned immutable tokens;
there is no shared ownership that extends a Context lifetime. Capturing a new
request replaces both maps and sets the receiving root reason from the explicit
graph entry comment. Graph traversal retains the deterministic first route,
unannotated descendants share their parent token, and standalone pending/new/
deleted items receive the root scope plus their nonblank entity-local annotation.
Durable dynamic mutation keys await G16 and are not silently considered covered.

Cross-domain copying walks parent chains iteratively, preserving shared ancestors
with a copy cache across both maps. Late identity enrichment replaces immutable
tokens only where an ID or parent changed; existing tokens remain unchanged.
One cache preserves aliases across entity keys, inherited descendants, parent
links and unassigned coordinates. Keys and mutation values remain retryable.
clear_committed clears both maps before Context reclamation.

Ledger execution now allocates the complete ordered insert set before recovering
any command lineage. An item prepared before its ancestor therefore receives that
ancestor's allocated ID. Provider execution frames remain distinct from the
recovered AuditReason scope chain; full canonical SQL traces remain G17 work.

## Save entry responsibilities

The original plain AuditedSaveExt entry does not create a generated ledger Fix
scope unless a dynamic carrier routes it to ledger saving. Generated models use
an explicit generated_ledger_carrier adapter marker while the full G16 carrier
protocol is pending. They enter the existing ledger save and operation Fix scope;
ordinary handwritten entities retain the plain entry. This replaces the earlier
overbroad blanket Fix scope, whose ordinary-entity behavior differed from source.
It does not implement or qualify durable dynamic storage through a boolean marker.

Automatic capture follows preflight/merge ordering; direct ledger saving validates
and captures intent before executor/policy/transaction dispatch. The saver fixture
now supplies an explicit entry reason required by 5.0.6, preserving its commit,
rollback, cancellation and authoritative-read responsibilities.

## Source evidence and remaining scope

intent_trace_5_0_6_cases.json now contains 4,571 direct original calls: 486 intent
values/errors, 147 branching token recoveries, 2,048 actual QueryRequest
constructions and 1,890 actual mutation constructions/canonical traces through both standalone
MutationCommand::request and MutationRequest creation routes. Envelope cases cover implicit/explicit intent, invalid entry fields,
Unicode whitespace, stale diagnostic frames, scope selection, nested routes and
all capture-flag combinations. Complete core/runtime intent Debug and Display
remain covered. No source output
normalization is applied. The unchanged original private unit test
rebinding_preserves_shared_immutable_ancestors_and_cleanup also passes. Private
capture/enrichment is separately checked through source review, corresponding
native lifecycle tests and actual ledger provider dispatch.

Native evidence covers shared ancestors, repeat enrichment without new tokens,
Global retention after actual User deletion, commit cleanup, standalone reasons,
invalid root intent and a child batch whose root ID is allocated later. These
checks qualify specific mechanics; they do not replace source-based acceptance
of complete root/nested/ledger authority, every request/SQL boundary, dynamic
storage provenance or all P0-P7 requirements.

The preceding immutable-token checkpoint passed focused native gates 10/10. The mixed Global/User ancestor-copy check passes
as an additional targeted native run. Module import passes 1/1; all 1,334 catalog
exports and seven canonical facade archives verify. Selected Java regressions
pass 12/12, and all five eleven-file C++ fixture sets reproduce. Expanded compiler
contracts pass two positive and twelve negative cases. Targeted ASan/UBSan gates
pass 7/7 with leak scanning disabled, including actual User deletion, mixed-owner
ancestor copies and ledger provider dispatch.

The query-envelope checkpoint passes nine targeted native contracts: Context
query, stream, request/policy ordering and isolated derived scopes, SQL query
execution, three data-service models/scopes, generated API graph/nested relation
execution and generated business IDs. The source corpus passes 2,681/2,681 both
at generation and through the committed-corpus comparator with a live original
Rust executable. This includes actual Global retention after User deletion.

The new destruction fixture initially left arena-backed stack temporaries alive
past User deletion. ASan detected this fixture misuse. The fixture now destroys
all User-owned temporary values before deleting that Context and checks only the
explicit Global copy afterward. This does not change Context invalidation rules.

Historical Context/stream/SQL query corpora still reject the active lock and
await separate requalification. Mutation envelope ownership, complete protected
I/O/privacy and dynamic provenance remain open. Query-envelope comparisons and
these native gates do not qualify every runtime save/query boundary or all G17
responsibilities.

Latest query-envelope gates also pass targeted ASan/UBSan 6/6 with
ASAN_OPTIONS=detect_leaks=0, including generated nested relation execution and
actual User destruction. Native module import passes 1/1, all 1,334 catalog
exports verify, and seven facade archives retain distinct initializers without
duplicating out-of-line definitions. These are scoped checkpoint results.

Final compiler API verification passes two positive and twelve negative cases
after the query-envelope and derived-scope changes.

## Mutation envelope checkpoint

MutationRequest requires explicit validated MutationIntent, including batches.
There is no command-only constructor. Checked create returns RequestIntentError;
allocator-aware construction supports a validated intent or an explicit reason.
Intent and optional diagnostic_redactions are private destination-owned values.
comment reads the root intent rather than the last diagnostic frame. Copying a
request explicitly into Global also copies nested commands, intent and private
redactions before the source User is destroyed.

execution_trace_chain replaces only the first AuditReason comment, retaining
its entity/ID and all subsequent local reasons and routes. When no such frame
exists, it inserts a typed root reason with the command's positive ID. Batch
trace materialization remains per item. The 945 direct original cases cover all
four leaf command kinds and batches, null/negative/zero/max/text IDs, Unicode
blank reasons and existing ancestor/local frames without output normalization.

Private RuntimeDataService and ContextDataService copies carry mutation intent
and diagnostic privacy into each physical request. Audited mutation services
bind the same explicit reason to state-loading QueryIntent; child services inherit
both. An unbound actual base write returns typed RequestIntent before executor
I/O. Empty base sequences retain the source's zero-operation behavior. Entity
mutation dispatch also checks intent before behavior/policy or ID allocation.
Self-binding an owned intent/privacy object preserves it without destructing the
aliased source. Privacy never becomes a command value or ambient Context cache.

Single-statement SQL metadata uses canonical mutation traces and owned comments.
Inherited redactions are retained when current statement bindings are collected,
including readback/failure diagnostics. This implements carrier propagation; it
does not yet qualify complete native SQL batch ancestor scopes, top-level comment
propagation, future-sibling privacy classification or dynamic-field provenance.
Historical request fixtures/probes still require explicit entry reasons and
5.0.6 source requalification. Complete G17/P1/P7 acceptance remains open.

Mutation-envelope native gates pass 7/7, including actual generated model graph
save/query execution and business-ID integration. The focused SQL failure test
also proves inherited privacy survives current binding classification during
readback. The committed corpus matches the live original Rust executable and
C++ adapter 3,626/3,626. Source inventories and all seventeen complete capability
records remain unqualified beyond their separately recorded scoped evidence.

Mutation-envelope final gates pass targeted ASan/UBSan 7/7 with leak scanning
disabled and module import 1/1. All 1,334 catalog exports and seven facade archives
verify without duplicate out-of-line definitions. The separate public
MutationCommand API mapping, moved-from intent states, native SQL batch scope/
privacy and complete historical probe migration remain open; P0/P1 and the
seventeen full capability records retain their active/unqualified status.

## Native SQL batch ancestry and readback checkpoint

MutationExecutionScope snapshots top-level MutationIntent. BatchIntentScope nodes
are immutable Context-owned parent links; they carry no invented batch entity
identity and cannot extend their owning Context lifetime. Adjacent equal reasons
share one node. Statement materialization prefixes real batch ancestry, replaces
a repeated root/local reason in the correct slot and preserves its actual item
ID. Logical mutation and nested batch summaries retain the top-level comment;
physical trace ancestry remains per statement.

Before the first physical statement, batch_mutation_redactions traverses the
whole request tree, copying trusted private redactions, current and old values,
field policies and target IDs. It queries descriptors but performs no SQL compile
or transport work. Unknown descriptors remain conservatively classified rather
than causing an early compile failure. Captured binding intent remains local to
its statement; inherited batch provenance is added independently. No ambient
Context registry/cache owns this invocation state.

Readback carries canonical Comment/Purpose frames with the source purpose
"verify the persisted mutation result", followed by the real typed lineage.
Successful SELECT metadata is retained even without an observer. A successful
mutation item keeps its logical summary and ordered physical write/SELECT children;
native batches keep the item grouping. Failure observers retain the existing
journal order, without inventing a future statement or duplicating a successful
item. Retention into Global copies metadata/provenance before actual User deletion.

Focused native checks cover four repeated/distinct reason arrangements, real
item ID retention, top-level comments, no-observer physical readback grouping,
first-readback failure and privacy of future sibling old masked/credential values
without treating a declared plain value as a secret. They run through actual
SQLite execution and generated API/business-ID integration. The unchanged
original Rust teaql-sql mutation_diagnostic_tests suite passes 21/21, including
native_batch_repeated_root_is_one_slot_with_real_item_identity,
inherited_batch_provenance_covers_old_values_and_failed_readback_without_changing_binds
and successful_batch_readbacks_keep_item_grouping_and_local_lineage. These original
regressions and native counterparts are distinct evidence; they do not represent
21 C++/Rust output comparisons or complete G17 acceptance.

The preceding batch-scope TODO describes the earlier checkpoint. Current mechanics
are implemented with focused evidence; full source/output matrix, dynamic storage,
standalone command mapping, moved-from intents and historical fixture migration
remain required work.

Native batch/readback final gates pass targeted ASan/UBSan 3/3 with leak scanning
disabled and module import 1/1. All 1,334 catalog exports and seven facade archives
verify. Native SQLite/generated integration passes 3/3 after successful readback
retention was added; the unchanged Rust diagnostic suite passes 21/21. Existing
3,626 envelope observations remain a separate corpus, not native batch execution
comparisons. No full capability or phase is declared complete by these gates.

## Standalone command and C++ moved-state checkpoint

MutationCommand is a distinct allocator-aware public type for insert/update/
delete/recover/batch payloads without an entry intent. Its checked request method
validates the reason before consuming a staged command. Const commands can issue
new snapshots; an explicit Global copy retains complete payloads after actual
User deletion. MutationRequest keeps its existing public data member backed by
the same variant representation, preserving the C++ compatibility surface without
adding a reference alias, duplicated payload or implicit shared ownership.

Both public namespaces and the named data-service module expose MutationCommand.
Standalone command Debug uses the command variant shape; complete envelope Debug
retains independent intent and diagnostic provenance. The source matrix now
matches 4,571 direct original observations, adding 945 MutationCommand::request
calls alongside the prior 945 direct mutation-envelope calls. Invalid normal
entry fields still return the source RequestIntentError.

Rust prevents use of a moved value statically. In C++, a moved-from object remains
addressable. Private intent fields prevent fabrication but ordinary noexcept move
can leave the source text empty. Validation therefore rejects reuse at scoped
allocator copies/moves, request assignment, service binding and SQL execution
boundaries. C++ object-state misuse throws invalid_argument carrying the intent
error message; this adaptation does not add a fabricated Rust error variant.
Ordinary move remains noexcept as required by assign_scoped's prepare/commit
memory-safety contract. Validation of valid scalar intents allocates no temporary
strings or object table entries. Batch validation walks descendants before backend
work so a valid parent cannot admit partial I/O through a consumed later child.

Native tests preserve the previous valid service binding after rejected reuse,
reject an invalid explicit intent before consuming its command, retain a reusable
Global command, and verify consumed query/mutation requests plus invalid later
batch children cause zero schema lookups and zero SQLite transport reads/writes.
Scoped fixture snapshots reject misuse before creating third-party executor work;
third-party executors remain responsible for their own direct low-level entry
contracts. This checkpoint does not establish complete request/provider parity.

Standalone-command final gates pass native 6/6 (with the additional focused SQLite
boundary check), targeted ASan/UBSan 6/6 with leak scanning disabled, and module
import 1/1. Both MutationCommand namespace exports raise the catalog to 1,336;
seven facade archives retain distinct initializers without duplicate out-of-line
definitions. The original/C++ comparator verifies all 4,571 observations live.
These are scoped API/ownership results; complete G17 and P0-P7 qualification remains
open, including historical fixtures and full governed/dynamic provenance.

## Direct SQL execution qualification for 5.0.6

sql_execution_5_0_6_cases.json.gz records 2,216 direct public executor observations
from unchanged original Rust 5.0.6 and the C++ adapter. The historical ordinary/
transaction/kind input cases were replayed with explicit per-request entry reasons
into this separate corpus. Existing 5.0.5 inputs, observations and commit labels
remain unchanged. Newly constructed cases cover repeated/distinct nested reasons,
ancestor/item identities, future sibling old values, unknown descriptors and
trusted invocation-local redactions at root/nested/child/sibling levels.

The matrix compares complete trace nodes, result/statement metadata, binding
positions and values, exact error text, pending/cancellation/panic outcomes,
observer journals, guarded readbacks, physical write/SELECT grouping, secret lists
in safe/debug modes and dialect-kind observation where requested. No source output
normalization is applied. The observer is in an isolated current Rust fixture crate;
the frozen implementation and original generator checkout are not modified.
Trusted redaction JSON keys are fixture instructions to construct the private
invocation carrier; they are not production command/wire options.

The retained deterministic gzip artifact is about 140 KB rather than 7.5 MB of
repeated pretty-printed JSON. Generation checks the frozen runtime first, calls both
executables, verifies every observation and then writes the corpus. CTest
sql_execution_5_0_6_rust_parity checks the active corpus; --rust on its comparator
also verifies the source observations live. Native CTest passes 1/1 covering all
2,216 cases. This is execution/output qualification, distinct from the earlier
21 unchanged Rust unit tests and 4,571 intent/command/token observations.

Full G17 still requires governed runtime/graph/ledger privacy, dynamic provenance,
remaining historical request/query probes and complete source configurations.
Neither the new executor corpus nor the prior focused checks closes P0-P7.

The final direct-execution corpus also passes targeted ASan/UBSan CTest 1/1 over
all 2,216 inputs with ASAN_OPTIONS=detect_leaks=0. All compiler/linker and oracle
processes reached terminal success before artifact comparisons. Frozen runtime
verification, generator ledger integrity and pinned C++ formatting checks pass.
Modules and MIPS are not re-run for this comparison-adapter/corpus-only checkpoint.

## Governed plan and typed-ledger provenance checkpoint

SqlIntentRedactions::field_policy is the shared data-service responsibility used
by SQL bindings and governed runtime collection. Moving the existing classifier
here preserves credential/name/column classification, declared-mask rules and
unknown-field behavior; the SQL helper delegates without introducing a second
policy implementation or merging executor/runtime layers.

Before executing a graph plan, the private service requires each batch descriptor
and classifies every item's mapped current and old fields. Before typed ledger
execution, it collects keys from deletes and current changes, requires their
descriptors and classifies mapped original/current fields. ID, version and unmapped
metadata are excluded exactly as in the original runtime collector. This is
different from native SQL batch fallback, where missing descriptors conservatively
classify values; governed plans reject missing metadata before a physical write.

Collected provenance is separately copied into private mutation invocation data
and the mutable state-read ancestry accumulator. Child services snapshot it;
physical write requests carry it independently of values, while query diagnostics
inherit it before runtime projection. Outer direct/transactional ledger-save
services retain the same initial privacy for authoritative readback. No ambient
UserContext cache/registry owns the per-invocation data. Explicit Global retention
copies strings and policy values before actual User deletion.

Current typed ledger collection does not implement durable dynamic mutation
operands/originals. G16 must add its own keys and conservative operand policies;
this checkpoint must not be read as dynamic provenance or complete G17 acceptance.

Focused native checks cover mapped old/current/deleted values, masked versus
credential/plain fields, exclusion of unmapped/ID/version data, whole-plan privacy
on a physical Context/base request, independent state-read snapshots, missing
future descriptors and actual Global retention. Ledger fixtures now supply the
explicit root reason required by 5.0.6 rather than inheriting trace/Context values;
their required-create, cancellation, version and rollback behaviors still pass.
Native request/ledger/generated/SQL gates pass 5/5. The full 2,216 direct original
SQL observations still match after extracting the shared classifier. Complete
governed public source-output/configuration qualification remains open.

Governed provenance final gates pass native 5/5, targeted ASan/UBSan 5/5 with leak
scanning disabled, and module import 1/1. All 1,336 catalog exports and seven facade
archives verify. The original/C++ SQL corpus remains 2,216/2,216. The historical
M1a workspace-move verifier still rejects the active lock because its move ledger
is pinned to 5.0.5; that historical baseline mapping remains a separate P0 refresh,
not new source/behavior acceptance. No historical commit label is rewritten to
make that check pass. Complete G17 and all full phase qualifications remain open.
