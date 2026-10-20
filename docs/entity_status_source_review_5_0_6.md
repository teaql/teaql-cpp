# Entity lifecycle status source review: Rust 5.0.6

Current source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Status: complete source reconciliation; scoped execution is recorded separately.
The source remains pending formal disposition and complete current native
integration. This does not accept higher graph, Save or policy responsibilities.

All 363 lines of `teaql-runtime/src/entity_status.rs` are byte-identical to the
retained 5.0.5 reference `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The current replay checks that identity directly rather than relabeling old
execution results.

## Complete responsibility reconciliation

| ID | Original responsibility | C++ mapping and boundary |
| --- | --- | --- |
| ES01 | Four `EntityAction` variants | Strong enum preserves Update, Delete, Persist and Recover independently of status. No wire representation or identical ABI is promised. |
| ES02 | Seven `EntityStatus` variants and default New | Strong enum preserves New, Persisted, PersistedDeleted, Updated, UpdatedDeleted, UpdatedRecover and Refer; value initialization selects New. |
| ES03 | Eleven valid transition pairs | `next_status` implements the complete source table, including repeated Update/Delete/Recover and the distinct deleted/recovered persistence outcomes. |
| ES04 | Seventeen invalid pairs and exact reasons | All remaining state/action combinations reject. C++ `std::invalid_argument` adapts Rust `Result<_, String>`; the complete message retains both source variant names and punctuation. |
| ES05 | Five independent predicates | `need_persist`, `is_new`, `is_updated`, `is_deleted` and `is_recover` preserve every state, including Refer and PersistedDeleted. Deleted and pending persistence are separate conditions. |
| ES06 | Display on both enums | `to_string` returns the exact variant spelling as a view of a static literal. It does not allocate or retain Context data. |
| ES07 | Derived Debug on both enums | `entity_status_debug.hpp` writes the variant name through the shared Debug protocol. Normal, pretty, hexadecimal and pretty hexadecimal modes are compared independently. |
| ES08 | Clone, Copy, PartialEq, Eq, Hash and status Default | C++ value copies and enum equality preserve identity. Standard enum hash containers distinguish all seven/four variants; numeric hash bytes are language-specific. Trivial-copy compile assertions and default construction retain their separate tests. No serde protocol is invented. |
| ES09 | Ownership and error adaptation | These scalar enums carry no Context allocation or borrow. Copying one does not extend any owning Context. Temporary native exceptions adapt source owned String errors; this review makes no allocation-free error-path claim. Invalid numeric enum casts are outside the Rust variant domain. |
| ES10 | Sixteen embedded tests | The unchanged original source is compiled directly as a Rust test crate. The complete handwritten C++ counterpart covers the original assertions and intermediate create/update/delete/recover lifecycle states, plus complete invalid-pair diagnostics and enum hash capabilities. Higher execution callers retain their own reviews. |

The implementation remains in `entity_status.hpp` and
`entity_status_debug.hpp`. No production declaration, function body, dependency,
generated artifact or canonical C++ test body changes for this review.

## Current execution and provenance

The [scoped replay](parity/entity_status_source_replay_final_5_0_6.json.gz)
checks the original source, original observer driver, manifest and lock,
reference lock and helper hashes before execution. It uses the original
`runtime_error_debug_baseline_5_0_6_cases.json.gz` corpus and its matching
`rust_final_baselines_5_0_6` project. Registry versions/checksums remain a subset
of the exact reference lock. Inputs, expected results and observers are unchanged.

The first [selection attempt](parity/entity_status_source_replay_5_0_6.json.gz)
incorrectly paired the public-check corpus with the final-baselines project;
its manifest guard rejected that selection before any compilation. The two
projects and corpora remain intact. The corrected replay uses the actual
manifest/lock provenance recorded by the diagnostic corpus.

The receipt retains executed commands, compiler results, complete original/native/
imported observations, selected JUnit and frozen input/reference checks. An
incomplete receipt is not a passing result. In particular, unrelated diagnostic
observations do not substitute for the 28 complete status/action combinations.

The corrected scoped gate is terminal and passes: **16/16** original Rust tests,
the complete freshly compiled native counterpart, **2/2** imported contract/
comparator checks and **1,174/1,174** diagnostic observations through each of
the original, fresh native-header and imported probes. Exactly **28** observations
cover every status/action pair, both Display implementations, both Debug
implementations in four modes, the default and all five predicates. Every
captured execution input and both reference identities remain unchanged.
The independent full native/source-disposition requirement remains open.

No new sanitizer, MIPS, physical-device, full-suite or complete capability result
is claimed. The seven status variants remain distinct from mutation-plan,
entity-snapshot and generated-value types.
