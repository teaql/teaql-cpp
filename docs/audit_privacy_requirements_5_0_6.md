# G14 audit/privacy and identity requirements

Last complete native integration: the
[hardened-controller full epoch](parity/current_full_native_hardened_terminal_1066_5_0_6.json.gz)
passes **1,066/1,066** tests with zero failures, errors or skips on 11,816
unchanged inputs, exact JUnit/registry names, fixed references and a no-work
post-build. This establishes SQ05 execution for that source state. Later
relation-source and direct private-key/classifier verifier checks pass **4/4**
and **5/5** in the current 1,072-test registry, without a new full epoch. The
separate P0 inventory gate passes **1/1**. AP02 metadata completeness,
protected consumers, SQ02 provenance, complete G14/G17 and the remaining
phase exits remain open.

Current AP02 cross-branch regression:
[The installed 162-case corpus](audit_grammar_branch_review_5_0_6.md#installed-cross-branch-composition-regression)
matches **80,160/80,160** locked Rust/current Java metadata outcomes and
passes **154/154** Java tests. It is parser metadata evidence; the other 159
new inputs have no compiled generated caller. The first full epoch has one
[isolated admission-fixture failure](custom_case_admission_fixture_5_0_6.md);
the [repaired epoch](parity/ap02_cross_composition_full_native_repaired_terminal_5_0_6.json.gz)
passes **1,063/1,063** on **11,811** unchanged inputs. Later comparator guards
have selected gates only.
Complete AP02/G14/G17 and P0-P7 remain open: **14 qualified / 3 partial**.

Current generated Credential receipt admission:
[The independent source and protected-publication check](audit_generated_credential_execution_5_0_6.md#current-receipt-admission)
validates 533 current inputs, 59 Java dependencies, twelve Save pairs, twelve
query-file cases and four password UPDATE file cases. Fourteen controlled
corruptions reject; selected native CTest passes 1/1. This is retained
execution admission, with complete G14/G17 and P0-P7 still open:
**14 qualified / 3 partial**.

Current column-attribute candidate requalification:
[Two current-source original descriptor executions](field_attribute_boundary_5_0_6.md#current-source-requalification)
confirm the locked Rust attribute-injection behavior and exact C++ refusal;
one valid model matches all 24 property descriptors, and 13 strict controls
reject. The separate compatibility decision is pending. Complete AP02/G14/G17
and P0-P7 remain open: **14 qualified / 3 partial**.

Current expression child-rule source checkpoint:
[A 330-case control-flow, closure, cast, range and match matrix](audit_grammar_branch_review_5_0_6.md#expression-child-rule-matrix)
extends the retained metadata corpus to **79,836/79,836** matching locked
Rust/current Java outcomes and **152/152** passing Java tests. It adds no
generated-caller or protected-publication evidence. Complete AP02/G14/G17 and
P0-P7 remain open: **14 qualified / 3 partial**.

Current generated Credential execution checkpoint:
[A complete School model with a password property](audit_generated_credential_execution_5_0_6.md)
matches original/C++ `Credential` field policy, an actual SQLite
`password LIKE` query, twelve audited Save pairs and twelve fresh-process
ordinary/debug file cases. Its password canary differs from the masked name
and intent canary. Ordinary and authorized debug SQL redact the password
binding while preserving the query result. Complete AP02/G14/G17, other
credential/model/sink paths and P0-P7 remain open: **14 qualified / 3 partial**.

Current audited Credential mutation continuation:
[The generated School password update](audit_generated_credential_execution_5_0_6.md#audited-password-mutation)
matches twelve original/C++ SQLite Save pairs with distinct initial/updated
password values, rollback/conflict readback, Policy approval/denial and a
post-commit event-sink error. Delivered safe events force `[REDACTED]` and
`masked=true` with the locked Rust reason even though the explicit mask list
only names `name`. The separate query and file-sink controls remain intact.
Complete AP02/G14/G17 and P0-P7 remain open: **14 qualified / 3 partial**.

Current Credential mutation SQL publication:
[Four actual original/C++ UPDATE file cases](audit_generated_credential_execution_5_0_6.md#mutation-sql-file-publication)
match the exact redacted `password` UPDATE line in ordinary and authorized
debug sinks. Neither the initial nor updated password appears, and the
debug file is absent without acknowledgement. The same 45-step run retains
the twelve Save pairs and twelve query-sink controls. Complete AP02/G14/G17,
arbitrary sinks and P0-P7 remain open: **14 qualified / 3 partial**.

Current generated binding-policy execution checkpoint:
[Four complete generated School configurations](audit_generated_binding_policy_execution_5_0_6.md)
carry `Masked`, explicit-empty `Plain`, omitted-attribute `Plain` and
partial-parse `Unknown` into actual SQLite Q, audited Save and file logs.
The governed `Unknown` Save event and SQL diagnostics redact the public
synthetic binding, including authorized debug output; a direct safe-audit
projection without diagnostic redactions retains it in both languages. This
distinction remains an explicit publication boundary. Each model has twelve
matching Save pairs and twelve actual file-sink cases. Complete AP02/G14/G17,
other model/sink combinations and P0-P7 remain open: **14 qualified / 3 partial**.

Current generated audit-to-SQL/sink checkpoint:
[Three source-derived inheritance model configurations](audit_generated_sql_sink_expression_5_0_6.md)
each pass original/C++ compiled School, Academy and ResearchAcademy metadata
(**9/9** across the independent generations), twelve audited SQLite Save outcomes including explicit policy
approval, School/Enrollment graph composition and two-operation failure/
conflict readback, direct safe audit projection with generated mask/limit, and
actual School query/safe SQL comparison. The three runs cover **36 Save pairs**
and **36 fresh-process ordinary/debug file cases**, enforcing exact plaintext permission;
**532** inputs and dependency hashes are unchanged in each run. This is bounded AP02/AP05/
AP07/AP08/AP09 evidence. Other graph/approval combinations, arbitrary sinks and
complete G13/G14/G17/P0-P7 remain open: **14 qualified / 3 partial**.

Current generated binding-redaction checkpoint:
[Three source-derived domains](audit_generated_binding_redactions_5_0_6.md)
now pass original/C++ generated descriptor-to-`from_bindings` execution across
**42 fields**. The **7** safe captures and **3** forced captures in the
plaintext-enabled path match, with **9** descriptors and **526** unchanged
input hashes. SQL and log sinks remain separate; G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**.

Current generated metadata callers:
[Three selected nested/delegated inputs](audit_delegated_nested_callers_5_0_6.md)
now pass complete original/C++ generated entity execution, matching **9
descriptors / 42 field-privacy policies**. The service replacement is a
descriptor result; SQL and protected publication are not executed. Complete
AP02/G14 and G13/G17/P0-P7 remain open: **14 qualified / 3 partial**.

Current delegated/nested source checkpoint:
[The two historical diagnostics](audit_delegated_nested_checkpoint_5_0_6.md)
are now permanent source-derived corpora. Fresh original/current execution
matches **79,346/79,346** outcomes: **60,215** accepted effects and **19,131**
checked refusals; **150/150** Java tests pass. The new scope contains 6,016
diagnostic cases in addition to the preceding nested-item continuation.
Complete AP02 grammar, modeled configurations and protected consumers remain
open: **14 qualified / 3 partial**. The quoted-column decision remains pending.

Current nested-item parser checkpoint:
[The historical 2,143-case declaration matrix](audit_nested_item_checkpoint_5_0_6.md)
now runs against fresh locked Rust and the installed Java parser as a permanent
source-derived corpus. The expanded gate matches **73,330/73,330** metadata
outcomes, including **56,113** complete accepted effects and **17,217** checked
refusals, and passes **148/148** Java tests. Complete AP02 grammar, other model
configurations, protected consumers and G13/G14/G17/P0-P7 remain open:
**14 qualified / 3 partial**. The quoted-column decision remains pending.

Current adapted inherited-business source-provenance checkpoint:
[Inherited identifiers, governed saves and updates](inherited_business_driver_provenance_5_0_6.md)
binds **2 complete corpora / 80 observations**, all matching adapted
source/corpus/native results. A fresh reference-locked original-source project
reproduces **20 expected compiler errors**; the legacy lock's **46 differing
registry identities** remain historical. The unchanged adapter creates **43
derived files**, with exactly **5 changed entity files**; untouched producer
acceptance stays false. The **610-file** archive and **75 copied inputs** are
verified. Five-model helpers, native **5/5**, prior key/classifier checks **2/2**
and negative controls **35/35** pass. Source records cover all **305 disjoint
direct-corpus locators / 323,568 primary observations** under their declared
private-function, adaptation and profile scopes. Registry **934/305/197** is
enumeration. All **1,814 C++ inputs**, producer implementations and **80 SDKs /
3,560 files** remain unchanged. Complete custom/protected execution, metadata
and current integration remain open: **14 qualified / 3 partial**.
The column-attribute compatibility decision remains pending.

Previous private key/classifier source-provenance checkpoint:
[Query metadata keys and bootstrap retry classification](private_key_classifier_driver_provenance_5_0_6.md)
binds **2 complete corpora / 2 unchanged drivers / 757 observations**, all matching
source/corpus/native results. The **610-file** archive, **55 copied inputs** and
reference-locked dependencies are verified. Native **5/5**, retained SQLite schema
checks **2/2** and negative controls **31/31** pass. Cumulative bindings cover
**303 disjoint corpora / 323,488 observations**; **2 adapted inherited-business
direct entries** remain. Registry **932/305/196** is enumeration. Original key
identity shells, native 64-bit scope and private classifier extraction remain
explicit; the complete 31-file generated SDK is an artifact, not a compiled
package in this checkpoint. All **1,814 C++ inputs**, producer implementations
and **80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom/protected
execution and G13/G14/G17/P0-P7 remain open: **14 qualified / 3 partial**.
The column-attribute compatibility decision remains pending.

Previous public SQLite schema source-provenance checkpoint:
[Public schema creation and reconciliation](sqlite_schema_driver_provenance_5_0_6.md)
binds **1 complete corpus / 1 unchanged driver / 735 observations**, all matching
source/corpus/native results. The **610-file** archive, **10 copied inputs** and
reference-locked dependencies are verified. Native **4/4**, retained runtime-bridge
checks **2/2** and negative controls **19/19** pass. Cumulative bindings cover
**301 disjoint corpora / 322,731 observations**; **4 other direct entries** remain.
Registry **930/305/195** is enumeration. All **1,814 C++ inputs**, producer
implementations and **80 SDKs / 3,560 files** remain unchanged. Complete metadata,
custom/protected execution and G13/G14/G17/P0-P7 remain open:
**14 qualified / 3 partial**. The column-attribute decision remains pending.

Previous runtime-bridge source-provenance checkpoint:
[Context events, typed aggregates and loaded graphs](runtime_bridge_driver_provenance_5_0_6.md)
binds **3 complete corpora / 3 complete driver bodies / 4,802 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**19 copied inputs** remain unchanged. Native **5/5**, previous private-protocol
checks **2/2**, current controls **23/23** and retained private-protocol controls
**26/26** pass. Source bindings cover **300 disjoint corpora / 321,996
observations**; **5 other direct entries** remain. The **928/305/194**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Original runtime bodies and test bodies retain their exact prefixes;
declared bridge constants and wrapped complete driver bodies are appended.
Existing observation projections and fixture-path adaptation remain explicit. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous private-protocol source-provenance checkpoint:
[Private runtime protocols and Debug/Release behavior](runtime_protocol_driver_provenance_5_0_6.md)
binds **7 complete corpora / 6 observer modules / 23,657 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**24 copied inputs** remain unchanged. Native **9/9**, previous generated-model
checks **2/2**, current controls **26/26** and retained generated-model controls
**27/27** pass. Source bindings cover **297 disjoint corpora / 317,194
observations**; **8 other direct entries** remain. The **926/305/193**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Original runtime bodies and test bodies retain their exact prefixes;
only declared observer modules are appended. Rust Debug/Release and the C++
probe's header-counter mode keep separate scopes. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous generated-model source-provenance checkpoint:
[Generated entities, inheritance, constants and runtime assembly](generated_model_driver_provenance_5_0_6.md)
binds **26 complete corpora / 20 original targets / 16,674 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**490 copied inputs** remain unchanged. Native **28/28**, previous Context
checks **2/2**, current controls **27/27** and retained Context controls
**20/20** pass. Source bindings cover **290 disjoint corpora / 293,537
observations**; **15 other direct entries** remain. The **924/305/192**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Nine generated packages compile completely; two source-only
models retain their unchanged entity/Checker library scope and original full-crate
failures. Historical observer projections remain explicit. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous Context capability source-provenance checkpoint:
[Context, logs, I18n, telemetry and business IDs](context_public_driver_provenance_5_0_6.md)
binds **17 complete corpora / 16 original targets / 16,376 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**41 copied inputs** remain unchanged. Native **19/19**, previous core-model
checks **2/2**, current controls **20/20** and retained core-model controls
**19/19** pass. Source bindings cover **264 disjoint corpora / 276,863
observations**; **41 other direct entries** remain. The **922/305/191**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. The original logging shell and human-time observer adapter
retain their documented limits; complete Context-event bridges remain separate. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous core-model source-provenance checkpoint:
[Core values, query state and graph/request models](core_model_driver_provenance_5_0_6.md)
binds **34 complete corpora / 27 original targets / 10,625 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**93 copied inputs** remain unchanged. Native **36/36**, previous numeric/time
checks **2/2**, current controls **19/19** and retained numeric controls
**19/19** pass. Source bindings cover **247 disjoint corpora / 260,487
observations**; **58 other direct entries** remain. The **920/305/190**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Each corpus retains its existing semantic scope. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous numeric/time source-provenance checkpoint:
[Numeric, time and auxiliary values](numeric_time_driver_provenance_5_0_6.md) binds
fifteen complete corpora to **nine original targets / 41,382 observations**,
all matching source/corpus/native results. The complete 610-file archive,
30 copied inputs, exact local packages and reference checksums are verified.
Native **17/17**, prior SQLite-provider tool checks **2/2**, current controls **19/19**
and retained SQL controls **19/19** pass. Source bindings cover
**213 disjoint corpora / 249,862 observations**; **92 other direct entries**
remain. The **918/305/189** registry/direct/custom counts are locators, not a
full 918-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous SQLite provider source-provenance checkpoint:
[SQLite provider and diagnostics](sqlite_provider_protocol_provenance_5_0_6.md) binds
eleven complete corpora to **seven original targets / 9,152 observations**,
all matching source/corpus/native results. The complete 610-file archive,
27 copied inputs, exact local packages and reference checksums are verified.
Native **13/13**, prior SQL-tool checks **2/2**, current controls **19/19**
and retained SQL controls **19/19** pass. Source bindings cover
**198 disjoint corpora / 208,480 observations**; **107 other direct entries**
remain. The **916/305/188** registry/direct/custom counts are locators, not a
full 916-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous SQL/SQLite pipeline source-provenance checkpoint:
[SQL and SQLite pipeline](sqlite_pipeline_driver_provenance_5_0_6.md) binds
eighteen complete corpora to **fifteen original targets / 12,064 observations**,
all matching source/corpus/native results. The complete 610-file archive,
42 copied inputs, exact local packages and reference checksums are verified.
Native **20/20**, prior document-tool checks **2/2**, current controls **19/19**
and retained document controls **20/20** pass. Source bindings cover
**187 disjoint corpora / 199,328 observations**; **118 other direct entries**
remain. The **914/305/187** registry/direct/custom counts are locators, not a
full 914-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous document/reference source-provenance checkpoint:
[Document and secure-reference protocols](document_protocol_driver_provenance_5_0_6.md)
bind seven complete corpora to **six original targets / 2,941 observations**,
all matching source/corpus/native results. The complete 610-file archive,
twenty copied inputs, exact local packages and reference registry checksums
are verified. Native **9/9**, prior query-tool checks **2/2**, document controls
**20/20** and retained query controls **24/24** pass. Source bindings cover
**169 disjoint corpora / 187,264 observations**; **136 other direct entries**
remain. The **912/305/186** registry/direct/custom counts are locators, not a
full 912-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous generated-query source-provenance checkpoint:
[Generated queries and governance](query_governance_driver_provenance_5_0_6.md)
bind fifteen retained corpora to **13 original targets / 18,055 complete observations**,
all matching source/corpus/native results. The full 610-file runtime archive,
136 copied inputs, original School fixture, generated provenance and actual
locked dependencies are verified. Native **17/17**, previous pagination-tool
checks **2/2** and **24** effective controls pass. Source bindings cover
**162 disjoint corpora / 184,323 observations**; **143 other direct entries**
remain. The **910/305/185** registry/direct/custom counts are locators, not a
full 910-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous dynamic-governance source-provenance checkpoint:
[Dynamic-field governance and audit](dynamic_governance_driver_provenance_5_0_6.md)
bind nine retained corpora to **7 original targets / 5,915 complete observations**,
all matching source/corpus/native results. The full 610-file runtime archive,
73 copied inputs, original School fixture, generated provenance and actual
locked dependencies are verified. Native **11/11**, previous pagination-tool
checks **2/2** and **19** effective controls pass. Source bindings cover
**156 disjoint corpora / 172,183 observations**; **149 other direct entries**
remain. The **910/305/185** registry/direct/custom counts are locators, not a
full 910-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous pagination source-provenance checkpoint:
[Generated execution and pagination](pagination_driver_provenance_5_0_6.md)
bind five retained corpora to fresh original Rust/C++ execution: **4 targets /
10,888 observations**, all matching. The complete 610-file runtime archive,
65 copied source/project/model files, generated provenance, input recipes and
reference-compatible dependency graph are verified. Native **7/7**, previous
archive checks **2/2** and **19** effective controls pass. Source bindings now
cover **147 unique corpora / 166,268 observations**; **158 other direct entries**
remain. The **908/305/184** registry/direct/custom counts are locators, not a full
908-test run. All **1,814 C++ inputs**, producer implementations and SDKs remain
unchanged during this cohort. The [column-attribute diagnostic](field_attribute_boundary_5_0_6.md)
confirms a separate reference escaping defect; its compatibility decision is
pending and is not semantic acceptance. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.

Previous scalar-operand assignment checkpoint:
[Source-shaped sample assignment](sample_operand_assignment_5_0_6.md) fixes
24 actual Boolean getter differences and mixed-type SDK compilation failures.
Shared runtime conversion retains original mutation operands, source NULL
distinctions and Context ownership. Existing runtime function bodies are
byte-identical after removing the newly inserted helpers. Actual original/C++
sample comparisons pass **432 field outcomes**; four JSON descriptor callers
retain eight explicit approved type differences. Java **141/141**, scoped native
**6/6**, imported kernel **1/1** and ASan/UBSan kernel **1/1** pass. All **80 SDKs /
3,560 files** reproduce: only two reviewed JSON sample delegates change.
Ten body-scope controls and an input-epoch exit control pass. Four old C++ files
change and one new kernel test is added; frozen native893 identities are not
reused as current whole integration. Core corpus totals remain **142 / 155,380**,
with **163** other direct entries open. Complete metadata/provenance/protected
execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.

Previous large-text and sample-branch checkpoint:
[Large-text descriptor and sample precedence](large_text_metadata_5_0_6.md)
fixes 24 descriptor type differences and a public-SDK sample compilation
failure. Fresh Rust/C++ comparisons pass **2 models / 6 descriptors / 52
properties** and **144 deterministic scalar field outcomes**. Java **139/139**
includes replay of the **63,918 retained metadata contracts**; their original
Rust parser results reuse unchanged source/corpus identities. Scoped native
**10/10**, thirteen strict controls and pinned format/check pass. All **80 SDKs /
3,560 bodies** reproduce unchanged, with refreshed source fingerprints.
Only CppCoreGenerator production source changes; all **1,813 frozen C++ inputs**
remain unchanged. The approved JSON exception is preserved. The core source
corpus totals remain **142 / 155,380**, with **163** other direct entries open.
Complete metadata/provenance/protected execution and G13/G14/G17/P0-P7 stay open:
**14 qualified / 3 partial**. There is no new full native, module or sanitizer run.

Previous property-shape normalization checkpoint:
[Generated length and numeric shape metadata](property_shape_metadata_5_0_6.md)
now reuse the locked producer's normalization. Two complete model callers,
six descriptors and 44 property rows match fresh Rust/C++ execution; the old
emitter differed in 16 rows / 22 values. Current metadata regression is
**63,918/63,918**, Java **137/137** and scoped native **4/4**. A separate scoped
ASan/UBSan replay verifies the compiler observer's Context-close correction.
All **80 SDKs / 3,560 files** reproduce unchanged bodies; producer provenance
is refreshed. All **1,813 frozen C++ inputs** remain unchanged. This is a new
generator/test-harness epoch, not a new full native893 run. The existing
142 source corpora / 155,380 observations and remaining 163 direct entries
are unchanged; these model callers are a separate metadata gate.
G13/G14/G17 and P0-P7 stay open: **14 qualified / 3 partial**.

Previous declared-driver completion checkpoint:
[the final three SQL/mutation drivers](sql_mutation_driver_provenance_5_0_6.md)
match **2,048** fresh original/corpus/native observations. All 610 archive files,
eight copied source/project files, actual dependency metadata and complete input
recipes are retained. **25** controls, native **5/5**, focused **2/2** and source
guards **29/29** pass. Native/modules configurations pass without fresh imported
execution. Source bindings now cover **142 unique corpora / 155,380 observations**.
All **107 declared-driver project/view responsibilities** are covered across
their distinct source epochs. **163 other direct registrations** and remaining
custom CLI, dependency and normalization duties stay open. The **905/305/183**
registry/direct/custom counts are locators, not a full 905 run. All 1,813 C++ inputs,
producer implementations and SDKs remain unchanged. Frozen native893 keeps its
original identities. Complete metadata, provenance, protected execution and
G13/G14/G17/P0-P7 remain open; capability status stays **14 qualified / 3 partial**.

The preceding [entity API](entity_api_driver_provenance_5_0_6.md),
[private-runtime](private_runtime_driver_provenance_5_0_6.md) and
[query-store](query_store_driver_provenance_5_0_6.md) checkpoints retain their
original scopes, failures and sealed input identities.

Previous Registry provenance checkpoint:
[all seven source-project targets](registry_driver_provenance_5_0_6.md) match
**27,257** fresh original/native observations. Both exact runtime appendices,
prepared targets, manifests, dependency locks and complete output are retained.
**31** controls, native **13/13**, focused **2/2** and source guards **21/21** pass;
native/modules configurations pass. The runtime's 273 test names are enumerated;
Registry has zero embedded tests, and enumeration is not a test execution claim.
Cumulative source bindings cover **72 corpora / 106,987 observations**;
**233 direct entries** and **68 declared-driver project/view responsibilities**
remain. The **897/305/179** registry/direct/custom counts are locators.
Frozen native893 keeps its original inputs; this is no full897 run. All 1,813
C++ inputs, producer implementations and SDKs remain unchanged. Complete metadata,
provenance, protected execution and G13/G14/G17/P0-P7 stay open.
Capability status remains **14 qualified / 3 partial**.

Previous entity-runtime provenance checkpoint:
[all seven source-project targets](entity_runtime_driver_provenance_5_0_6.md)
match **4,226** fresh original/native observations and pass all **21** original
unit tests. Exact runtime source/view, observer, project and dependency identities
are retained; **29** controls, native **19/19**, focused **2/2** and current
source guards **19/19** pass. Cumulative source bindings cover **65 corpora /
79,730 observations**; **240 direct entries** and **72 declared-driver project/view
responsibilities** remain. The **895/305/178** registry/direct/custom counts are
locators. Frozen native893 retains its original inputs; this is no full895 run.
All 1,813 C++ inputs, producer implementations and SDKs remain unchanged.
Complete metadata, provenance, protected execution and G13/G14/G17/P0-P7 exits
stay open. Capability status remains **14 qualified / 3 partial**.

Previous field-column input checkpoint: [FC01-FC04](field_column_model_metadata_5_0_6.md)
qualifies eight actual model inputs, 24 ordinary/inherited columns and 72 typed
JSON hydration outcomes, with 29 strict controls and actual Context deletion.
The proposed production change was unnecessary and was never installed.
Current source **63,918/63,918**, Java **134/134**, source guards 17 and scoped
native/imported tests two each pass. Producer implementation, all 80 SDKs,
3,560 generated files, 2,467 protected artifacts and 1,813 C++ inputs retain
their preceding identities. SDK generation and the full native893 run are
explicitly reused with byte/hash checks. Complete grammar, other model
consumers, protected execution, SQ02 and G13/G14/G17/P0-P7 remain open.
Status stays **14 qualified / 3 partial**.

Previous default-model checkpoint: [DM01-DM08 reconciliation](default_model_literal_metadata_5_0_6.md)
repairs eleven of thirteen complete generated-model comparisons, including
undeclared versus empty masks and the resulting Unknown versus Plain
privacy policies. Actual StringTemplate CR removal is qualified separately
from direct Rust source parsing. Current source **63,918/63,918**, Java
**133/133**, 270 retained AST witnesses, thirteen model pairs, 39 descriptors,
182 privacy classifications and seventeen strict comparison controls pass.
Source guards pass 17 tests; scoped native and imported-module gates pass two
each. All 80 SDKs, 3,560 files and 2,467 protected artifacts reproduce unchanged;
manifests refresh only three helper fingerprints and retain all four shared
metadata helpers. All 1,813 C++ inputs retain the
[full native 893-test identities](native_provenance_integration_5_0_6.md), with
separate current source/caller proofs. This closes the named default container
consumer responsibilities. Complete delegated grammar, other model consumers,
protected execution, SQ02 and G13/G14/G17/P0-P7 signoff remain open. Status stays
**14 qualified / 3 partial**. Earlier checkpoints retain their original inputs.

Previous literal/source-line checkpoint: [LC01-LC08 reconciliation](audit_lexical_child_rules_5_0_6.md)
repairs 502 source-parser differences and two actual generated raw-string
metadata differences. Direct raw parsing and compiler source normalization
retain distinct contracts. Current source **63,918/63,918**, Java **132/132**,
270 retained AST witnesses, twelve generated caller pairs and sixteen comparison
controls pass. Source guards pass 17 tests; native and imported-module gates
pass two each. Two JSONL boundaries preserve Unicode content through twelve
controls; direct verification dependencies are fingerprinted. All 80 SDKs,
3,560 files and 2,467 protected artifacts reproduce unchanged. Twenty-three
legacy manifests gain the missing decoder identity; all record four shared
metadata helpers, and 31 provenance controls reject. All 1,813 C++ inputs retain
their [full native 893-test identities](native_provenance_integration_5_0_6.md),
with separate current source/caller proofs. Complete delegated grammar,
default model string consumers, SQ02 provenance and G13/G14/G17/P0-P7 signoff
remain open. Status stays **14 qualified / 3 partial**. Earlier checkpoints
retain their original evidence and scope.

Previous generic/bound checkpoint: [GB01-GB08 source reconciliation](audit_generic_child_rules_5_0_6.md)
qualifies declaration/binder ownership, const-number trivia, ordinary versus
strict nested-bound completion and preserved capture/path rules. The preceding
parser reproduces 2,371 differences in 7,607 new contracts across 903 shapes,
eight ordinary contexts and 383 additional encoded Type cases. Current source
**58,041/58,041**, Java **128/128**, 270 retained AST witnesses, twelve fresh
generated caller pairs and sixteen comparison controls pass. Source guards
pass 17 tests; scoped native and imported-module gates pass two each. All 80
SDKs, 3,560 generated files and 2,467 protected artifacts remain byte-identical;
manifests update only parser hashes. All 1,813 C++ inputs retain their
[full native 893-test identities](native_provenance_integration_5_0_6.md), joined
to explicit current source/caller proofs. Complete delegated AP02 grammar,
configurations/callers, SQ02 provenance and G13/G14/G17/P0-P7 exits remain open.
Status stays **14 qualified / 3 partial**. Earlier checkpoints retain their own
inputs, outcomes and scope.

Previous nested item checkpoint: [IT01-IT08 source reconciliation](audit_item_child_rules_5_0_6.md)
qualifies the named direct signature/modifier/field/where/container rules. The
preceding parser reproduces 2,200 differences in 8,776 new contracts across
1,097 shapes and eight metadata contexts. Current source **50,434/50,434**,
Java **127/127**, 270 AST witnesses, twelve fresh generated caller pairs and
sixteen comparison controls pass. Source guards pass 17 tests; scoped native and
imported-module gates pass two each. All 80 SDKs, 3,560 generated files and 2,467
protected artifacts remain byte-identical; manifests update only parser hashes.
All 1,813 C++ inputs retain their [full native 893-test identities](native_provenance_integration_5_0_6.md),
joined to explicit current source/caller proofs. Remaining delegated AP02
grammar/configurations, G13/SQ02 provenance and complete G17/P0-P7 exits remain
open. Status stays **14 qualified / 3 partial**. Earlier checkpoints below retain
their historical scope.

Previous statement checkpoint: [ST01-ST06 source reconciliation](audit_statement_child_rules_5_0_6.md)
fixes compound != macro lookahead and reconciles the named direct statement
rules. The preceding parser reproduces 216 differences in 8,120 new contracts
across 1,015 shapes and eight metadata contexts. Current source **41,658/41,658**,
Java **126/126**, 270 AST witnesses, twelve fresh generated caller pairs and
sixteen comparison controls pass. Source guards pass 17 tests; scoped native and
imported-module gates pass two each. All 80 SDKs, 3,560 generated files and 2,467
protected artifacts remain byte-identical; manifests update only parser hashes.
All 1,813 C++ inputs retain their [full native 893-test identities](native_provenance_integration_5_0_6.md),
with separate current metadata/caller proofs. Delegated AP02 grammar/configurations,
G13/SQ02 provenance and complete G17/P0-P7 exits remain open. Status stays
**14 qualified / 3 partial**. Earlier checkpoint statements below retain their
historical scope.

Previous block/array checkpoint: [BC01-BC04 source correction](audit_block_child_rules_5_0_6.md)
repairs 1,944 actual metadata differences in 6,544 new contracts across 818
expression shapes and eight metadata contexts. Current source **33,538/33,538**,
Java **125/125**, 270 AST witnesses, twelve fresh generated caller pairs and
sixteen comparison controls pass. Source guards pass 17 tests; scoped native and
imported-module gates pass two each. All 80 SDKs, 3,560 generated files and 2,467
protected artifacts remain byte-identical; manifests update only parser hashes.
All 1,813 C++ inputs retain their [full native 893-test identities](native_provenance_integration_5_0_6.md),
with separate current metadata/caller proofs. Full G14/AP02 grammar, G13/SQ02
provenance and complete G17/P0-P7 exits remain open. Status stays **14 qualified /
3 partial**. Earlier checkpoint statements below retain their historical scope.

Current governed-runtime reconciliation: [GP01-GP16](governance_qualification_requirements_5_0_6.md) preserves all22 historical G17 questions, reconciles13 runtime contracts against35 accepted source owners and retains3 full exits. Fresh original/current generated Save360, ten strict result controls and native40/imported40 pass. Four missing permanent imported comparator registrations and one existing canonical probe/preset entry are added; runtime functions, generated SDKs and module exports remain unchanged. Pinned format/check1,808, canonical1,164,75 root locators and reference/path guards pass. GP11/G14 metadata completeness, GP13/SQ02 provenance and GP16/SQ05 whole integration remain open; capabilities stay14 qualified/3 partial and P0-P7 incomplete. Earlier scopes below retain their original evidence.

Current type child-rule follow-up: [TC01-TC06 requalification](audit_type_child_rules_5_0_6.md) repairs156 source-proven metadata differences across900 type contracts. Installed9,494 complete source outcomes, 121 Java tests and twelve fresh original/native model pairs (36 descriptors/168 privacy tuples) pass. Fourteen strict descriptor controls reject, including twelve actual previous false acceptances. All80 SDKs/3,560 generated files and2,467 protected artifacts remain unchanged; only80 parser fingerprints refresh. Native2/imported2, pinned format/check1,808 and reference/path guards pass. Remaining AP02 child-rule/lexical/caller, complete SQ02 provenance, G13/G14/G17 and P0-P7 requirements stay open. Older scopes below retain their own inputs and outcomes.

Current audit-grammar follow-up: [witnessed branch review](audit_grammar_branch_review_5_0_6.md) repairs45 source-proven builtin-expression differences. Installed8,594 complete metadata outcomes, 270 actual AST witnesses and120 Java tests pass; nine fresh original/C++ model pairs match27 descriptors/126 privacy tuples. Eight strict metadata controls reject, including five actual previous false acceptances. All80 SDKs/3,560 generated files and2,467 protected artifacts remain unchanged; only80 parser fingerprints refresh. Scoped native2/imported2 pass. AP02 child-rule/lexical/caller reconciliation, complete SQ02 provenance, G13/G14/G17 and P0-P7 remain open. Prior counts below retain their own epochs.

Current forward-owner follow-up: [FRI01 requalification](generated_forward_relation_identity_5_0_6.md) preserves the original declared model key for forward getters independently of descriptor-based reverse/JSON views. Source/current4800, exact preceding-producer76 differences, native10/imported4/Java89, twelve result/three strict-prefix controls and80-SDK restricted reproduction pass. G01 is restored; complete G13/G14/G17/P0-P7 remain open. Earlier counts below retain their own input epochs.

Current caller continuation: [generated identity Save and Fix provenance](generated_identity_save_5_0_6.md)
corrects48 canonical-model/descriptor-label differences after inspecting complete
Context Fix output. Installed360/360 original/current SQLite calls, current
Java87/native9/module2 and80-SDK byte/argument verification pass. The source
protected-output and commit boundaries are observed independently. This restores
G02 without accepting complete AP01-AP12/G14/G17. The following source grammar
and full-native counts retain their original frozen checkpoint scopes.

The current [installed syntax continuation](audit_postfix_grammar_5_0_6.md)
matches **8,252/8,252** unchanged source observations and **114/114** Java tests;
24 fresh original/C++ model pairs match 72 descriptors and 336 field-policy
tuples. The exact preceding parser reproduces 1,694 differences and fails the
new permanent regression. All 80 ordinary SDK destinations reproduce identical
implementation bytes with provenance-only refresh. The preceding frozen C++
epoch passes **877/877**; its subsequent [source cohort](runtime_core_source_closure_5_0_6.md)
records 122 accepted/0 pending/2 external exclusions. These current scopes
supersede stale counts below without relabeling historical receipts. AP02's
full grammar/identity/caller reconciliation, AP03's approved naming exception and final signoff
and complete AP01–AP12/G14/G17/P0–P7 obligations remain open.

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

Latest AP02 continuation: the [installed type/pattern regression and exploratory
consumer follow-up](audit_grammar_branch_review_5_0_6.md#installed-type-and-pattern-continuation)
match 79,998 locked-source metadata outcomes and 153 Java tests at that
checkpoint. The [installed cross-branch continuation](audit_grammar_branch_review_5_0_6.md#installed-cross-branch-composition-regression)
now matches 80,160 outcomes and 154 Java tests. Three new
generated caller pairs match nine descriptors and 42 field-privacy tuples; one
related School configuration also matches twelve audited SQLite Save outcomes
and twelve sink observations. The previous post-install full native epoch
completed 1,063/1,063 before the newer Java-test and comparator edits.
These finite cases do not complete AP02, G14, G17 or P0-P7.

| ID | Responsibility | Current evidence and remaining work |
| --- | --- | --- |
| AP01 | Declared/inherited masks, unknown names, declaration state and credential priority | [Compiled metadata](generated_privacy_contracts_5_0_6.md) and [mask resolution](generated_privacy_metadata_5_0_6.md) retain original model keys, declaring-parent resolution and property/column classification. A relation mask need not match its FK property; preserve the actual source result rather than silently improving it. |
| AP02 | Audit-length literal interpretation, ignored expressions and target width | The [latest delegated/nested checkpoint](audit_delegated_nested_checkpoint_5_0_6.md) extends the fresh original/installed source corpus to 79,346 matching cases and 150 Java tests. It includes the earlier [2,143-case nested-item checkpoint](audit_nested_item_checkpoint_5_0_6.md) and the two separately retained diagnostic matrices. Three [generated caller pairs](audit_delegated_nested_callers_5_0_6.md) additionally match 9 descriptors and 42 field-privacy classifications, without executing SQL. [FC01-FC04](field_column_model_metadata_5_0_6.md) qualify actual inline column parsing before source literals: 8 models/24 columns/72 JSON outcomes/29 controls. Their source63,918/Java134/source17/native2/imported2 gate retains its own epoch. Prior DM and TC/AG/PT/OP/BC/ST/IT/GB/LC scopes remain valid. The earlier [delegated grammar diagnostic](audit_delegated_grammar_diagnostic_5_0_6.md) matches 1,043 observations: 801 accepted effects and 242 checked refusals. The separate [nested signature/pattern diagnostic](audit_nested_signature_diagnostic_5_0_6.md) matches 4,973 outcomes: 3,301 accepted effects and 1,672 checked refusals. Their permanent replay is still bounded metadata evidence; complete delegated grammar, other configurations and remaining consumers keep AP02 partial. |
| AP03 | Canonical own-field wire mappings, empty aliases and naming failures | [Wire mappings](generated_wire_mappings_5_0_6.md) qualify twelve valid profile/model combinations and nested ownership. **Approved exception (2026-10-24):** preserve explicit C++ rejection of invalid `json_field_naming`, with the original successful empty-wire artifacts retained. The [contract](invalid_wire_naming_compatibility_exception_5_0_6.md) and immutable manifest bound this difference; fresh115 Java tests include three invalid profiles. G12 is qualified through its complete R01-R18 signoff; complete G14 acceptance remains independent. |
| AP04 | Entity, storage, scalar, FK/reverse-relation and diagnostic identities | [Relationship identities](relationship_identity_contracts_5_0_6.md), [scalar diagnostics](scalar_diagnostic_repairs_5_0_6.md), [expression entries](expression_entry_identity_5_0_6.md) and [mixed graph composition](generated_ledger_composition_5_0_6.md) retain canonical metadata independently of C++ escaping and application method names. [Container identities](container_metadata_5_0_6.md) additionally qualify runtime keys/layout names, independent table/service/relation metadata, typed JSON lookup and embedded-NUL strings in their recorded scope. Reuse accepted G01/G04/G06/G07/G10 behavior without inferring whole protected execution. [Core metadata MD01-MD12](core_metadata_source_review_5_0_6.md) additionally accepts the underlying descriptor source; whole generated/protected execution remains separate. [DM01-DM08](default_model_literal_metadata_5_0_6.md) qualify default table/service/container literal interpretation, partial effects and absent/empty masks through thirteen actual original/installed model pairs. Other consumers and complete protected execution remain separate. [FC01-FC04](field_column_model_metadata_5_0_6.md) additionally qualifies named inline field-column parsing and72 actual typed JSON consumers; physical SQL identifiers and other configurations remain independent. |
| AP05 | Metadata reaches actual SQL binding classification and safe rendering | [Complete SQL-log source acceptance](sql_log_source_review_5_0_6.md#durable-source-and-frontend-qualification) qualifies SL01-SL16, including every query-expression fallback, exact field/column policy and no-I/O traversal. Its joint source gate matches 3,091 observations per frontend and passes native19/imported19. [Logging source replay](logging_baselines_5_0_6.md), [42 generated binding redactions](audit_generated_binding_redactions_5_0_6.md) and [private generated queries](dynamic_json_privacy_5_0_6.md) retain their own scopes. The binding gate invokes actual `from_bindings` but no SQL provider or sink. Descriptor emission alone cannot accept publication; whole governed runtime responsibilities remain under G17/SQ03. |
| AP06 | Request-owned/nested intent, readonly/facet/query shapes and protected reads | [Complete Context logging/vector source acceptance](context_logging_source_review_5_0_6.md) qualifies CL01-CL18, including all twelve original paths, request-owned root reason across five operations and independent-query isolation. Original2/formatter4, native15/imported15 and 3,373 source observations per frontend pass. The private-query matrix retains 1,728 real results and 936 complete timeless safe logs, its conservative executor fallback and nested diagnostic source. Capturing fixture-authored metadata does not replace generated/provider execution; complete governed callers remain separate. |
| AP07 | Raw/safe audit identity, bootstrap provenance, snapshots and field limits | [Audit event repair](audit_event_provenance_5_0_6.md) and [dynamic projection](dynamic_audit_projection_5_0_6.md) supersede the old claim that those fields are absent. [Generated School Save](audit_generated_sql_sink_expression_5_0_6.md) matches three source-derived audit lengths through direct projection and persisted updates delivered to a safe event sink, including masked outputs with and without truncation. Raw events and safe publication are distinct; raw Debug is not a safe sink. Preserve explicit target identity and immutable provenance snapshots. |
| AP08 | Metadata through governed changes, actual persistence and committed audits | [Complete dynamic storage acceptance](dynamic_storage_acceptance_5_0_6.md) joins actual typed saves, policy plans, authoritative readback, rollback/conflict and committed audit-delivery failures. Its exact-dependency save replay has 5,144 observations, including the 1,152 typed-governance cases. The [generated School Save](audit_generated_sql_sink_expression_5_0_6.md) separately checks three source-derived audit-length configurations through twelve SQLite outcomes each, including explicit single-entity and graph approval, a two-entity pending-mutation graph Save, whole-plan denial, later School write failure and child/root optimistic conflicts. The mixed graph fixture separately covers keyword relations and explicit ledger composition. Neither is ordinary CRUD substituted for graph Save. |
| AP09 | Ordinary/sensitive sinks, exact permission, credentials, truncation and errors | [Complete privacy/masking source acceptance](runtime_privacy_source_review_5_0_6.md#durable-source-and-frontend-qualification) qualifies LP01-LP16 and all 34 SQLite masking contracts. [Complete formatter/publication source acceptance](log_publication_source_review_5_0_6.md#durable-source-and-frontend-qualification) qualifies LF01-LF18: all four original tests, 927 formatter/configuration cases, 94 fresh manager fixtures per implementation and canonical ownership/failure contracts pass. Its joined gate is native15/imported15. The original PostgreSQL masking case remains C++-excluded. [Private query sinks](dynamic_json_privacy_5_0_6.md) and the [three generated School sink matrices](audit_generated_sql_sink_expression_5_0_6.md) retain their own scopes; arbitrary custom sinks, whole assembly and governed publication remain independent G17 obligations. |
| AP10 | Context allocation, explicit retention, expiration and pending cleanup | Wire-map and event ownership contracts disable default PMR, copy into an explicit destination and actually destroy UserContext. Accepted G15/G16 state and provider contracts remain dependencies. Ordinary wrappers must not extend Context lifetime or create another ownership domain. |
| AP11 | Generated artifacts, accurate guidance and cross-layer integration | [Explicit current identity reuse](parity/column_model_installed_delta_5_0_6.json.gz) preserves all producer implementations,80 SDKs/3,560 files/2,467 protected artifacts and all manifests. Only regression/corpus/verifier inputs change. Earlier regeneration, helper-provenance and protected artifact proofs retain their scopes. Complete SQ04/G14/P7 exits stay open. |
| AP12 | Complete current integration and honest source/capability status | [Full native 934 and installed verifier continuation](custom_cli_admission_milestone_5_0_6.md) retain their historical identities. The [post-install AP02 native epoch](parity/ap02_type_pattern_full_native_terminal_5_0_6.json.gz) passes 1,063/1,063 with zero failures/errors/skips and unchanged frozen inputs. The first [cross-branch/comparator epoch](parity/ap02_cross_composition_full_native_terminal_5_0_6.json.gz) runs all 1,063 tests on 11,811 unchanged inputs and ends with one [isolated admission-fixture failure](custom_case_admission_fixture_5_0_6.md). The [repaired full epoch](parity/ap02_cross_composition_full_native_repaired_terminal_5_0_6.json.gz) passes 1,063/1,063 with zero failures, errors or skips on 11,811 unchanged inputs; later Python comparator guards have selected gates only. Earlier 1,058/1,058 and parser/caller checkpoints retain their own scopes. Earlier 893, MIPS query-key 451 and metadata/caller evidence retain their original scopes. Complete AP01-AP12, SQ02 and G13/G14/G17/P0-P7 remain open; status stays 14 qualified / 3 partial. |

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


The [creation identity correction](generated_creation_identity_5_0_6.md) separately
qualifies raw model ID allocation, returned allocator-error handling and Context
graph binding before thin metadata emission. Its 90 complete observations span
five original SDK configurations and preserve source/result separation. AP04's
whole governed Save/Policy/audit caller reconciliation and complete G14/G17 remain
open; original query-provider identity evidence does not cover creation by itself.
