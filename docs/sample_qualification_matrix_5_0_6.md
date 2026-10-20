# Generated sample qualification matrix

This matrix tracks the complete sample template responsibilities within G12/P6.
It does not replace runtime assembly, progressive Assist or final G13/P7 gates.
The locked source is
`generator/src/main/resources/generator/rust-lib-core/sample_data/index.stg`
at generator `45e1572c4956ca963410d8ea436ead3c8857087a`, executed against
Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

Historical documents retain the scope and remaining work recorded when written.
Later evidence below supersedes their statements about inherited fields,
Business-ID skipping and progress logging; it does not relabel old runs.

| Source responsibility | Existing execution evidence | Remaining requirement |
| --- | --- | --- |
| Plan/state/report/error types; reference selection and reuse | [State](generated_sample_state_5_0_6.md), including empty pools and used-ID selection | Preserve in final combined sample regression. |
| Root loading; failed reference query becomes an empty pool | [Ordinary execution](generated_sample_execution_5_0_6.md) and [failures](generated_sample_failures_5_0_6.md) | Preserve exact skips and optional-reference behavior in final integration. |
| Dependency order, required skip, optional reference, reverse reference publication | Ordinary, failure and [inherited](inherited_sample_execution_5_0_6.md) full-row comparisons | [Constant/candidate sample execution](catalog_sample_execution_5_0_6.md) now matches all 240 original/native cases; retain the declared dependency states in final integration. |
| Non-constant generation versus loading existing constants | [Constant/candidate sample entry](catalog_sample_execution_5_0_6.md) preserves every constant-table snapshot across 368 original/native invocations, including removed/deleted/query-failed pools | Preserve this branch in final combined sample regression. |
| Per-entity fanout and Tiny/Small/Medium scales | Ordinary/inherited execution and [progress](sample_progress_logging_5_0_6.md) | Candidate/constant Tiny/Small fanout now matches actual original/native execution; preserve in final integration. |
| String, number, boolean, optional and temporal recipes | [Scalar/temporal fields](generated_sample_fields_5_0_6.md) | [Configured ordinary sample types](sample_type_execution_5_0_6.md) now qualify six accepted declarations through 360 actual original/native/imported cases and repair numeric-alias compilation without changing storage or source errors. Preserve the declared matrix and review wider type/metadata combinations under G13. |
| JSON-literal recipe | Actual original compiler rejection and ordinary JSON Text runtime failure in scalar evidence | Preserve these source outcomes; do not claim successful JSON seeding. Recheck guidance against those outcomes. |
| Business-ID recipe delegates to Checker/Fix | [Business-ID execution](business_sample_execution_5_0_6.md): four factories, missing roots/keys, repeats and real failures | Native/imported/instrumented and artifact gates pass; preserve in final combined sample regression. |
| Actual audited save, stop-on-error, earlier commits and repeated invocation | Ordinary/failure/inherited/Business-ID comparisons retain exact report/error and persisted rows | Application-owned allow/approval/denial now matches 3,248 complete policy plans and 24 denied invocations in the constant/candidate matrix. [Custom sample extensions](custom_sample_execution_5_0_6.md) add complete original/native/imported Checker/Behavior/Policy/Info comparison, including repaired fields, rejection, query ordering/failure and actual User deletion. Preserve pending/composed regressions in final integration. |
| Pending save resume/cancel/close and allocation ownership | Failure lifecycle gates, inherited and Business-ID null-PMR/actual User deletion | Preserve pending callback cleanup and cross-domain retention checks in final integration. |
| Start/progress/skip/model-completion/whole-completion log sites | Actual original INFO collector and Context-owned C++ sink in progress evidence | Custom sample extensions now compare 1,204 original Info messages through custom checker/policy/transport failure and disabled sinks, with actual User deletion; preserve source messages, target/ownership adaptation and pending cleanup in final integration. |
| Generated sample guidance | Reproduced model-aware Assist explains current methods and Business-ID setup/failures; five runtime/keyword/identifier spelling defects are repaired with actual guide compiler checks | Complete action/field guidance audit under the wider G12 developer-workflow requirements. |

Every remaining row requires actual generated-source execution or an explicit
source-failure/C++ ownership adaptation. Method inventories and model counts
alone cannot close a row. Final acceptance also checks the separate
[runtime assembly matrix](runtime_assembly_qualification_matrix_5_0_6.md) and
[full generator plan](cpp_generator_parity_plan.md).
