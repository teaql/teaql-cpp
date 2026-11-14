# Executed helper validation candidates for Rust 5.0.6

These candidates continue SQ02 after the
[registered-boundary checkpoint](registered_boundary_optimized_checks_5_0_6.md).
They are not CTest-installed replacements or capability acceptance. G13, G14
and G17 remain partial at **14 qualified / 3 partial**.

The static import review starts from 614 current registered Python entries and
finds 685 local files. Twenty dependencies contain 281 assertions in mixed
roles: actual helper calls, uncalled generator/replay entry points, historical
counterexamples and explicitly enforced legacy boundaries. Static reachability
alone does not establish which assertion decisions execute.

## Confirmed helper-boundary omissions

The [pre-edit witness](parity/executed_helper_validation_preedit_5_0_6.json.gz)
executes two imported APIs under ordinary Python and `python -O`:

| Helper | Invalid helper input that ordinary Python rejects |
| --- | --- |
| `corpus_input_contracts_5_0_6.generate` | Wrong source hash, wrong result count, unknown builder kind and a dictionary where a result list is required |
| `resolved_mutation_privacy_inputs_5_0_6.assert_privacy_contract` | A credential canary, numeric privacy flag, wrong audit intent, incomplete case set and missing unmasked positive controls |

All nine invalid cases pass at these helper boundaries under optimization.
Three valid cases pass in both modes, giving **24** actual process outcomes.
The witness does not claim that the outer source fixtures accept the same
mutations: those fixtures can have independent checks. The recipe type case
uses an owned source fixture; the other cases use explicit copies of actual
retained contracts and privacy observations. No project source or corpus is
mutated.

## Inactive replacements and unchanged valid behavior

The [helper transition](parity/executed_helper_validation_candidate_transition_5_0_6.json.gz)
retains the original files and replaces exactly fourteen assertions in two
separate copies. Whole-file AST comparison permits only unconditional
`if`/`raise` replacements. The
[candidate controls](parity/executed_helper_validation_candidate_controls_5_0_6.json.gz)
reject all nine invalid cases in both modes while retaining the valid outcomes.
All **6,281** inputs and recipe proofs across seven datasets, all **5,486**
privacy inputs and the complete privacy summary remain identical in both modes.

The [five-file activation candidate](parity/executed_helper_validation_activation_candidate_5_0_6.json.gz)
adds copied callers and controls with exact import/input-tracking adaptations.
The retained producer input hashes still identify the original helper; the
candidate does not pretend the earlier Rust execution used new helper bytes.

The [candidate CLI execution](parity/executed_helper_validation_candidate_cli_5_0_6.json.gz)
passes all six ordinary/optimized runs: seven retained source bindings,
twenty-four existing source corruptions plus five CLI controls per mode, and
the actual **5,486-case native privacy comparison** per mode. No new Rust or
C++ compilation runs. Owned source/control/record directories are removed.

Activation still requires a permanent source/control guard, explicit CTest
registration admission and the affected native selections. The original
registered callers remain active until that checkpoint is qualified.

## A separately enforced caller boundary

The [inherited input boundary witness](parity/inherited_input_outer_boundary_5_0_6.json.gz)
executes the actual inherited-business caller. Its two supported recipes are
function recipes; wrong source hash, recipe hash, builder kind and result count
are rejected before entering the historical shared input helper, in both
Python modes. The valid two-recipe path passes. This gives a bounded caller
disposition for those ten process cases, not acceptance of every historical
helper entry point or the complete import graph.
