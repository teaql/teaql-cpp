# Protected Behavior CLI source binding, Rust 5.0.6

The installed `protected_behavior_rust_parity` comparison now requires a
source fixture. A fresh Rust build executed the complete locked 5.0.6 runtime
and generated Note Behavior through real SQLite, with one append-only test
access bridge for private deletion and recovery. Its 1,296 complete outputs
match the frozen original corpus; fresh C++ execution also matches
**1,296/1,296** outcomes. The matrix covers delete, hard delete and recover,
Behavior callbacks, RequestPolicy, stale/missing/trigger rows, event-sink
failures, mutation intent and trace switches.

The [source contract](../tests/parity/protected_behavior_cli_contract_5_0_6.json)
reconstructs all 610 original source-file hashes from the exact Rust Git
archive. It verifies that the sole compiled change appends a test access
bridge to `resolved.rs`, leaving original method bodies byte-identical. The
contract also pins the original generated Checker model and all 19 files,
reference-seeded Cargo dependencies, complete input matrix, and
Behavior/Policy/event/SQL/result counts.

The [fresh original execution](parity/protected_behavior_cli_original_execution_5_0_6.json.gz)
retains all 1,296 outcomes, source and executable hashes, and the successful
Rust/C++ comparison. The task-owned Cargo target and derived archive were
removed after capture; the source fixture verifies the archive directly from
the locked Git revision. The historical source and terminal receipts each
have one stale comparator hash after a CTest split. Their other recorded
source hashes remain current.

The [15 negative controls](parity/protected_behavior_cli_controls_5_0_6.json.gz)
reject altered sources, cases, outcomes, archive counts, CTest commands and
fixture removal. The [native receipt](parity/protected_behavior_cli_native_5_0_6.json.gz),
[selected CTest receipt](parity/protected_behavior_cli_selected_ctest_5_0_6.json.gz),
and [complete selected log](parity/protected_behavior_cli_ctest_log_5_0_6.txt.gz)
record four passing tests, including the installed comparison. The temporary
JUnit file was removed. The
[milestone receipt](parity/protected_behavior_cli_milestone_5_0_6.json.gz)
records the bounded result and artifact hashes.

The [incremental dispatcher lookup](parity/protected_behavior_cli_remaining_dispatchers_5_0_6.json.gz)
retains 222 rows: 154 of 160 comparison locators have source contracts, with
six remaining. The 1,043 CTest registrations are enumeration. The
[previous-input delta](parity/protected_behavior_cli_previous_input_delta_5_0_6.json.gz)
rehashes earlier protected inputs against the current checkout.

This is raw EntityDataService autocommit execution. It does not establish
audited graph Save or whole-plan Mutation Policy equivalence. The change
binds evidence and test admission; it changes no runtime, generator,
generated SDK, original Rust method, C++ Behavior algorithm, or Context
ownership. Complete G13/G14/G17, GP13/GP16, AP02, and P0–P7 remain open. No
new sanitizer, MIPS, module, GPU, physical-router, or full-suite gate was run
for this source-binding change.
