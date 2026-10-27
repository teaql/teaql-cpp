# Comparator checks under Python optimization

The 1,064-test native registry at commit `100242a9` ran 286 distinct
`compare_*_5_0_6.py` scripts. Fifty-one scripts still contained 425 Python
`assert` statements after the earlier optional Rust observer repair. Python
removes these checks under `-O` or a nonzero `PYTHONOPTIMIZE`, including checks
for corpus baselines, source and producer hashes, dependency identities, input
matrices and record paths. This is a source finding, not a claim that each
assertion had a separately reproduced false acceptance.

A [pre-edit witness](parity/dynamic_json_policy_baseline_optimized_preedit_5_0_6.json.gz)
changes only the retained dynamic JSON policy corpus baseline and uses a saved
result stand-in for all 3,072 cases. The old comparator rejects it under normal
Python but reports 3,072/3,072 under `python -O`. This demonstrates a
provenance failure; the stand-in is not fresh C++ runtime evidence.

| Source form | Assertions replaced |
| --- | ---: |
| Single-line standalone statement | 409 |
| Single-line inline suite | 13 |
| Multiline standalone statement | 2 |
| Statement followed by another statement after a semicolon | 1 |

All 425 checks now use `if not (...): raise AssertionError(...)`. The
[installed manifest](parity/remaining_comparator_assert_rewrite_installed_5_0_6.json.gz)
pins all 51 before/after file hashes. Its complete AST comparison shows that
each condition, failure message, statement position and surrounding statement
is preserved. The permanent
[normal/optimized control](../tests/parity/verify_remaining_comparator_assert_controls_5_0_6.py)
rechecks this equivalence against the pre-edit Git source, runs valid and
altered 3,072-case comparisons under both Python modes, and rejects six
corrupted Clock source transitions. Its
[installed receipt](parity/remaining_comparator_assert_controls_installed_v2_5_0_6.json.gz)
records those focused checks. An earlier control receipt predates the Clock
negative cases and remains historical.

Sixteen active JSON source contracts replace 57 exact current source hashes.
Four existing contracts retain their older optional-observer transition and
pin a second, exact comparator transition. Clock retains the original Rust
source receipt and validates a separate current overlay for its changed
comparator. The transition helper checks the installed manifest and actual
file bytes. The optional-observer control still rejects `/bin/false` in 50
normal and 50 optimized runs, plus 28 transition corruptions. Clock source
contracts and source controls pass their focused native gates. Original Rust
execution receipts are unchanged.

The preceding [1,064-test native attempt](parity/current_native_interrupted_1064_5_0_6.json.gz)
was deliberately interrupted after 786 passes with no observed failures.
It has no terminal JUnit and is not a complete-suite result. The current
registry adds the permanent comparator control for 1,065 tests. The historical
candidate patches and manifests under `docs/parity/remaining_comparator_*`
record the reviewed pre-install transformations; the installed manifest and
current files are authoritative.

The [selected native run](parity/remaining_comparator_selected_native_5_0_6.json.gz)
passes **69/69** tests: all 53 affected comparator registrations and 16
automatically required source fixtures. Its terminal JUnit has no failures,
errors or skips; all selected names are present, frozen source hashes are
unchanged, and the task-owned temporary directory was removed. This is a
bounded gate, not a complete 1,065-test current-source epoch.

These changes advance comparator execution and source provenance for the named
cohort. Complete SQ02, G13/G14/G17 and P0-P7 qualification remains open.
