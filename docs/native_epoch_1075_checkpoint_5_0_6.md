# Native 1,075-test checkpoint (incomplete)

This checkpoint records two attempts to run the current full native integration
epoch from C++ commit `a4cd024e` and generator `cpp_support` commit `efe43593`.
The locked reference remains Rust 5.0.6. Neither attempt establishes a complete
native pass or generator parity acceptance.

The first attempt placed its output prefix under `docs/parity`. Its frozen input
snapshot therefore included the controller's own mutable state file. Upstream
and generator checks, CMake configuration, and both build checks passed, but the
controller stopped before CTest when that state file changed. Preserve the
[input snapshot](parity/current_full_native_hardened_inputs_1075_5_0_6.json.gz),
[terminal receipt](parity/current_full_native_hardened_terminal_1075_5_0_6.json.gz),
and [terminal state](parity/current_full_native_hardened_1075_preflight_state_5_0_6.json.gz)
as a failed preflight, not as a test result.

The retry placed its prefix under the ignored `build_native_current` directory.
Upstream and generator checks, CMake configuration, both build checks, and CTest
discovery passed. The registry contained 1,075 tests. The CTest log reached
794 reported completions with one failure:
`generated_credential_mutation_receipt_contracts` raised
`ValueError: current source and dependency identities`. The earlier credential
receipt pins source identities that have since changed; its current-source
contract still needs a source transition review. Under the user's standing
pause instruction, the exact controller process group and its CTest/compiler
children were stopped; no group members remained. This run was interrupted, so
the partial terminal receipt
has no completed CTest step or final test verdict.

The retry's [input snapshot](parity/current_full_native_hardened_inputs_1075_retry_5_0_6.json.gz),
[partial terminal receipt](parity/current_full_native_hardened_terminal_1075_retry_5_0_6.json.gz),
and [CTest log](parity/current_full_native_hardened_ctest_1075_retry_5_0_6.log.gz)
preserve the evidence. The partial receipt reports no input changes at the
point it was written; it does not report a completed post-run input check.
No source files changed in this checkpoint. A repaired receipt contract and a
new complete native epoch remain necessary after work resumes.
