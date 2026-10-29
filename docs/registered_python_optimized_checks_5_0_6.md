# Registered Python checks under optimization

The current 1,066-test native registry invokes 588 distinct Python entry
scripts. A current AST scan finds 103 registered scripts with 1,344
`ast.Assert` nodes after the three installed repairs below. The preceding
[1,065-test source inventory](parity/registered_python_assert_inventory_after_remaining_1065_5_0_6.json.gz)
found 107 registered scripts with 1,467 nodes. The earlier
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
all other assertion rows equal the preceding inventory. The later 1,065-test
native epoch completed; complete SQ02 remains open.

## Entity-runtime source validator repair

The [pre-edit witness](parity/entity_runtime_optimized_preedit_witness_5_0_6.json.gz)
changes only `complete` from `true` to `false` in the retained entity-runtime
execution. The former validator rejects it under ordinary Python but accepts
it under `python -O`; the untouched record passes in both modes. The
[rewrite manifest](parity/entity_runtime_assert_rewrite_candidate_5_0_6.json.gz)
retains both original sources and installed replacements for 68 verifier
assertions and seven control-runner assertions. Their whole ASTs match the
unconditional-check transformation, and no direct assertion remains in either
file. Source and negative-control CTests pass. The permanent
[optimized source guard](../tests/parity/verify_optimized_source_guards_5_0_6.py)
accepts the valid record and rejects the incomplete record in both modes.

## Expression diagnostic framing repair

The [fresh native-output witness](parity/expression_framing_optimized_preedit_witness_5_0_6.json.gz)
uses a generated C++ diagnostic for `Person(id=7).parent.name`. An extra native
field was rejected by the former presentation adapter under ordinary Python
but produced the unchanged original text under `python -O`. Its source fixture
also imports the adapter's negative controls, so the removable assertions
weakened the fixture's own checks. The
[nine-assertion rewrite](parity/expression_framing_assert_rewrite_candidate_5_0_6.json.gz)
preserves the adapter's whole AST after unconditional-check replacement.
The permanent guard accepts valid native output and rejects an extra field in
both modes. The current source fixture and 1,200-frame comparison CTests pass;
current source-contract hashes were updated without rewriting the historical
execution receipt.

## Four-corpus source validator repair

The [context-helper witness](parity/corpus_source_optimized_preedit_witness_5_0_6.json.gz)
changes only `complete` from `true` to `false` in one retained source receipt.
The former four-corpus validator rejects that receipt under ordinary Python
but accepts it under `python -O`; the untouched receipt passes in both modes.
The [48-assertion rewrite](parity/corpus_source_assert_rewrite_candidate_5_0_6.json.gz)
replaces 43 source-verifier and five control-runner assertions with
AST-equivalent unconditional checks. The permanent guard accepts the valid
receipt and rejects the incomplete receipt in both modes. All three registered
source-only variants and their negative controls pass focused CTests.

## Hardened native epoch controller

The [frozen-input witness](parity/native_epoch_controller_optimized_preedit_witness_5_0_6.json.gz)
extracts the controller's actual `check_inputs` function. A changed
input hash is rejected under ordinary Python but accepted under `python -O`
while locked references remain identical. The live controller for the current
completed full native epoch had `PYTHONOPTIMIZE` unset, so this finding does not
change that epoch's interpretation. The
[ten-assertion rewrite](parity/native_epoch_controller_assert_rewrite_candidate_5_0_6.json.gz)
preserves the controller's whole AST after unconditional-check replacement.
It is installed at
[`scripts/run_native_integration_epoch_hardened_5_0_6.py`](../scripts/run_native_integration_epoch_hardened_5_0_6.py).
The original controller remains unchanged at its historical path because eight
retained source contracts pin its hash. The permanent guard verifies both exact
source hashes, accepts unchanged inputs and rejects changed inputs under both
ordinary and optimized Python for the hardened controller, and checks the
historical controller under ordinary Python. Use the hardened controller for
new full native epochs; historical receipts remain bound to the old path.

## Full native epoch boundary

The earlier [1,065-test attempt](parity/current_full_native_terminal_1065_5_0_6.json.gz)
ran all registered CTests and ended with 1,064 passes and one failure:
`parity_custom_public_cli_source_controls`. That control dynamically loaded
its validator before adding `scripts/` to the import path, so the newly used
transition helper could not be imported. The isolated control now adds a
scoped import path and recognizes the helper's exact transition rejection;
its focused CTest passes 1/1.

The controller encountered a broken output pipe after CTest returned, before
it could ingest the terminal JUnit or run its post-build check. The compressed
record retains the full CTest terminal log, registry, input snapshot identity
and the controller error. It is a failed, incomplete integration epoch.

The [subsequent full native epoch](parity/current_full_native_optimized_terminal_1065_5_0_6.json.gz)
completed on 11,814 unchanged inputs, with all **1,065/1,065** CTests passing,
zero failures, errors or skips, matching JUnit/registry names and successful
post-build stability. Its [frozen input record](parity/current_full_native_optimized_inputs_1065_5_0_6.json.gz)
belongs to the source state before the three repairs and the 1,066th CTest were
installed. A [newer 1,066-test attempt](parity/current_full_native_optimized_terminal_1066_5_0_6.json.gz)
encountered three controller-hash contract failures and was stopped before a
terminal JUnit; its source snapshot includes the temporarily installed
controller candidate. The locked controller source was restored afterward.
The restored-controller [1,066-test snapshot](parity/current_full_native_optimized_inputs_1066_restored_5_0_6.json.gz)
contains 11,815 frozen source and producer inputs. Its first
[CTest fragment](parity/current_native_interrupted_fragment_1066_5_0_6.json.gz)
passed 789 distinct tests before the user requested a pause during
`parity_lifecycle_cli_admission`; that fragment has no terminal JUnit. After
resumption, the [remaining-test receipt](parity/current_native_remaining_1066_5_0_6.json.gz)
ran all 277 missing names plus 38 required fixture tests. It passes **315/315**
with zero failures, errors or skips; the union covers **1,066/1,066** unique
registered names. Both fragments used the same input and locked-reference
hashes, and the later native build had no work. This is complete split coverage
of the current registry, not one uninterrupted full-controller epoch. The
other 103 optimization-sensitive registered scripts require separate review;
no capability verdict changes here.

The later [hardened-controller full epoch](parity/current_full_native_hardened_terminal_1066_5_0_6.json.gz)
passes **1,066/1,066** unique registered CTests with zero failures, errors or
skips on [11,816 unchanged inputs](parity/current_full_native_hardened_inputs_1066_5_0_6.json.gz).
All eight controller steps succeed, the JUnit names exactly match the registry,
both locked references remain fixed and the post-test build has no work. This
is one uninterrupted native execution on the source state containing the new
hardened controller, distinct from the earlier split fragments. It does not
qualify the remaining optimized-Python scripts or complete SQ02/G13/G14/G17.
