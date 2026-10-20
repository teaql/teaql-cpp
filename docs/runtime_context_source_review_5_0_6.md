# Context data-service source review: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: all 362 lines of `teaql-runtime/src/data_service/context.rs`, SHA256
`c0207f22d4935d6ab67ca5558cccaf5fc6697b1b6f8806939164f701f6fcef48`.
Current disposition: **accepted** under CD01-CD15 at the [joined source acceptance](parity/runtime_context_source_acceptance_5_0_6.json.gz). This source has no embedded unit tests; the six added original-body callers are additional evidence. Earlier pending/checkpoint paragraphs below retain their historical scope.

## Complete responsibility map

| Review | Locked responsibility | C++ counterpart and current boundary |
| --- | --- | --- |
| CD01 | Private Context factory | Missing metadata is checked before requiring the typed executor. The service starts with no mutation intent or privacy. The actual public-factory caller exercises this original private delegation; the 96-case witness checks the equivalent composed C++ metadata/executor branch and its defaults. |
| CD02 | Public entity-service factory | Check entity-service registration first, then delegate metadata/executor assembly. Empty trace, request intent and query privacy are defaults. `entity_service.hpp` keeps this distinct from direct compatibility execution. |
| CD03 | Executor registration | Install the type-erased GraphSaverFor before registering the typed executor. `graph_saver.hpp` retains both roles. The 96-case actual caller distinguishes plain resource insertion from executor registration and observes both resource identities and saver presence. The approved C++ owner-thread/resource-borrow adaptation replaces source Send/Sync transfer guarantees; this is registration evidence rather than graph-save execution or a thread-transfer claim. |
| CD04 | Replace mutation intent | Change only intent, retaining privacy. Both one-argument C++ setters now preserve an already configured privacy value; the actual-source eighteen-case supplemental witness passes both frontends. |
| CD05 | Noop observation | With noop telemetry, await work without evaluating the name closure or starting either scope. The independently instrumented helper witness verifies name-call counts, skipped throwing closures, pending completion and cancellation against the actual original FnOnce entry. |
| CD06 | Outer observation | Create the requested family/name with the entity-type attribute. Scope activation encloses polling, not suspension. Completion records success or `data_service_error`; callback failures remain fail-open through the telemetry SPI. |
| CD07 | Nested provider observation | Inside the outer activation, start a provider scope using the actual executor type and operation family. Preserve both provider attributes, nested activation, completion and drop order. C++ uses its authentic type spelling under the documented language adaptation. |
| CD08 | Base-service forwarding | Create a separate RuntimeDataService and forward intent and privacy. `declarations.hpp` retains checked metadata/executor borrows and explicit destination copies instead of implicit shared ownership. The independent preparation/forwarding witness covers both binding orders, absent intent, present empty/nonempty privacy and explicit clearing. |
| CD09 | Plain row query | Observe `query` / `entity.list` and delegate owned query execution. The 5.0.6 method preserves the request comment and does not substitute a user/trace-derived diagnostic string. |
| CD10 | List, typed and enhanced queries | Observe the same family/name and delegate each distinct base entry. Entity decoding and final metadata behavior stay in the base layer; this Context wrapper does not implement relation loading itself. |
| CD11 | Insert and update | Observe `mutation` with the corresponding action name, delegate to base, then invalidate cache only after success. Zero successful affected rows still trigger invalidation unless base semantics classify a conflict. The 4,608-case actual Context witness covers these success/error branches with separately registered caches. |
| CD12 | Batch insert and update | Preserve the separate action names and base batch semantics. Empty successful batches still invalidate cache; a partial execution followed by an error does not take the success invalidation path. |
| CD13 | Delete and recover | Delegate through observed base execution, propagate errors and invalidate only after success. This does not bypass base optimistic version semantics. |
| CD14 | Cache backend identities | Invalidate both separately registered trait-object and concrete in-memory caches. Form `cache_namespace::entity:Entity` through the shared helper. No external cache provider is introduced. |
| CD15 | Diagnostic comment helper | A nonempty trace takes precedence over the optional comment, folds every node in order with numeric ID or `pending`, and prefixes a nonempty user identifier. Empty-string checks do not trim whitespace. This helper supplies no request intent and is no longer a plain-query rewrite. |

## Real Context queries and lifecycle counterparts

The earlier query fixture shadowed UserContext and service carriers to record
metadata counts around byte-identical extracted query methods. Its retained
468 plus 1,404 cases are unchanged. The new caller instead uses the actual
UserContext, ContextDataService, UserContextMetadata, RuntimeDataService,
telemetry and logging implementations in a complete byte-checked reference view.
Only fixture model/executor/telemetry implementations and an access-only test
module are appended. No runtime type or method is copied or shadowed.

The [first attempted receipt](parity/runtime_context_queries_installed_5_0_6.json.gz)
retains a fixture compile failure: the new observer omitted `std::io` after
removing the old input loop. The correction adds that fixture import and removes
an unused import; it changes no original runtime body or retained expectation.

The [corrected installed replay](parity/runtime_context_queries_installed_final_5_0_6.json.gz)
is complete: source/native/imported each match **1,872/1,872** unchanged cases.
The added caller passes; original embedded test count is zero. Native **4/4**
and imported **3/3** scoped gates pass, canonical module consumers validate,
and selected builds finish with no work. Native checks include the complete
Context query program and foreign-query cleanup. The module gate imports the
complete Context query program for the first time, retaining its lifetime,
closed-borrow, non-Clone completion, fail-open and pending/cancel assertions.

Provider type names necessarily differ across languages. Both observers first
assert that the operation name, provider kind, provider operation and common
operation-name attribute match the actual fixture executor type and `query`.
Only then do they map the two executor-specific strings to the retained fixture
spelling. Business names, intent, entity attributes and event ordering are not
normalized. This rejects an incorrect provider identity instead of hiding it.

The C++ probe now disables default PMR allocation and uses a fresh GlobalContext
and UserContext for each input. Complete original C++ fixture/program bodies are
preserved by canonical module preparation; no production implementation is
copied into a test fixture. These immediate and pending fixture observations do
not qualify every application telemetry implementation or the whole log-privacy
pipeline. Registration, independent privacy state and mutation/cache branches
retain the scope described by the additional witnesses below; independent
privacy state and the remaining private helper/caller contracts still require
qualification before complete Context source acceptance.

## Intent/privacy continuation

Source RuntimeDataService and ContextDataService intent setters preserve the
existing independent privacy field. The
[actual-source caller](parity/mutation_intent_privacy_source_5_0_6.json.gz) exercises
both original setters and executes through their original mutation methods.
Eighteen controls distinguish absent privacy, present empty privacy and a
nonempty target-ID redaction, with initial, equal-valued or different intent.
The [pre-repair comparison](parity/mutation_intent_privacy_before_5_0_6.json.gz)
finds **8/18** differing cases per C++ frontend. Changing intent discarded both
kinds of existing privacy, while ten controls retained their original result.

The cause is the C++ optional privacy parameter's `nullptr` default, which
conflates intent-only replacement with explicit privacy clearing. Both service
types now provide a one-argument intent setter that preserves their own privacy
field and delegates to the existing two-argument operation. Explicit two-argument
replacement/clearing stays available. The fix adds no ownership domain and
does not change the query, provider, logging or policy layers.

The [reopened qualification record](parity/mutation_intent_privacy_reopened_5_0_6.json.gz)
preserves the earlier base-source ledger/review and temporarily restores its
pending status. The [installed supplemental gate](parity/mutation_intent_privacy_installed_5_0_6.json.gz) passes source/native/imported18 each and native5/module5, including the unchanged mutation8832 and Context-query1872 corpora and complete lifecycle programs. The [requalification receipt](parity/mutation_intent_privacy_requalified_5_0_6.json.gz) restores only the base source status. Earlier accepted
receipts remain historical; neither they nor the expectation corpora are
rewritten to hide the counterexample. Complete Context source acceptance remains
pending even when the focused correction passes.

## Actual mutation/cache execution

The [installed mutation/cache replay](parity/context_mutation_cache_installed_5_0_6.json.gz)
matches **4,608/4,608** actual source/native/imported results per frontend.
It appends one new caller to the complete original Context module; no Context,
metadata or cache runtime type/method is shadowed. All six mutation entries run
through their original Context wrappers and base methods. Native **5/5** and
imported **5/5** selected CTest checks pass with no failures/skips, canonical
Runtime-only consumers and final no-work builds. The same scoped gates retain
the eighteen intent/privacy controls, both Context query corpora and the complete
Context query lifecycle program.

Inputs vary intent, row count, ID-list length, versions including unprocessed
tail entries, per-row affected counts, first/second provider failure and four
combinations of custom/concrete cache registration. Observations include full
success/error outcomes, checked-bounds panic equivalents, provider request
counts, callback order and the concrete cache's remaining keys. Both cache types
are registered under their actual independent resource identities. The custom
backend records the exact invalidation namespace; the concrete cache checks
that Payment rows are removed while Payments, Other, another cache namespace
and the unsuffixed namespace key remain.

This confirms the locked source's success-only rule: successful zero-row writes
and empty successful batches invalidate; conflicts, missing intent and provider
errors do not. A failure after an earlier successful row does not take the final
success invalidation path. This is the source's behavior and is not a claim of
atomic batch writes or a general cache-coherence guarantee after partial writes.
Logging and telemetry are disabled in this particular fixture to isolate cache
behavior; the separate real-Context query witness retains their own scope.

## Actual factory and executor registration

The [installed factory replay](parity/context_factory_installed_5_0_6.json.gz)
matches **96/96** actual source/native/imported observations per frontend.
It varies metadata presence, modeled entity presence, an independent entity
registry, no/plain/registered executor, matching versus different executor type,
and existing versus unknown entity name. The actual public factory delegates
through original `data_service_internal`; no test reimplements its branch logic.
C++ executes its existing composed public factory. Each successful result checks
the entity and default trace, query intent/privacy and mutation intent/privacy.
Failures retain their actual error variant and assert the entity/resource name;
typed-resource failures assert the authentic executor type spelling before
mapping that language-specific value to the shared logical executor stage.

Plain insertion supplies the typed executor without a graph saver. Registration
supplies both the typed executor and the actual type-erased saver resource,
even when a later factory call fails for missing metadata or an entity. The
fixture transaction/query/mutation methods panic or throw if called: this
observes registration/lookup, not database or graph-save execution. The original
Send/Sync bound is satisfied by its test types; C++ keeps the approved Context
owner-thread adaptation rather than claiming an equivalent transfer guarantee.

The added original-body caller passes; there are no original embedded tests in
this source. Native **5/5** and imported **5/5** selected gates pass with canonical
consumers and final no-work builds. These gates also rerun mutation/cache4,608,
Context queries1,872 and the complete Context lifecycle program. Default PMR is
disabled and each C++ input has a fresh GlobalContext/UserContext.

These focused witnesses advance no complete source or generator verdict. Totals
remain **83 accepted / 39 pending / 2 excluded_external**. Independently
instrumented noop name-closure behavior, diagnostic comment folding, pre-intent
privacy preparation/forwarding and the joined remaining Context contracts are
still required. No new sanitizer, Release, MIPS or full native-suite claim is
made for these fixture/registration changes.

## Independent privacy preparation and forwarding

The [original-body preparation caller](parity/context_privacy_preparation_source_5_0_6.json.gz)
uses the real entity service's `with_plan_privacy`, then the real Context and
Runtime methods. Its twenty-four controls combine empty versus sensitive-value
plans, direct Context execution, intent-before-forwarding versus
forwarding-before-intent, optional intent binding and explicit privacy clearing.
Both the query privacy snapshot and independent mutation privacy are observed.
The descriptor/plan are fixture data, not copies of collection or execution code.

The [pre-repair witness](parity/context_privacy_preparation_before_5_0_6.json.gz)
reproduces **24/24 differences** on each C++ frontend. Rust prepares privacy
without granting mutation intent; execution later rejects absent intent with
zero provider calls. C++ rejected privacy preparation itself. The prior RT01-RT17
shared-type witnesses had bound intent first and therefore did not distinguish
these valid preparation sequences. The
[reopening record](parity/context_privacy_preparation_reopened_5_0_6.json.gz)
retains that earlier ledger/review and temporarily reopens the shared-type
source responsibility. No original expectation is overwritten.

RuntimeDataService and ContextDataService now provide a separate trusted
`with_mutation_privacy` infrastructure operation. It copies or clears the opaque
redactions in the bound Context and does not grant intent. Existing one-argument
intent setters retain privacy; explicit two-argument intent/privacy calls still
work. Conversion to RuntimeDataService forwards privacy unconditionally, including
when intent is still absent. A later intent bind therefore cannot lose the
prepared privacy. The entity plan/ledger binding helper now installs privacy
independently rather than adding an intent prerequisite absent from the source.
The existing base mutation intent check still guards actual executor dispatch.

This introduces no ownership domain, ambient capability or application JSON
override. Redactions and their nested values remain explicit Context copies;
the reusable parent scope and the executor protocol remain independent. The
source's Arc retention is not copied as shared C++ ownership.

The [installed replay](parity/context_privacy_preparation_installed_5_0_6.json.gz)
matches all 24 source observations per frontend, with native4/module4 selected
gates passing. The [unchanged shared-type regression](parity/service_types_privacy_requalified_source_5_0_6.json.gz)
passes all 258 observations per frontend and native5/module5 gates, including
lifetime programs and final no-work builds. The [scoped sanitizer replay](parity/context_privacy_preparation_sanitize_5_0_6.json.gz)
passes its single registered ASan/UBSan test with leak scanning disabled.
Only the two reviewed implementation guard hashes were refreshed in the
[contract update](parity/context_privacy_contract_refresh_5_0_6.json.gz);
original observers, corpora and earlier receipts remain unchanged.

The [supplemental shared-type acceptance](parity/context_privacy_preparation_requalified_5_0_6.json.gz) restores only types.rs. Current source totals are **83 accepted / 39 pending / 2 excluded_external**.
Complete Context remains pending on the remaining helper/caller review; these
focused gates do not advance capability verdicts or generator phase exits.


## Comment and lazy-name helpers

The [installed helper replay](parity/context_helpers_installed_5_0_6.json.gz)
matches **1,128/1,128** actual source/native/imported observations per frontend.
The caller is appended to the complete original Context module and calls its
private helpers directly; it does not copy their implementations. The original
source has no embedded tests. The added caller is counted separately.

The 1,080 comment controls cross absent, empty, whitespace and Unicode user IDs
and business comments with all nine trace kinds, absent/zero/maximum unsigned
IDs, multiple nodes, embedded NUL/newline characters and a 108-node chain.
They preserve trace-over-comment precedence, ordered arrows, `pending`, numeric
formatting and the source's non-trimming empty-string behavior. This helper does
not grant query/mutation intent or rewrite ordinary query comments.

The 48 observation controls independently instrument the name closure and work
poll count. They vary noop telemetry, a throwing name, work success/error,
immediate/pending work and cancellation before or during polling. Rust supplies
an actual FnOnce closure that consumes its name token. C++ supplies a Context-bound
non-owning callback; the existing operation adapter guarantees at most one
invocation. Both are lazy, skip the name and both scopes when telemetry is noop,
and preserve result/error propagation. Fixture executors reject database calls.

Both frontends pass **8/8** selected registered gates, covering helper1,128,
factory96, mutation/cache4,608, independent privacy24, Context query1,872,
intent/privacy18 and the complete Context lifecycle program. Canonical imported
consumer checks and final no-work builds pass. Default PMR is disabled in the
new probe and each input has its own GlobalContext/UserContext. No production
function changes are needed for these helper outcomes.

## Joined complete-source acceptance

CD01-CD15 covers every constructor, bound, method and branch in the complete
362-line source. The joined receipt verifies six original-body source callers,
**7,746** exact outcomes per installed frontend and the current native8/module8
gates. Earlier source witnesses are reused only after checking the same locked
original commit and unchanged retained fixture/probe/corpus bytes; current
frontends rerun their matching corpora. Fixed mutation operation families and
all six action suffixes are additionally reviewed against the original call
sites and `MutationOperationName`/Context mutation entry mappings. Query and
mutation services remain distinct from executors, graph savers and providers.

The approved C++ adaptation keeps explicit Context-bound metadata/executor
borrows, destination allocator copies, owner-thread checks and cancellation
before arena reclamation. It does not promise Rust Send/Sync transfer or shared
ownership. The complete lifecycle programs retain pending cancellation, closed
borrow rejection, foreign work, non-Clone results and explicit Global retention.
The earlier scoped privacy sanitizer receipt retains its own scope; this
fixture-only continuation does not add a new full sanitizer, full native-suite,
Release, MIPS, physical-router, external-provider or graph-save execution claim.

Only `data_service/context.rs` advances at this checkpoint: **84 accepted /
38 pending / 2 excluded_external**. All seventeen capability verdicts remain
unchanged. Full runtime/entity service, governance/log privacy, G13/G17 and all
P0-P7 exits retain their independent requirements.
