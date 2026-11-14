# Active generated-model source checks under optimized Python

This is a bounded SQ02/G13 repair for eight currently registered generated-model
source verifiers and their eight negative-control entries. The original Rust
5.0.6 executions, historical Python verifier and control sources, registries,
corpora and generated SDKs remain unchanged. It does not close SQ02, G13, G14,
G17 or P0-P7.

The [pre-edit witness](parity/generated_driver_optimized_preedit_5_0_6.json.gz)
changes only `complete` to `false` in a task-owned copy of the retained dynamic
governance execution. The historical verifier rejects it under ordinary Python
but exits successfully under `python3 -O`, reporting complete generated-model
source bindings. This is a saved-evidence false acceptance, not a new Rust or
C++ execution result.

The active [hardened verifier](../scripts/generated_driver_provenance_hardened_5_0_6.py)
and [hardened control entry](../tests/parity/verify_generated_driver_controls_hardened_5_0_6.py)
carry the same 77 and two source checks as unconditional `if`/`AssertionError`
checks. The [source transition](parity/generated_driver_hardened_source_transition_5_0_6.json.gz)
pins both old and new complete sources, whole-AST equivalence for the 79 check
replacements and the five declared routing/lifecycle edits. The active verifier
checks the historical verifier's saved runtime hash; it refuses optimized
archive replay before the shared archive runner can create an output. The eight
existing CTest source fixtures and eight control entries now call the hardened
paths. Their historical receipts still bind the unchanged original paths.

The [permanent guard](../tests/parity/verify_generated_driver_hardened_sources_5_0_6.py)
checks exact current CTest commands and the eight retained contract/execution
hash pairs. Five source/registration corruptions reject. In both Python modes,
the unchanged representative execution passes, the incomplete copy rejects,
and its 24 existing evidence controls reject. Its
[control receipt](parity/generated_driver_hardened_source_controls_5_0_6.json.gz)
records the results and removal of the task-owned incomplete record.

All **17/17** selected native CTests pass under ordinary Python and **17/17**
under `PYTHONOPTIMIZE=1`: eight source contracts, eight existing control entries
and the permanent guard. The current registry contains **1,085** tests and
**603** distinct registered Python entry scripts; **78** retain **754** direct
`ast.Assert` nodes. The historical verifier is still imported by other
historical drivers, so this active-entry change is not a claim that every
imported helper is optimized-safe. No full 1,085-test native epoch has run.
Production C++, generator, generated SDKs and locked references are unchanged.
