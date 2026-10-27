# Registered Python checks under optimization

The current 1,065-test native registry invokes 587 distinct Python entry
scripts. After this focused repair, the
[exact source inventory](parity/registered_python_assert_inventory_1065_5_0_6.json.gz)
finds 111 registered scripts with 1,579 `ast.Assert` nodes. Python removes
those nodes under `-O` or a nonzero `PYTHONOPTIMIZE`. This is an optimization
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
post-repair source set has no complete native epoch yet. The other 111
optimization-sensitive registered scripts require separate review and
qualification; no capability verdict changes here.
