# Current protected module consumers

The module mutation-privacy comparison now uses the same unconditional validator
as native execution and requires its locked-source corpus fixture. Previously,
the module registration still selected the historical assertion-based helper;
Python optimization could remove its independent privacy checks. Runtime,
generated SDK and module-export implementations are unchanged.

The reference remains Rust 5.0.6 runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. The inspected runtime parent is
`ed302de6791ed8d5fc02fead4178f8f38bdd6cb3`; the producer remains
`ec8eac64f0b69808b0017cf72d74e4aef1523e1d`.

## Imported execution and the registration repair

Review of the earlier governance module receipt found four changed C++ inputs
among its 1,795 recorded files: two runtime headers and two generated sample
headers. Their changes belong to the qualified
[scalar sample assignment repair](sample_operand_assignment_5_0_6.md).

The [fresh imported epoch](parity/current_protected_modules_43_5_0_6.json.gz)
builds the 40 governance consumers and three sample/kernel comparisons with
the committed `modules_native` preset. All **43/43** tests pass, with exact
JUnit membership and a final no-work build. Its 42 executable/library hashes,
11,999 source inputs and locked references remain stable. This execution
precedes the module comparator registration repair.

Comparison with the native commands then exposed the remaining historical
mutation-privacy validator. The
[registration snapshot](parity/current_module_privacy_registration_before_5_0_6.json.gz)
preserves the exact preceding CMake text. The only source change switches that
comparison to `compare_resolved_mutation_privacy_hardened_5_0_6.py` and adds
the required module corpus-source fixture, using the existing native source
validation command.

The [completed follow-up](parity/current_module_privacy_registration_final_5_0_6.json.gz)
passes **2/2 actual tests in each Python mode**: the source fixture and repaired
module comparison. Both ordinary and `PYTHONOPTIMIZE=1` execution match all
**5,486/5,486** mutation-privacy observations. Exactly one preceding module
command/property set changes. The other 42 runtime-consumer results are
explicitly reused from the fresh epoch after checking their unchanged commands,
properties and executable bytes.

This joins 43 runtime consumers and their new required source fixture. The
module registry contains 722 tests; this is a scoped qualification.

## Source and native boundaries

The exact input transition changes only `modules/CMakeLists.txt`. All C++
implementations, generated artifacts, producer sources and trusted configuration
data retain their preceding bytes. Module paths and the locked generator ledger
verify successfully.

Native configuration still registers the same 1,101 commands and properties;
its build reports no work and all 707 native executable hashes match the
[complete native checkpoint](current_full_native_after_primitive_5_0_6.md).
That earlier whole-registry execution is explicitly reused within this unchanged
native scope. No new native full suite, sanitizer, MIPS or router execution is
claimed.

## Receipt validation and retained diagnostics

The initial imported receipt has ordinary and optimized admission records,
each rejecting sixteen corruptions. The registration follow-up has separate
[ordinary](parity/current_module_privacy_registration_acceptance_5_0_6.json.gz)
and [optimized](parity/current_module_privacy_registration_acceptance_optimized_5_0_6.json.gz)
admission, also rejecting sixteen corruptions per mode. They check the exact
CMake transition, fixture dependency, interpreter modes, complete result names,
unchanged native scope and scoped executable identities.

Two earlier controller admission failures remain separate diagnostics. The
[first](parity/current_module_privacy_registration_5_0_6.json.gz) incorrectly
required the initial build to report no work even when CMake only refreshed
dependency-scanning files. The
[second](parity/current_module_privacy_registration_repaired_5_0_6.json.gz)
assumed every unbuilt registry entry contained a `command` member. The final
controller checks no-work status after execution and handles absent commands
on unrelated unbuilt entries. All selected consumers still require actual
commands and successful execution. Neither diagnostic is a compiler or runtime
failure, and neither is accepted as passing execution.

All owned controller/log/JUnit directories are removed after their children
exit. Measured log removals total 2,203,526 bytes; the two small failed-run
directories were removed without size measurements. Compact reports and stable
preset build outputs are retained; shared caches are preserved.

This supplies current imported-consumer and source-fixture evidence to E02/E05
in the [remaining exit audit](current_goal_exit_audit_5_0_6.md). Complete AP02
configuration/identity signoff, pending compatibility decisions and every
capability/phase exit remain required. Status stays **14 qualified / 3 partial**.
