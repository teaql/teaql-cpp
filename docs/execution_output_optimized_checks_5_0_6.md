# Execution comparator checks under optimized Python

This SQ02/G13 repair preserves the locked Rust 5.0.6 references: runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. It changes four Python acceptance
entry points and their native/module registrations. It does not change C++
runtime behavior or generated code.

The [pre-edit witness](parity/execution_output_optimized_preedit_5_0_6.json.gz)
runs the actual historical comparator CLIs against five valid and fifteen
invalid subprocess fixtures. Ordinary Python distinguishes them correctly.
Optimized Python incorrectly accepts all fifteen invalid fixtures. In the
service-error comparator, optimization also removes the calls that start the
C++ and optional Rust programs: the missing-program fixture reports success
without attempting execution.

| Active comparator | Unconditional source checks | Required behavior |
| --- | ---: | --- |
| [Service errors](../scripts/verify_runtime_service_errors_hardened_5_0_6.py) | 3 | Execute the supplied programs and compare all five retained diagnostic branches. |
| [Log callback close](../tests/parity/check_sample_log_callback_close_hardened.py) | 1 | Require protected termination with exit code 86 before Context arena reclamation. |
| [Optimistic conflict output](../tests/parity/teaql_runtime/optimistic_conflict_output_hardened.py) | 3 | Require successful execution and the test listing; reject the internal conflict message in either output stream. |
| [Dynamic warning privacy](../tests/parity/check_search_warning_canary_hardened.py) | 4 | Require the test listing and omission marker; reject either secret canary in either output stream. |

The original four scripts remain byte-for-byte unchanged at their historical
paths. The [transition manifest](parity/execution_output_hardened_transition_5_0_6.json.gz)
retains the complete original and active sources. All eleven replacements
have exact whole-AST equivalence to changing each `assert` into an
unconditional `if`/`AssertionError`; there are no other source adaptations.
The only change to each of the eight original CTest commands is its comparator
path. Source/build directory templates permit the guard to verify the current
build without depending on a second configured profile.

The [permanent guard](../tests/parity/verify_execution_output_optimized_controls_5_0_6.py)
checks source hashes, the historical witness, imported helpers and the four
registrations for the selected profile. It rejects five source/registration
corruptions. Subprocess markers prove that required programs actually ran;
a missing executable must fail. All five valid fixtures pass and all fifteen
invalid fixtures reject in ordinary and optimized Python. A direct imported
source-validator check rejects substituted service observations in both modes.
The [control receipt](parity/execution_output_hardened_controls_5_0_6.json.gz)
retains the forty fixture results, two imported-helper rejections and removal
of its owned temporary directory.

The [selected execution receipt](parity/execution_output_selected_tests_5_0_6.json.gz)
runs the actual C++ binaries through the four comparators and profile-specific
guard. Native CTests pass **6/6** normally and **6/6** with
`PYTHONOPTIMIZE=1`, including the required source fixture. Module CTests pass
**5/5** in each mode. JUnit membership matches the selected registries exactly,
with zero failures, errors or skips. All 24 source/evidence inputs and
eight executable hashes remain unchanged during the runs. These are targeted
executions; no full 1,090-test native epoch has run.

Synthetic fixtures qualify the acceptance decisions separately from those
actual C++ executions. Service-error comparisons use retained original Rust
observations; this repair does not establish a fresh Rust execution or
complete SQ02 provenance. The native registry has **1,090** tests and **608**
distinct Python entry scripts; **59** retain **174** direct `ast.Assert` nodes.

The three receipts record removal of **27,396 bytes** of owned fixture and
JUnit files. Repeated guards also remove their own subprocess directories.
Compact compressed evidence remains in the repository; compatible native and
module outputs remain in the committed presets' stable build directories.

The remaining direct subprocess comparators include generated expression
diagnostics/values, runtime assembly, sample execution/failures/state and
SQLite raw error debugging. Their source-bound dispatch contracts require
explicit transitions when registrations change. Other assertion-bearing
source/control entry points and imported helpers also need review. Removing
an assertion count alone cannot close SQ02, G13/G14/G17 or P0-P7.
