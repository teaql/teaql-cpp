# G02/G03 Checker, Behavior and fix requirements: Rust 5.0.6

Status: G02/G03 qualified with documented C++ language/ownership adaptations.
The ten original requirements below are retained. Their final source/body,
customization, failure-disposition and integration review is recorded in
[checker_behavior_acceptance_5_0_6.md](checker_behavior_acceptance_5_0_6.md).
The [pre-acceptance archive](parity/checker_behavior_pre_acceptance_5_0_6.json)
preserves this checklist and both capability entries before acceptance.
P1 retains its other integration responsibilities.

| Requirement | Evidence | Acceptance disposition |
| --- | --- | --- |
| Typed logic, no-op implementation and Checker wrapper for every supported model | Twenty generated workspaces, positive/negative compiler controls, ordinary/optional and five inherited Checker matrices. | The complete source branch and customization review is met, including actual model-role selection and preserved failed original bindings. |
| Required boolean/optional/text and Unicode scalar min/max helpers | Original generated School logic helper corpus; optional Reading custom logic and exact findings in three wire naming profiles. The new standalone closure verifies actual compiled observer bytes and replays all 274 helper/annotation results at the reference dependency lock. | Met at the current dependency lock; exact locations, operands, rule order and valid Unicode semantics are preserved. |
| Generated Behavior defaults and custom overrides | Ordinary, inherited and constant module factories; existing SQLite extension probes exercise select/insert/update. The direct Behavior matrix observes all six hooks through actual generated registration. [Protected Behavior](protected_behavior_5_0_6.md) adds 1296 real SQLite deletion/recovery outcomes with policy, intent, version, event and trace boundaries. | Met for all six default/custom hooks and protected deletion/recovery dispatch, with their distinct scopes retained. |
| Registration and application-owned extensions | Four module modes and existing root/bootstrap comparisons. A handwritten business example now extends generated ProductCheckerLogic/ProductBehavior. | Met through actual initialization/replacement, SQLite operations, explicit cleanup, generated guidance and compiler contracts. |
| Predefined fixes, loaded-update checks, Business-ID immutability and custom callback order | Tenant/Note, optional fields, Business IDs, five inherited models, inherited custom governed saves and bootstrap extension matrices. Current reference dependencies retain the new 450-case composition and the unchanged 1044 ordinary Business-ID helper observations. The shared runtime and thin string-clock adapter repair the demonstrated uniStr compilation failure, with 48 exact direct outcomes and native evidence/lifecycle controls. | Met through the source two-pass ordering, direct and governed matrices, configured fields and retained original binding failures. |
| Trusted Context-root creation and update rules | Absent/matching/mismatched roots; zero and populated FK values; Create/Update/Unknown; inherited and bootstrap models. | Met through exact direct findings/evidence and actual committed, denied, rolled-back and stale-version save configurations. |
| Create/update clocks and operation evidence | Shared-clock native/source fixtures, duplicate create-time evidence for updateTime, nested graph-fix scope, immutable policy-plan projections, and 24 scalar clock configurations with 828 exact original field outcomes. Timestamp-to-JSON conversion and non-convertible scalar storage remain distinct. | Met through clock multiplicity and operation-clock evidence, complete scalar/JSON/composition branches and explicit Business-ID/relation/optional-Timestamp dispositions. |
| Business-ID allocation/validation and evidence | Original profile/key/root/missing-service cases, SQLite recovery and accepted inherited binding adaptations. [450 inherited custom Checker/Policy saves](inherited_business_extensions_5_0_6.md) add committed, denied, trigger-failed and stale-version outcomes across all five models. | Met through original allocation/validation/helpers, inherited custom governed saves, explicit binding adaptations and SQLite recovery evidence. |
| Protected execution and failure boundaries | Successful Tenant/Note save, trigger rollback, optimistic conflict, policy denial, constant/bootstrap governance, private runtime saver replays and real protected delete/recover SQL. Raw autocommit event failures retain 42 actual committed row changes. | Met for generated protected dispatch/failure responsibilities. Full runtime/provider/transaction obligations remain under G13/G15-G17; raw autocommit evidence does not replace them. |
| Ownership, API guidance and final gates | Managed logic borrows, Context callback quiescence, explicit output copies and actual User deletion in existing source/native/lifetime matrices. | Met: current native29/module7/ASan-UBSan8, full compiler18-positive/44-negative, Java48, 23-workspace/44-SDK strict reproduction and accurate generated guidance. |

configureRuntime installs default generated registrations. Application overrides
must be applied after that configuration, before ensure_schema or business work.
Calling configureRuntime/setup again replaces the registrations. This follows
the implemented assembly contract; the new example must execute that ordering.

General assembly/sample data/complete Assist remains G12. Indexed snapshot/cache
semantics are G15, privacy metadata is G14, durable dynamic storage is G16 and
canonical intent/trace is G17. These boundaries preserve the full goal scope.
