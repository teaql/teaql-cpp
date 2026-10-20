# Runtime data-service shared types: Rust 5.0.6 source review

Current requalification: **pending_review** after the [independent privacy preparation counterexample](parity/context_privacy_preparation_before_5_0_6.json.gz). The accepted cohort paragraph below records the previous checkpoint, whose callers bound intent before privacy.

Historical source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: the complete 333-line `teaql-runtime/src/data_service/types.rs`, SHA256
`2a5a666d476fd77772d6b7ea71cb53754db18063ef1f823f216806b033127bf9`.
The source remains pending review in the active disposition ledger. This
reconciliation and its scoped witnesses do not accept the other runtime
data-service files, complete graph execution or generator phases.

The [historical service review](runtime_data_service_api_contract.md) covers
the earlier 166-line 5.0.5 file. Its earlier integration results remain
historical. The current diff adds mutation intent/privacy to the two lower
service wrappers, private query intent, plan/ledger privacy collection and
mutation-derived state-read intent. The existing statement-inheritance and
metadata-view responsibilities still require reconciliation on the new baseline.

## Complete responsibility map

| Review | Original contract | Current C++ mapping and boundary |
| --- | --- | --- |
| RT01 | Three separate service carriers | `RuntimeDataService`, `ContextDataService` and `EntityDataService` retain independent responsibilities in `declarations.hpp` and `entity_service.hpp`. Metadata/executor access does not collapse into the entity facade or provider. |
| RT02 | Explicit origins and lifetime | Rust borrows become checked Context resource borrows. Owned strings, trace vectors and intent/privacy copies use the destination Context. Constructing a wrapper cannot extend the Context or executor lifetime. |
| RT03 | Mutation state in lower wrappers | Runtime and Context wrappers independently carry optional validated mutation intent and privacy. Conversion to the lower adapter retains both. These fields do not become public query-builder defaults or application request JSON. |
| RT04 | Fresh entity service | `for_executor` binds the supplied executor rather than resolving an ambient connection. A fresh service has empty trace, no private request intent, no query privacy and no mutation intent/privacy. The additional C++ resource origin supplies ownership validation, not a second application execution argument. |
| RT05 | Trace replacement | `with_trace_context` replaces the complete sequence. The C++ setter copies into its Context, adapting Rust's moved vector. Trace nodes, including AuditReason nodes, cannot grant request intent. |
| RT06 | Plan privacy collection | `mutation_privacy.cpp::graph_plan_privacy` requires each batch descriptor before processing its items. Current and old mapped values are classified in source order; unmapped fields and descriptor-defined ID/version properties are excluded. Binding preserves separate query and mutation snapshots. |
| RT07 | Ledger identity union | `entity_ledger_privacy` visits deleted keys, scalar change keys and dynamic-only change keys. Descriptor lookup remains mandatory before classification, even for a key with no scalar pending row. Original and current mapped scalar values both contribute. |
| RT08 | Dynamic privacy | Pending Set values and present dynamic originals contribute Unknown policy, including when plaintext diagnostics are enabled. Delete and Set(NULL) remain different mutations. A deleted entity retains its original mapped scalar provenance after pending changes are cleared. |
| RT09 | Mutation-derived read scope | The access adapter binds the validated mutation comment and exact purpose `runtime: load state for an audited graph mutation` to a private state-read scope. It retains the trace and propagates mutation intent without modifying the reusable parent repository. |
| RT10 | Request intent precedence | Private owned query intent wins; otherwise comment/purpose must come from the current query. Both remain required even with logging disabled. Supplied valid whitespace is retained, while blank values fail with the original error. |
| RT11 | Query privacy snapshot | A snapshot is absent for a reusable unscoped repository and is a separate value for a private scope. Rust's mutex/poison recovery is part of its thread-capable implementation; the existing approved C++ Context owner-thread contract uses Context-owned storage and checked borrows, without a cross-thread promise. |
| RT12 | Per-statement provenance | `mod.cpp::record_query_metadata` clones metadata, recursively visits statement leaves and accumulates normalized provenance sequentially. Each statement uses its own parameters, SQL text and policies; positional policies are not interpreted against a sibling statement. Input metadata remains unchanged. |
| RT13 | Publication boundary | With no private scope, metadata delegates directly to Context. With a scope, provenance updates precede Context publication. The Rust scope lock ends before callbacks; the C++ path retains the existing Context callback/quiescence boundary. Sensitive-data projection remains the Context/logging responsibility. |
| RT14 | Query observer | Observer presence follows select metadata capture and does not itself execute a query. The callback delegates to the same scoped record path. C++ observer storage uses the explicit Context allocation domain. |
| RT15 | Live metadata view | `UserContextMetadata` delegates entity lookup, enumeration and metadata publication to its Context. An absent metadata store yields an empty list. C++ enumeration rejects a destination that would let borrowed descriptors escape their origin. |
| RT16 | Independent diagnostic switches | Query-debug and execution-metadata capture follow the select option. Mutation observer presence follows the separate mutation option. Neither switch grants missing query or mutation intent. |
| RT17 | RelationLoadPlan model | All six string fields, `many`, optional query and recursive children remain distinct. Clone, equality and complete structural Debug are preserved. C++ copies require a destination allocator; the public model is not replaced by an executor-specific plan. |

The plan/ledger collection and binding are separate C++ internal helpers. Actual
graph/ledger callers first create a private service and bind validated mutation
intent, then collect and bind privacy. The witnesses follow that order. This
does not add a Context argument to generated Q entries or an extra dependency
argument to application execution.

## Actual source and C++ witnesses

The [private-source replay](parity/runtime_service_types_original_5_0_6.json.gz)
appends one caller test to a byte-checked complete runtime view. No original
function, type, test or vector body changes. This file has no embedded original
unit tests; the new caller is identified separately. The source view uses exact
reference registry versions/checksums and omits only the existing disabled
external-exporter development dependencies.

| Family | Cases | Observations |
| --- | ---: | --- |
| Intent | 64 | Missing, empty, blank and padded comment/purpose; owned overrides; misleading AuditReason trace; no implicit mutation or privacy scope. |
| Metadata | 8 | Absent/present metadata and all four select/mutation switch combinations; lookups, enumeration and observer presence. |
| Plan privacy | 8 | Current/old values, excluded identity/version/unmapped values and a missing future descriptor; separate query/mutation provenance and unchanged parent scope. |
| Ledger privacy | 12 | Dynamic Set, Set(NULL) and Delete, scalar changes and entity deletion; original values and forced Unknown provenance. |
| Statement inheritance | 4 | Scoped/unscoped and flat/nested statement trees, independent parameter policies, unchanged input metadata and parent scope, and Context log delivery. |

The [C++ replay](parity/runtime_service_types_cpp_5_0_6.json.gz) freshly compiles
the complete new probe on native and canonical imported frontends. Both match
all **96** actual-source outcomes without normalization. The original helper
methods perform every intent, privacy and metadata operation. Test executors
reject database query/mutation calls. Both C++ links use runtime and lower
layers without SQLite or its provider, and default PMR is disabled in the probe.
The complete probe, unchanged shared fixture, commands, output and binary/archive
hashes are retained. Linked archives remain unchanged throughout.

## Relation model and ownership counterparts

The [relation-model replay](parity/runtime_service_relation_model_5_0_6.json.gz)
adds **162** actual-source/native/imported comparisons for RelationLoadPlan.
All retained inputs, their order and complete expected outputs remain unchanged.
The original observer exercises all nine fields, nested queries/children, Clone,
equality and nine Debug formats for both value and clone. Its published source
body and fixture remain intact.

This replay avoids the historical field-layout workspace's divergent registry
lock. A disposable view of the existing reference-locked public observer project
changes only path spelling and adds the retained original bin. Cargo builds with
`--locked`; its lock stays byte-identical. The missing imported model consumer
is prepared from the complete unchanged C++ probe and fixtures, using the current
canonical adapter and a Runtime-only link. It is provisional rather than a
permanent CTest registration. The native executable comes from the completed
consumer build and retains its hash. All 162 outcomes match each frontend.

The [ownership counterpart replay](parity/runtime_service_types_lifetimes_5_0_6.json.gz)
selects the complete request/trace-scope, data-service-scope and dynamic-ledger
C++ programs on both frontends. Their existing assertions cover explicit Global
retention after actual User deletion, request ownership, private derived scopes,
governed provenance, copied dynamic definitions/originals and missing-descriptor
rejection. All three programs pass independently on each frontend; 1,048
canonical consumer/fixture bodies verify and every selected source/binary hash
remains unchanged. This is separate evidence: functional JSON equality alone
does not prove these lifetime contracts.

## Permanent package

The [twelve-file package](parity/service_types_staged_package_5_0_6.json.gz)
contains the complete new probe and private Rust caller, the 96-case source
corpus, a comparator, a reference-locked public observer workspace, a checked
source/asset contract, a portable verifier and CMake/module registrations. The
162-case relation corpus, original driver and complete existing C++ probe stay
unchanged. Both new module consumers explicitly import/link runtime. The package
preserves the preceding Event/Policy and key-snapshot registrations in all four
shared build files rather than replacing them with earlier contents.

The [portable staged replay](parity/service_types_portable_staged_5_0_6.json.gz)
freshly executes both actual original observers and all 258 observations through
each C++ frontend. It passes with unchanged execution inputs and references.
Its explicit provisional mode does not claim installed CTest coverage. The
default installed mode builds five targets per frontend, checks exactly five
registered tests each, validates canonical bodies and requires selected no-work
rebuilds. Source, observer, corpus, fixture and implementation hashes are guarded.
The standalone Cargo workspace is development-only; no runtime dependency is
added to C++ deployment.

The [guarded coordinator](parity/service_types_installed_cohort_final_5_0_6.json.gz)
waits for the key-snapshot coordinator to exit successfully and verifies all
expected execution inputs and proposed before hashes. It then installs the
twelve files, applies/checks the pinned formatter and executes the portable
verifier's default gates. It stores its before-input dictionary separately from
the mutable expected after state. The initial waiting coordinator was stopped
before installation under the documented
[snapshot-copy correction](parity/installation_snapshot_copy_correction_5_0_6.json.gz).
An assembled package or waiting coordinator is not installed acceptance.

## Acceptance boundary

The functional corpus has **258 observations per frontend** across the 96 new
private calls and 162 retained relation-model cases. These counts describe
executed observations, not API counts or a proof of complete runtime parity.
Whole-source formal disposition still requires its explicit audit, installed
observers/consumers and relevant current integration evidence. The current full
native epoch and already queued Event/Policy/key-snapshot cohorts are independent.
No production C++ body, module export, generator, generated SDK or runtime
dependency changes in this reconciliation. No sanitizer, MIPS, physical-router
or complete P0-P7 result is claimed.

## Privacy preparation checkpoint

The independent preparation counterexample is repaired. The [installed replay](parity/context_privacy_preparation_installed_5_0_6.json.gz)
matches all 24 actual-source outcomes on native and imported frontends, with
native4/module4 selected gates passing. Privacy may be prepared before intent
without authorizing execution; absent intent still produces zero provider calls.
The [unchanged shared-type regression](parity/service_types_privacy_requalified_source_5_0_6.json.gz)
passes all 258 observations per frontend and native5/module5 selected gates,
including the complete lifetime programs and final no-work builds. The
[focused sanitizer check](parity/context_privacy_preparation_sanitize_5_0_6.json.gz)
passes its single registered test with ASan/UBSan and leak scanning disabled.

This commit retains the reopened pending disposition until the supplemental
evidence is joined into a formal source acceptance record. Current source
totals are 82 accepted, 40 pending and 2 excluded_external. Earlier acceptance
receipts remain unchanged; complete Context and generator parity stay separate.
