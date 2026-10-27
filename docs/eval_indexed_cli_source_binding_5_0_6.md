# Indexed LoadState CLI source binding, Rust 5.0.6

The installed `eval_indexed_rust_parity` comparison now requires a source
fixture. The retained original Rust execution covers six LoadState variants
and three field layouts: 18 scenarios and 180 load/unload operations. Fresh
C++ execution matches the original **common state semantics in 18/18
scenarios**. Four initialized diagnostics are checked for each initial and
final state, including snapshot wrapping.

The [source contract](../tests/parity/eval_indexed_cli_contract_5_0_6.json)
pins the original corpus and full pre-repair Rust execution receipt. The
corpus pins Rust `eval.rs`, `load_state.rs`, the original observer, its Cargo
manifest and lock. The binder reconstructs all six-by-three inputs, verifies
the complete retained Rust stdout against the frozen outcomes, and validates
the exact installed CTest command and source fixture. The original Rust
executable was removed after its earlier run; this checkpoint did not rerun
Rust.

The original and current C++ **whole outputs are not equal**: initialized raw
diagnostics contain implementation-specific source and Context representations.
The installed comparison checks the shared semantic fields and verifies that
each C++ diagnostic is complete and self-consistent. This evidence does not
claim byte-for-byte raw diagnostic parity. The original
[pre-repair receipt](parity/eval_indexed_initial_5_0_6.json.gz) retains that
boundary.

The [15 negative controls](parity/eval_indexed_cli_controls_5_0_6.json.gz)
reject source, matrix, outcome, diagnostic, command, and fixture substitutions.
The [native receipt](parity/eval_indexed_cli_native_5_0_6.json.gz) records the
18 semantic matches and whole-output inequality. The
[selected CTest receipt](parity/eval_indexed_cli_selected_ctest_5_0_6.json.gz)
and [complete CTest log](parity/eval_indexed_cli_ctest_log_5_0_6.txt.gz)
record five passing tests, including the installed comparison and Context
self-test. The task-owned temporary JUnit file was removed. The
[milestone receipt](parity/eval_indexed_cli_milestone_5_0_6.json.gz) records
the bounded result and artifact hashes.

The [incremental dispatcher lookup](parity/eval_indexed_cli_remaining_dispatchers_5_0_6.json.gz)
retains 222 rows: 151 of 160 comparison locators have source contracts,
with nine remaining. The 1,034 CTest registrations are enumeration. The
[previous-input delta](parity/eval_indexed_cli_previous_input_delta_5_0_6.json.gz)
rehashes earlier protected inputs against the current checkout.

This change binds evidence and test admission. It changes no runtime,
generator, generated SDK, Rust observer, LoadState algorithm, or Context
ownership. Complete G13/G14/G17, GP13/GP16, AP02, and P0–P7 remain open. No
sanitizer, MIPS, module, GPU, physical-router, or full-suite gate was run for
this evidence-only change.
