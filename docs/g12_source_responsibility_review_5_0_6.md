# G12 source responsibility review

Status: eighteen finite responsibility groups identified against all 25 locked
G12 source bodies. G12/P6 remains partial; this is not a source-inventory claim
of parity. Runtime reference is Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`;
generator reference is `45e1572c4956ca963410d8ea436ead3c8857087a`.

The [complete review inputs](parity/g12_responsibility_review_before_integration_5_0_6.json.gz)
retain every source body/hash, exact source anchor, prior ledger and evidence
mapping. The groups below replace an indefinite request for more models with
specific responsibilities to verify; they do not waive any existing requirement.

| ID | Responsibility | Evidence and remaining disposition |
| --- | --- | --- |
| R01 | Crate exports and progressive discovery | [cpp generator](cpp_generator.md), [generated application workflows 5 0 6](generated_application_workflows_5_0_6.md), [generated field runtime assist 5 0 6](generated_field_runtime_assist_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R02 | Config/environment/error alternatives and provider path preparation | [runtime configuration 5 0 6](runtime_configuration_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R03 | Single SQLite connection sharing, typed aliases and explicit lifetime adaptation | [business runtime configuration 5 0 6](business_runtime_configuration_5_0_6.md), [generated registry schema 5 0 6](generated_registry_schema_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R04 | Conditional Business-ID infrastructure and managed environment keys | [Generated startup qualification](generated_business_environment_startup_5_0_6.md) closes the historical missing environment step, typed failure channel, constants and pre-executor order. Native/module/instrumented complete outcomes and explicit panic/exception disposition are retained; preserve in final combined integration. |
| R05 | Separate schema provider and model-domain executor; delegated query/stream/mutation/transaction | [generated registry schema 5 0 6](generated_registry_schema_5_0_6.md), [generated execution 5 0 6](generated_execution_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R06 | Three independent registries, default callbacks, replacement and checked ownership | [generated registry schema 5 0 6](generated_registry_schema_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R07 | Four module factories, passive assembly and governed root/constant bootstrap | [generated runtime assembly 5 0 6](generated_runtime_assembly_5_0_6.md), [constants acceptance 5 0 6](constants_acceptance_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R08 | Own-field wire mappings and separate aliases; three valid profiles and retained invalid-profile difference | [generated wire mappings 5 0 6](generated_wire_mappings_5_0_6.md); valid-profile forwarding and invalid-profile source defect remain separate G13/G14 dispositions. |
| R09 | Sample ID conversion, scale/plan/state/reference/report/skip/error contracts | [generated sample state 5 0 6](generated_sample_state_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R10 | Root/constant loading, source load-error fallback and dependency ordering | [generated sample execution 5 0 6](generated_sample_execution_5_0_6.md), [generated sample failures 5 0 6](generated_sample_failures_5_0_6.md), [catalog sample execution 5 0 6](catalog_sample_execution_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R11 | Fanout/scale, required skips, reference reuse and reverse publication | [inherited sample execution 5 0 6](inherited_sample_execution_5_0_6.md), [catalog sample execution 5 0 6](catalog_sample_execution_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R12 | All scalar/reference/Business-ID recipes and inherited/configured carriers | [generated sample fields 5 0 6](generated_sample_fields_5_0_6.md), [sample type execution 5 0 6](sample_type_execution_5_0_6.md), [business sample execution 5 0 6](business_sample_execution_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R13 | Actual audited save, failure/earlier commits/repeated invocation and pending cleanup | [generated sample failures 5 0 6](generated_sample_failures_5_0_6.md), [custom sample execution 5 0 6](custom_sample_execution_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R14 | All Info sites, custom callbacks and explicit Context sink/target adaptation | [sample progress logging 5 0 6](sample_progress_logging_5_0_6.md), [custom sample execution 5 0 6](custom_sample_execution_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R15 | Seven model-aware action workflows and field-specific APIs | [generated application workflows 5 0 6](generated_application_workflows_5_0_6.md), [generated field runtime assist 5 0 6](generated_field_runtime_assist_5_0_6.md), [dynamic json assist grammar 5 0 6](dynamic_json_assist_grammar_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R16 | Schema/debug/customization/telemetry/governance guidance with application-owned document boundaries | [generated field runtime assist 5 0 6](generated_field_runtime_assist_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R17 | Application vocabulary/catalog generation, managed installation and locale/checker behavior | [generated i18n 5 0 6](generated_i18n_5_0_6.md); preserve current integration and documented ownership/failure boundaries. |
| R18 | External Tool package availability; no fabricated C++ facades/endpoints | [generated field runtime assist 5 0 6](generated_field_runtime_assist_5_0_6.md); external Tool Context2 is explicitly unavailable, not relabeled as kernel support. |

## Repaired source startup behavior

The [before-repair C++ counterexample](parity/business_runtime_missing_key_counterexample_5_0_6.json.gz)
created a fresh checked in-memory connection with both key environment variables
absent. The partial C++ factory returned successfully. The retained actual
original pool factory returns typed MissingEnv for the same missing key. This
was a functional difference, not just different allocation or API spelling.

The earlier manual key injection path remains supported, but does not satisfy
this source factory responsibility. Follow the
[runtime-first repair plan](business_runtime_environment_alignment_5_0_6.md).
Keep the full error channel, conditional constants, setup order, three factory
entries and cleanup requirements; do not replace them with a success-only test.

The [key/environment kernel](business_key_environment_kernel_5_0_6.md) and
[provider initializer](business_runtime_initialization_5_0_6.md) now qualify the
shared runtime mechanics. The later
[generated startup checkpoint](generated_business_environment_startup_5_0_6.md)
now closes R04 with thin adapters, conditional constants, all three typed entry
paths and actual startup/save/readback/filesystem comparisons. It preserves every
unrelated SDK body/data/workflow and keeps the Rust panic/C++ exception
disposition explicit. Native21 plus an isolated predicate identity-guard follow-up,
module7, sanitizer4, Java75, compiler2 positive/4 negative and strict77
destinations qualify this responsibility. Full combined G12 integration remains
required.

## Current integration boundary

The selected native run builds 43 targets and executes 45 related contracts.
Its initial result is 44/45 with one 180-second bootstrap-extension timeout.
The imported selection builds its recorded targets and runs 34 related
contracts: initially 33/34 with the same timeout. Exact-prefix native/module
registration differences are recorded; these are not claimed to be identical
test sets.

The complete 2,304-case bootstrap comparator subsequently passes in an isolated
120.88-second run without changing source, binary, corpus, process order, values
or timeout properties. The native CTest then passes unchanged in 123.86 seconds.
The imported isolated CTest also passes unchanged; its exact elapsed time and
output are retained in the [follow-up record](parity/g12_bootstrap_ctest_followup_5_0_6.json.gz). Do not relabel the
initial cohorts as uninterrupted green runs or discard their failures.

The new field/runtime Assist checkpoint separately qualifies Java74, full
189-block compilation across63 SDKs, strict77-destination reproduction, native2/
module2 and pinned1644-file formatting. These gates establish those interfaces,
not complete source startup or P0-P7 parity on their own. The newer R04 checkpoint
above supplies the missing startup evidence without relabeling these older runs.

The [current acceptance audit](g12_acceptance_5_0_6.md) reconciles all eighteen
groups with the later R04 repair and adds the missing generated-catalog imported
checks. Current native51/module40 integration runs in recorded stages; R08
invalid-profile behavior still awaits user disposition. The three library/runtime/
sample generator drivers are now locked alongside their templates.

## Completion rule

Close each group with the applicable original comparison, typed failure,
ownership, artifact and current integration evidence. Keep source defects and
explicit lifetime adaptations visible. R04 is now qualified; G12 still requires
the final combined review/integration of all eighteen groups. Overall
G08/G13-G17 and final P7 acceptance remain separate requirements.
