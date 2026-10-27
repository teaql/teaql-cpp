# Registered Python checks under optimization

The current 1,065-test native registry invokes 587 distinct Python entry
scripts. The [latest source inventory](parity/registered_python_assert_inventory_after_remaining_1065_5_0_6.json.gz)
finds 107 registered scripts with 1,467 `ast.Assert` nodes. The preceding
[declared-core inventory](parity/registered_python_assert_inventory_after_core_1065_5_0_6.json.gz)
found 109 scripts and 1,537 assertions; the earlier
[inventory](parity/registered_python_assert_inventory_1065_5_0_6.json.gz)
found 111 scripts and 1,579 assertions before the declared-core repair below.
Python removes those nodes under `-O` or a nonzero `PYTHONOPTIMIZE`. This is an optimization
risk inventory; it does not claim a reproduced false acceptance for every
remaining assertion. Complete SQ02 and G13/G14/G17 qualification remains open.

## Reproduced declared-driver failure

The original
[declared-driver validator](../scripts/verify_declared_driver_contracts_5_0_6.py)
used 39 assertions, including its receipt-completeness and raw-output checks.
The [pre-edit witness](parity/declared_driver_optimized_preedit_witness_5_0_6.json.gz)
changes only `complete` from `true` to `false` in the retained eight-contract
runtime-foundation execution receipt. The old validator accepts the untouched
receipt in both Python modes, rejects the altered receipt under normal Python,
and accepts it under `python -O`. No new Rust or C++ execution is claimed by
this saved-record witness.

The [installed rewrite](parity/declared_driver_assert_rewrite_5_0_6.json.gz)
replaces those 39 assertions and five assertions in the
[negative-control runner](../tests/parity/verify_declared_driver_controls_5_0_6.py)
with unconditional `if`/`AssertionError` checks. The receipt retains both
pre-edit sources and hashes, both current hashes and whole-AST equivalence;
no assertion remains in either file. The runner rechecks this transformation
and executes five CLI controls: valid evidence under normal and optimized
Python, an incomplete receipt rejected under both modes, and missing native
output rejected under optimized Python. Its
[control receipt](parity/declared_driver_optimized_controls_5_0_6.json.gz)
records all five expected outcomes and temporary-directory cleanup. The
focused native source and control tests pass 2/2; the control runner also
passes directly under `python -O`.

## Declared-core source validator

The separate fourteen-contract declared-core verifier checked 5,656 retained
observations with 37 assertions, and its negative-control runner used five.
The [pre-edit witness](parity/declared_core_optimized_preedit_witness_5_0_6.json.gz)
changes only `complete` from `true` to `false` in the retained execution record.
The old verifier rejects it under ordinary Python but accepts it under
`python -O`; the untouched record passes both modes. This is a saved-record
validation failure, not a new Rust or C++ execution result.

The [rewrite manifest](parity/declared_core_assert_rewrite_5_0_6.json.gz)
retains the complete pre-rewrite source for both scripts and verifies that all
42 assertions became unconditional checks without other AST changes. The
negative-control runner rechecks those source identities and runs five actual
subprocess controls: valid evidence in both modes, an incomplete record
rejected in both modes, and missing native output rejected under `python -O`.
The [current control receipt](parity/declared_core_optimized_controls_current_5_0_6.json.gz)
records all five outcomes and owned temporary-directory cleanup. Current
focused native source/control tests pass 2/2, and the runner passes directly
under `python -O`. The new inventory recomputes every registered entry script
from the unchanged 1,065-test registry and verifies that all other assertion
rows match the preceding source inventory exactly. Complete SQ02 remains open.

## Seven-corpus source validator

The seven-corpus source verifier checks 6,281 retained original/C++ observations
and its provenance through 66 assertions; its 24-case negative-control runner
used four more. The
[pre-edit witness](parity/remaining_corpus_optimized_preedit_witness_5_0_6.json.gz)
changes only the retained execution record's `complete` field from `true` to
`false`. The old CLI rejects that record under ordinary Python but accepts it
under `python -O`, while the untouched record passes in both modes. This
witness rechecks saved evidence and does not rerun Rust or C++.

The [rewrite manifest](parity/remaining_corpus_assert_rewrite_5_0_6.json.gz)
records complete pre-rewrite source and whole-AST equivalence for all 70
replacements. The permanent negative-control runner checks the current source
hashes and runs five subprocess controls: valid evidence in both modes,
incomplete evidence rejected in both, and missing native output rejected in
optimized Python. The [control receipt](parity/remaining_corpus_optimized_controls_5_0_6.json.gz)
retains all five outcomes, the 24 prior corruption outcomes and cleanup of
both owned temporary directories. Current focused source/control CTests pass
2/2, and the control runner itself passes under `python -O`. The latest
inventory recomputes every registered Python entry script and verifies that
all other assertion rows equal the preceding inventory. Complete SQ02 and
the post-repair native epoch remain open.

## Full native epoch boundary

The [1,065-test attempt](parity/current_full_native_terminal_1065_5_0_6.json.gz)
ran all registered CTests and ended with 1,064 passes and one failure:
`parity_custom_public_cli_source_controls`. That control dynamically loaded
its validator before adding `scripts/` to the import path, so the newly used
transition helper could not be imported. The isolated control now adds a
scoped import path and recognizes the helper's exact transition rejection;
its focused CTest passes 1/1.

The controller encountered a broken output pipe after CTest returned, before
it could ingest the terminal JUnit or run its post-build check. The compressed
record retains the full CTest terminal log, registry, input snapshot identity
and the controller error. It is a failed, incomplete integration epoch. The
post-repair source set has no complete native epoch yet. The other 107
optimization-sensitive registered scripts require separate review and
qualification; no capability verdict changes here.
