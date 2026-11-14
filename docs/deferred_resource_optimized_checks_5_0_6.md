# Deferred-resource comparison and embedded replay integrity

This SQ02/G13 repair keeps all 143 C++ suspended-resource scenarios and all
163 unchanged-data business comparisons. References remain Rust 5.0.6 runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. C++ kernel implementation and
generated artifacts are unchanged.

The [resource pre-edit witness](parity/deferred_resource_optimized_preedit_5_0_6.json.gz)
reproduces four false acceptances under optimized Python: empty output, a
missing observation, an incorrect pending flag and an incorrect business
payload. The valid fixture passes both modes; ordinary Python rejects all four
invalid fixtures. These ten CLI runs use source-derived outputs and establish
the comparator defect, not a new runtime result.

The existing comparator-control program also embeds a Python observer in a
string. Its [separate witness](parity/deferred_resource_embedded_preedit_5_0_6.json.gz)
shows an invalid lookup request rejected normally but accepted under `-O`.
The outer file has no direct assertions; five assertions are inside executable
fixture text. A direct-AST assertion inventory cannot detect this path.

The [source transition](parity/deferred_resource_hardened_transition_5_0_6.json.gz)
preserves both historical files and retains their full active copies. The
[resource comparator](../tests/parity/verify_deferred_resource_replacement_hardened_5_0_6.py)
replaces nine assertions with unconditional checks, including complete output,
pending/closed state, business results, expiration, denial, cancellation and
existing-record preservation. The
[control program](../tests/parity/verify_comparator_result_contracts_hardened_5_0_6.py)
replaces the five embedded assertions with an exactly equivalent unconditional
AST. Four declared adaptations select the active comparator, install that
embedded conversion, add seven optimized deferred controls and isolate child
optimization/Python-path settings and diagnostics in the owned directory.

The [permanent guard](../tests/parity/verify_deferred_resource_optimized_controls_5_0_6.py)
checks source identities, exact AST transitions, retained data and the active
registrations. Its [registered control receipt](parity/deferred_resource_hardened_controls_5_0_6.json.gz)
passes **34** ordinary/optimized resource cases and **four** embedded request
cases. An [earlier candidate receipt](parity/deferred_resource_unregistered_controls_5_0_6.json.gz)
explicitly records that registrations were not yet checked; it is not installed
runtime evidence.

The [full control-program receipt](parity/deferred_resource_controller_execution_5_0_6.json.gz)
retains all **69** child commands and outcomes: **18** valid controls pass and
**51** invalid controls reject. It includes the seven new optimized deferred
controls. This receipt supplies the full results that CTest's bounded JUnit
stdout truncates. These saved-observation replays are separate from actual C++
execution, and their record marks `current_cpp_runtime_evidence=false`.

The [selected execution receipt](parity/deferred_resource_selected_tests_5_0_6.json.gz)
passes **5/5** native CTests normally and **5/5** with `PYTHONOPTIMIZE=1`.
Module selections pass **2/2** in each mode. The actual resource binaries each
run all 143 scenarios and compare 163 unchanged-data payloads. Exact JUnit and
registry membership has zero failures, errors or skips; **42** source/evidence
inputs and both executable hashes remain unchanged. The native selection also
checks the existing custom-source prerequisite and the newly registered
column-attribute candidate's two original/refusal cases, 24 valid properties
and thirteen corruptions. That candidate remains unapproved.

This C++ resource mutation/destruction adaptation does not claim that Rust's
immutable borrowed Context permits the same setters while a future is pending.
Only the unchanged-data business outcomes are compared with the original Rust
corpus. No query algorithm or lifetime ownership domain is replaced.

The source/fixture/JUnit receipts measure removal of **342,622 bytes** of owned
disposable files. The full control-program directory is also verified absent;
its size was not measured. Native/module builds remain in their stable preset
directories; compact reports and source snapshots remain in the repository.

The current native registry has **1,093** tests and **611** Python entries;
**49** retain **139** direct assertions. Other imported and embedded executable
checks still require review. Complete SQ02, G13/G14/G17 and P0-P7 remain open
at **14 qualified / 3 partial**. No full 1,093-test native epoch has run.
