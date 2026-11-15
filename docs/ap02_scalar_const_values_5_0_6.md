# AP02 scalar constant value candidate

The copied candidate passes 203 Java tests and 65 complete generated-library
compiler contracts against runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`. Production remains
the separately qualified array-constant installation. The proposed changes
are generation-time Java code and tests; they add no C++ runtime dependency.

## Source observations and semantic boundaries

The [complete scalar diagnostic](parity/ap02_scalar_const_value_diagnostic_repaired_5_0_6.json.gz)
builds 24 original/C++ model pairs and finds fifteen C++ refusals for valid
character, byte-character and floating-point configurations. Successful
original executables also check that the reverse relation remains present.
The first diagnostic was stopped to correct the outer Rust string escaping;
its partial receipt is preserved without whole-run acceptance.

The [const cast matrix](parity/ap02_scalar_cast_const_matrix_5_0_6.json.gz)
compiles every source/destination pair among the seventeen kinds implemented
by this evaluator, plus five inference/identity cases: 294 observations,
229 accepted and 65 refused. The [earlier function-context matrix](parity/ap02_scalar_cast_language_matrix_5_0_6.json.gz)
and [unit-context observations](parity/ap02_scalar_const_unit_semantics_5_0_6.json.gz)
retain their narrower scopes. Boolean, character and unit identity casts are
legal. Unit comparison operators remain unavailable in the tested const context.

Full generated libraries establish an additional boundary that standalone
const compilation cannot substitute for. The
[generated-literal diagnostic](parity/ap02_scalar_const_literal_generation_diagnostic_5_0_6.json.gz)
checks nine complete model pairs. The original derive expansion accepts
integer literals that wrap to the selected kind and floating literals that
overflow to infinity. Conditional expressions with a divide-by-zero alternative
verify the actual folded values. Checked arithmetic overflow and executed
division by zero retain their separate refusals; tokens beyond `u128` retain
a compiler error.

One existing Java expectation incorrectly treated a `usize` literal beyond
64 bits as a mandatory refusal. The proposed patch moves that literal into
the accepted generated-context cases, backed by a complete original library.
It does not relabel a historical standalone observation or invent earlier
original-library acceptance.

## Copied implementation and actual qualification

The candidate retains separate character, byte/integer, `f32`, `f64`, boolean
and unit kinds. It preserves contextual literal inference, direct single
precision rounding, exact binary floating-to-integer truncation, saturating
casts, signed zero and NaN comparisons. Existing lexer validation is reused.
The native reference uses 64-bit `usize`/`isize`; this is not new MIPS or
physical-router verification.

The [first copied candidate](parity/ap02_scalar_const_value_candidate_5_0_6.json.gz)
passes 202 Java tests and completes 36 compiler contracts before a standalone
literal-range assumption is contradicted by actual generated Rust. The
[second candidate](parity/ap02_scalar_const_value_candidate_v2_5_0_6.json.gz)
then exposes the stale existing Java expectation. Neither incomplete planned
run is accepted as complete.

The [qualified copied continuation](parity/ap02_scalar_const_value_candidate_v3_5_0_6.json.gz)
passes 203 Java tests, including the immutable 294-case cast corpus. All 65
compiler contracts agree: 51 accepted and fourteen refused. Twenty-nine pairs
are freshly compiled; 36 completed results are explicitly reused only after
fresh complete original/C++ SDK byte equality. Every accepted C++ model retains
its typed reverse projection.

The [complete proposed patch](proposals/scalar_const_value_projection.patch)
contains the evaluator, the corrected existing expectation, nine new regression
methods and a generated compressed source corpus. The
[proposal receipt](parity/ap02_scalar_const_value_proposal_5_0_6.json.gz)
binds the binary patch, corpus bytes and copied source. `git apply --check`
passes against the production producer checkpoint. No generated C++ fixture
has been hand-edited.

## Remaining continuation

The [actual SQLite/E/JSON matrix](parity/ap02_scalar_const_value_candidate_sql_5_0_6.json.gz)
executes eight freshly generated original/C++ model pairs and forty complete
outcomes with no differences. It covers character wrapping, Unicode, byte
escapes, single precision, exact binary integer conversion, NaN, unit identity
and a modeled child projection. The
[candidate checkpoint](parity/ap02_scalar_const_value_candidate_checkpoint_5_0_6.json.gz)
binds the copied source, original witnesses, complete patch and unchanged
production inputs. All task-owned probe processes finish before their
disposable output directories are removed; compact evidence remains.

Install the exact reviewed patch, then requalify production Java, preserved/new
complete SDKs, all eighty retained SDKs and affected source guards. Keep
historical inputs and source admissions exact, including the existing-test
expectation transition. The production installation gate is not accepted by
this copied checkpoint.

This candidate does not close AP01-AP12/G14, G17 or P0-P7. Capability status
remains **14 qualified / 3 partial**. The pending Option-model and quoted-column
attribute decisions remain independent.
