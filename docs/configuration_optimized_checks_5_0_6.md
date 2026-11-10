# Runtime configuration provenance under Python optimization

Four registered configuration and source validators still used 35 Python
`assert` statements. Python omits these checks under `-O` or a nonzero
`PYTHONOPTIMIZE`, including checks for retained Rust observations, generated
model hashes, original record identity, actual C++ output and observation
cardinality.

Before the repair, four independent inputs failed under ordinary Python but
returned success under `python -O`:

| Validator | Rejected input | Incorrect optimized result |
| --- | --- | --- |
| Runtime configuration source | Altered `source_inputs` hash in a loaded original record | Accepted the changed record |
| Business runtime source | Wrong generated model/source digest | Accepted the wrong digest |
| Legacy observation identity | Changed retained Business-ID record bytes | Accepted the changed record |
| Generated registry/schema | Successful observer that returned zero of thirteen required rows | Claimed thirteen outcomes were verified |

The [exact source transition](parity/configuration_optimized_assert_rewrite_5_0_6.json.gz)
retains all four pre-edit sources and hashes. Each original `assert` became an
unconditional `if`/`AssertionError` at the same statement position; whole-AST
comparison preserves its condition, message and surrounding code. The
[permanent control](../tests/parity/verify_configuration_optimized_controls_5_0_6.py)
checks those identities and replays the four negative inputs plus valid actual
C++ kernel, Business-ID and registry probes and the retained record, in both
normal and optimized Python: **16/16 controls**. It requires the expected
failure diagnostics, owns its temporary files and removes them on exit.

Selected native CTests pass **7/7**, including both source fixtures, four
affected parity checks and the new control. Selected imported-module CTests
pass **5/5**, including four parity checks and the same control against module
probes. No Rust observation, generated SDK, C++ implementation or generator
source was changed. The historical
[`runtime_configuration_kernel_execution_5_0_6.json`](parity/runtime_configuration_kernel_execution_5_0_6.json)
retains its original source hash and execution scope.

The current native registry has **1,074** tests and **593** Python entry
scripts. **94** registered scripts still contain **1,163** removable assertions.
This closes the four demonstrated false acceptances and the 35 checks in these
validators. Complete SQ02 provenance, G13/G14/G17, the current full native
epoch, and P1/P7 remain open.
