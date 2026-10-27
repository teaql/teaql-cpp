# Inherited Checker CLI source binding, Rust 5.0.6

The five installed `inherited_checker_helpers_*_rust_parity` comparisons now
require one source fixture. The original corpus has 7,200 observations, 1,440
for each of five models. It covers direct and governed Checker calls, changed
and evaluation snapshots, three naming profiles, Policy preflight, ordered Fix
evidence, and two fixed wall clocks. Fresh C++ execution matched 7,200 of
7,200 frozen Rust outcomes.

The [source contract](../tests/parity/inherited_checker_cli_contract_5_0_6.json)
pins the runtime and generator locks, original corpus and terminal, original
Rust observer, reference-seeded Cargo dependency closure, and all 43 original
generated inheritance files. The binder recomputes every input and checks
7,200 callbacks, 594 Policy reviews, 2,880 errors, 12,072 findings, and 6,400
Fix evidence entries. It validates the five exact CTest commands, model
arguments, clock paths, and fixture dependencies before fresh native execution.

The historical terminal has one stale source hash: its Python comparator was
subsequently split into per-model CTests. The Rust observer, corpus, manifest,
Cargo lock, and frozen outcomes are unchanged; the current comparator is pinned
separately. Historical evidence is retained at its original scope.

Fifteen [negative controls](parity/inherited_checker_cli_controls_5_0_6.json.gz)
cover source, input, outcome, count, clock, command, and fixture substitutions.
The [native receipt](parity/inherited_checker_cli_native_5_0_6.json.gz),
[selected CTest receipt](parity/inherited_checker_cli_selected_ctest_5_0_6.json.gz),
and [complete selected CTest log](parity/inherited_checker_cli_ctest_log_5_0_6.txt.gz)
record eight passing CTests, including all five original comparisons. The
temporary JUnit file was removed. No new Rust execution was performed.
The [milestone receipt](parity/inherited_checker_cli_milestone_5_0_6.json.gz)
summarizes the bounded result and artifact hashes.

The [incremental dispatcher lookup](parity/inherited_checker_cli_remaining_dispatchers_5_0_6.json.gz)
retains 222 rows and maps five more comparison locators to this source
contract: 149 of 160 are mapped, with 11 remaining. The 1,028 CTest
registrations are enumeration. The
[previous-input delta](parity/inherited_checker_cli_previous_input_delta_5_0_6.json.gz)
rehashes the earlier protected inputs against the current checkout.

This change adds evidence binding and test admission. It changes no runtime
or generator behavior, generated SDK, Rust observer, Checker algorithm, or
Context ownership. Governed cases include Policy preflight denials; they do
not establish committed graph save. Complete G13/G14/G17, GP13/GP16, AP02,
and P0–P7 remain open. No sanitizer, MIPS, module, GPU, or physical router
gate was run for this evidence-only change.
