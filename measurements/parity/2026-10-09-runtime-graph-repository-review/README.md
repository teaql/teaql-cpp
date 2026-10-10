# Graph repository capability review, 2026-10-09

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

Original EntityDataService exposes graph_node_from_entity<T> with an entity-
boundary check and ordered field capture before model consumption. The installed
C++ repository lacks this public entry; before.cpp uses a complete existing
executor and Entity fixture, and before.txt records the actual compile failure.
The broader free Context extraction implements the separate Audited save role
and cannot replace the repository contract silently. The pre-repair checkpoint records the missing method. The corrected public
member now preserves the repository-specific getter and reserved-field contract.
Complete graph/save acceptance remains partial.

See docs/runtime-graph-save-api-contract.md for reviewed production responsibilities.
Physical-router execution remains deferred; generator/dependency scope is unchanged.

The actual pinned public method produced 141 observations in two matching runs.
Native, ASan/UBSan and MIPS/QEMU live comparisons match all 141, including complete Debug
output in nine modes, getter order, boundary/error behavior and explicit Global
copy diagnostics. C++ uses a move-only model from another Context, destroys that
Context before observation and disables default PMR. The broader free audited
extraction is unchanged. ASan/UBSan and MIPS CTest selections each pass 1/1 with no-work post-test
builds; sanitizer leak scanning is disabled. Final whole-project consumer
results are recorded only after the compiler/linker processes complete.

The unchanged pinned Rust runtime unit suite passes 224 tests; one ignored
file_log_child entry is the subprocess fixture exercised by its parent gate.
All 75 root tests pass. This confirms source observations and fixtures; it
does not demonstrate C++ parity for the remaining production responsibilities.
The suite log is stored in original-runtime-tests.txt.

Complete native integration passes 381/381 in 77.70 seconds with four CTest
workers after all 340 consumer build tasks finish successfully. All 423 compiled
object dependency records are nonempty. Pre/post-test builds report no work.
The complete consumer snapshot includes the graph Debug umbrella and restored
repository extraction. The ledger stays 53 accepted / 2 partial: these checks
do not close retained recursive graph/save responsibilities or stages 5–6.
