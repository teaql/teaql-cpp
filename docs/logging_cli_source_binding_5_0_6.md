# Logging CLI source binding, Rust 5.0.6

## Post-edit banner scope

The current comparator corrects the Rust/C++ help-header difference only in
the complete banner context: the audit-help line, the target header and the
following performance-testing section. The [fresh raw Rust replay](parity/log_manager_fresh_raw_source_post_edit_5_0_6.json.gz)
rebuilds the locked 5.0.6 observer in a task-owned target and matches all
**94/94** retained normalized results, including **77** raw Rust banner
headers. [Current native execution](parity/log_manager_full_banner_scope_post_edit_5_0_6.json.gz)
also matches **94/94** and preserves ordinary text that contains either the
phrase alone or a copied section without the preceding audit-help line.
The revised source contract, negative controls and **4/4** selected logging
CTests pass. The one-off Rust target was removed (159,773,335 bytes). The
preceding full 1,063-test native epoch predates this comparator edit; complete
current-source integration and G13/G14/G17/P0–P7 remain open.

The installed `log_manager_rust_parity` and `audit_event_rust_parity`
comparisons now require one source fixture. Fresh builds of the unchanged
original Rust observers produced all **94 log-manager** and **1,427 audit-event**
outputs. They match the retained Rust 5.0.6 corpora. Current C++ execution
matches all 94 normalized log-manager results and all 1,427 native 64-bit audit
results. The audit comparator also checks the target pointer width and count
rejection before execution; this native run has zero rejected inputs.

The [source contract](../tests/parity/logging_cli_contract_5_0_6.json) pins the
locked clean Rust checkout, observer files, probe/comparator sources, input
corpora, full original outputs and the new narrow Cargo manifest/lock. Its 115
package dependency closure is checked against the exact reference lock. The
older shared `rust_values` lock still contains Rust 5.0.5 local packages and
is not used as source-closure evidence. The source observers themselves were
not edited. An initial narrow-manifest failure for missing direct imports is
retained as a [diagnostic](parity/logging_source_cli_initial_manifest_failure_5_0_6.json.gz);
the corrected manifest compiled and ran successfully.

The [fresh log execution](parity/log_manager_cli_original_execution_5_0_6.json.gz)
and [fresh audit execution](parity/audit_event_cli_original_execution_5_0_6.json.gz)
retain all original outputs and build/source hashes. Log comparison normalizes
only the volatile Human UTC timestamp and the documented Rust versus C++ help
header correction. It does not claim raw byte equality for those fields.
The historical input matrices are unchanged. Fourteen log output cases in
the earlier historical corpus differ from the later frozen 5.0.6 corpus;
fresh original Rust execution confirms the later outputs used here.

The [15 negative controls](parity/logging_cli_controls_5_0_6.json.gz) reject
changed input or output cases, source hashes, original-execution receipts,
CTest commands and removed fixtures. The [native receipt](parity/logging_cli_native_5_0_6.json.gz)
and [five selected CTests](parity/logging_cli_selected_ctest_5_0_6.json.gz)
cover both installed comparisons and the source fixture. The one-off Rust
Cargo target was removed after the successful replay.

The [incremental dispatcher lookup](parity/logging_cli_remaining_dispatchers_5_0_6.json.gz)
maps **157/160** comparison locators, leaving three SQLite/runtime-example
comparisons. The **1,049** CTest registrations are enumeration, not full-suite
acceptance. This milestone changes no runtime, generator, generated SDK, C++
algorithm or Context ownership. Complete G13/G14/G17, GP13/GP16, AP02 and
P0–P7 remain open. No new sanitizer, MIPS, module, GPU, physical-router or
full-suite gate was run.
