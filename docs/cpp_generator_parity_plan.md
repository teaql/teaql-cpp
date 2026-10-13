# C++ generator parity plan

Recorded: 2026-10-11. Status: active, authorized implementation goal.
The completed Context-bound Q/E migration is a prerequisite, not evidence of
complete Rust generator parity. This plan governs the remaining generator work.

## Objective and fixed references

Align the in-scope C++ entity, query, expression, validation and runtime assembly
responsibilities with the locked Rust generator. Preserve core abstractions and
semantics while applying the agreed C++ Context ownership and naming adaptations.

- Runtime reference: Rust v5.0.6, commit
  `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, from upstream.lock.json.
- Generator reference: commit
  `45e1572c4956ca963410d8ea436ead3c8857087a`, from cpp_generator.lock.json
  in the independent cpp_support workspace. Sources are inspected in the frozen
  .reference/teaql_code_gen worktree. The independent cpp_support
  implementation workspace retains its original 77b301e9b29a4928aabebdbe2ccc6d03d618f247 base.
- Inspect locked source, not an advancing branch. Record template locations and
  reconcile any template/runtime version mismatch without changing the runtime lock.
- Preserve all current uncommitted work. Keep the original teaql-code-gen checkout
  and other-language templates unchanged. Generate and reproduce artifacts; never
  patch generated headers manually.

## Scope and adaptations

SQLite only; C++20 and existing runtime dependencies. Preserve independent
runtime, data-service, SQL executor, transport and provider layers. Implement
shared mechanics in runtime before emitting thin model-specific adapters.

Application Q/E/model methods use lowerCamelCase. Kernel protocols, canonical
metadata, SQL identifiers and external names retain their authentic spelling.
Use local `Q = user_context.queries<domain::query_factory>()`; entries take no
Context argument. Execution accepts exactly one matching UserContext. Purpose
stages execution; purpose and comment remain nonblank requirements. Saving
retains its audited mutation/graph contracts.

All managed data belongs to GlobalContext or UserContext. Application code
accesses Global allocation through UserContext. Propagate nested allocators;
retention needs explicit destination copies. Preserve checked entity/list borrows,
Value/Null/NotLoaded and eager scalar snapshots. No implicit SQL, hidden owner,
singleton, default-resource allocation or lifetime extension is permitted.

MySQL, PostgreSQL, cloud, web frameworks, external cache/search services and Rust
Cargo integrations remain outside scope. Existing pagination/aggregation
optimization protocols may be exposed without adding external components.
SQLite-incompatible operations must fail explicitly and preserve their API
contract; unsupported backends are not implemented to make an operation work.
Native named-module ownership of generated domain declarations is a separate
C++ packaging task; existing runtime module consumption must keep working.

## Required capability inventory

| ID | Capability | Required alignment |
| --- | --- | --- |
| G01 | Entity helpers | Per-field changed/eval methods, load-state behavior, relation handles and mutation/original snapshot separation. |
| G02 | Checker and behavior generation | Typed model checkers, business extension scaffolds, registration and their runtime responsibilities. |
| G03 | Model-driven fixes | Required/length rules where specified by source, loaded-update checks, Context root validation/fixes, create/update time fixes and fix evidence. |
| G04 | Inheritance and polymorphism | Parent/child mappings, type filters, child projections/enhancements and graph/save responsibilities. |
| G05 | Constants and candidates | Constant initialization/seeding and typed candidate predicates, updates and E checks. |
| G06 | General Q surface | Deleted-row modes, property/criteria filters, computed/raw projections with explicit trust boundaries, return-type adaptation and projection removal. |
| G07 | Predicate/projection extensions | Negative text predicates, temporal/range helpers, function projections, aliases and source-defined ordering variants. |
| G08 | Aggregation and relationship analysis | Counts, statistical/function aggregates, relation presence/absence, relation grouping/details, roll-up and facets. |
| G09 | Execution and pagination | Page/exists/count/first variants; page/offset conveniences; continuous-page and ID-set optimization contracts. Preserve accepted one/first semantics. |
| G10 | Dynamic JSON queries | Model-aware filter, ordering, paging and nested relation decoding with deterministic validation and failure behavior. |
| G11 | E diagnostics | Full missing-load diagnostic fields and suggested preload/fix information, without concealing Null or issuing SQL. |
| G12 | Assembly and development support | Model-driven checker/behavior/runtime registration, dependency-aware sample data, accurate model-aware Assist and generation instructions. |
| G14 | Audit/privacy and identity metadata | Preserve generated audit masking/value limits and authentic entity/field identities; qualify logging provenance through existing runtime policies. |
| G15 | Shared indexed load state | Rust 5.0.6 fixed/overflow availability, shared immutable snapshots, isolated views and copy-on-write; qualified before generated checker/model access. |
| G16 | Dynamic storage and readonly properties | Trusted Context metadata, SQLite persistence, explicit NULL/delete, readonly authority, JSON/stream hydration and cleanup. |
| G17 | Request-owned intent and trace | Canonical root/nested/ledger lineage, privacy and provenance gates, readback and SQL diagnostics from 5.0.6. |
| G13 | Semantic qualification | Existing generated field types, aggregates, forward/reverse one/many relations and supported model configurations compared against actual Rust behavior. |

Every capability has a ledger entry with locked source anchors, C++ API mapping,
runtime dependency, status and evidence. Distinguish missing, partially emitted,
implemented and semantically qualified. Source inventories, method counts, naming
checks and passing existing examples do not establish behavioral parity.

## Runtime 5.0.6 prerequisite

The user changed the runtime target to 5.0.6 during P0. See
[baseline_5_0_6_upgrade.md](baseline_5_0_6_upgrade.md). New G15-G17 contracts
join the existing inventory: shared loaded-state/Checker and request-intent
mechanics belong to P1; durable dynamic storage/JSON/stream work belongs to P5.
Review interdependent SQL/graph behavior before qualification. Historical 5.0.5
corpora and acceptance labels are preserved until original calls are replayed.

## Execution phases

| Phase | Work | Exit criteria |
| --- | --- | --- |
| P0 | Create the locked source/capability ledger and representative fixtures. Audit version mismatches and existing runtime support. | Every in-scope responsibility mapped; existing uncommitted checkpoint preserved; scope exclusions explicit. |
| P1 | Entity changed/eval helpers, typed checker/behavior generation, Context/time fixes and registration (G01-G03, G14-G15, G17, assembly part of G12). | Positive and negative model fixtures, source comparisons, real audited save/check/fix behavior and accurate Assist; no duplicate governance implementation. |
| P2 | Inheritance and polymorphic generation (G04). | Parent/child decode, queries, enhancements, graph save and lifecycle checks; unsupported guard removed only after qualification. |
| P3 | Constants and candidates (G05). | Deterministic seeding and candidate Q/model/E semantics; missing/unknown candidate failures; correct centralized naming. |
| P4 | General Q, predicate/projection extensions, execution and pagination (G06-G07, G09). | All mapped APIs emitted over qualified runtime, meaningful type rejections, SQLite behavior and documented dialect limits. |
| P5 | Aggregates, relation analysis/facets and dynamic JSON (G08, G10, G16). | Group/alias/empty/null semantics, actual relation keys, facet outer-filter rules, JSON validation and runtime integration match source. |
| P6 | E diagnostics, sample data and full development/assembly support (G11-G12). | Structured diagnostics and suggested fixes, dependency-aware deterministic sample fixtures, faithful extension registration and usable Assist. |
| P7 | Close the semantic qualification matrix and integration gates (G13 and all earlier capabilities). | No unresolved in-scope ledger entry; artifacts reproduce; source comparisons and relevant compiler/native/module/lifetime gates pass. |

Within each phase: qualify generic runtime behavior, then emit adapters, regenerate
fixtures, verify actual application calls and update evidence. Do not use later
phases to waive earlier responsibilities. Routine implementation choices require
no additional approval. A source ambiguity or genuine scope conflict is recorded
and clarified while independent authorized work continues.

## Verification and completion contract

Use locked toolchains and the pinned formatter. Default to relevant native CMake/
CTest checks. Use targeted ASan/UBSan for changed lifetimes, allocators and async
cleanup; final leak scanning remains disabled. Use module consumers for public
interface changes. MIPS/QEMU is reserved for architecture/toolchain changes or
explicit requests; physical-device testing remains deferred.

Generator regressions cover shared naming and model configurations; byte
reproduction checks all checked-in fixture sets. Add focused fixtures where
necessary. Preserve original handwritten examples and their Rust comparisons.
Use meaningful negative compiler/model tests for unsupported/incorrect usage.
Wait for all builds and linkers before consuming outputs. Do not mutate generated
compiler inputs while builds or compiler gates are running.

The goal is complete only after P0-P7 exit criteria are satisfied. Report remaining
capabilities honestly; a partial implementation, phase completion, test count or
nearing a time/token limit is not completion of this goal. Commit/push and new tags
are separate actions when requested.

## Progress ledger

| Phase | Status | Evidence |
| --- | --- | --- |
| P0 | Active | 5.0.6/producer locks verified; 36 generator sources and 17 capabilities indexed; 124 runtime sources have pending dependency/semantic dispositions. |
| P1 | Active | FieldLayout, Indexed models/snapshots, sparse Checker reconstruction, supported-model Checker/behavior assembly, relation evaluation and Business-ID helpers/fixes/immutability are implemented. Direct 5.0.6 corpora cover 44 layout, 427 snapshot/row, 56 original, 274 Checker/comment, 128 generated-relation and 1,044 Business-ID inputs. Same-model entity helpers now have 1,624 direct comparisons, including every field in the types.xml Person model and optional/reverse to-one views. Another 1,548 optional-field/Checker comparisons cover custom Context resources and callback ordering, with generated SQLite/stream/lifetime integration. Another 1,620 same-model Context/time Checker and public governed-preflight observations qualify selected native serialization, source nullability/property order and fix evidence. A further 540 actual tenant/note SQLite saves compare commit, rollback, conflict and ledger clearing. Context-root assignment setters now follow source internal visibility; remaining guarded model configurations and G15/G17 acceptance remain open; see the scoped checkpoint records below. |
| P2 | Active qualification | Inherited fields have 48 paired observations; construction has 1944; SQLite row/typed enhancement has 144; source/result list/one/stream outcomes and type-column failures have 702; audited saves have 1350; forward/nested/reverse graph configuration has 2313; branched/fieldless-intermediate scalar/clock variants have 375. Another 36 parent fixed-layout API and 432 inherited stream observations pass. Default generation remains guarded because inherited Business-ID original compilation fails with 20 recorded errors; see inheritance_qualification_matrix_5_0_6.md for scoped qualifications. |
| P3 | Active qualification | 388 original/C++ candidate model/Q/E/query/save observations; constant output is qualification-only. Generated root/constant bootstrap has 99 public original/C++ cases and typed query lifecycle gates, plus 36 field/topology cases over 72 rounds. The complete original scalar/invalid-root configurations retain 17 compiler errors; C++ scalar integration passes separately. Full temporal/governance/metadata/save-cleanup rows remain in constant_qualification_matrix_5_0_6.md. |
| P4 | Active qualification | Count/exists/first/page execution, result-type adaptation, ID-set/continuous hints and distinct generated application executors have 1,440 direct 5.0.6 execution/SQL observations. General Q/predicate/projection and remaining pagination conveniences/configurations remain open. |
| P5 | Active qualification | Scalar aggregation has 1,668 direct observations including 596 SQLite metric executions and nullable field/group/count evidence; full cache/relationship/roll-up/facet execution remains open. Generated dynamic JSON has 1,181 source comparisons including 40 SQLite list queries, with explicit raw/quoted identifier adaptation and separately recorded original schema failure. Context/lowering privacy and metadata-retention gates supplement this scoped evidence. Other model configurations and complete relation/facet/readonly execution remain open. |
| P6 | Active qualification | Structured E diagnostics have 196 original diagnostic and 108 Value/Null observations, with Context-owned root/exception lifetime contracts. Sample state adds 144 original observations / 2,304 reference choices; generated execution adds 18 SQLite observations, failure paths 28, scalar/temporal fields 85 and ordinary runtime assembly 72. Generated Assist describes metadata-only registration, configuration versus schema initialization and root reuse. Complete diagnostics/sample/configuration/log/extension qualification and final integration remain open; scoped checkpoints below retain their exact limits. |
| P7 | Pending | Full generator semantic matrix is not closed. |

## Related contracts

- [Generator frontend scope](cpp_generator.md)
- [Generator integration plan](cpp_generator_plan.md)
- [Completed Context-bound Q/E change](query_expression_change_plan.md)
- [E binding and snapshots](runtime_expression_binding.md)
- [Core design parity](design_parity.md)

The machine-readable [capability ledger](cpp_generator_parity_ledger.json) records
locked template hashes, source anchors, dependency mappings and status. Inventory
entries are not parity evidence. G14 was added by the initial source audit because
generated audit/privacy metadata and identity spelling need explicit qualification.

P1 first implementation evidence: [field_layout_5_0_6.md](field_layout_5_0_6.md).
This scoped kernel checkpoint does not qualify the complete G15 capability.

Shared-state kernel evidence: [loaded_snapshot_5_0_6.md](loaded_snapshot_5_0_6.md).
The cache is explicitly scoped to the result Context; complete source/generator
integration and dynamic storage protocols remain pending.

Fixed row-shape checkpoint: 60 further direct Rust 5.0.6 CompactRow observations
qualify ordered fixed projections, primary/additional typed snapshot sharing and
isolated row values. The combined snapshot corpus has 411 inputs. Focused native
passes 3/3, sanitizer 2/2 and module import 1/1. See the fixed result-row section in
[loaded_snapshot_5_0_6.md](loaded_snapshot_5_0_6.md). This does not close generated
batch hydration, relation/schema metadata, original snapshots or Checker assembly.
P1 remains active and G15 remains partial.

Generated fixed metadata and initial/setter/row Indexed state are now implemented
with per-application Global layouts and result-Context pools. The generator's
C++-only metadata adapter matches the retained 5.0.6 School layout revision and
fixed ordering. Source metadata declarations join the frozen ledger. See
[generated_indexed_models_5_0_6.md](generated_indexed_models_5_0_6.md). Neither this
checkpoint nor existing core source observations qualify the complete generated
model, original snapshot, relation/dynamic or Checker behavioral matrix.

This generated-model checkpoint finishes with native 9/9, targeted sanitizer 3/3,
module import 1/1, selected Java 8/8, both ten-file reproduction sets and one
positive/nine negative compiler cases. Current native probes replay the 44/411
source corpora. Remaining P1 work starts with shared original snapshots, complete
typed Checker reconstruction and model changed/eval helpers; later phase scope
and feature guards are preserved.

Original compact-row promotion and generated changed/eval helpers are implemented.
Fifty-six unchanged Rust runtime call sequences compare original reads, clone,
replacement and shared ledger behavior. See
[original_snapshot_field_helpers_5_0_6.md](original_snapshot_field_helpers_5_0_6.md).
The remaining per-entity original dictionaries and typed Checker reconstruction
are still P1 prerequisites; G01/G15 remain partial.

Original-row/model-helper checkpoint gates finish native 4/4, sanitizer 4/4,
module import 1/1, selected Java 8/8, both ten-file reproduction sets and one
positive/nine negative compiler cases. No phase or top-level capability is closed
by this scoped result. The next P1 dependency is per-key original snapshot lookup
and complete typed Checker reconstruction.

Per-key original dictionaries, receiver-preserving view rebinding and the core
loaded-state hook are implemented. Generated model fixtures now exercise sparse
Indexed reconstruction through TypedEntityChecker, including readonly and explicit
fix behavior. See [keyed_originals_checker_5_0_6.md](keyed_originals_checker_5_0_6.md).
Actual generated checker logic, predefined rules, Context/time fixes, registration
and governed reconstruction remain open; this prerequisite does not close P1.

Keyed-original/sparse-Checker checkpoint: native 4/4, targeted sanitizer 4/4,
module import 1/1, selected Java 8/8 and both ten-file reproduction sets pass.
The updated 56 public-source cases include receiver-preserving view rebinding;
private dictionary behavior is separately covered by native/source-inspection
contracts. The next step remains generated Checker logic/registration and its
predefined loaded-update, Context and clock-fix responsibilities.

Generated Checker/behavior wrappers and RuntimeModule assembly are implemented for
supported models. Runtime-first Context/time fixes, source-ordered evidence and
predefined update-load checks are exercised by a new generated Tenant/Note fixture
and actual audited SQLite save. Enabling graph decoders also required named
relation metadata and runtime-backed getters; 16 additional direct Rust row inputs
raise the snapshot/row corpus to 427. See
[generated_checkers_5_0_6.md](generated_checkers_5_0_6.md). P1 stays active until the
complete source/model/governance matrix and all scoped responsibilities are proved.

Checker/registration checkpoint final gates: focused native 7/7 plus relation
hydration 1/1, targeted sanitizer 6/6, module import 1/1, Java 9/9, three eleven-file
reproduction sets and one positive/nine negative compiler cases. Public module
catalog is now 1,318 exports. Complete supported-model/source qualification and
remaining guarded features/provenance stay open; no P0-P7 exit criterion is waived.

Direct release-generated Checker helper qualification adds 220 complete diagnostic
comparisons in three naming profiles. Another 54 original runtime compositions
qualify entity-local comments separately from the receiving root request reason.
The combined corpus has 274 inputs; generated model adapters now use the keyed
comment API. See [generated_checkers_5_0_6.md](generated_checkers_5_0_6.md).
This is a scoped P1/G02/G17 checkpoint; complete canonical lineage, audit privacy,
protected-I/O rejection and remaining model configurations still need evidence.

Generated relation evaluation now lives in shared runtime helpers and model
evalX() methods; E adapters delegate those methods. Reverse XHandle() methods
expose checked query views without changing existing getters. A fourth supported
fixture covers optional forward and reverse to-one configuration. The 128-input
retained-source corpus compares actual School/Platform relation helpers without
renaming diagnostics. See [generated_relations_5_0_6.md](generated_relations_5_0_6.md).
The source comparison and native lifecycle cases do not close complete G01/G15
or P1; remaining guarded models and complete governance remain required.

Relation checkpoint gates pass native 10/10, targeted sanitizer 6/6, module import
1/1 with 1,320 catalog names, Java 10/10 and one positive/nine negative compiler
contracts. All four supported generated fixture sets have eleven files. Continue
P1 with the full field/model matrix, Business-ID helpers/fixes/immutability and
canonical trace/governance; complete optional/reverse to-one source qualification
is still required before the relation subset can close a top-level capability.

Business-ID checkpoint adds distinct field value types while preserving text
storage, explicit Context copies, profile validation, allocator slots and immutable
update checks. Generated SQLite setup registers the existing infrastructure;
source-derived metadata/default/rule adapters remain separate from other-language
templates. An unchanged original producer-generated Rust fixture supplies 1,044
direct wrapper/Checker observations, preserving raw diagnostics alongside the sole
declared resource type-name adaptation. See
[generated_business_ids_5_0_6.md](generated_business_ids_5_0_6.md). Supported helpers
have implementation/evidence; complete model configurations and canonical governed
save/trace/privacy/provenance acceptance remain open. P1 and all seventeen
top-level capabilities remain unqualified.

Business-ID checkpoint final gates: native 10/10, targeted ASan/UBSan 6/6 with
leak scanning disabled, module import 1/1 with 1,324 catalog names, selected Java
12/12 and two positive/twelve negative compiler cases. All five eleven-file C++
fixture sets reproduce; the frozen Rust fixture reproduces as 25 original files
plus provenance. Live original/C++ comparison confirms 1,044 cases under the
declared type-name adaptation. This is evidence for the supported mechanics,
not complete P1 or generator parity.

Request-intent/trace checkpoint adds validated core intent models and the distinct
RuntimeError payload, plus immutable ledger token maps and unassigned coordinates.
Scopes retain first-route graph ancestry and standalone annotations; all ordered
new IDs are allocated before command lineage recovery. The direct public-source
corpus has 633 inputs, and the original private ancestor-sharing regression passes.
See [request_intent_trace_5_0_6.md](request_intent_trace_5_0_6.md). Existing request
envelopes/derived query propagation and full SQL/privacy/provenance authority are
still required; introducing the models alone does not qualify request ownership.

Intent/scope checkpoint final gates pass native 10/10 plus the targeted mixed-owner
copy check, sanitizer 7/7, module import 1/1 with 1,334 catalog exports, Java 12/12,
two positive/twelve negative compiler cases and five eleven-file reproduction
sets. The live original public corpus matches 633/633; the unchanged original
private sharing/cleanup test passes 1/1. Plain handwritten save routing is restored
to its source behavior while generated carriers enter ledger saving. Next G17 work
must migrate the actual mandatory request envelopes and SQL/privacy/provenance
boundaries; complete qualification remains open.

Query-envelope checkpoint supersedes the preceding carrier TODO for query
requests. SelectQuery owns purpose; QueryRequest requires independent QueryIntent
and canonicalizes diagnostics from it. Context/base entry no longer derives
intent from diagnostic frames. Private root/child services and audited state-read
queries propagate explicit intent. Typed entity enhancement now binds intent
before pagination and nested relation loading. Mutation request/privacy ownership
and historical query-corpus requalification remain required G17 work.

The corpus expands to 2,681 direct original observations, including 2,048 actual
QueryRequest cases. Focused native contracts pass 9/9, targeted sanitizer gates
pass 6/6 with leak scanning disabled, and module import passes 1/1 with all 1,334
exports/seven archives verified. This checkpoint retains P0/P1 active and P2-P7
pending; none of the seventeen full capability records is declared qualified.

Final compiler API verification passes two positive and twelve negative cases
after the query-envelope and derived-scope changes.

Mutation-envelope checkpoint implements mandatory root MutationIntent, private
allocator-owned diagnostic redactions, canonical leaf traces and private
base/Context/child-service propagation. Audited services bind mutation intent
before state loads/writes; missing intent rejects before actual base executor I/O
and entity mutation preparation. Self-binding preserves the existing owned
intent/privacy without consuming its aliased source. Scope copies remain Context
borrows; explicit Global retention copies the complete request and provenance.

The direct original corpus expands to 3,626 cases, adding 945 actual mutation
request constructions and canonical trace calls. Native checks pass 7/7, plus a
focused SQLite readback failure assertion preserving both inherited secrets and
current binding policy. Native SQL batch ancestor scopes, inherited root comments,
complete future-sibling privacy and historical request probe migration remain
open G17 work. These checkpoint results do not close any full capability or phase.

Mutation-envelope final gates pass targeted ASan/UBSan 7/7 with leak scanning
disabled and module import 1/1. All 1,334 catalog exports and seven facade archives
verify without duplicate out-of-line definitions. The separate public
MutationCommand API mapping, moved-from intent states, native SQL batch scope/
privacy and complete historical probe migration remain open; P0/P1 and the
seventeen full capability records retain their active/unqualified status.

Native SQL batch checkpoint implements immutable Context ancestry, repeated-reason
identity preservation, top-level metadata comments, whole-tree privacy collection
before SQL, canonical derived-readback traces and ordered successful physical
write/SELECT metadata inside each item. Focused native SQLite/generated integration
passes 3/3; unchanged Rust mutation diagnostic source tests pass 21/21. Source
regressions and C++ counterparts are not counted as output comparisons. Complete
G17 qualification, dynamic provenance, command API/moved-from intent mapping and
historical probe requalification remain open. P0/P1 remain active.

Native batch/readback final gates pass targeted ASan/UBSan 3/3 with leak scanning
disabled and module import 1/1. All 1,334 catalog exports and seven facade archives
verify. Native SQLite/generated integration passes 3/3 after successful readback
retention was added; the unchanged Rust diagnostic suite passes 21/21. Existing
3,626 envelope observations remain a separate corpus, not native batch execution
comparisons. No full capability or phase is declared complete by these gates.

Standalone-command checkpoint implements the distinct MutationCommand type,
checked request method, compatible request data representation, both public
namespaces and named-module consumption. Explicit Global command copies remain
usable after User deletion. C++ moved-state checks retain noexcept arena moves
while rejecting invalid snapshots/bindings and preventing schema/SQL access,
including invalid later children of a batch. Focused native gates pass 6/6 plus
the SQLite boundary extension; original/C++ public observations match 4,571/4,571.
Neither these C++ misuse checks nor command construction closes the remaining
historical probe/configuration and governed/dynamic privacy qualification. P0/P1
and all seventeen top-level capability records remain open/unqualified.

Standalone-command final gates pass native 6/6 (with the additional focused SQLite
boundary check), targeted ASan/UBSan 6/6 with leak scanning disabled, and module
import 1/1. Both MutationCommand namespace exports raise the catalog to 1,336;
seven facade archives retain distinct initializers without duplicate out-of-line
definitions. The original/C++ comparator verifies all 4,571 observations live.
These are scoped API/ownership results; complete G17 and P0-P7 qualification remains
open, including historical fixtures and full governed/dynamic provenance.

Direct SQL execution checkpoint records 2,216 original/C++ output comparisons
against frozen 5.0.6, covering ordinary, transaction, guarded, nested batch,
readback, failure/pending/panic journals, complete trace nodes and safe/debug
privacy. Historical inputs replay into a new compressed corpus with explicit
entry reasons; their 5.0.5 evidence remains unchanged. Native CTest passes the
whole corpus, and a live source comparison verifies it without normalization.
This closes the executor-output evidence gap for the recorded configurations;
governed graph/ledger, dynamic authority and other historical probe/configuration
work remain required before G17 or any full phase can be qualified.

The 2,216 direct-execution observations pass native CTest 1/1, the live original
source comparator and targeted ASan/UBSan CTest 1/1 with leak scanning disabled.
The deterministic fixture occupies about 140 KB. This is additional G17 evidence;
all seventeen full capability records and P0-P7 completion remain unqualified.

Governed mutation provenance checkpoint centralizes field policy in data service
and collects mapped plan/typed-ledger current/original fields before protected
writes. Deleted originals and private state-read/readback snapshots retain their
privacy; ID/version/unmapped fields are excluded. Missing governed descriptors
reject before writes, while native SQL fallback remains conservative. Focused
native gates pass 5/5 and all 2,216 direct SQL observations still match. Ledger
fixtures acquire explicit 5.0.6 reasons without changing their checked write/
cancellation/rollback contracts. Durable dynamic operands/originals are still a
G16 dependency; complete governed source configurations remain unqualified.

Governed provenance final gates pass native 5/5, targeted ASan/UBSan 5/5 with leak
scanning disabled, and module import 1/1. All 1,336 catalog exports and seven facade
archives verify. The original/C++ SQL corpus remains 2,216/2,216. The historical
M1a workspace-move verifier still rejects the active lock because its move ledger
is pinned to 5.0.5; that historical baseline mapping remains a separate P0 refresh,
not new source/behavior acceptance. No historical commit label is rewritten to
make that check pass. Complete G17 and all full phase qualifications remain open.

P0 path/source checkpoint separates active 5.0.6 workspace ownership from the
historical unchanged-body move proof. Seven packages, 110 production source units,
124 pinned source/disposition links and 769 retained destinations verify; 15
positive/negative controls pass. Explicit historical replay passes 309 canonical
unchanged production files at fixed C++ 0.0.3 without altering original ledger
commits/hashes. This supersedes the preceding historical-verifier rejection and
resolves its baseline mismatch. No source disposition or semantic capability is
promoted by a path check; P0/P1 and the remaining full phase criteria stay open.

Final path/source gates pass native CTest 2/2 and all 15 Python structure/source
controls. Historical replay and fixed-source inventory verification pass. No C++
function body or runtime ownership implementation changed in this checkpoint;
sanitizer/architecture/module rebuilds are not required for these tooling/docs
changes. The active integrity verifier preserves every pending semantic status.


Durable dynamic core checkpoint adds distinct definitions, selections, values,
Set/Delete mutations and checked borrowed field access, separate from readonly
properties. Immutable batch/union shapes are shared inside Context lifetimes;
row payloads remain independent and explicit Global copies survive User deletion.
The unchanged source and C++ observers agree on 296/296 observations, including
initial/final geometry identities, validation and loaded/NULL/missing distinctions.
Final gates pass native 3/3, targeted ASan/UBSan 2/2 with leak scanning disabled,
and module import 1/1. All 1,368 catalog exports and seven facade archives verify;
formatting checks 766 handwritten files. Active paths now include 111 source units.
See dynamic_fields_5_0_6.md for C++ ownership/API adaptations and reproduction.
G16 remains partial: entity/load-state/dynamic-ledger integration, governed privacy,
trusted storage protocols, SQLite persistence, JSON/streams and thin generated
adapters remain required. No complete phase or capability is newly qualified.


Dynamic loaded-state/row checkpoint adds source-defined durable selection
replacement, readonly schema propagation/clearing, separate relation/dynamic read
sidecars and checked enhancement merge. Native overwrite rejects incompatible
property types or durable provenance before changing values. Default entity hooks
reject absent carriers and presentation separates framework metadata from loaded
extensions. Original/C++ row/snapshot observations match 176/176; the previous
296 dynamic-field observations still match live. Native regression gates pass
10/10, including generated API, compact row/collection and loaded-state contracts.
Module import passes 1/1 with 1,368 catalog exports and seven facade archives.
See dynamic_fields_5_0_6.md for arena sidecar retention and borrowed-source API
adaptations. Typed runtime carrier/dynamic ledger integration, storage services,
governed privacy and SQLite/JSON/stream persistence remain required. G16 stays
partial; no full phase/capability is promoted by these focused comparisons.


Final row/loaded-state ownership gates pass targeted ASan/UBSan 6/6 with leak
scanning disabled. Native 10/10, live 176/176 row/snapshot and 296/296 value-carrier
comparisons, module import 1/1, 1,368 exports, seven facade archives and formatting
of 768 handwritten files pass. Fixed-source/path/generator integrity checks pass
without promoting any of the seventeen unqualified capability records. No MIPS,
physical-router execution or generator emission change is claimed here.


Runtime dynamic-ledger checkpoint adds isolated loaded view values, separate
pending Set/Delete maps, immutable definition provenance and first originals.
Native field access remains separate; frame clearing does not roll back view data.
Definition conflicts reject before any native intent import. Governed ledger
privacy now collects dynamic-only keys, proposed operands and actual originals
under the source's Unknown policy. Original compact persistence snapshots strip
read sidecars. Typed RuntimeError::DynamicField preserves the source error payload;
unimplemented transaction storage rejects before any executor read/write and keeps
all pending changes, preventing partial native persistence and silent loss.

Original public runtime observations match 147/147. Private first-original/map
checks have native execution plus source review, not fabricated public Rust access.
Final native gates pass 6/6, targeted ASan/UBSan 4/4 with leak scanning disabled,
and module import 1/1. All 1,370 catalog exports and seven facade archives verify;
formatting checks 770 handwritten files. See dynamic_ledger_5_0_6.md for exact
scope, reproduction and remaining typed carrier/store/audit/diagnostic work.
G16/G17 and all seventeen top-level records remain partial/unqualified; no full
phase exit criterion is newly claimed from these focused state/ownership gates.


Data-service dynamic storage checkpoint preserves the low-level store/future/rows,
trusted write carrier, cursor-local plan and executor defaults from 5.0.6. Store
binding/source identities are compared without retaining another connection;
owner/selection/batch validation precedes attachment, with isolated duplicate-owner
payloads. Write Debug omits namespace, owner ID and operands and matches source
ordinary/pretty non-exhaustive output. The generic DebugWriter extension retains
existing two-argument behavior. Original/C++ pure plan observations match 49/49.

Final native gates pass 5/5, targeted ASan/UBSan 4/4 with leak scanning disabled,
and module import 1/1. All 1,390 catalog exports and seven facade archives verify;
formatting checks 774 handwritten files. The existing executor SPI fixture adds
explicit 5.0.6 query comment/purpose after its historical omission was rejected;
its provider/delegation/transaction/consumption/cancellation checks remain intact.
See dynamic_storage_spi_5_0_6.md for scope and lifetime/protocol adaptations.
Concrete SQLite storage, runtime provider configuration, actual cursor/transaction
binding, dynamic audit/readback and generated typed carriers remain required.
All seventeen top-level records and full P0-P7 completion remain unqualified.


Concrete SQLite dynamic storage checkpoint adds the original table/codecs, explicit
NULL/Delete, full-width integers, exact Decimal/f64 representation, safe driver
errors and 400-ID read batches. Ambient stores cannot write; native transaction
stores validate complete batches and track dynamic pending operations. Executor
clones preserve binding/source identity while distinct instances differ, and SQL
executor/transaction adapters forward the store. Native connection exclusion now
persists for the complete owned transaction and consuming terminal failures roll
back before releasing waiting operations.

Original real SQLite/C++ observations match 123/123, including commit/rollback,
ambient read pending, corrupt storage and ten variants. Final native gates pass
6/6, targeted ASan/UBSan 5/5 with leak scanning disabled, and module import 1/1.
All 1,390 exports and seven facade archives verify; formatting checks 778 files.
Selected historical fixtures gain explicit 5.0.6 request reasons/purposes; native
transaction assertions now require foreign cursor exclusion and rollback before
failed consuming commit returns, rather than retaining the earlier permissive
behavior. Their remaining execution/cancellation/error assertions still pass.
See sqlite_dynamic_storage_5_0_6.md for Scope, statement-cache adaptation and
reproduction. Actual Context provider/graph/audit/readback/cursor generation paths
remain pending; the runtime missing-bound-storage guard stays until that path is
connected. Neither G16/G17 nor any full phase is newly qualified.


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


P1 same-model entity checkpoint records original producer-generated types and
relations fixtures with immutable file provenance. A direct oracle identifies
optional text trimming versus raw ledger operands; the shared runtime setter
now preserves that distinction and generated adapters delegate to it. Optional
forward and reverse to-one relation views are compared using the complete
original relations crate. The original types crate has three documented
request/runtime compilation errors; an entity-only path-import harness compiles
unmodified entity files and provides narrowly scoped evidence. See
[generated_entity_helpers_5_0_6.md](generated_entity_helpers_5_0_6.md). The broader
field/model matrix and full G01/G13 qualification remain open.


P1 optional-field checkpoint adds the sixth C++ fixture family and a fourth
untouched original generated fixture. Every optional scalar family has direct
getter/changed/eval/original/clone and typed Checker comparisons; the custom hook
reads Context metadata after predefined checks. A qualified runtime identifier
casing collision is fixed in the generator and all artifacts are regenerated.
Native SQLite/Q/E/stream, explicit Global copies and actual UserContext deletion
are checked in the shared integration test. The original complete optional crate
has three documented request/sample compilation errors; its entity and Checker
files are compiled unchanged. See
[generated_optional_fields_5_0_6.md](generated_optional_fields_5_0_6.md). Full
G01-G03/G12/G13 and the remaining P1 requirements stay open.


P1 Context/time governed-preflight checkpoint adds the fifth complete original
fixture (tenant/note), which compiles unchanged against the frozen runtime.
1,620 direct original/C++ observations compare typed Checker order, active-root
validation/fixes, clocks, exact translated diagnostics, evidence and public
audited-ledger policy denial. Hidden unselected stored values are removed from
native serialization through a shared runtime helper, and generated descriptors
now follow source storage optionality and derive property order. See
[generated_checker_flow_5_0_6.md](generated_checker_flow_5_0_6.md). Successful
governed source commit, bootstrap and inherited/business combinations remain
open; this is not full P1 or generator qualification.


P1 successful governed-save checkpoint adds 540 original/C++ SQLite observations
using the complete original tenant/note generated crate and public audited-ledger
API. The matrix includes 36 successful commits, 36 trigger failures and four
optimistic conflicts; failed database state remains unchanged and successful
ledger/new-key state clears. Clock epochs and volatile IDs remain outside the
explicit projection; typed errors, exact diagnostics and persistent scalar values
remain observed. See
[generated_checker_commit_5_0_6.md](generated_checker_commit_5_0_6.md). At this checkpoint the original
Context-root setter is pub(crate), while C++ exposes it publicly. The subsequent
visibility checkpoint below closes this model-API gap. Inherited/business configurations and
bootstrap assembly are not qualified by this fixture.


P1 Context-root visibility checkpoint restricts both generated root-ID and root
object assignment to internal model/Checker code, preserving public read/query
surfaces and the actual assignment implementations. Original pub(crate) setter
responsibility is adapted to C++ private/friend access. Assist no longer suggests
application calls to these setters. Compiler and named-module consumers exercise
the boundary and actual trusted-root save; existing source observations remain
unchanged. See [context_root_visibility_5_0_6.md](context_root_visibility_5_0_6.md).
This closes that named API gap without qualifying remaining inherited/candidate,
bootstrap or full-generator responsibilities.

P2 inherited-save checkpoint adds 1,107 original/C++ SQLite observations using
the complete original inherited crate and public audited-ledger save: 122 saves,
358 optimistic conflicts, 318 Graph failures and 309 Check failures. Failed
transactions preserve all four observed tables; successful ledgers clear.
Inherited relation definitions now use the full inherited field set, and ordinary
flat hydration preserves the source distinction between graph publication and
relation availability. Related attachment failures are compared faithfully;
successful multi-node graphs and the generated attachment adapter remain open.
See [generated_inheritance_save_5_0_6.md](generated_inheritance_save_5_0_6.md).

The subsequent attachment/creation extension replaces the harness-only state
binding with generated attachRuntimeStateRecursive and a shared runtime
operation that adopts changes while preserving each entity's loaded view.
Stored forward objects recurse; reverse collections and graph lookups do not,
matching the original template. Cross-Context attachment rejects before mutation.
81 additional original/C++ observations include inherited creation, successful
creation with an existing secondary-row update and trigger rollback; the retained
save corpus now has 1,188 cases. Full nested graph shapes, inherited deletion and
complete model configurations remain open. Normal inheritance generation is
still guarded.

The inherited deletion extension adds application markForDeletion over the
existing runtime ledger marker, preserving the original derive macro's chaining
contract. Another 162 original/C++ observations qualify soft deletion, sparse
readback, marker clearing/retention, optimistic conflicts and transaction
rollback; the save corpus now has 1,350 cases. Native checks pass 3/3, module
integration 1/1 and generator tests/reproduction pass. Broader nested graph
shapes and complete model configurations still prevent removal of the guard.

The second inherited graph configuration adds an optional inherited advisor,
its nested Platform and an explicit human/boolean model. 702 original/C++
observations qualify load/evaluation/snapshot state, NULL/dangling foreign IDs,
four actual two-entity commits, source attachment ordering failures and rollback.
Runtime installers now keep unfetched detail edge-owned, mark compact forward
relations only for fetched/NULL targets and avoid a false NULL scalar in ordinary
flat rows. Native regression passes 8/8, targeted sanitizer 3/3 and module
integration 1/1. See
[generated_inheritance_graphs_5_0_6.md](generated_inheritance_graphs_5_0_6.md).
Eight generated C++ artifact sets now reproduce 88 files. Reverse graph saves
and remaining model configurations are not qualified by generating Enrollment.

The subsequent reverse-collection extension adds 1,395 actual original/C++
observations over unselected/empty/filtered/minimal/nested views, member mutation
and deletion, explicit adoption and root/member/second-member failures. Six
cases actually commit member changes; all failures preserve six tables. The
combined graph corpus contains 2,097 cases. Native application validation covers
rollback of earlier writes, retry and explicit Global list copying after actual
source Context deletion. This qualifies the tested reverse ledger-save shapes;
remaining inherited configurations and broader graph shapes keep the guard open.

The P2 closure audit replaces the blanket configuration TODO with
[inheritance_qualification_matrix_5_0_6.md](inheritance_qualification_matrix_5_0_6.md).
It found four missing projection/relation shortcuts. Thin generated adapters now
preserve the original alias and duplicate append semantics; 144 additional
construction cases bring that corpus to 1,944. Another 216 SQLite shortcut cases
bring the graph/save corpus to 2,313. These exposed and qualified the original
compact-forward ordering: install the ancestor first, hydrate descendants, then
retain the edge's exact projection instead of replacing identity-table peers.
See [generated_projection_shortcuts_5_0_6.md](generated_projection_shortcuts_5_0_6.md).
The remaining named configurations, parent-input failures and inherited stream
cleanup are required before removing the guard.

The branched/fieldless-intermediate checkpoint adds 375 same-model original/C++
observations for sibling and third-level descendants, compatible redeclaration
and inherited optional/boolean/clock fields. The 135 save cases include 21
commits and preserve failed database states. See
[generated_inheritance_variants_5_0_6.md](generated_inheritance_variants_5_0_6.md).
C++ inherited Business-ID validation/emission now shares one type per inherited
FieldDescriptor and supports actual C++ creation. The separate frozen original
combination fails with 20 undefined type/slot errors; its untouched fixtures and
dedicated compile audit preserve that gap. It does not qualify source parity.
Ten C++ artifact sets reproduce 110 files. This historical checkpoint left the
parent/stream gates and explicit original Business-ID source disposition pending.

The subsequent parent/stream checkpoint adds 36 exact original/C++ fixed-layout
metadata API comparisons and 432 public generated inherited stream outcomes.
Whole-chunk decode failure now matches source: a bad second row prevents any
entity from that chunk being delivered, while earlier successful chunks survive.
Shared runtime batch decoding handles this without model-specific generator code.
Connection reuse is checked with real queries, preserving prepared statement
caching. See [generated_inherited_streams_5_0_6.md](generated_inherited_streams_5_0_6.md).
These observations do not imply full malformed KSML frontend diagnostic parity
or identical asynchronous startup/error timing. The original inherited Business-ID
compile failure still prevents removing the default inheritance guard; P2/G04
and full generator acceptance remain partial.

P3 candidate checkpoint: [generated_candidates_5_0_6.md](generated_candidates_5_0_6.md)
records 160 model, 128 construction, 64 SQLite-read and 36 actual audited-save
observations. Existing runtime mutation/load/expression protocols implement the
semantics; generated methods are thin adapters. The default constant guard
remains until generated bootstrap and remaining configuration gates are satisfied.

P3 runtime bootstrap prerequisite: [generated_bootstrap_sequence_5_0_6.md](generated_bootstrap_sequence_5_0_6.md)
records shared sequential recipes, source conflict classification, five-attempt
retry/yield, wake forwarding and schema-owned cleanup. Model recipes, public
original initialization/SQLite comparisons and constant-guard removal remain
pending; scheduler support does not satisfy those obligations.

P3 model bootstrap checkpoint: [generated_constant_bootstrap_5_0_6.md](generated_constant_bootstrap_5_0_6.md)
records actual creation/reconciliation/no-op, ID floors, active-root conflicts
and real SQLite trigger retry across 99 cases/162 rounds. The default constant
guard remains until the named configuration and cleanup matrix is complete.


Constant field/topology checkpoint: 36 further complete original/C++ public
bootstrap cases compare 72 actual SQLite rounds (60 successes, 12 failures).
They cover signed/floating/Decimal/Date/optional fields, late ordinary roots,
multiple/dependent constants and standalone catalogs. Source Domain Root excludes
constants; C++ qualification now rejects the formerly accepted constant-only
configuration. Preserve 17 exact complete-original compiler failures for that
invalid root and boolean/JSON candidate output. See
[the scoped checkpoint](generated_constant_variants_5_0_6.md); G05/P3 remains
partial and default constant generation stays guarded.

The 5.0.6 public root-export audit still rejects missing `CompactRowLayout` and
`EntityJsonTraversal` in the core facade. Module catalog integrity is a separate
C++ packaging check and does not resolve those source API responsibilities.
Retain this P0/P7 obligation; do not relabel the historical root-export evidence.


Core result-layout/presentation checkpoint resolves the previously missing core
root names with actual shared-layout schema/equality behavior, path-local JSON
traversal and distinct borrowed/consuming Entity presentation defaults. The 200
new original/C++ comparisons cover 144 layout, 42 traversal and 14 custom-model
presentation inputs. Native 5/5 and targeted ASan/UBSan 2/2 pass; generated graph
presentation and complete source-group acceptance remain pending. See
[the scoped checkpoint](core_shape_presentation_5_0_6.md). The complete root audit
now passes core/data-service/SQL names and reports all 13 missing runtime
`document_round_trip` exports. This supersedes the earlier core-name failure
without turning declaration or module catalog checks into semantic acceptance.


Document round-trip checkpoint implements the thirteen previously missing
5.0.6 runtime exports as distinct snapshot, contextual/submitted/accepted/verified,
request, error and asynchronous service types. Public source constructors and
actual tqd1 issue/consume calls match 106 original/C++ cases, including exact
token bytes and authorization ordering. UserContext delegates through a separate
service SPI, with actual pending-child close, wake forwarding, foreign-resource
rejection and explicit Global JSON-copy contracts. Native 3/3, targeted sanitizer
2/2 and module service/import 2/2 pass; declaration/module catalog checks pass.
See [the runtime checkpoint](document_round_trip_5_0_6.md). This supersedes the
missing-name audit while retaining structured Debug, complete malformed/rotation/
size/configuration and paired original service obligations. Source dispositions
and the full P0-P7 objective remain open.


Document protocol expansion raises the corpus from 106 to 661 exact original/C++
observations: 64 actual UserContext service open/accept calls, 38 rotation,
configuration, text-boundary and duration inputs, and 453 independently authenticated
malformed payloads. Eighty cases succeed and 581 retain their original failures.
Forged token bytes, callback/authorization ordering, submitted/accepted state and
all decoder results compare directly without normalization. Fresh native and
targeted sanitizer corpus/Context selections pass 2/2 each. See
[the expanded checkpoint](document_round_trip_5_0_6.md); structured Debug/copy,
collection/token maxima, custom providers/replacement and further asynchronous
source observations remain open. This is a prerequisite checkpoint, not closure
of the seventeen capability records or any remaining P0-P7 exit criterion.

Dynamic JSON checkpoint: [generated_dynamic_json_5_0_6.md](generated_dynamic_json_5_0_6.md). G10 now has shared runtime translation, thin generated adapters and same-model source/SQLite evidence; its broader configuration and lifetime/execution matrix remains partial. Typed LIKE original-operand privacy is restored as a 5.0.6 prerequisite, with invocation-local cache provenance and separate original/native tests.

Final dynamic JSON checkpoint gates: native 10/10, targeted ASan/UBSan 4/4 (leak scanning disabled), module import 1/1, unchanged original LIKE tests 5/5, Java 34/34, two positive/four negative compiler contracts, 15 C++ artifact sets/165 files and 50 original fixture files reproduce. Formatter checks 912 handwritten files and the module catalog has 1,456 exports. G10 is partial, all 17 full capabilities remain unqualified and the complete goal remains active.

Execution extensions and application executor checkpoint: [generated_execution_5_0_6.md](generated_execution_5_0_6.md). Shared runtime mechanics preceded thin generated count/exists/first/page adapters. The frozen producer’s distinct executor/default observation/privacy role is restored, while SQL/stream/transaction mechanisms continue to delegate. G09/G12/G17 remain partial.

Final execution checkpoint gates pass: native 10/10, targeted sanitizer 5/5 (leak scanning disabled), module import 1/1, Java 35/35, three positive/three negative compiler contracts and 15 C++ artifact sets/165 files reproduce. The catalog contains 1,458 entries. Distinct domain executor registration preserves the existing constant/candidate/checker source corpora. Full G09/G12/G17 and P0–P7 remain unqualified.

General Q checkpoint: [general_query_5_0_6.md](general_query_5_0_6.md) records 892 complete query/failure observations including 112 actual SQLite compact-row calls. Context-bound general modifiers and source field removal/pagination/raw semantics now delegate through runtime. Native validation uses the existing locked SQLite 3.53.2 source instead of the older host library, preserving exact driver input diagnostics. G06/G07/G09 and complete phases remain partial.

Final general Q checkpoint gates: native 11/11, targeted ASan/UBSan 4/4 (leak scanning disabled), module import 1/1, Java 36/36, three positive/two negative application compiler checks and all 15 C++ artifact sets/165 files reproduce. Formatter checks 919 handwritten files; the export catalog has 1,458 entries. Native/sanitize/module profiles use the same locked SQLite 3.53.2 source as the reference engine, with a separately built host archive. All 17 complete capabilities and P0–P7 remain unqualified.


Scalar aggregation checkpoint: [generated_aggregation_5_0_6.md](generated_aggregation_5_0_6.md).
Generic runtime methods and thin generated field methods preserve source grouping,
count, basic/statistical/function aggregation and literal aliases, including ID
and version eligibility. The 1,668 direct observations include 596 actual SQLite
compact-row executions and source unsupported-function failures. G08/P5 remain
partial: relationship analysis, roll-up, facets and cache execution require their
own contracts and evidence; scalar construction does not qualify them.


Relationship analysis checkpoint: [generated_relation_analysis_5_0_6.md](generated_relation_analysis_5_0_6.md).
Runtime builders now retain relation count/statistics, object grouping, roll-up and
membership protocols; list/one/page pass relation aggregate descriptions to the
existing enhanced-entity layer. Raw-column/reference lookup and runtime-installed
reverse views follow the source model responsibilities. The original quoted-Line
FK attachment failure has an explicit pending decision and is not qualification.
Generated facets and actual cache/trace/lifecycle matrices remain open; G08/P5
stay partial.

Relationship checkpoint validation: native 14/14, targeted sanitizer 4/4,
module 1/1, compiler 2 positive/2 negative, selected Java 27/27 and generated
15-set/165-file reproduction pass. The 6,576 source matches include 3,498 SQLite
executions; 546 recorded quoted-FK result differences remain unqualified pending
the user decision. The replay gate reports this distinction and rejects pending
records when --require-complete is requested. No G08/P5 completion is claimed.


Facet checkpoint: [generated_facets_5_0_6.md](generated_facets_5_0_6.md).
Shared runtime list coordination and thin generated facet adapters retain full
builder snapshots, outer-filter merge/restriction and compact result maps, with
source execution-mode boundaries. G08/P5 remain partial; the quoted-Line decision
and full cache/model/asynchronous/trace/privacy matrices remain open.

Facet checkpoint validation: 1,088 exact source observations include 720 SQLite
executions and 180 source missing-comment failures. Native 6/6, targeted
sanitizer 3/3, actual module facet attachment 1/1, compiler 2 positive/2 negative,
selected Java 26/26 and 15-set/165-file reproduction pass. Full G08/P5 and all
17 capability qualifications remain open; the original quoted-Line decision is
still pending, independently of these facet checks.


Facet asynchronous/error checkpoint: [generated_facets_async_5_0_6.md](generated_facets_async_5_0_6.md).
The source matrix adds 96 real public metadata/registry and SQLite failure calls,
reaching 1,184 exact observations (816 SQLite executions). Deferred executor tests
exercise true Pending/wake and actual Context destruction at outer/facet stages,
including explicit cancellation and host-buffer lifetime checks. No production
runtime change was needed for these source errors; G08/P5 remain partial.

Asynchronous/error follow-up validation: native 4/4 and targeted sanitizer 2/2
pass, with 926 handwritten formatting checks and locked source/path integrity.
The expanded facet corpus contains 231 execution errors matching original source.
Only tests, source evidence and documentation changed; earlier generator/module
reproduction checks were not repeated. Full capability status remains partial.


Aggregation-cache checkpoint: [generated_aggregation_cache_5_0_6.md](generated_aggregation_cache_5_0_6.md).
Owned base query execution now matches the source's query-owned/explicit aggregate
normalization. Direct backend events expose actual keys, TTL, hits, SQL absence and
invalidation behavior; scope and final gates are recorded separately. Full G08/P5
remain partial, including the pending quoted-Line result disposition.

Aggregation-cache checkpoint validation: 204 exact source scenarios/588 actual
SQLite executions include 394 cache hits, 362 misses/puts and 20 governed entity
namespace invalidations. Native 5/5, targeted sanitizer 3/3, actual module 1/1,
928 handwritten formatting checks and locked reference/export/path integrity pass.
No generator/application API changed; prior Java/compiler/reproduction gates were
not repeated. Full G08/P5 and all capability qualifications remain open.

Structured E diagnostic checkpoint: [generated_expression_diagnostics_5_0_6.md](generated_expression_diagnostics_5_0_6.md). Runtime provenance and typed Context-borrowed errors replace the path-only exception, with explicit C++ syntax/English presentation adaptations. G11 and P6 remain partial.

E diagnostic follow-up qualifies selected forward/reverse/list, constant and inherited failure paths against unchanged producer code. Nested suggestions now use actual C++ `select...With(child)` syntax; Assist documents the retained source leaf-only path limitation. G11/P6 remain partial; see the diagnostic checkpoint and source observation.

E Value/Null checkpoint: [generated_expression_values_5_0_6.md](generated_expression_values_5_0_6.md). Actual generated nullable fields, selected to-one/list views and fallback callbacks compare without normalization. E list evaluation now follows the installed graph view even when its payload flag is false, restoring 18 previously differing observations. Broader G11/P6 qualification remains open.

Sample runtime foundation: [generated_sample_state_5_0_6.md](generated_sample_state_5_0_6.md). Shared scale/plan/state/reference/report/skip/error contracts precede the still-missing generated dataset execution. Source-specific ordering, load-error fallback, temporal clock and audited save rules are retained as explicit implementation requirements; G12/P6 remain partial.

Generated sample execution checkpoint: [generated_sample_execution_5_0_6.md](generated_sample_execution_5_0_6.md). Shared async model/sequence orchestration and thin generated recipes are implemented; 18 original SQLite observations cover 26 invocations and 6,880 persisted rows. Automatic relations bootstrap fails in the source; inherited sample execution succeeds in the source; C++ ancestor field omission is corrected and native inherited save/readback passes. Full inherited sample configuration qualification remains open. Full G12/P6/P7 remain open.

Sample failure/pending-save checkpoint: [generated_sample_failures_5_0_6.md](generated_sample_failures_5_0_6.md). Another 28 original/C++ observations over 42 invocations preserve save failure ordering, earlier commits, subsequent generation, exact errors and failed-reference-load fallback. Twelve native real-saver gate scenarios exercise pending/resume/cancel and actual UserContext deletion. G12/P6 remain partial.

Sample scalar/temporal checkpoint: [generated_sample_fields_5_0_6.md](generated_sample_fields_5_0_6.md). Another 85 original/C++ observations over 89 invocations and 1,980 persisted rows preserve wall-clock dates/timestamps/text, nullable scalars, exact numeric operands and generated dual-reference pools. Numeric attribute handling now covers float; JSON sample Text is retained and rejected like the source. The full quoted JSON source fixture has one retained compiler failure. Full G12/P6/P7 remain open.

Ordinary runtime assembly checkpoint: [generated_runtime_assembly_5_0_6.md](generated_runtime_assembly_5_0_6.md). Metadata-only module generation and ordinary bootstrap hooks now accompany scoped SQLite IDs and the distinct application executor. Seventy-two actual original/C++ observations cover four registration modes, repeated initialization, preservation of edited roots, inherited roots and the original one-to-one schema failure. Full G12/P6/P7 remain open.

Ordinary assembly follow-up: generated Assist documents all four module factories, `configureRuntime` versus `setup`, automatic root reuse and failure propagation while preserving runtime `ensure_schema` spelling. Generator regression passes 27/27 and all 18 workspaces / 216 files reproduce. Post-repair native scoped gates pass 6/6, ASan/UBSan 3/3 (leak scan disabled), and modules 3/3. Nine maintained native SQL/provider/entity-service contracts also pass after explicit request intent migration and source-verified purpose identity in pagination/aggregation caches. Historical Rust fixtures remain unchanged; these maintained tests do not close G17 source requalification. The broad native build and full P7 acceptance remain open.

Generated audit metadata checkpoint: [generated_privacy_metadata_5_0_6.md](generated_privacy_metadata_5_0_6.md). A failing inheritance regression exposed raw unknown parent masks becoming active for child-only fields. The C++ adapter now inherits resolved parent fields like the original producer. Twelve unchanged original model/template/C++ comparisons cover 48 entity metadata records, including normalized keys, ordered inherited masks, storage identity and explicit audit limits; fresh replay passes. Generator regression is 28/28. Full G14 runtime privacy/configuration and P1/P7 acceptance remain open.

Native integration follow-up: the complete native build now exits successfully after ten request-interface/helper-name target repairs. Eighteen generated workspaces / 216 files reproduce after the audit-mask adapter fix. Eleven maintained native contracts pass, including six generated root-reuse fixtures and five SQLite/transaction/stream/dynamic-relation/move-only contracts. Full CTest remains a separate failing gate: historical 5.0.5 corpora reject the 5.0.6 lock, and additional native/example fixtures require semantic migration. Do not claim P7 acceptance from the successful build or selected repairs.

Historical auxiliary replay: [system_time_error_scope_5_0_6.md](system_time_error_scope_5_0_6.md). A separately named 5.0.6 corpus replays all 7,056 original standard SystemTime/Duration/Result/Error observations and matches live Rust plus native C++. The historical corpus/oracle/comparator remain intact; the active CTest family selects the new comparator. Complete Value/time/G13/P7 and other historical oracle migrations remain open.

Full native integration diagnostic: [native_integration_5_0_6.md](native_integration_5_0_6.md) and its terminal per-test record preserve the initial 292/489 passes and 197 failures. All generated compiler positive/negative contracts pass. Debugger diagnosis fixes a test reading through the parent SQLite transport while its transaction owns the lease; the test now reads through the transaction, with native pass and ASan/UBSan 1/1 (leak scan disabled), and has a 30-second timeout. Five inherited C++ probes reuse prepared roots; six unchanged 5.0.6 comparison families pass again (5,316 scoped observations). Final complete native build has no work pending. Full CTest/P7 and remaining native/example/historical-oracle migrations remain open.

Runtime entry/example migration checkpoint: [runtime_intent_entry_5_0_6.md](runtime_intent_entry_5_0_6.md). Explicit fixture intent restores callback-close, foreign-operation rejection/cancellation, move-only and generic Base/Context/Entity completion paths, graph/ledger work and pending cleanup. Native selection passes 10/10; targeted ASan/UBSan 7/7 with leak scan disabled. Original handwritten business operations remain, adding mandatory query comments. Two separate 5.0.6 Rust scenario observations and 34 ordered SQL provenance entries match and reproduce using unchanged original models; old corpora/oracle remain intact. Complete G17/P7 and other native/example/historical-source migrations remain open.

Runtime query-fixture follow-up: enhanced entity, ID-set page and typed aggregate entries now supply explicit intent and preserve their hydration/decoder/cache coordination/pending/cancel/Global retention/actual Context deletion assertions. Native 3/3 and targeted ASan/UBSan 3/3 (leak scan disabled) pass, supplementing the 10 native / 7 sanitizer runtime-entry/example checkpoint. Old Rust corpora for these additional families remain unchanged and require actual source replay; complete G09/G17/P7 remain open.

Embedded child state checkpoint: [generated_child_query_states_5_0_6.md](generated_child_query_states_5_0_6.md). Actual Rust requires plain child requests for QuerySelection; runtime and thin generated adapters now enforce that boundary. Thirty-two original/C++ call-state cases and 32 callable-declaration constraints pass, including both parent states/reference categories with explicit original inner/re-purpose adaptation. Native/source 9/9, modules 2/2, generator 29/29, 18-workspace/216-file reproduction and full native build pass. No memory/architecture profile is inferred; broader G06/G08/G09/G17 and P7 remain open.

ID membership checkpoint: [generated_matching_any_5_0_6.md](generated_matching_any_5_0_6.md). Runtime matchingAnyOf/matchAnyOf and thin generated aliases match 528 original/C++ observations, with independent child snapshots and Context/Global ownership. The 144 SQLite attempts all preserve the original name-only descriptor unknown-ID compile failure; no successful row execution is claimed. Eight additional original/C++ child-purpose state cases and eight callable constraints pass; native ownership/parity 2/2 and generator 29/29 pass. Complete conversion/model coverage, successful execution policy and broader G06/P7 remain open.

ID membership final gates: native ownership/membership/general-query/source-binding regression 4/4; ASan/UBSan ownership 1/1 and membership corpus 528/528, with leak scan disabled. The first ownership sanitizer failure was a test-local Context debug string destructing after actual Context deletion; lexical scoping fixes the test and the rerun passes. Eighteen workspaces / 216 files reproduce; pinned format/check passes 982 handwritten files. Complete G06 conversion/successful-execution decisions and P7 remain open.

ID membership conversion follow-up: [generated_matching_any_5_0_6.md](generated_matching_any_5_0_6.md). Runtime now accepts direct QuerySelection/SelectQuery and different child executor types; constrained generated public bridges preserve Context/state checks without private-domain coupling. A separate 1,248-case corpus matches actual unchanged original Rust/C++ (768 direct constructions, 192 independent generated-domain constructions, 288 execution attempts preserving the original unknown-ID failure; zero successful rows). Both the new and unchanged 528-case corpus pass live replay. Native ownership/base membership 2/2, purpose-state 8+8, Java 29/29 and separate targeted ASan/UBSan ownership/conversion checks pass. Complete model/result/domain configurations, successful execution contract and G06/P7 remain open.

ID membership conversion final gates: native ownership/base/new-corpus selection 3/3; the same canonical ownership test is added to the modules manifest/preset and passes as an import-only consumer 1/1, retaining independent generated domains, null-default PMR and actual Context destruction/Global retention checks. Final Assist refresh reproduces 18 workspaces / 216 files. Pinned format/check passes 982 files; source/module-path checks pass and both frozen source worktrees stay clean. All 17 complete capabilities remain unqualified.

Generic field criteria checkpoint: [generated_field_criteria_5_0_6.md](generated_field_criteria_5_0_6.md). Runtime-first generic operators and checked Context-owned independent criteria support thin generated field/range/before/after adapters. Actual unchanged Rust/C++ comparison passes 8,160 observations, including 2,424 exact panic outcomes and 360 successful existing SQLite executions (444 rows; 72 requested cases reject before SQL). The source comparison corrected range adapters replacing filters; the test now explicitly rebinds Global access IDs after actual UserContext deletion. Native 2/2, separate targeted ASan/UBSan ownership/source gates, canonical import-only module consumer 1/1, Java 30/30, 18-workspace/216-file reproduction and pinned format/check of 983 handwritten files pass. Complete model/result/custom-conversion and G06/G07/P7 matrices remain open.

Native contract/trace migration: [native_contract_migration_5_0_6.md](native_contract_migration_5_0_6.md). A separate 20-failure diagnostic and final 21/21 selected native record preserve progression without erasing the initial full-suite record. Seventeen native contracts now reach their intended operation paths with explicit intent; candidate E catches its actual exception, cache writes use a separate service and move-only SQL logs include authoritative readback. Original Rust confirms Unicode whitespace filtering for both scope traversals, own Remove scopes and positive trace IDs; 242 dual-scope plus 336 full public graph-plan observations pass live replay. Targeted ASan/UBSan 12/12, canonical import-only modules 2/2 and pinned format/check 984 pass. SQLite ID-space, automatic audited plain entity and relation execution from the selected diagnostic, other historical-source migrations and complete G13/G17/P7 remain open.

SQLite entry/provider replay follow-up: [sqlite_entry_followup_5_0_6.md](sqlite_entry_followup_5_0_6.md). The last three fixtures from the 20-failure native diagnostic are corrected without weakening lease or intent validation. Combined native selection passes 28/28 (all 20 diagnosed families plus reentrancy/source/shared-fixture regressions), targeted ASan/UBSan 3/3 and canonical modules 3/3. Separate live Rust 5.0.6 transport and ID-space corpora match 798 + 261 actual provider observations; historical files remain unchanged, active comparators select new evidence. Pinned format/check passes 984 handwritten files. Full native/historical transaction/business-ID/source/capability and G13/P7 acceptance remain open.

SQLite lease checkpoint: [sqlite_async_lease_5_0_6.md](sqlite_async_lease_5_0_6.md). Actual unchanged Rust futures and C++ operations match 144 async scenarios, including held parent queries/writes/second begin, cancellation and actual transaction UserContext destruction corresponding to drop rollback. All 183 historical synchronous transaction inputs match live 5.0.6 in a separate corpus. Relevant native 4/4, targeted ASan/UBSan async 1/1 and pinned format/check 985 pass. The 183 historical async inputs include 69 repeated-begin and 12 outside-operation profiles whose ready-only driver needs scheduling migration; old evidence remains intact and is not qualified by the new subset. No production behavior/API change or full G13/G17/provider/P7 acceptance is claimed.

Historical async transaction scheduler replay: [sqlite_async_schedule_5_0_6.md](sqlite_async_schedule_5_0_6.md). All 183 unchanged historical async input sequences compare actual 5.0.6 futures/C++ operations through an explicit shared submit/poll/resume/exhaustion scheduler. Full observations match (75 pending submissions/resumptions, 72 active transaction drops); exact ordered historical input coverage is checked by the active composite comparator, with live 183 sync + 183 async = 366/366. Native transaction gates 5/5, targeted ASan/UBSan async schedule 1/1 and pinned format/check 986 pass. Runtime/public APIs and old source/corpus/comparator remain unchanged. Full provider/error/interleaving and G13/G17/P7 gates remain open.

SQLite business-ID recovery checkpoint: [sqlite_business_id_recovery_5_0_6.md](sqlite_business_id_recovery_5_0_6.md). Actual source replay exposed one deferred-FK commit mismatch among 198 old input calls: C++ synchronous allocation cleanup incorrectly held an async transaction owner and blocked caller recovery. Separate synchronous recovery identity now preserves original commit failure/nested BEGIN/explicit rollback and Context cleanup without affecting later leased transactions. Live original/C++ 198/198, native 5/5, targeted ASan/UBSan 5/5 and pinned format/check 986 pass. First complete native refresh was deliberately interrupted (130) before source edits; restarted full compilation and full CTest are still outstanding. No complete capability/P7 or full-build success is inferred.

Collection API source replay checkpoint: [collection_api_replay_5_0_6.md](collection_api_replay_5_0_6.md). Byte-identical historical public Rust drivers replay all 401 CompactRow and 1,291 SmartList inputs against the locked 5.0.6 source; separate corpora reproduce 1,692/1,692 outputs in a second live run. Native C++ agreement and active CMake comparator migration remain pending while the complete native refresh runs. No full capability or P7 acceptance is inferred from source-only evidence.

Generated readonly-property checkpoint: [generated_readonly_properties_5_0_6.md](generated_readonly_properties_5_0_6.md). A shared allocation-free runtime helper now underlies generated support markers and underscore-key presence/non-NULL value accessors. Actual generated Rust/C++ 243/243, related native 7/7, targeted sanitizer lifecycle 1/1, Java 30/30 and 18-workspace/216-file reproduction pass. The same native selection qualifies the newly replayed 1,692 collection observations. Complete native refresh first exits successfully at 492/492; the same generated module consumer passes 1/1; the subsequent full build/CTest chain is running. No complete capability or P7 acceptance is inferred.

Historical corpus backlog checkpoint: [historical_corpus_replay_backlog_5_0_6.md](historical_corpus_replay_backlog_5_0_6.md). The 499-test native registry contains 214 parity-script entries: 62 direct active-baseline bindings, 131 historical bindings requiring replay and 21 unresolved custom dispatchers. A static inventory records hashes/counts without claiming parity. Three additional original-call families (DataType, Entity defaults and SafeExpression) reproduce all 560 historical inputs against actual Rust 5.0.6, with unchanged logic and fixture-import-only adaptation. Native comparison/selection migration and the full build/CTest chain remain pending; complete capabilities and P7 stay open.

Extended API/source-gap checkpoint: [query_raw_like_source_gap_5_0_6.md](query_raw_like_source_gap_5_0_6.md). Nine original-call families replay 4,007 historical inputs against actual 5.0.6; eight whole families match native 2,918/2,918. Query raw like/not_like incorrectly use typed LikePattern leaves in 42/1,089 cases; the minimal correction matches 1,089/1,089 in an isolated native variant and remains staged to preserve full CTest inputs. Complete native build succeeds at 160/160; the 499-test run is active. A terminal-record tool requires exact registry/JUnit test-name coverage and preserves earlier evidence. No full capability/P7 acceptance is inferred.

Core protocol replay checkpoint: [core_protocol_replay_5_0_6.md](core_protocol_replay_5_0_6.md). Ten additional original-call families reproduce all 5,070 historical inputs. Nine complete native families match 4,901/4,901; all 169 model/row/list diagnostics expose the old CompactRow formatting structure omitting existing 5.0.6 layout/cache/read metadata. Diagnosis and exact failed paths are retained; runtime formatting and broader initialized-state qualification remain pending after the final full-run compiler gate. Current full CTest has 498/499 completed with its compiler still live; no terminal whole-suite result or complete capability/P7 acceptance is inferred.

Layered protocol replay checkpoint: [layer_protocol_replay_5_0_6.md](layer_protocol_replay_5_0_6.md). Ten original families reproduce 4,398 source calls; seven complete native families match 3,374/3,374. Request facets share 15 row-format failures, data-service fixtures abort on typed invalid intent, and event inspection proves missing safe target identity/bootstrap attribution plus raw diagnostic-redaction provenance. These are named runtime/model/projection responsibilities requiring scoped ownership and privacy/lifetime gates, not expectation edits. Current full CTest remains 498/499 with its compiler gate live; active comparator migration and full capability/P7 acceptance remain open.

Applied raw-LIKE/source-selection checkpoint: complete native CTest terminates with 355/499 passes and 144 reviewed historical-baseline failures; generated API compiler gate passes. Both whole-run records remain intact. The raw like/not_like runtime correction and explicit provenance regression are applied after termination; production Rust/C++ 1,089/1,089 and related native 30/30 pass. Twenty-five actual-source families (12,282 observations) now use separate 5.0.6 comparators. Current static registry inventory is 87 active-baseline direct/106 historical/21 custom-dispatcher entries. Audit-event identity/bootstrap/provenance, row diagnostics and invalid-intent fixture correction remain named work; full capabilities and P7 remain open.

Audit event responsibility checkpoint: [audit_event_provenance_5_0_6.md](audit_event_provenance_5_0_6.md). Safe target identity/bootstrap and raw invocation provenance are implemented with explicit nested scoped copies; shared projection and real mutation dispatch preserve source responsibility. Historical event models 112/112 and unchanged original private-module projections 104/104 match and reproduce. Native 12/12 plus real dispatch 5/5, targeted sanitizer 4/4 plus dispatch 1/1 and canonical module ownership 1/1 pass. Active event selection now uses 5.0.6 evidence. Remaining row/request diagnostics and full source/generator/configuration/integration qualifications stay open.

Shared row/request diagnostic checkpoint: [row_diagnostics_5_0_6.md](row_diagnostics_5_0_6.md). Actual core layout/cache/read metadata now formats through shared writers; invalid data-service intent returns the authentic error rather than aborting. Historical model/request-helper/service observations match 169/872/40 (1,081 total), native 8/8 passes with nonempty metadata/cache and explicit Global retention checks. Three active selections migrate to versioned evidence. Sanitizer/module gates run. Initialized Rust/C++ cache and private numeric schema-hash representation remain explicitly unqualified; no full capability/P7 acceptance is inferred.

Row/request diagnostic final scoped gates: targeted ASan/UBSan 2/2 (leak scan disabled) and canonical module consumer 1/1 pass after their builds finish; 96 canonical consumer bodies verify. Current direct-corpus inventory is 92 active-baseline/102 historical with 21 unresolved custom dispatchers. These are cumulative selection bindings, not full source or generator acceptance. Exact nonempty private schema-hash/cache diagnostic representation and remaining full-goal capabilities stay open.

Readonly schema hash checkpoint: [readonly_schema_hash_5_0_6.md](readonly_schema_hash_5_0_6.md). Core schema hashing now shares the pinned SipHash helper with existing runtime query keys, preserving former hasher method bodies and full stored u64 hash. Actual original/native cached hash plus complete nonempty schema/query/uninitialized row/layout diagnostics match and reproduce 69/69; native 5/5 passes. A limited 32-bit OpenWrt MIPS/QEMU helper/accessor smoke passes, with explicitly native Rust expected values and no full-runtime/device claim. Canonical module gate runs. Broader cache/model/configuration and initialized-state source qualifications remain open.

Readonly schema hash final scoped gate: the canonical model-debug module build completes successfully and its import-only consumer passes 1/1. The terminal evidence record replaces the running marker; broader cache/model/configuration and complete G01-G17/P7 qualifications remain open.

Runtime model replay checkpoint: [runtime_models_5_0_6.md](runtime_models_5_0_6.md). Seven complete historical input families reproduce and match actual locked Rust/native C++ 1,213/1,213. Runtime mutation bindings now wait for first clone/mutation and preserve shared first mutation; complete diagnostics use actual dynamic/ledger/carrier metadata. The observer follows original diagnostic-before-clone ordering. Native 13/13 and canonical module 1/1 pass; sanitizer build remains running. Seven active selections migrate without relabeling old corpora. Full capabilities and P7 remain open.

Runtime model final scoped gates: targeted ASan/UBSan 3/3 (leak scan disabled), canonical module 1/1 and 97 unchanged consumer/fixture bodies pass after terminal builds. The static direct-corpus inventory is now 100 active-baseline / 95 historical / 21 unresolved custom dispatchers; it is not behavioral acceptance. Pinned formatting/check covers 992 handwritten files. All 17 full capabilities and P7 remain unqualified.

Graph/governance replay checkpoint: [graph_governance_replay_5_0_6.md](graph_governance_replay_5_0_6.md). Seven complete historical public-call families match and reproduce actual locked Rust/native C++ 3,659/3,659. The shared readback validates every mapped property, and public planning delegates to it; scoped target provenance does not modify parent state or SQL binding policy. Initial 196 semantic mismatches and first scoped failures remain recorded; complete mock rows and correct safe-log inspection preserve the intended contracts. Native 16/16, targeted ASan/UBSan 5/5 (leak scan disabled), canonical module 1/1 and 97 consumer bodies pass after terminal builds. Seven active comparators migrate without relabeling historical evidence. Complete capabilities and P7 remain open.

Private save replay checkpoint: [private_save_replay_5_0_6.md](private_save_replay_5_0_6.md). Nine complete historical fix/preparation/order/governance/graph/node/adapter/recursive/ledger families match and reproduce actual original/native C++ 11,040/11,040. Access bridges append to byte-preserved originals (610 files), and registry versions/checksums retain the original lock. The historical ledger observer explicitly rejects dynamic-readback payloads rather than discarding them. Graph execution now validates plan-owned mutation intent before root structure, with an original-source difference retained and a no-I/O parent-intent regression. Native 17/17, canonical module 1/1, 98 canonical consumer bodies and 993-file pinned format/check pass. Nine active selections migrate; direct-corpus inventory is 116 active / 79 historical / 21 unresolved custom dispatchers. All complete capabilities and P7 remain open.

Layered mutation/transaction saver checkpoint: [mutation_saver_replay_5_0_6.md](mutation_saver_replay_5_0_6.md). Five complete historical families match and reproduce actual locked original/native C++ 4,710/4,710. Internal preparation retains source behavior/policy/ID/fix effects before physical intent rejection; the initial 350 differences remain recorded. Raw transaction stimuli retain absent intent through actual fallible constructors rather than an invented comment. A complete non-ledger readback fixture restores its intended assertion; first native failures remain recorded. Native 12/12, targeted ASan/UBSan 4/4 (leak scan disabled), canonical module 1/1, 98 canonical bodies and 993-file formatting pass. Direct-corpus inventory is 121 active / 74 historical / 21 unresolved custom dispatchers. Full capabilities, positive explicit raw transaction intent/configuration matrices and P7 remain open.

Typed query replay checkpoint: [typed_query_replay_5_0_6.md](typed_query_replay_5_0_6.md). Five complete historical query/stream/enhancement/decoder/ID-set families match and reproduce actual locked source/native outputs 943/943. Compact SmartList now uses the existing owned compact prepared operation; typed entity paths remain unchanged. Initial four trace differences and source import failures are preserved. Native 10/10, targeted ASan/UBSan 2/2 (leak scan disabled), canonical application/entity query module 1/1 and 107 canonical consumer/fixture bodies pass after terminal builds. Five active comparators migrate; direct binding inventory is 126 active / 69 historical / 21 custom dispatchers. Complete configurations, profile-specific boundaries and all full capabilities/P7 remain open.

Independent query capture checkpoint: [entity_query_capture_5_0_6.md](entity_query_capture_5_0_6.md). Explicit public SELECT/MUTATION option combinations add 768 matching and reproducible actual source/native stimuli, 488 provider calls, 244 captured requests and 512 stimuli rejected before provider. Source assertions preserve compact owned trace extraction, direct calls despite cache/hints and independent mutation options. The earlier default-enabled observer already captured SELECT metadata; this does not replace that evidence. Native 3/3; direct binding inventory is 127 active / 69 historical / 21 custom dispatchers. Complete capabilities, cursor build-profile boundaries and P7 remain open.

Layered query intent checkpoint: [layered_query_replay_5_0_6.md](layered_query_replay_5_0_6.md). Byte-verified original base/Context method fragments qualify 724 complete historical inputs and 2172 explicit valid/blank/padded purpose inputs against actual source/native outputs. Historical inputs retain absence of purpose and make no provider calls; explicit inputs add 1224 provider calls, with cancellation/errors and metadata call counts recorded independently. Existing metadata-call spies and provider-kind projections are retained; this does not qualify final SQL/audit projection. Native 5/5 and 993-file pinned format/check pass. Two historical comparators migrate and two explicit comparators are added; binding inventory is 131 active / 67 historical / 21 custom. Full capabilities and P7 remain open.

Relation planning replay checkpoint: [relation_planning_replay_5_0_6.md](relation_planning_replay_5_0_6.md). Complete original EntityDataService planning methods replace extracted field wrappers; 112/112 actual source/native observations reproduce, including 17 planning errors. Initial 73 differences retain shifted fixture tags and NULL identity effects. C++ shared relation keys now skip ordinary/typed NULL; mixed SQLite and all-null no-provider regressions preserve pending/cancel/global-retention contracts. Initial fixture compiler and sanitizer ownership errors remain recorded. Broader native 8/8, final changed native 1/1, targeted ASan/UBSan 1/1 (leak scan disabled), canonical module 1/1, 107 canonical bodies and 993-file format/check pass. Binding inventory is 132 active / 66 historical / 21 custom. Adjacent facet-clearing and grouped top-N source gaps are explicitly open; complete capabilities/P7 remain unqualified.

Relation planning extension checkpoint: [relation_planning_extensions_5_0_6.md](relation_planning_extensions_5_0_6.md). Actual complete original normal/compact/base methods match and reproduce 1152/1152 native observations, including 3456 planned and 1152 reusable caller snapshots. The shared child constructor clears copied facets and appends only missing group-field orders for grouped/aggregate top-N; ordinary row ID ordering remains intact. Initial 777 differences and fixture build errors remain recorded. C++ SQLite grouped per-parent counts/offset/limit exercise both capture modes; Context-owned diagnostics explicitly copy to Global before close. Native 13/13, targeted ASan/UBSan 2/2 (leak scan disabled), canonical module 2/2, 109 unchanged consumer bodies, module paths and 994-file format/check pass after terminal processes. Binding inventory is 133 active / 66 historical / 21 custom. The prior two adjacent gaps are resolved only within this matrix; complete capabilities/P7 remain open.

Native relation dispatch/enhancement checkpoint: [relation_dispatch_enhancement_replay_5_0_6.md](relation_dispatch_enhancement_replay_5_0_6.md). Byte-verified original native/enhancement method blocks match actual source/native 698 complete historical plus 111 explicit child-intent/sibling-key variants. The native branch excludes dynamic selection and recording child-fetch SPI does not qualify full physical provider/SQL/privacy. Shared runtime requires two parent IDs for provider batching, validates child intent before fetch, preserves canonical aggregate relation trace and captures/prevalidates local keys before sibling alias replacement. Initial 336/432 and 231/266 differences retain observer labels and semantic effects. Broader native 11/11, final changed native 1/1, targeted ASan/UBSan 2/2 (leak scan disabled), canonical module 2/2, 110 unchanged consumer bodies and 994-file format/check pass after terminal builds. Initial checked-borrow compile and fixture-query destruction failures remain recorded. Binding inventory is 136 active / 64 historical / 21 custom. Full hydration/dynamic/privacy/generated/model matrices and complete capabilities/P7 remain open.

Embedded hydration sibling key checkpoint: [hydration_sibling_keys_5_0_6.md](hydration_sibling_keys_5_0_6.md). Complete original enhance_query_relations_internal and actual prepared child-fetch executor boundaries match/reproduce 8 controls for nested batches, local-key aliasing and explicit inverse projection. Initial 4 aliasing differences are retained; shared embedded forest/descendant key snapshots now drive fetch and attachment. Flat modes remain unchanged. Native 9/9, targeted ASan/UBSan 2/2 (leak scan disabled), canonical module 3/3 using the same source corpus, 111 unchanged bodies, module paths and 995-file format/check pass. Fixture include and zero-case module configure failures are preserved. Binding inventory is 137 active / 64 historical / 21 custom. This does not qualify the complete 356-input historical hydration family, physical SQL, dynamic/facet/privacy/flat graph or generated/model configurations. Complete capabilities/P7 remain open.

Embedded owner facets checkpoint: [hydration_owner_facets_5_0_6.md](hydration_owner_facets_5_0_6.md). Complete original enhance_query_relations_internal/relation_facets/execute_facets paths match and reproduce 48 native observations including plain/faceted, include-all/restricted, slice, cardinality and valid/null/missing owner keys. Initial 24 missing dispatch and 12 fixture construction trace differences remain recorded. Shared hydration now invokes per-owner facets, clears page restrictions from membership and stores loaded SmartList facets through a thin existing-service/runtime adapter. Existing TeaqlRuntime/facet declarations move to a shared dependency-safe header without public identifier or existing function-body changes. Native SQLite verifies complete owner relation membership despite page limit, empty missing-key owner and explicit Global retention. Native 8/8, ASan/UBSan 2/2 (leak scan disabled), canonical module 2/2, 112 bodies, 1477 export catalog, paths and 998-file format/check pass after terminal processes. Binding inventory is 138 active / 64 historical / 21 custom. Flat graph facets, dynamic readback, async error/cancel/source-SQL/model matrices and complete capabilities/P7 remain open.

Embedded owner facet asynchronous checkpoint: [hydration_owner_facets_async_5_0_6.md](hydration_owner_facets_async_5_0_6.md). Actual complete original enhancement/facet futures match and reproduce 432/432 outputs (60 success, 132 failure, 240 cancellation), including partial parent updates, request sequences and stop counts. Public enhancement Context-close cleanup checks active host storage before all arena blocks are reclaimed. Native 2/2, targeted ASan/UBSan 2/2 (leak scan disabled), import-only module 2/2, 113 canonical bodies, 1477 exports, paths and 999-file format/check pass after terminal processes. The first private-type module fixture failure is preserved; the fixture now uses the existing exported enhancement entry without changing production exports or functions. Binding inventory is 139 active / 64 historical / 21 custom. Flat graph facet storage/readback, dynamic/physical SQL/privacy/model matrices and complete capabilities/P7 remain open.

Graph relation facet storage checkpoint: [graph_relation_facets_5_0_6.md](graph_relation_facets_5_0_6.md). Actual public unchanged source graph APIs reproduce 72/72 native observations, including canonical owners, missing keys, empty updates, whole-map replacement, list installation, fresh graph views and independent ledgers. Shared graph builder stores optional query-only facet metadata, supports readback independent of cardinality and deeply copies it into destination Context memory. Actual User destruction and original-snapshot exclusion are checked. Native 4/4, targeted ASan/UBSan 3/3 (leak scan disabled), import-only module 2/2, 114 canonical bodies, 1477 exports, paths and 1000-file format/check pass after terminal builds. Original test-only accessor/fixture allocator failures and 72 extra-field mismatches are retained; first CTest false passes are discarded after fixing the new comparator and proving a known mismatch exits 1. Binding inventory is 140 active / 64 historical / 21 custom. Flat/compact-flat query dispatch, loaded-row reconstruction, JSON view coordinates, generated model facade and full capabilities/P7 remain open.

Flat owner facet dispatch checkpoint: [hydration_flat_facets_5_0_6.md](hydration_flat_facets_5_0_6.md). Complete original flat/compact-flat methods and real registry decoders reproduce 2304/2304 native outputs, including partial graph state, shape availability, facet requests, owner-ID errors, pending/cancel and stop counts. Initial 1728 missing-dispatch differences remain preserved; the final matrix adds availability markers. Shared runtime installs facets at original per-owner/forward-batch boundaries and retains SmartList metadata through the new source-shaped graph decoder. Original installation signatures remain through overloads and native/imported function-pointer checks. SQLite all three hydration modes prove full membership despite child page limits, typed facet metadata and Global retention; pending Context close stops children before arena reclamation. Native 8/8, targeted ASan/UBSan 3/3 (leak scan disabled), import-only module 3/3, 115 canonical bodies, 1477 exports, paths and 1001-file format/check pass after terminal builds. Source/fixture compiler and pre-mutation edit-search failures remain recorded. Binding inventory is 141 active / 64 historical / 21 custom. Complete historical hydration, nested/dynamic relations, loaded-row reconstruction, JSON view coordinates, original SQL/privacy, generated model APIs, full decoder metadata and complete capabilities/P7 remain open.

Loaded-row graph reconstruction checkpoint: [loaded_row_graph_5_0_6.md](loaded_row_graph_5_0_6.md). Complete original attach/recursive flatten and real registry methods reproduce 2032/2032 native outputs (1594 success, 438 errors), including recursive facet metadata, rebuilt outer list metadata, forward foreign-key recovery, filtered identity edge views, first shared table projection, source marker boundaries and exact errors. Initial 856 differences are retained. The earlier audit overstated metadata retention: actual source preserves facets and resets other outer fields. SQLite embedded owner facets reconstruct typed graphs and retain Global copies after actual database/User destruction; Context handles expire. Native 10/10 including generated/typed integration, targeted ASan/UBSan 3/3 (leak scan disabled), import-only module 3/3, 116 canonical bodies, 1477 exports, paths and 1002-file format/check pass after terminal processes. Binding inventory is 142 active / 64 historical / 21 custom. The audited loaded-row facet/projection gap is resolved within this matrix; complete historical hydration, dynamic/JSON view/privacy/generated/model configurations and all full capabilities/P7 remain open.

Complete historical hydration replay checkpoint: [relation_hydration_replay_5_0_6.md](relation_hydration_replay_5_0_6.md). All 356 retained inputs now match/reproduce complete original/native hydration through real Context/EntityDataService/prepared QueryExecutor/registry boundaries. Raw absence of intent is retained (231 comment errors; zero executor calls). A separate 362-case explicit comment/purpose and duplicate-key matrix matches/reproduces 348 executor calls and full graph/request/telemetry/error/cancel snapshots. Initial explicit shared-target difference is preserved; flat/compact installation reuses non-ID membership while consuming ID buckets once, fixing inverse provenance and manufactured unfetched edges. SQLite verifies both owners use the fetched shared target. Native 7/7, targeted ASan/UBSan 3/3 (leak scan disabled), import-only module 3/3, 118 canonical bodies, 1477 exports, paths and 1003-file format/check pass after terminal builds. Fixture compile failures are retained. The active comparator migrates; original corpus/probe/observer remain historical. Binding inventory is 144 active / 63 historical / 21 custom. Full dynamic/JSON views, facet/decoder/model configurations, final telemetry/privacy/original SQL and all complete capabilities/P7 remain open.

JSON graph coordinate prerequisite checkpoint: [json_graph_views_5_0_6.md](json_graph_views_5_0_6.md). Complete original private/public graph storage, selected typed-option identity index, view/copy/read/debug methods reproduce 2304/2304 native observations. Shared runtime adds the view dimension and JSON-only optional index, preserves ambiguity, ordinary table priority, source replacement behavior and separate JSON/ordinary facet indexing, and carries coordinates through graph/loaded-view copies. Debug now reports actual coordinates. Explicit Global deep copies retain index/key/payload isolation after User destruction; User handles expire. Native 11/11 including existing diagnostics/hydration/facet/reconstruction, targeted ASan/UBSan 3/3 (leak scan disabled), import-only module 2/2, 119 canonical bodies, 1477 exports, paths and 1004-file format/check pass after terminal builds. State/key layout changes require consumer rebuilding; existing public identifiers remain. Binding inventory is 145 active / 63 historical / 21 custom. Actual JSON parser/capability/typed graph registry, dynamic/SQL/privacy/generated/full configurations and all complete capabilities/P7 remain open.

Typed JSON decoder commit checkpoint: [json_graph_decoder_5_0_6.md](json_graph_decoder_5_0_6.md). Shared runtime registry callbacks and weak-state view moves reproduce 24,576/24,576 actual original Rust/native observations. Full JSON parsing, recursive graph assembly, generated facades and all complete capabilities/P7 remain open.
