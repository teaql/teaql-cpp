# Generated and SQLite comparator integrity under optimized Python

This SQ02/G13 repair covers seven installed comparison CLIs. References remain
Rust 5.0.6 runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. C++ runtime behavior, generated
artifacts and original source executions are unchanged.

The [pre-edit witness](parity/generated_cli_optimized_preedit_5_0_6.json.gz)
runs the actual historical CLIs in an owned workspace, using output fixtures
derived from their retained observations. In all seven CLIs, incorrect source
metadata rejects normally but passes under optimized Python. Six also accept
truncated and extra-case matrices in both modes. SQLite raw Debug accepts an
empty matrix as `0/0 match` in both modes. The generated-diagnostic CLI already
rejects incomplete matrices and retains that behavior. The witness records
58 executions, exact sources, corpora, installed commands and temporary cleanup.

| Active CLI | Direct checks made unconditional | Retained cases |
| --- | ---: | ---: |
| [Generated E diagnostics](../tests/parity/compare_generated_expression_diagnostics_hardened_5_0_6.py) | 7 | 196 |
| [Generated E values](../tests/parity/compare_generated_expression_values_hardened_5_0_6.py) | 3 | 108 |
| [Runtime assembly](../tests/parity/compare_generated_runtime_assembly_hardened_5_0_6.py) | 3 | 72 |
| [Sample execution](../tests/parity/compare_generated_sample_execution_hardened_5_0_6.py) | 3 | 18 |
| [Sample failures](../tests/parity/compare_generated_sample_failures_hardened_5_0_6.py) | 3 | 28 |
| [Sample state](../tests/parity/compare_generated_sample_state_hardened_5_0_6.py) | 3 | 144 |
| [SQLite raw Debug](../tests/parity/compare_sqlite_from_sql_raw_debug_hardened_5_0_6.py) | 1 | 36 |
| Expression/query/dynamic-JSON and SQLite control runners | 3 | Separate source/command controls |

All eleven historical source files remain unchanged. The
[transition manifest](parity/generated_cli_hardened_transition_5_0_6.json.gz)
retains their complete sources and active copies. It qualifies 26 exact
whole-AST assertion replacements before eighteen explicit adaptations:
six matrix-shape/count admissions and their imports, two source validators'
imports and dispatch admissions, and two control runners' validator imports.
The registered shared expression control also serves query and dynamic JSON;
all three registrations now execute the unconditional mutation checks.

The [dispatch adapter](../scripts/generated_cli_source_transition_5_0_6.py)
maps exactly seven historical comparator identities to their active copies,
checking both source hashes. Historical source contracts keep their original
comparators and hashes. Active source validators still require exact CTest
commands and source prerequisites. The sole change to each of the 24 existing
native/module commands is its active script path. The remaining source,
dependency, profile, result and normalization requirements are retained.

The [permanent guard](../tests/parity/verify_generated_cli_optimized_controls_5_0_6.py)
checks every transition and command. For each CLI, a valid matrix passes while
foreign source metadata, empty/partial/extra matrices, additional row metadata,
wrong results, missing/extra outputs and blank output records reject. The
[control receipt](parity/generated_cli_hardened_controls_5_0_6.json.gz)
records all **140** runs across ordinary and optimized Python, seven source or
dispatch corruptions and removal of its owned workspace. These fixtures test
the comparators; they do not constitute fresh C++ or Rust observations.

The [affected build receipt](parity/generated_cli_selected_build_5_0_6.json.gz)
records successful native and module builds in the stable preset directories.
The [selected execution receipt](parity/generated_cli_selected_tests_5_0_6.json.gz)
passes **21/21** actual native CTests in ordinary Python and **21/21** with
`PYTHONOPTIMIZE=1`, including the required generated-model source fixture.
Module selections pass **6/6** in each mode. Exact JUnit/registry membership
has no failures, errors or skips. All **50** source/evidence inputs and **33**
affected executable hashes remain unchanged. The existing source-control suites
reject **151** substitutions per mode: 41 expression, 49 query, 46 dynamic-JSON
and 15 SQLite cases. These targeted runs do not establish a full native epoch.

The final [source-format admission](parity/generated_cli_source_format_admission_5_0_6.json.gz)
preserves that execution's original input hashes. Removing extra EOF blank
lines from two active copies leaves their compiled Python code identical in
ordinary and optimized modes. Only those two source files, the manifest and
its guard's manifest identity differ from the retained epoch. The admission
retains the complete prior manifest and guard source, verifies all other inputs
and 33 binaries unchanged, and passes the current permanent guard in both
Python modes. This is an exact source transition after the recorded execution,
not a second 21-test native epoch.

The pre-edit fixture, permanent guard and selected JUnit receipts record removal
of **4,581,570 bytes** of disposable files. Compact receipts remain in the
repository, and compatible builds remain in the stable preset directories.

The native registry contains **1,091** tests and **609** Python entry scripts.
No directly registered `compare_*` entry retains an `ast.Assert`; **50** other
entry scripts retain **148** direct assertions. This inventory does not qualify
their imported helpers or admit historical comparator invocations inside other
controllers. In particular, lifecycle, helper, query-input and query-surface
controls still import the historical `raw_change` helper and require separate
review. Executable Python stored in fixture strings also needs review; a
direct-AST inventory does not include it.

The [next deferred-resource witness](parity/deferred_resource_optimized_preedit_5_0_6.json.gz)
reproduces four additional optimized-mode false acceptances in a `verify_*`
entry that performs actual result comparison: empty and missing observations,
a false pending flag and an incorrect business payload. Its valid 143-scenario,
163-payload fixture passes both modes. These are ten comparator fixture runs,
not native resource executions. Repair this acceptance path before treating the
remaining fifty scripts as control-only work. Complete SQ02, G13/G14/G17 and
P0-P7 remain open at **14 qualified /
3 partial**. No full 1,091-test native epoch has run.
