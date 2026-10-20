# G14 audit/privacy and identity requirements

Status: partial. This finite review preserves the complete G14 responsibility.
It replaces neither source-based qualification nor the remaining G17 execution
contracts. The active references are Rust 5.0.6
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and producer
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The frozen entity template emits authentic entity/table/service identities,
resolved model mask keys and the declared audit length. The frozen runtime
template emits own-field wire mappings and a separate empty alias map. Their
direct model helpers and the actual Rust derive determine the observed values;
template text or a fabricated descriptor is insufficient evidence.

## Finite acceptance obligations

| ID | Responsibility | Current evidence and remaining work |
| --- | --- | --- |
| AP01 | Declared/inherited masks, unknown names, declaration state and credential priority | [Compiled metadata](generated_privacy_contracts_5_0_6.md) and [mask resolution](generated_privacy_metadata_5_0_6.md) retain original model keys, declaring-parent resolution and property/column classification. A relation mask need not match its FK property; preserve the actual source result rather than silently improving it. |
| AP02 | Audit-length literal interpretation, ignored expressions and target width | The [container/identity continuation](container_metadata_5_0_6.md) matches 1,452/1,452 retained audit inputs (1,424 exact valid/all 28 source rejections) and 1,519/1,519 full parsed-effect inputs, with four archived/current improvements and zero regressions. Ten descriptor pairs, one embedded-NUL policy pair and nine typed-JSON pairs/54 graph views agree; the authentic foreign-type compiler rejection is retained. Existing parser bodies and locked rest-buffer behavior are unchanged. Full expression/type/pattern/body/lexical grammar and complete identity-override execution remain required; AP02 stays partial. |
| AP03 | Canonical own-field wire mappings, empty aliases and naming failures | [Wire mappings](generated_wire_mappings_5_0_6.md) qualify twelve valid profile/model combinations and nested ownership. **Pending source-behavior decision:** invalid `json_field_naming` succeeds with empty source wire strings while C++ rejects. The unanswered G12 R08 clarification remains binding; time or other green tests do not approve an adaptation. |
| AP04 | Entity, storage, scalar, FK/reverse-relation and diagnostic identities | [Relationship identities](relationship_identity_contracts_5_0_6.md), [scalar diagnostics](scalar_diagnostic_repairs_5_0_6.md), [expression entries](expression_entry_identity_5_0_6.md) and [mixed graph composition](generated_ledger_composition_5_0_6.md) retain canonical metadata independently of C++ escaping and application method names. [Container identities](container_metadata_5_0_6.md) additionally qualify runtime keys/layout names, independent table/service/relation metadata, typed JSON lookup and embedded-NUL strings in their recorded scope. Reuse accepted G01/G04/G06/G07/G10 behavior without inferring whole protected execution. [Core metadata MD01-MD12](core_metadata_source_review_5_0_6.md) additionally accepts the underlying descriptor source; whole generated/protected execution remains separate. |
| AP05 | Metadata reaches actual SQL binding classification and safe rendering | [Complete SQL-log source acceptance](sql_log_source_review_5_0_6.md#durable-source-and-frontend-qualification) qualifies SL01-SL16, including every query-expression fallback, exact field/column policy and no-I/O traversal. Its joint source gate matches 3,091 observations per frontend and passes native19/imported19. [Logging source replay](logging_baselines_5_0_6.md), compiled descriptor/field-policy tuples and [private generated queries](dynamic_json_privacy_5_0_6.md) retain their own scopes. Descriptor emission alone cannot accept publication; whole governed runtime responsibilities remain under G17/SQ03. |
| AP06 | Request-owned/nested intent, readonly/facet/query shapes and protected reads | [Complete Context logging/vector source acceptance](context_logging_source_review_5_0_6.md) qualifies CL01-CL18, including all twelve original paths, request-owned root reason across five operations and independent-query isolation. Original2/formatter4, native15/imported15 and 3,373 source observations per frontend pass. The private-query matrix retains 1,728 real results and 936 complete timeless safe logs, its conservative executor fallback and nested diagnostic source. Capturing fixture-authored metadata does not replace generated/provider execution; complete governed callers remain separate. |
| AP07 | Raw/safe audit identity, bootstrap provenance, snapshots and field limits | [Audit event repair](audit_event_provenance_5_0_6.md) and [dynamic projection](dynamic_audit_projection_5_0_6.md) supersede the old claim that those fields are absent. Raw events and safe publication are distinct; raw Debug is not a safe sink. Preserve explicit target identity and immutable provenance snapshots. |
| AP08 | Metadata through governed changes, actual persistence and committed audits | [Complete dynamic storage acceptance](dynamic_storage_acceptance_5_0_6.md) joins actual typed saves, policy plans, authoritative readback, rollback/conflict and committed audit-delivery failures. Its exact-dependency save replay has 5,144 observations, including the 1,152 typed-governance cases. The mixed graph fixture separately covers keyword relations and explicit ledger composition. Neither is ordinary CRUD substituted for graph Save. |
| AP09 | Ordinary/sensitive sinks, exact permission, credentials, truncation and errors | [Complete privacy/masking source acceptance](runtime_privacy_source_review_5_0_6.md#durable-source-and-frontend-qualification) qualifies LP01-LP16 and all 34 SQLite masking contracts. [Complete formatter/publication source acceptance](log_publication_source_review_5_0_6.md#durable-source-and-frontend-qualification) qualifies LF01-LF18: all four original tests, 927 formatter/configuration cases, 94 fresh manager fixtures per implementation and canonical ownership/failure contracts pass. Its joined gate is native15/imported15. The original PostgreSQL masking case remains C++-excluded. [Private query sinks](dynamic_json_privacy_5_0_6.md) retain their own scope; arbitrary custom sinks, whole assembly and governed publication remain independent G17 obligations. |
| AP10 | Context allocation, explicit retention, expiration and pending cleanup | Wire-map and event ownership contracts disable default PMR, copy into an explicit destination and actually destroy UserContext. Accepted G15/G16 state and provider contracts remain dependencies. Ordinary wrappers must not extend Context lifetime or create another ownership domain. |
| AP11 | Generated artifacts, accurate guidance and cross-layer integration | The [current strict SDK gate](parity/container_metadata_sdk_refresh_5_0_6.json.gz) reproduces 78 SDKs / 3,457 generated files and 2,201 protected artifacts, refreshing producer fingerprints only. Java101/native4/module3 pass. All 41 earlier compiled SDK pairs and ten new descriptor pairs reproduce complete original/C++ trees; nine typed-JSON pairs/54 views, one source compiler rejection and the embedded-NUL policy pair have separate fresh execution/compiler evidence. Complete G12 assembly and SQ04 integration remain open. |
| AP12 | Complete current integration and honest source/capability status | The latest [terminal full native epoch](native_integration_5_0_6.md) passes 735/736 with one provenance-guard failure; its separate source-verified follow-ups do not relabel that epoch. The earlier 718/721 epoch and all failures remain preserved. Later scoped source/import qualifications bring discovery to 751 and source status to 50 accepted/72 pending/2 external exclusions, with unchanged capability statuses. These focused cohorts do not establish a current full native or G14/P7 pass. |

These rows retain all five historical G14 questions: mask/identity emission maps
to AP01/AP04; invalid limits/configurations to AP02/AP03/AP11; event-model omissions
to AP07; governed/private/async consumers to AP05-AP10; final integration to AP12.
The historical questions remained in the frozen capability ledger during the full
native epoch. After terminal completion, the
[exact prior G14 entry](parity/g14_ledger_before_literal_reconciliation_5_0_6.json.gz)
was archived and its stale implementation statements reconciled with AP01-AP12.
The scope and partial capability status are unchanged; historical failures are
not relabeled as newly passing evidence.

## Initial audit-length differences (preserved evidence)

The [six compiled model comparisons](parity/generated_privacy_unicode_suffix_initial_5_0_6.json.gz)
invoke the actual original producer and complete original derives, then compile
and invoke the complete C++ facade. Both ASCII controls agree. All four Unicode
cases compile successfully on both sides, but the original Item descriptor has
`Some(17)` and C++ has no limit:

- an integer with a Unicode suffix;
- an ordinary string with that suffix;
- a raw string with that suffix;
- an integer suffix containing a combining character.

The [59-case parser comparison](parity/generated_audit_literal_parser_initial_5_0_6.json.gz)
executes the complete unchanged original `teaql-macros/src/attr.rs` and the then-current
compiled Java adapter. It retains all 33 earlier cases and adds finite lexical
branches. Sixteen observations differ: the four suffixes, a new Unicode letter,
two exponent forms, four comment forms, three source-whitespace forms and two
outer-attribute forms. This parser-level evidence is separate from full generated
Rust compiler acceptance of every added shape.

The reference exponent `1e2` yields no audit limit; the initial C++ adapter incorrectly emitted `1`.
Conversely, the actual reference parser retains `17` for the tested `f32`, `f64`,
`f16` and `f128` suffixes. A repair must follow those observations rather than
guessing literal categories from suffix names.

The current Java runtime's Unicode identifier predicate also rejects the added
letter accepted by the reference parser. Merely enabling Java Unicode regexes
does not establish reference-equivalent suffix handling. Resolve lexical
classification against the reference's locked `syn`/`unicode-ident` behavior,
without adding a deployed C++ dependency or duplicating runtime privacy rules.

The initial parser harness omitted its direct `proc-macro2` import. Its
[compiler failure](parity/generated_audit_literal_parser_harness_failure_5_0_6.json.gz)
is retained; the corrected harness uses only reference-locked dependencies.
It is not recorded as a runtime defect or a successful comparison.

## Qualification after the frozen native epoch

1. The [terminal native receipt](parity/p7_native_intent_epoch_terminal_5_0_6.json.gz)
   preserves all three failures, unchanged inputs and a successful no-work build.
   Complete matching timing profiles justify the two narrowly changed budgets.
2. The [shared literal adapter](generated_audit_literal_5_0_6.md) preserves the
   initial 59 controls and matches 444 valid-source cases. Resolve its remaining
   repeated-setting and source-invalid lexical boundaries without relabeling that observation.
3. Regenerate affected fixtures and run the relevant Java, source/compiler,
   reproduction and actual metadata-consumer gates. Generated files are never
   patched by hand.
4. Reconcile AP01-AP12 with current evidence. Leave AP03 and any missing protected
   execution responsibility open until its own decision/evidence exists.

During the initial frozen review, no runtime algorithm, SDK, model input or
producer source changed. Class files were copied and hash-checked before the read-only Java
experiments; Maven was not invoked. All new observations and temporary harnesses
are retained separately from frozen execution inputs. The subsequent shared-adapter repair is recorded separately. This document does
not promote G14 or remove a P0-P7 requirement.

The [actual nested audit-metadata witness](audit_nested_metadata_5_0_6.md)
reproduces5 differences across7 successfully compiled original/C++ SDK pairs,
retaining21 descriptors and98 field-policy tuples. Later recognized assignments
can replace or clear a limit; ignored booleans and unknown assigned values have
different continuation rules. An injected empty mask changes Item's effective
runtime descriptor/policy while leaving Child unchanged. Preserve the separation
between model inheritance and macro-resolved runtime metadata when repairing
AP02/AP01. No producer/runtime/checked-in SDK changes or new acceptance; G14,
G17 and P0-P7 remain open.

## Durable request envelope and private provenance source continuation

The [complete data-service/request review](data_service_source_review_5_0_6.md)
qualifies owned intent, command/envelope separation, statement-local trace
projection and immutable request-private provenance for three complete source
files. Ten original tests and 1,308 exact source/native/imported outcomes per
frontend pass; complete default-PMR-disabled native 12/imported 12 contracts
retain Context copies and actual lifetime boundaries. Request Debug excludes
private redactions; raw model Debug still does not become a safe publication API.

Source status is 43 accepted/79 pending/2 external and discovery 744. All capability
statuses remain unchanged. This supplies source-envelope evidence to AP06/AP07;
it does not accept every protected caller, provider, SQL sink or asynchronous
publication path. AP03/R08 is unanswered; AP12 retains both failed full epochs
and current full-integration obligations. No new whole-suite, sanitizer/MIPS or
physical-device result is claimed.

## Core Expr provenance continuation

The [whole Expr review](core_expression_source_review_5_0_6.md) accepts all
fifty methods and twelve current variants. Typed-LIKE helpers retain the exact
execution pattern and original bytes separately; raw LIKE remains an ordinary
literal without inferred provenance. Changing either source string changes
equality. All nine Debug modes reproduce for actual helper results and leaves;
raw structural diagnostics remain separate from safe publication.

69 unchanged core unit tests, 1,403 actual-source outcomes per frontend and
native 6/imported 6 pass. Existing runtime/generator/SDK bodies are unchanged.
Source status is 44 accepted/78 pending/2 external and discovery 746; no capability
status changes. This supplies expression-model evidence without accepting every
SQL/runtime protected caller, AP12 full integration, AP03/R08 or G14/G17/P0-P7.

## SafeExpression source continuation

The [whole SafeExpression review](safe_expression_source_review_5_0_6.md)
qualifies all twenty-four methods, six emptiness implementations and source
Clone/callback boundaries. Four original tests, 580 exact original/native/imported
outcomes per frontend, one executed compiler positive/eight exact negative
controls and native 5/imported 5 pass. This is value-evaluation and lifetime
contract evidence, not authorization or complete protected publication. Raw
borrowed state remains invalid after owner close; same-domain handles do not
keep a Context alive.

Current source status is 45 accepted/77 pending/2 external and native discovery
747, with unchanged capability statuses. AP03/R08, AP12 full integration and
whole G14/G17/P0-P7 remain open. Historical full-suite failures are preserved.

## Core mutation model source continuation

The [complete mutation model review](core_mutation_source_review_5_0_6.md)
qualifies separate mutation input, provider-generated output and persisted
snapshot roles, all six commands and complete raw derived diagnostics. All
2,048 actual-source outcomes per frontend, one positive/fourteen precise
negative source compiler controls and native 5/imported 5 pass. Every batch
vector stays independent and raw Debug remains separate from safe publication.

Source status is 46 accepted/76 pending/2 external and discovery 748. All
capability statuses remain unchanged. This supplies model/type evidence, not
whole guarded mutation, governance, SQL/provider or audit/privacy execution.
AP03/R08, AP12 full integration and G14/G17/P0-P7 remain open; historical full
epoch failures retain their outcomes.

## Complete binding provenance and original tests

The [binding source review](sql_bindings_source_review_5_0_6.md) qualifies
per-compilation policy/value/original alignment, scope precedence, scalar versus
logical/subquery isolation, exact retained-LIKE validation and invocation-local
rebinding. All eleven original tests, 1,209 actual-source outcomes per frontend,
native 6/imported 6 and focused sanitizer 1 pass. Nonempty original strings retain
explicit destination ownership after actual User deletion; allocation faults
restore all logical vectors. The only production change corrects raw pretty
Debug tuple formatting and does not change privacy or execution algorithms.

Source status is 48 accepted/74 pending/2 external and discovery 749. All
capability statuses remain unchanged. This supplies binding/compiler evidence
without accepting complete SQL/runtime/protected publication. AP03/R08, AP12
full integration and G14/G17/P0-P7 retain their independent requirements and
historical failed full epochs.

## Complete SQLite renderer and SQL error source continuation

The [whole SQL type review](sql_types_source_review_5_0_6.md) qualifies all
models/error payloads, comment escaping, strict SQLite lexical accounting,
callback failure/omission and literal/finiteness behavior. Six original tests,
2,210 actual-source outcomes per frontend and native 6/imported 6 pass with no
C++ body changes. Renderer callbacks receive indices only; diagnostic output does
not replace execution SQL or authorize plaintext. Non-SQLite implementations
remain excluded and new maximum-width tests qualify native64 only.

Source status is 50 accepted/72 pending/2 external and discovery 751, with
unchanged capability statuses. AP03/R08, AP12 full integration, protected runtime
publication and whole G14/G17/P0-P7 remain open. Historical failed full epochs
retain their outcomes.
