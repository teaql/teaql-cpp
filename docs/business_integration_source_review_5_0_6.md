# Business clock and permutation test source review: Rust 5.0.6

Current source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: the complete 29-line `teaql-runtime/tests/business_clock.rs` and
128-line `teaql-runtime/tests/business_id_permutation.rs`, including fixture
helpers. This does not accept the separate production clock, Context, business-ID
service or persistence sources. Formal source disposition and complete current
native integration remain open.

Both files are byte-identical to the retained 5.0.5 reference
`62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The [current receipt](parity/business_integration_source_replay_5_0_6.json.gz)
compares the complete source text against that commit before execution;
historical results are not relabeled.

## Complete assertion and fixture reconciliation

| ID | Original responsibility | Retained C++ counterpart |
| --- | --- | --- |
| BI01 | Fixed clock produces the complete 2032-02-29 10:15:30 UTC instant and its date | `business_clock.cpp` uses the identical leap-date/time fixture and checks both independent values. The clock is allocated and registered through its UserContext. |
| BI02 | BusinessDate resource overrides date without replacing time | The second complete test installs 2040-01-02 through the typed resource boundary and retains the original fixed instant. |
| BI03 | Complete cross-language vector fixture | Both files have identical bytes: header, all ten rows and all ten columns. The C++ program verifies each encoding and complete `ORD-<period>-<code>` value. |
| BI04 | Retained-range determinism after reconstruction | All 20,000 sequences encode twice with reconstructed scope/key. Equality, six-character length, uppercase/digit alphabet and uniqueness are checked for every sequence. |
| BI05 | Domain, key-version and scope rejection | Domain-size sequence rejects as Exhausted; key version zero rejects; a blank namespace rejects as InvalidDefinition. |
| BI06 | Invalid allocation range | Initial sequence 2 with maximum 1 rejects as InvalidDefinition. The test makes no subsequent allocation; this review does not invent a post-failure sequence assertion. |
| BI07 | Fixture construction helpers | Tenant, aggregate, namespace and period retain their exact strings. Both helpers decode the same 32-byte hex key. C++ additionally checks complete fixture parsing; malformed fixture diagnostics are not a business API contract. |
| BI08 | Explicit C++ allocation adaptation | The clock program owns Global/User scopes. The permutation program exercises allocator-taking kernel APIs with an explicit local Arena and allocator-propagating containers. Both disable default PMR allocation. No borrow escapes either program and no asynchronous/provider behavior is introduced. |

The complete handwritten programs and CSV remain unchanged. Native CTest already
registers them as `original_runtime_business_clock_contracts` and
`original_runtime_business_permutation_contracts`. This replay compiles those
same bodies from the current native compile flags into isolated outputs, using
the six completed runtime archives. It does not replace a CMake/full-suite run.

## Current execution and boundaries

The receipt retains complete original sources, fixture bytes, compiler commands,
original test output, native program results and frozen execution inputs/reference
identities. It checks all six native archive hashes before and after execution.
An incomplete receipt is not a passing gate. No output normalization, shortened
retained range, fixture substitution or generated implementation is used.

The current scoped receipt is terminal and passes all **6/6** original Rust
tests and both freshly compiled complete native programs. All ten CSV vectors
and all 20,000 retained-range iterations execute without reducing their original
assertions. Every captured execution input, both references and all six completed
native archives remain unchanged throughout the replay. Formal source disposition
and complete current native integration remain separate requirements.

These six original test groups do not establish general generator parity,
all business-ID profiles, concurrent allocation, provider transactions, graph
Save or complete ownership qualification. Existing wider source and lifetime
reviews remain separate. No new sanitizer, MIPS, physical-device, imported
counterpart or full native result is claimed here.
