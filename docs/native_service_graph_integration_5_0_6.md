# Native integration checkpoint: Rust 5.0.6

The current frozen native integration completed successfully on 2026-10-22
(local development time): **877/877** registered tests passed, with no failures,
errors or skipped tests. Both stable-build checks reported no work. The
[terminal receipt](parity/native_service_graph_integration_terminal_5_0_6.json.gz)
retains every command, complete CTest output, the exact registry and terminal
JUnit. Its `complete_generator_goal` remains false.

The [input snapshot](parity/native_service_graph_integration_inputs_5_0_6.json.gz)
covers **11,399** runtime, script, example, test, CMake, module, JSON-ledger and
checked-in producer inputs. Before/after hashes and both reference revisions
are unchanged. The Rust reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`;
the generator reference is `45e1572c4956ca963410d8ea436ead3c8857087a`.

The sequence verifies locked references and generator provenance, configures the
native preset, completes its all-target build, checks a stable build, captures
the unique test registry, runs the complete preset and checks another stable
build. The compiler acceptance test is included in the 877 tests. The owning
wrapper waits for the controller to exit and then removes its temporary state,
logs and JUnit directory: **1,207,478 bytes** reclaimed. The committed compressed
receipts retain those diagnostic contents. The reusable native preset build
remains available for subsequent affected-target work.

This supersedes the earlier failed or interrupted full-native integration
attempts for this exact source checkpoint; those receipts remain immutable.
It does not relabel later changes as having passed this full run. It does not
supply a new sanitizer, MIPS/QEMU, physical-device, GPU or external-provider
result. Complete source dispositions still require their explicit responsibility
and caller reconciliation. All six partial capabilities and full P0-P7 exit
criteria remain separate.

During this freeze, the [generated identity and stream continuation](container_identity_execution_5_0_6.md)
used owned temporary Java/C++ source overlays. Its passing 35-entry proposal and
16-case real SQLite lifecycle matrix are **not installed inputs** in this native
checkpoint. The [post-probe freeze audit](parity/native_service_graph_identity_freeze_audit_5_0_6.json.gz)
confirms zero execution-input changes. Installation needs its own current
producer/runtime/compiler/module and relevant lifetime verification.

The [approved JSON payload exception](json_payload_compatibility_exception_5_0_6.md)
continues to preserve C++ structured JSON instead of reproducing the original
JSON/LargeText conflict. The exception is not counted as exact Rust equality
and does not authorize changes to unrelated source behavior.
