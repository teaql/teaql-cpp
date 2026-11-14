# Compiled-model source checks under optimized Python

This is a bounded SQ02/G13 repair for the registered compiled-model source
verifier and its existing control entry. The locked Rust 5.0.6 execution,
historical verifier and control sources, registry, corpora and generated SDKs
remain unchanged. It does not close SQ02, G13, G14, G17 or P0-P7.

The [pre-edit witness](parity/model_driver_optimized_preedit_5_0_6.json.gz)
changes only `complete` to `false` in a task-owned copy of the retained
generated-model execution. The historical verifier rejects this record under
ordinary Python but exits successfully under `python3 -O`. This is an actual
saved-evidence false acceptance, not a new Rust or C++ execution result.

The active [hardened verifier](../scripts/model_driver_provenance_hardened_5_0_6.py)
replaces its 74 removable assertions with unconditional checks. The
[hardened control entry](../tests/parity/verify_model_driver_controls_hardened_5_0_6.py)
imports that verifier and the already hardened generated-model controls. The
[source transition](parity/model_driver_hardened_source_transition_5_0_6.json.gz)
pins the complete old and new sources, the exact assertion rewrite and seven
declared routing/lifecycle edits. The active verifier checks the historical
verifier's saved runtime hash and refuses optimized archive replay before the
shared runner can create an output. The two affected CTest entries now call the
hardened paths; their historical receipts still bind the unchanged old paths.

The [permanent guard](../tests/parity/verify_model_driver_hardened_sources_5_0_6.py)
checks those source transitions, dependencies and current CTest commands. Four
source or registration corruptions reject. In both Python modes, the unchanged
execution passes, the incomplete copy rejects, and all 27 existing evidence
controls reject their invalid inputs. Its
[control receipt](parity/model_driver_hardened_source_controls_5_0_6.json.gz)
records optimized replay refusal and removal of the task-owned temporary data.

Selected native CTests pass **4/4** under ordinary Python and **4/4** under
`PYTHONOPTIMIZE=1`: the compiled-model verifier, its controls, the permanent
guard and the actual generated entity-helper C++ comparator. The current
registry contains **1,086** tests and **604** distinct registered Python entry
scripts; **77** retain **680** direct `ast.Assert` nodes. Imported helpers
and the remaining registered entries still need review. No full 1,086-test
native epoch has run, and this repair does not establish complete current
integration. Production C++, generator, generated SDKs and locked references
are unchanged.
