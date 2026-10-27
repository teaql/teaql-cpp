# Isolated custom-corpus admission fixture repair

The [frozen native epoch](parity/ap02_cross_composition_full_native_terminal_5_0_6.json.gz)
registered and executed all 1,063 tests on 11,811 unchanged inputs. Its JUnit
records 1,062 passes, one failure, no errors and no skips. The reference
commits remained locked and the post-test build had no work. The task-owned
temporary directory was removed after the controller exited.

The sole failure was `parity_custom_case_admission`: 53 of 60 controlled
comparisons passed. Its seven failing cases were all for the generated dynamic
JSON comparator. That comparator imports `quoted_fk_debug_adapter_5_0_6.py`,
but the admission test's isolated fixture list omitted the helper. The
complete case could not load the comparator, and six malformed-corpus cases
failed before reaching their required early corpus guard. This is a test
fixture dependency, not evidence of a generated JSON execution difference.

The fixture list now includes the exact helper file. The
[post-repair receipt](parity/custom_case_admission_fixture_repair_5_0_6.json.gz)
records **60/60** passing controls, hashes the copied helper, and confirms
that its 33,754,860-byte temporary fixture directory was removed. The
registered native CTest selection passes **1/1**. No production runtime,
producer, generated SDK or reference source changed in this repair.

A new complete native epoch is running on the repaired input set. The failed
epoch remains failed evidence; the selected passing test does not turn it into
a complete green run. G13/SQ02, G14/AP02, G17 and P0-P7 retain their separate
exit requirements.
