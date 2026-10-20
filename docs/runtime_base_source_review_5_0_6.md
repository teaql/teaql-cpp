# Runtime base data-service source review: Rust 5.0.6

Current supplemental qualification: **accepted** after the [intent/privacy correction](parity/mutation_intent_privacy_requalified_5_0_6.json.gz). The original full-source checkpoint below is retained with this additional evidence; its earlier corpus did not cover replacing an existing intent/privacy pair.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: all 491 lines of `teaql-runtime/src/data_service/base.rs`, including its
original statement-identity test. The source SHA256 is
`67ab3d9e2bc6ba8b816a197d72d8194e005ba9454d58fe2bbd565c17387faff7`.
Disposition: **accepted**, limited to this complete base-service source under
DB01-DB18. The [acceptance receipt](parity/runtime_base_source_acceptance_5_0_6.json.gz)
joins the complete-source execution, both installed frontends, isolated Release
counters and scoped sanitizer evidence. Context/entity services and complete
generator phases retain their separate acceptance requirements.

## Complete responsibility map

| Review | Locked responsibility | C++ counterpart and evidence boundary |
| --- | --- | --- |
| DB01 | Constructor and borrowed metadata/executor | `declarations.hpp` keeps a separate `RuntimeDataService` with checked Context-bound metadata and executor borrows. Construction starts without mutation intent or privacy. It does not create a provider or merge Context/entity service responsibilities. |
| DB02 | Required mutation intent | The shared mutation operation rejects absent intent with the Mutation/comment request-intent error. Trace comments do not supply intent. An empty many/batch loop can return zero without invoking this guard. |
| DB03 | Physical command identity | `base_mutation.hpp::mutation_item` uses the last Entity frame, replaces its ID for a matching type, and otherwise appends a physical target. Insert IDs come from values; other IDs come from the command. The [installed statement fixture](runtime_statement_trace_5_0_6.md) covers all ten entry points and preserves caller lineage. |
| DB04 | Query intent and callback ordering | Original query methods validate nonblank comment and purpose before reading either metadata capture switch or creating the observer. The actual-source query fixture records the complete callback sequence, including invalid-intent early exits. The installed C++ correction validates first, then assigns the two capture flags in explicit statement order. |
| DB05 | Independent request flags and observers | Debug and execution-metadata flags come from separate MetadataStore methods. The query observer is a third independent SPI call. A custom observer can exist with metadata capture disabled or be absent with capture enabled. |
| DB06 | Plain row fetch | `base_query.hpp` clones the request and trace, calls `query_observed`, maps executor errors and returns rows. `fetch_all` does not record final metadata locally; an installed diagnostic observer may still receive metadata. |
| DB07 | Smart-list fetch | `fetch_smart_list` records successful final metadata, wraps rows as a loaded SmartList and preserves an empty result's loaded state. Failure does not record successful final metadata. |
| DB08 | Typed and enhanced compatibility fetch | Typed fetch records final metadata before decoding, returns the first Entity decode error and otherwise marks the list loaded. This layer's enhanced method delegates to typed fetch; relation enhancement belongs to the separate higher service. |
| DB09 | Insert | Clone values and lineage, assign physical identity, require intent, attach diagnostic redactions, execute through the observed mutation SPI, record successful final metadata and return affected rows. The joined original-body mutation fixture additionally covers provider failures, observer delivery and final metadata ordering. |
| DB10 | Update and delete | After the same request/metadata path, zero affected rows conflict only when an expected version exists. Conflict IDs prefer decimal `try_u64` output and otherwise retain Value Debug. Final metadata is recorded before classifying a conflict. |
| DB11 | Recover | Zero affected rows always conflict. Its conflict ID deliberately uses Value Debug, unlike update/delete's numeric preference. It has the same observed request and final-metadata path. |
| DB12 | Batch insert | Expand each value row into an insert, take a row trace when present and otherwise start an empty trace, and execute sequentially. Stop on the first error and accumulate successful counts. Empty input returns zero. |
| DB13 | Batch update row construction | Index the corresponding ID, filter values to declared update fields and preserve optional row versions, old snapshots and traces. A short ID list fails rather than silently truncating. The statement fixture covers physical IDs and missing second-row traces; the joined execution fixture independently varies value/ID/version lengths and verifies filtered values and old snapshots. |
| DB14 | Batch update final guard | A row with an expected version conflicts immediately on zero affected rows. After the loop, any present version anywhere in the version list requires total affected rows to equal the full ID-list length; unprocessed version entries are not ignored. |
| DB15 | Four many-operation methods and counters | Insert/update/delete/recover many iterate in order, delegate single-command semantics and stop on failure. C++ `mutation_count_add` preserves checked Debug overflow and wrapping Release behavior without signed-overflow undefined behavior. The joined Debug and isolated NDEBUG witnesses distinguish 76 counter outcomes against actual original Debug/Release methods; this is not a full Release runtime or module profile. |
| DB16 | Privacy and diagnostic provenance | Mutation intent and redactions remain distinct. Each request receives the configured privacy data before execution and the MetadataStore supplies its observer. The approved C++ destination copy replaces Rust Arc retention without extending Context lifetime. Whole logging/privacy acceptance remains separate. |
| DB17 | Error types and lifecycle | Executor, Entity and Runtime errors remain distinct in `DataServiceError`. C++ operation wrappers retain checked borrows and explicit nested allocator propagation, reject foreign result domains, forward cancellation and finish actual child work before releasing referenced Context storage. Existing complete foreign-work and generic-completion programs are separate C++ adaptation evidence. |
| DB18 | Original embedded test | The unchanged original test checks appended physical targets, idempotence, preservation of audit lineage and enrichment of an unknown ID. It runs in complete source views; the C++ statement matrix is an additional counterpart rather than a relabeled original Rust test. |

## Existing evidence needs its actual execution scope

The retained 1,502-case `runtime_mutation_5_0_6_cases.json.gz` has **no provider
request in any expected result**. Its base408/context372/entity710/prepared12
partition is useful early-exit evidence: 402/366/526/12 respectively report
missing mutation intent, and remaining cases stop at earlier checks or empty
input. It must not be presented as executed positive persistence, optimistic
conflict, provider-error or metadata-order coverage on 5.0.6.

The newer statement fixture directly executes original mutation methods with
valid intent and matches 1,638 source/native/imported cases, including both
commands in many/batch calls. Its fixture always returns one affected row, so
it does not close the missing failure and counter-boundary coverage above.
This difference in scope is why complete base-source acceptance remains open.

The older query oracles contain a local RuntimeDataService declaration and
byte-identical extracted method fragments. Their 256 plus 768 retained cases
remain unchanged controls. The new query caller appends only fixture code to
a byte-checked complete reference runtime and calls its actual constructor and
all four actual query methods. It replays those same 1,024 controls before
exercising 4,096 new protocol inputs; no runtime method is copied into the caller.

## Query protocol fixture and repair boundary

`tests/parity/runtime_base_probe.cpp` retains the legacy output when protocol
observation is disabled. Added protocol inputs record capture-switch calls,
custom observer construction, provider invocation, observer delivery and final
metadata recording. Debug/capture/observer switches vary independently. Provider
failure and handwritten Entity decode failure distinguish their error paths.
The executor modifies only its owned query copy; the output also reports the
caller's original comment, purpose and trace size. Both direct and Runtime-only
module frontends execute the complete same probe body. The default PMR resource
is disabled and each input has a fresh GlobalContext/UserContext.

Eight intent pairs include absent, blank and Unicode-whitespace values and valid
surrounding whitespace. Together with four fetch modes and seven boolean
controls they produce 4,096 protocol observations. The fixture's observer is
explicit application-owned test behavior; it does not establish that every
possible MetadataStore implementation or external logging sink is qualified.
The original unchanged statement test also runs in this source view.

The [complete-source replay](parity/runtime_query_protocol_source_5_0_6.json.gz)
passes the unchanged original test and the added caller test, reproduces all
1,024 retained observations and establishes the 4,096 protocol outcomes.
The [pre-repair witness](parity/runtime_query_protocol_before_5_0_6.json.gz)
preserves both C++ frontend observations, the old implementation, input guards
and the source-receipt identity. Each frontend matches the old 1,024 controls
but differs on all 3,072 invalid-intent protocol cases: the capture callbacks
were invoked before validation. The valid-intent cases are unchanged controls.

The correction removes the capture callbacks from `QueryRequest::from_query`
arguments. After successful validation, separate statements assign debug then
execution-metadata capture, matching the original field order. Observer creation
and provider invocation retain their position after those assignments. This
also avoids relying on C++ argument evaluation order for application SPI calls.

The [installed replay](parity/runtime_query_protocol_installed_5_0_6.json.gz)
is complete: actual source/native/imported match all **4,096 protocol** and
**1,024 legacy** observations per frontend. The original test and added caller
pass. Native **6/6** and imported **4/4** selected CTest checks pass without
errors, failures or skips; selected builds finish with no work. Module validation
checks the complete canonical bodies and the Runtime-only imported consumer.
The receipt retains all commands, output, JUnit, reproduction sources, exact
reference identities and guards for the unchanged execution inputs.

Native checks include both legacy base corpora, the new protocol corpus, both
Context query/intent corpora and foreign-query cleanup. Imported checks include
the same three base corpora and the complete generic-completion program. This
is not a later full native suite, sanitizer, MIPS or physical-device gate.
At this earlier query checkpoint, source counts were **82 accepted / 40 pending /
2 excluded_external**. No generator capability or P0-P7 phase verdict advanced.
The complete base-source acceptance below records the later source-only change.

To repeat actual-source and installed frontend verification:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_runtime_query_protocol_5_0_6.py \
  --record /tmp/teaql_query_protocol_fresh.json.gz \
  --cpp build_native_current/teaql_runtime_base_probe \
  --module build_modules_native/modules/teaql_module_runtime_base_probe \
  --check-gates
```

Use a fresh receipt path. Verification compares the retained corpus to live
original methods and does not overwrite expectations.

## Joined whole-source qualification

The [joined original-body execution](parity/runtime_base_execution_source_5_0_6.json.gz)
now passes the unchanged original test and three added callers, reproduces all
5,120 query and 1,638 statement observations, and supplies **8,832 mutation**
observations. These execute the actual original constructor and all fourteen
query/mutation entries rather than copies of their method bodies. Single,
many and batch inputs vary intent, observer availability, delivery, provider
failure, counts, versions, ID types, trace and privacy. Batch value/ID/version
lengths vary independently; update requests expose declared-field filtering,
optional old values and default missing row traces. Outcomes retain Runtime
versus Executor errors, full text, request data, callback order and metadata.

The [pre-repair comparison](parity/runtime_mutation_protocol_before_5_0_6.json.gz)
reproduces **240/8,832 differences** on each C++ frontend. Every difference has
a short batch-update ID list and absent mutation intent. Rust indexes the ID
while constructing the physical row command before checking intent. C++ checked
intent first. The comparison maps Rust's caught bounds panic to C++'s caught
`std::out_of_range`; overflow separately maps to `std::overflow_error`. It does
not equate ordinary Runtime/Executor errors with a panic or match panic text
across languages.

The correction makes the internal `mutation_item` return the existing
intent-free `MutationCommand`. After constructing that row and its physical
trace, the operation validates intent and creates `MutationRequest`. This
retains the command/request abstraction and removes the ordering discrepancy
without duplicating batch validation or introducing another ownership domain.
All prepared nested data and the execution envelope use the same bound Context.

The [installed joined gate](parity/runtime_base_execution_installed_5_0_6.json.gz)
matches all **15,590** source/native/imported observations per frontend: query5,120,
statement1,638 and mutation8,832. Original1 and added-caller3 pass in the complete
reference view. Native **11/11** and imported **6/6** selected checks pass with
canonical consumers, Runtime-only links and final no-work builds.

The [actual Release source](parity/runtime_base_execution_release_source_5_0_6.json.gz)
replays the same complete original bodies and three caller fixtures. Its query
and statement controls retain their Debug outcomes. The
[isolated C++ NDEBUG adapter](parity/runtime_base_execution_release_installed_5_0_6.json.gz)
matches all **8,832** mutation observations and passes its permanent CTest gate.
It differs from Debug on **76** actual counter outcomes, so this is an executed
wrapping check rather than two identical expectation sets. The receipt verifies
the actual compile command and that the linked normal runtime archive supplies
no competing `mutation_count_add` body. This is not a full Release runtime or
Release module profile. No public header data layout depends on NDEBUG; the
conditional runtime header bodies concern counters.

The [scoped ASan/UBSan gate](parity/runtime_base_execution_sanitize_5_0_6.json.gz)
passes **2/2**: the full mutation protocol corpus and foreign-mutation cleanup.
It checks actual sanitizer flags, binary identity, the shared source-input
checkpoint and a final no-work build. Leak scanning is disabled as required;
this is not a full sanitizer suite. Native SQLite/reentrant and complete generic
completion programs retain their independent scope inside the joined gate.

These receipts join DB01-DB18 for **only** the complete491-line `base.rs` source.
Source status becomes **83 accepted / 39 pending / 2 excluded_external**. All
seventeen generator capability statuses are unchanged; six remain partial.
Context service, entity service, whole governance/privacy, P0-P7, a later full
native suite, MIPS and physical-device acceptance do not follow from this file.
Earlier failed and pre-repair observations remain unchanged.

To repeat the complete original-body and installed Debug gates:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_runtime_base_execution_5_0_6.py \
  --record /tmp/teaql_base_execution_fresh.json.gz \
  --cpp build_native_current/teaql_runtime_mutation_protocol_probe \
  --module build_modules_native/modules/teaql_module_runtime_mutation_protocol_probe \
  --check-gates
```

For the separate isolated counter variant, use `--release`, a fresh receipt,
`--cpp build_native_current/teaql_runtime_mutation_protocol_release_probe` and
`--check-gates`, without a module argument. Ordinary verification never replaces
the retained Debug or Release expectations.


## Supplemental intent/privacy qualification

The [eighteen-case original caller](parity/mutation_intent_privacy_source_5_0_6.json.gz)
executes original Runtime and Context setters with independent preexisting
privacy and follows through their real mutation entries. The
[pre-repair witness](parity/mutation_intent_privacy_before_5_0_6.json.gz) reproduces
eight differences per frontend. Both present-empty and nonempty privacy were
cleared when an equal or different intent was set. The old complete-source
checkpoint had not exercised this setter sequence. Its receipts and expectations
remain unchanged, and the [reopening record](parity/mutation_intent_privacy_reopened_5_0_6.json.gz)
preserves the temporary pending disposition rather than hiding the new failure.

RuntimeDataService and ContextDataService now have explicit intent-only
overloads that retain their own privacy value and delegate to the existing
two-argument setter. An explicit privacy pointer still replaces the value;
explicit nullptr still clears it. Existing ordinary one/two-argument call sites
remain valid. Ownership domains, nested allocation and executor protocols are
unchanged; this is a configuration-state correction, not a new lifetime model.

The [installed supplemental gate](parity/mutation_intent_privacy_installed_5_0_6.json.gz)
matches 18/18 source/native/imported outcomes and passes native5/module5 with
final no-work builds and canonical consumers. Those gates also rerun the full
8,832-case mutation corpus, both retained Context query corpora (1,872 total)
and the complete Context query lifecycle program on each frontend. The separate
[real-Context replay](parity/runtime_context_queries_installed_final_5_0_6.json.gz)
removes the former shadow-Context integration gap and adds native4/module3.
It does not accept complete Context service responsibilities. No new full
sanitizer, Release, MIPS or full native suite is claimed for this correction.

The supplemental receipt restores only the base source status. Counts return
to 83 accepted / 39 pending / 2 excluded_external; seventeen capability verdicts
and all P0-P7 exits remain unchanged. Context factories, independent pre-intent
privacy forwarding, mutation/cache behavior and whole privacy governance remain
outside this completed base-source checkpoint.
