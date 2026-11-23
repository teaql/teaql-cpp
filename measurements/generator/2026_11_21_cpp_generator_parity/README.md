# Generator parity run report — Rust 5.0.6

The P0-P7 goal completed on **2026-11-21T05:11:48+08:00**. All **17
capabilities** and the original eight phase exits are qualified for the agreed
SQLite/core edge scope. This report was assembled from retained evidence on
2026-11-23; creating it did not rerun builds
or test suites. The machine-readable [run record](run_record.json) preserves
commands, results, timings, usage and SHA-256 evidence references.

## Source and environment

| Item | Recorded identity |
| --- | --- |
| Rust runtime | 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` |
| Reference generator | `45e1572c4956ca963410d8ea436ead3c8857087a` |
| Independent C++ producer | `cpp_support` at `df0c496d04a9fe738f49fc387be729804a46c0de` |
| Runtime checkout ancestor | `977166b874f4f29244621394229cc776e5c7c83c` |
| Native environment | Linux x86_64; C++20; GCC libstdc++ 16.2.0 |
| Locked tools | LLVM 23.1.3, CMake 4.4.4, Ninja 1.13.2, SQLite 3.53.2 |

The runtime workspace contained uncommitted changes. Its ancestor commit alone
is not the tested source snapshot: the retained native input hashes and exact
post-test ledger transition identify the actual sources. The raw goal-service
objective still contains its initial 5.0.5 baseline wording; the later approved,
locked **5.0.6** baseline above governed this qualification. Historical 5.0.5
results retain their own scope.

## Results and evidence scope

| Check | Result | What it proves |
| --- | --- | --- |
| Complete native epoch | **1103/1103**, zero failures/errors/skips | Exact registry/JUnit membership and final no-work build. |
| Generated API compiler contracts | **201**: 57 positive, 144 negative | Actual ordinary compiler checks inside that native epoch. |
| Java suite | **403**: 98 shared, 305 C++; 331 compiled sources | Execution at the latest producer change, reused through unchanged producer inputs. |
| Retained generation | **80 SDKs / 3560 code and Assist files** | Fresh reproduction at the last producer change and later complete file continuity. |
| Current source partition | **718** Python registrations: 514 comparisons, 204 controls; 65 primary owners | Complete source/observer/selection responsibilities joined to current execution. |
| Source and native identities | **12151** frozen inputs; 708 consumers; eight library/tool inputs | Stable tested sources and actual pre-test/post-test native artifacts. |
| Imported artifacts | **634** executables, seven BMI files, thirteen archives | Exact continuity of the prior full 785-test module epoch plus later affected executions. |
| Final inventory | **17 qualified, zero unqualified** | `--require-complete`, P0, locked upstream and both whitespace checks pass. |
| Requirement closure | **12 AP / 16 GP / 8 SQ / 8 phases** | The accepted semantic/phase reviews and exact final metadata checks. |

Counts overlap: compiler cases are inside the native gate, and the 718 Python
registrations are a subset of 1103 tests. They are not added into a larger test
total. Responsibility reviews, actual comparisons and scoped identities support
the qualification; counts alone do not establish parity.

The accepted final audit is [the repaired final audit](../../../docs/parity/final_generator_completion_audit_repaired_5_0_6.json.gz),
which passes under ordinary and optimized Python. Its earlier field-name failure
remains [a failed audit record](../../../docs/parity/final_generator_completion_audit_5_0_6.json.gz). The
completion data was preserved; correcting that reader did not change the result.

## Timing and goal accounting

| Measurement | Recorded value |
| --- | --- |
| Goal created, local time | 2026-10-11T12:26:12+08:00 |
| Goal marked complete, local time | 2026-11-21T05:11:48+08:00 |
| Goal calendar window | 3516336 seconds (40d 16h 45m 36s) |
| Goal-service cumulative runtime | 1277787 seconds (14d 18h 56m 27s) |
| Goal-service token counter | 129,198,840; no token budget recorded |
| Final native controller | 7349.863121 seconds (about 2h 2m 30s) |
| Final CTest execution | 7275.78 seconds (about 2h 1m 16s) |

The controller ran from 2026-11-21T02:37:53.426411+08:00 to
2026-11-21T04:40:23.088902+08:00. Goal cumulative runtime, elapsed
calendar time and this final native epoch are different measurements. These
are operational timings, not a controlled performance benchmark. The token
counter has no billing/input/output breakdown here; no cost or token-savings
claim is inferred.

## Native command record

Activate the locked environment with `source scripts/toolchains/activate.sh`.
The following are the actual retained native-controller commands, all from the
runtime checkout. The original owned JUnit directory has been reclaimed; its
contents remain in the retained terminal receipt.

| Stage | Recorded command | Exit |
| --- | --- | --- |
| `reference` | `python3 scripts/verify_upstream.py` | 0 |
| `generator` | `python3 scripts/verify_generator_parity.py` | 0 |
| `configure` | `cmake --preset native` | 0 |
| `build` | `cmake --build --preset native` | 0 |
| `stable_build` | `cmake --build --preset native` | 0 |
| `registry` | `ctest --preset native --show-only=json-v1` | 0 |
| `ctest` | `ctest --preset native --output-on-failure --output-junit /tmp/teaql_full_native_receipt_repairs_mvestim8/epoch_junit.xml` | 0 |
| `post_build` | `cmake --build --preset native` | 0 |

The final inventory also ran `python3 -B scripts/verify_generator_parity.py
--require-complete`, `python3 -B scripts/verify_p0_inventory_5_0_6.py`,
`python3 -B scripts/verify_upstream.py` and `git diff --check` in both repositories.
Exact argv, working directories, outputs and exits are in the run record and
[completion-check receipt](../../../docs/parity/final_generator_completion_checks_5_0_6.json.gz).

## Failures retained and resolved

The [first full native epoch](../../../docs/parity/current_full_native_after_keyword_terminal_5_0_6.json.gz) finished with
seven failures. They were source/provenance issues: the producer working tree
was dirty, five receipt validations exceeded their limits, and one registered
boundary used a stale dependency hash. The producer was locally committed with
unchanged qualified source bytes; five limits became 600 seconds; the boundary
admitted only the exact previously qualified dependency transition. Every old
failure passed in the [final full epoch](../../../docs/parity/current_full_native_after_receipt_repairs_terminal_5_0_6.json.gz):

| Component | First epoch | Final epoch |
| --- | --- | --- |
| `parity_p0_inventory_5_0_6` | Failed | Passed |
| `generated_credential_mutation_receipt_contracts` | Timeout | Passed |
| `parity_audit_cross_context_corpus_contracts` | Timeout | Passed |
| `parity_ap02_reverse_relation_type_corpus` | Timeout | Passed |
| `parity_ap02_reverse_relation_sql_receipt` | Timeout | Passed |
| `parity_ap02_cross_context_partial_sql_receipt` | Timeout | Passed |
| `parity_registered_boundary_hardened` | Failed | Passed |

The semantic reader initially rejected mixed string/object evidence through an
invalid set conversion. The repair preserves the complete ordered evidence
prefix. The final completion auditor initially used two mismatched field names;
its corrected version reads the existing fields and rechecks every original
capability/requirement/phase. Original failures were retained, not relabeled.

After native execution, only the capability ledger JSON changed: G13/G14/G17
became qualified, old questions remained archived, current evidence was appended,
and 23 rich historical entries received canonical `path` indexes with all
original fields preserved. The [exact transition](../../../docs/parity/final_generator_metadata_transition_5_0_6.json.gz)
and [four-mode validation](../../../docs/parity/final_generator_metadata_validation_5_0_6.json.gz) bind that step.

## Scope, reuse and retained outputs

Formal generation uses the locked original compiler for complete Rust grammar,
descriptors, projections and semantic storage. Missing configuration and original
compiler refusal publish no SDK. The approved JSON, quoted-FK, invalid naming
profile and column-attribute dispositions remain separate; the later shared
`option`/`Option` keyword policy supersedes the earlier Option-model exception.

The imported results are explicit historical/affected-execution reuse, not a
fresh full module run. No fresh full sanitizer/MIPS, physical-router, GPU-adapter
or external-service execution is claimed for this closing run. Relevant lifetime
and frontend evidence is retained through exact unchanged source/artifact
identities. SQLite/core scope and two explicit Context lifetimes remain intact.

The completion record verifies nine owned temporary directories absent and
**1787946 bytes** reclaimed in its closing
sequence. Later audit receipts record their additional cleanup separately.
Stable preset outputs, original source, generated fixtures, databases and shared
caches remain retained. This report indexes existing compact evidence instead
of copying full logs, executables, SDKs or build trees.

## Primary records

- [Complete requirement/phase result and source snapshot](../../../docs/parity/final_generator_completion_5_0_6.json.gz)
- [Accepted final audit, ordinary and optimized Python](../../../docs/parity/final_generator_completion_audit_repaired_5_0_6.json.gz)
- [Full native commands, output and JUnit](../../../docs/parity/current_full_native_after_receipt_repairs_terminal_5_0_6.json.gz)
- [Native controller, source/artifact stability and cleanup](../../../docs/parity/current_full_native_after_receipt_repairs_lifecycle_5_0_6.json.gz)
- [Untruncated 201-case compiler output](../../../docs/parity/current_full_native_compiler_after_receipt_repairs_5_0_6.json.gz)
- [Accepted semantic review validation](../../../docs/parity/final_generator_semantic_signoff_execution_repaired_5_0_6.json.gz)
- [403-test Java execution and counterfactual regression](../../../docs/parity/projection_trailing_java_5_0_6.json.gz)
- [80-SDK reproduction](../../../docs/parity/projection_trailing_sdks_5_0_6.json.gz)
- [Imported-artifact continuity and explicit reuse](../../../docs/parity/current_module_continuity_after_repairs_5_0_6.json.gz)
- [Detailed acceptance report](../../../docs/final_generator_acceptance_5_0_6.md)
- [Complete semantic responsibility review](../../../docs/final_generator_semantic_signoff_5_0_6.md)

Every indexed evidence file has its exact byte size and SHA-256 in
[run_record.json](run_record.json). This report consolidates retained run and goal
artifacts; it is not a verbatim conversation transcript. Original hash-bound
acceptance records remain unchanged.
