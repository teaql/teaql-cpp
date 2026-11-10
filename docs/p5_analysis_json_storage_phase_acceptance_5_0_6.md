# P5 analysis, JSON and storage phase acceptance: Rust 5.0.6

Status: **accepted for the P5 exit criteria** in the
[generator parity plan](cpp_generator_parity_plan.md). This phase joins the
qualified G08 aggregate/relation/facet, G10 dynamic-JSON query and G16
dynamic/readonly storage responsibilities. It preserves the approved
[quoted-FK](quoted_fk_compatibility_exception_5_0_6.md) and
[JSON payload](json_payload_compatibility_exception_5_0_6.md) exceptions at
their documented boundaries. It does not accept complete G13, G14, G17 or P7.

The locked generator is
`45e1572c4956ca963410d8ea436ead3c8857087a`; the locked Rust runtime is
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Each accepted capability
has a complete source-body/branch review, actual original/C++ observations and
its own negative controls. The phase conclusion below depends on those
observations and their composition, not on inventory or status labels alone.

## Exit-criterion review

| P5 requirement | Evidence and disposition |
| --- | --- |
| Scalar and relationship analysis | [G08 AG01–AG18](aggregation_qualification_matrix_5_0_6.md) covers all twelve aggregate functions, scalar/object field eligibility, forward/reverse matching, grouping, details, roll-up, relation statistics, facets, nested requests and real cache/storage protocols. The [post-approval review](approved_exception_capability_acceptance_5_0_6.md) retains complete source and integration evidence. The strict AG07 gate requires all 6,576 exact source results and 546 individually bounded approved C++ expectations; the original deficient quoted-FK results remain visible. |
| Dynamic-JSON query construction and execution | [G10 DJ01–DJ14](dynamic_json_acceptance_5_0_6.md) reviews parse errors, value/operator selection, self and relation paths, inheritance, ordering, pagination, sequential composition, trusted model recipes, Policy and Context ownership. Its original-dependency replay has 51,751 observations, including physical SQLite and protected execution matrices; the generated guidance uses the exact five wire keys. |
| Dynamic and readonly definitions, storage and loading | [G16 DS01–DS16](dynamic_storage_acceptance_5_0_6.md) retains definition/selection identity, indexed carriers, all ten SQLite dynamic types, provider/executor/store separation, typed root/relation/flat/compact hydration, readonly schemas and supported cursor enhancement. Its 918 protocol/loading and 5,144 Save/audit reference observations supplement provider/store corpora and the native 46/imported 45 acceptance checks. A selected shape is not conflated with a loaded NULL or with another entity's value. |
| Model and configuration composition | G08 AG17 retains nullable, inherited, constant/candidate, grouped, readonly and dynamic branches. G10 DJ09/DJ13 exercises authentic constant/candidate/Business-ID and inherited models. The [simultaneous JSON/readonly/persistent/facet matrix](dynamic_json_facets_5_0_6.md) compares 6,336 complete original/C++ executions: 3,672 successes, 1,584 Policy denials and 1,080 exact SQLite errors. Its loaded root Text/NULL, omitted fields, readonly values, facet sidecars, SQL and Policy observations retain distinct outcomes. |
| Explicit unsupported SQLite paths | Source-unsupported statistical/aggregate functions remain errors under AG01/AG17. The combined facet matrix retains the original two-column membership-subquery failure rather than turning it into a successful restricted facet. G16 DS12 leaves relation/aggregate stream enhancement unsupported; the generated compact facet helper does not invent persistent hydration. These failures and omissions are part of the accepted source-shaped contract. |
| Metadata authority, privacy and policy | G10 DJ14 and G16 DS15 prevent JSON/request values from installing definitions, readonly SQL, provider identity or authorization. [Private JSON execution](dynamic_json_privacy_5_0_6.md) and [simultaneous facets](dynamic_json_facets_5_0_6.md) retain pre-I/O Policy denial and safe SQL behavior. The current ordinary and imported sink tests pass. This verifies trusted metadata **authority** through the P5 runtime/provider path; complete attribute grammar and all protected publication configurations remain G14/G17 responsibilities. |
| Transactions, audit and cleanup | G16 DS10/DS14 retain whole-batch prevalidation, active transaction/store binding, native-before-extension writes, same-transaction authoritative readback, rollback/conflict and post-commit audit delivery. The [typed Save matrix](dynamic_typed_save_audit_5_0_6.md) compares all ten dynamic types across mutations and failure/policy modes; the current graph Save and SQLite storage contracts pass. G08 AG18, G10 DJ10/DJ11 and G16 DS12/DS16 retain checked Context borrows, provider/statement release, stream early close, cancellation/join and actual Context destruction. Explicit Global copies remain the only cross-lifetime retention path. |

The two approved compatibility exceptions preserve correct C++ JSON payload
behavior and quoted-FK relation loading without changing source observations or
expanding the exception to other models. Rust's authentic source failures,
supported operations and C++ ownership adaptations remain separately labeled
in the capability records.

## Current integrated check

Using the pinned tools, all selected native targets built with no pending work.
The combined native CTest selection passed **23/23** in 148.90 seconds. It
includes scalar/relation/cache comparisons, JSON/readonly/persistent/facet
composition, private sinks, SQLite storage and stream boundaries, typed dynamic
Save/audit, graph transaction behavior and the preset's source-contract checks.

Nine imported-module consumers were compiled and linked successfully. The
selected module CTest selection passed **12/12** in 98.29 seconds, covering the
same JSON/facet/privacy/storage/stream paths and typed relation/aggregate
inputs. The [current gate receipt](parity/p5_phase_current_gates_5_0_6.json)
retains exact test names, CTest log hashes, source checkout revisions and
per-profile outcomes. These are current selected integrations, not a new full
native or complete module-suite epoch.

The verification commands were:

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
    teaql_generated_aggregation_probe teaql_generated_relation_analysis_probe \
    teaql_generated_dynamic_json_facets_probe \
    teaql_generated_dynamic_json_persistent_probe \
    teaql_generated_dynamic_json_privacy_probe \
    teaql_generated_dynamic_json_probe teaql_sqlite_dynamic_storage_probe \
    teaql_dynamic_storage_probe teaql_generated_readonly_properties_probe \
    teaql_sql_dynamic_stream_source_probe teaql_typed_aggregate_scope_probe \
    teaql_generated_aggregation_cache_probe teaql_dynamic_save_audit_probe \
    teaql_dynamic_graph_save_tests teaql_sqlite_dynamic_storage_tests -j 4
ctest --preset native --output-on-failure -R \
    '^(generated_aggregation_rust_parity|generated_relation_analysis_rust_parity|generated_dynamic_json_facets_rust_parity|generated_dynamic_json_persistent_rust_parity|generated_dynamic_json_privacy_rust_parity|dynamic_json_execution_rust_parity|sqlite_dynamic_storage_rust_parity|dynamic_storage_rust_parity|generated_readonly_properties_rust_parity|sql_dynamic_stream_source_rust_parity|typed_aggregate_scope_rust_parity|generated_aggregation_cache_rust_parity|dynamic_typed_save_audit_rust_parity|dynamic_graph_save_contracts|sqlite_dynamic_storage_contracts|generated_dynamic_json_privacy_sink_contracts)$'
cmake --build --preset modules_native --target \
    teaql_module_generated_dynamic_json_facets_probe \
    teaql_module_generated_dynamic_json_persistent_probe \
    teaql_module_generated_dynamic_json_privacy_probe \
    teaql_module_generated_dynamic_json_probe \
    teaql_module_dynamic_storage_probe \
    teaql_module_sqlite_dynamic_storage_probe \
    teaql_module_sql_dynamic_stream_source_probe \
    teaql_module_typed_aggregate_scope_probe \
    teaql_module_generated_relation_analysis_inputs_probe -j 4
ctest --preset modules_native --output-on-failure -R \
    '^(module_relation_analysis_inputs_rust_parity|module_typed_aggregate_scope_rust_parity|module_dynamic_json_execution_rust_parity|module_dynamic_json_policy_rust_parity|module_generated_dynamic_json_facets_rust_parity|module_generated_dynamic_json_persistent_rust_parity|module_generated_dynamic_json_privacy_rust_parity|module_generated_dynamic_json_privacy_sink_contracts|module_dynamic_storage_rust_parity|module_sqlite_dynamic_storage_rust_parity|module_sqlite_dynamic_storage_boundaries_rust_parity|module_sql_dynamic_stream_source_rust_parity)$'
```

## Remaining boundaries

P5 accepts the locked source-selected analysis, JSON-query and dynamic/readonly
storage responsibilities, including the documented SQLite failures and bounded
exceptions. It does not qualify arbitrary custom sinks, every possible model
attribute grammar, complete provenance/privacy integration or the whole source
semantic matrix. G13/G14/G17, the P1/P6 exits and P7 remain active. No new
ASan/UBSan, MIPS/QEMU, GPU or physical-router result is claimed here.
