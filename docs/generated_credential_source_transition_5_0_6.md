# Generated Credential receipt source transition

The historical generated Credential receipt remains immutable. Its original
execution and input snapshot are evidence for that earlier source state. The
current-source verifier now admits exactly three later source revisions while
requiring the other 530 of its 533 inputs to retain their original hashes.

| Input | Historical SHA-256 | Current SHA-256 | Current evidence |
| --- | --- | --- | --- |
| `scripts/verify_generated_audit_sql_sinks_5_0_6.py` | `938778c2ea65a053a7bcc41328a1e8ca61c76b7e2eccf079e8ce230969a2dead` | `bdfc09b352dcf7e8b5e066d64753a43959b8670fc2b085218907636e0a3bbf93` | [AP02 generated SQLite execution](parity/ap02_index_consumer_sql_5_0_6.json.gz) |
| `tests/parity/verify_dynamic_json_privacy_sinks_5_0_6.py` | `b27a9f34616f12b91d68d89f15c42620450f3d4e631905b9376e93c2d470f119` | `1e2412dd22b0943c894d8a22fbe4db0eca558566df989d9a395a6abced41a88d` | [Optimized privacy controls](private_sql_sink_optimized_checks_5_0_6.md) |
| `generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java` in `teaql_code_gen_cpp` | `a3f16897f7fcfce33c863dc959e392850268621762ac2f957011cca4c6bee0ce` | `09c664a667ba1009e73add94adfbf1a1f4f69d7ed4e8f7cc52b380175d03179c` | [Installed parser source](parity/ap02_index_consumer_installed_source_5_0_6.json.gz) and AP02 SQLite execution |

The verifier pins both AP02 receipt bytes, checks their locked Rust and
generator revisions, validates 80,185 parser cases and 157 Java tests in the
installed-source receipt, and checks 41 successful AP02 SQL steps with 12 Save
cases and 12 sink cases. It also pins the permanent privacy control source.
The AP02 SQL input set differs from the Credential input set only by the two
declared source revisions above and one explicitly replaced metadata corpus;
the privacy verifier revision is separately admitted with its six normal and
optimized live C++ controls. All remaining Credential inputs retain their
historical hashes. The four source-specific negative controls reject a stale
unchanged file and each altered transition; the Java classpath check remains
strict.

The [current-source receipt](parity/generated_credential_source_transition_5_0_6.json.gz)
records 17 rejected corruption controls, 530 unchanged inputs and three
transitions. Focused native CTests pass 3/3 for the Credential receipt and
private SQL sink contracts/controls; the Credential verifier also passes under
`python3 -O` with zero removable assertions. This is receipt admission and
source provenance, not fresh generated Credential execution or full P7
acceptance. A complete native epoch is still required.
