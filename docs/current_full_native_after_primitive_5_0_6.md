# Full native integration after the primitive and governed-consumer changes

This is a fresh complete native execution at runtime source checkpoint
35f601fd0fd5a26a6b607e09681dee20f258b0be and producer
ec8eac64f0b69808b0017cf72d74e4aef1523e1d. References remain Rust 5.0.6 runtime
fe82a18982415f1746df9a2cf7c42f5e9abdc6d3 and generator
45e1572c4956ca963410d8ea436ead3c8857087a.

The [terminal execution](parity/current_full_native_after_primitive_terminal_1101_5_0_6.json.gz)
passes **1,101/1,101** registered tests with zero failures, errors or skips.
Every JUnit test name and run status matches the actual registry. Configure,
all native builds, registry collection, CTest and the final no-work build
complete successfully. Elapsed controller time is 6892.54 seconds.

The [input snapshot](parity/current_full_native_after_primitive_inputs_1101_5_0_6.json.gz)
retains all **11,999** execution inputs and exact references. They remain
unchanged through the complete epoch. Narrative documentation and compressed
evidence are outside that input set; implementation, generated files and JSON
ledgers remain frozen until completion.

## Complete compiler and source scopes

The [ordinary compiler receipt](parity/current_full_native_after_primitive_compiler_1101_5_0_6.json.gz)
retains the actual untruncated output of all **201** distinct contracts:
**57 positive / 144 negative**. The complete source-owned case inventory is
reconstructed before accepting the output. Selected cases, a missing case,
an altered summary or an optimized subset cannot stand in for this execution.

The [source and artifact identity receipt](parity/current_cpp_frontend_source_identity_after_primitive_5_0_6.json.gz)
verifies **1,817** unchanged C++ implementation/header/module/fixture inputs
against the preceding complete epoch. It also verifies all **80** retained
SDKs and their **3,560** unchanged code/Assist files against the exact
production reproduction. All **707** native consumer hashes remain unchanged.
This source identity permits only explicitly scoped reuse; it does not execute
an imported-module or lifetime profile.

The source/control aggregate and the earlier
[106-change source continuation](parity/sq02_primitive_const_governed_source_join_5_0_6.json.gz)
keep their original semantic/provenance scopes. This new epoch adds actual
whole-registry execution rather than relabeling the earlier targeted gates.

## Admission and artifact lifecycle

The [ordinary admission](parity/current_full_native_after_primitive_acceptance_1101_5_0_6.json.gz)
and [optimized admission](parity/current_full_native_after_primitive_acceptance_optimized_1101_5_0_6.json.gz)
use unconditional checks and reject sixteen corrupted variants each. They
validate the complete terminal/JUnit/registry/source/reference/consumer and
compiler selection. Controls include hidden JUnit failures, missing or skipped
tests, an unqualified source, changed native consumers, a non-native preset,
an incomplete compiler selection and a Boolean failure count.

These are two validation modes of the same actual ordinary native execution.
They are not new optimized compiler, sanitizer, module or MIPS runs.

The [lifecycle receipt](parity/current_full_native_after_primitive_lifecycle_1101_5_0_6.json.gz)
retains the controller source/output and confirms cleanup of the exact owned
log/state/JUnit directory after its children exit. Input/terminal receipts
retain their complete source and diagnostic payloads. Stable preset outputs,
shared caches, source files and retained evidence are preserved.

The [post-test milestone](parity/current_full_native_after_primitive_milestone_1101_5_0_6.json.gz)
allows only G13/G14/G17 evidence and native-status open-question text to change
after the tested state. Its before/after ledger texts and hashes retain the
single metadata-only transition. Capability totals remain **14 qualified /
3 partial**.

## Remaining goal scope

This closes the SQ05 native execution requirement for the frozen checkpoint.
The [remaining exit audit](current_goal_exit_audit_5_0_6.md) retains the complete
AP01-AP12, G13/G14/G17 and P0-P7 obligations. Pending language-configuration and
reference-defect decisions are not silently resolved by a passing test suite.
Generated metadata/identity signoff, relevant artifact/frontend joins and
whole-phase acceptance remain independent.

No full sanitizer, MIPS, GPU adapter or physical-router execution is claimed.
Native execution uses the committed profile:

    source scripts/toolchains/activate.sh
    cmake --preset native
    cmake --build --preset native
    ctest --preset native --output-on-failure

The retained epoch controller additionally freezes inputs/references, records
exact membership and checks the final no-work build.
