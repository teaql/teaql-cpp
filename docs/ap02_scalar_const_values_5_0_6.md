# AP02 scalar constant value projections

The reviewed scalar evaluator is installed on `cpp_support` commit `2816302e1f2579ffd6f1032e4e49441f68ec0c40`.
Production passes 203 Java tests and reproduces all 258 complete SDK contracts,
explicitly reusing their 183 accepted and 75 refused compiler outcomes.
The reference remains runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.
These generation-time Java changes add no C++ runtime dependency.

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

## SQLite proof and production qualification

The [actual SQLite/E/JSON matrix](parity/ap02_scalar_const_value_candidate_sql_5_0_6.json.gz)
executes eight freshly generated original/C++ model pairs and forty complete
outcomes with no differences. It covers character wrapping, Unicode, byte
escapes, single precision, exact binary integer conversion, NaN, unit identity
and a modeled child projection. The
[candidate checkpoint](parity/ap02_scalar_const_value_candidate_checkpoint_5_0_6.json.gz)
binds the copied source, original witnesses, complete patch and unchanged
production inputs. All task-owned probe processes finish before their
disposable output directories are removed; compact evidence remains.

The [production installation receipt](parity/ap02_scalar_const_value_installed_5_0_6.json.gz)
records fresh Java compilation, all 203 permanent Java tests and fresh complete
regeneration of 193 preserved plus 65 scalar SDK contracts. Every complete SDK
matches its qualified compiler witness. All 258 compiler outcomes are explicitly
reused; this production step invokes no new C++ compiler.

The [eighty-SDK refresh](parity/ap02_scalar_const_value_sdks_5_0_6.json.gz)
preserves 3,560 implementation and Assist files. Only one parser fingerprint
changes per manifest. The [production SQL reproduction](parity/ap02_scalar_const_value_production_sql_reuse_5_0_6.json.gz)
regenerates and formats all eight consumer SDKs identically to the actual copied
executables. Their forty runtime outcomes are explicitly reused; reproduction
does not claim forty new SQL executions.

The initial [source-control run](parity/ap02_scalar_const_value_admission_controls_5_0_6.json.gz)
and [affected CTest run](parity/ap02_scalar_const_value_native_5_0_6.json.gz)
fail because an older unit-source guard directly checks the corrected existing
test's previous fingerprint. The repair delegates exactly that one predicate
through the declared source epochs. Eight exact import/current-input predicates
in the array admission helper also admit the parser, test, SDK and older-guard
transitions. All other historical validation and corruption-control bodies
remain preserved. The [repair record](parity/ap02_scalar_const_value_admission_repair_5_0_6.json.gz)
and [existing-unit transition](parity/ap02_scalar_const_value_existing_unit_guard_transition_5_0_6.json.gz)
retain complete before/after source and exact edits.

The [repaired controls](parity/ap02_scalar_const_value_admission_controls_repaired_5_0_6.json.gz)
reject 37 new and 91 preserved corruptions in each Python mode, including six
direct checks of the production recipe's imported selection function.
The [affected registered checks](parity/ap02_scalar_const_value_native_repaired_5_0_6.json.gz)
pass 6/6 in ordinary Python and 6/6 under optimization, with exact JUnit membership.

The [current source join](parity/sq02_scalar_const_value_source_join_5_0_6.json.gz)
and its [optimized counterpart](parity/sq02_scalar_const_value_source_join_optimized_5_0_6.json.gz)
admit 101 exact source changes on 11,994 inputs. They preserve the complete
1,101-test registry, 716 Python commands and 707 native consumer hashes, and
reject ten source-transition corruptions per mode. Pinned formatting and module
path/provenance checks pass. This is affected-test execution and exact source
admission after the earlier full native epoch; it is not a fresh full native,
sanitizer, MIPS or physical-router execution.

The [installation milestone](parity/ap02_scalar_const_value_installation_milestone_5_0_6.json.gz)
binds the exact post-test G14 evidence/open-question update. Broader supported
configurations, AP01-AP12/G14, G17 and P0-P7 remain open. Capability status stays
**14 qualified / 3 partial**. The pending Option-model and quoted-column
attribute decisions remain independent.
