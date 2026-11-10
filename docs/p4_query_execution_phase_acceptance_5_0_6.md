# P4 query and execution phase acceptance: Rust 5.0.6

Status: **accepted for the P4 exit criteria** in
[the generator parity plan](cpp_generator_parity_plan.md). This phase joins the
qualified G06 general-query, G07 predicate/projection and G09 execution/
pagination contracts. It does not accept G13 semantic qualification, G14
metadata/configuration grammar, G17 governance/provenance integration or P7.

The locked references are generator
`45e1572c4956ca963410d8ea436ead3c8857087a` and Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The source and capability
lock check verifies 42 generator sources and 17 capability records, with three
still partial. Source inventory alone is not the acceptance basis.

## Exit-criterion review

| P4 requirement | Evidence and disposition |
| --- | --- |
| Complete declared Q surface | [G06's twelve-row review](general_query_acceptance_review_5_0_6.md) covers constructors, source/result typing, request state and conversion, live/deleted modes, matching, child selection, trusted raw/computed surfaces, removal and model guards. Its source/SQLite and compiler evidence includes the later [creation-boundary requalification](generated_creation_identity_5_0_6.md). |
| Complete field, predicate and projection surface | [G07's twelve-row review](predicate_projection_acceptance_5_0_6.md) covers scalar, text, temporal, membership and property criteria; aliases, aggregate/function projection, grouping, removal and four ordering routes. The [field requirements](predicate_projection_requirements_5_0_6.md) retain source omissions and negative compiler cases instead of treating C++ extensions as Rust APIs. Actual typed SQLite and safe SQL comparisons cover the generated routes. |
| List, one, page, exists, count, stream and optimization | [G09's EX01-EX14 review](execution_acceptance_5_0_6.md) reconciles every entry, result typing, page order, ID-set and continuous protocols, stream restrictions, conversion failures and Context cleanup against complete source bodies and executed comparisons. Its [resolved-service requalification](resolved_g09_requalification_5_0_6.md) preserves the accepted execution contract after the later service repair. |
| Positive and negative public typing | G07's accepted compiler controls cover field and alias guards. G09's public execution gate has one positive and 24 independent negative scenarios across all eight entries, including missing purpose, extra Context and GlobalContext arguments. G06 also retains Entity-result and creation guards. Foreign UserContext identity is checked at runtime. |
| Intent, Policy and trusted-query composition | [Composed real SQLite execution](composed_general_queries_5_0_6.md) compares 4,864 original/native outcomes across ordinary and inherited private models, all eight entries and ordinary/ID-set/continuous configurations. Its 768 blank-intent cases reject before Policy/I/O; 1,536 Policy denials issue no SELECT. Trusted raw input remains application responsibility; protected diagnostics retain the observed redaction. |
| Provider failures, cancellation and Context binding | [Deferred facet/page execution](deferred_facet_page_5_0_6.md) retains stage order, injected failures, cancellation, wake/retry and actual Context deletion. Its generated API controls reject foreign and expired bindings for all eight entries before SQLite SELECT and verify an explicit destination copy/rebind. [Protected stream execution](protected_stream_shapes_5_0_6.md) retains provider errors, early close, statement reuse and the explicit eager-native/lazy-Rust startup disposition. |

The G06, G07 and G09 acceptance records retain source hashes, original/native
observations, positive/negative compiler contracts, generated-artifact
reproduction, Context lifetime and imported-module gates. The review above
checks their composition against **each P4 exit condition**; it is not an
inference from three qualified labels or a method count.

## Current integrated check

With `scripts/toolchains/activate.sh` sourced, the existing native targets were
built successfully with no work pending. The selected native CTest run passed
**16/16**, including general Q, predicate/alias SQL, execution shapes, generated
execution, composed Policy/privacy, deferred facet/page, protected stream,
Context binding and the preset's source-contract checks. Its total wall time was
119.79 seconds. The six corresponding imported-module targets built and linked
successfully; their selected CTest run passed **6/6** in 131.15 seconds.
The [current gate receipt](parity/p4_phase_current_gates_5_0_6.json) retains the
exact test names, per-profile counts and CTest log hashes.

`scripts/verify_upstream.py`, `scripts/verify_module_paths.py` and
`scripts/verify_generator_parity.py --generator-dir
/home/philip/githome/teaql_code_gen_cpp` passed. The generator check reports
42 locked sources and 17 capability records, three still partial. These are
source/structure checks; the cited execution, compiler and reproduction records
provide the behavioral evidence. No current full native epoch, new sanitizer or
MIPS result is claimed by this phase review.

The combined execution commands were:

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
    teaql_general_query_probe teaql_query_alias_probe \
    teaql_query_predicate_execution_probe teaql_generated_execution_probe \
    teaql_generated_execution_shapes_probe teaql_composed_general_probe \
    teaql_deferred_facet_page_probe teaql_stream_shapes_probe \
    teaql_execution_context_binding_tests -j 4
ctest --preset native --output-on-failure -R \
    '^(generated_execution_binding_contracts|deferred_facet_page_rust_parity|generated_execution_shapes_rust_parity|generated_execution_rust_parity|stream_shapes_rust_parity|composed_general_rust_parity|query_alias_rust_parity|query_predicate_execution_rust_parity|general_query_rust_parity)$'
cmake --build --preset modules_native --target \
    teaql_module_generated_execution_probe \
    teaql_module_query_predicate_execution_probe \
    teaql_module_query_alias_probe teaql_module_stream_shapes_probe \
    teaql_module_composed_general_probe \
    teaql_module_deferred_facet_page_probe -j 4
ctest --preset modules_native --output-on-failure -R \
    '^(module_generated_execution_rust_parity|module_query_predicate_execution_rust_parity|module_query_alias_rust_parity|module_stream_shapes_rust_parity|module_composed_general_rust_parity|module_deferred_facet_page_rust_parity)$'
```

## Remaining boundaries

The source-staging, C++ ownership, startup-timing and authentic source-failure
dispositions remain exactly those documented in G06/G07/G09. P4 acceptance does
not promote an arbitrary model configuration or protected sink to parity.
The approved quoted-FK and generated JSON exceptions remain with their own
capabilities. The unresolved quoted-column attribute decision and complete
AP02 metadata grammar remain G14 work. Complete G13 source semantics and G17
governance/provenance joins, the P1/P5/P6 phase exits and P7 release audit remain
independent.
