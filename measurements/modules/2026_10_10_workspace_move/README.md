# M1a workspace move evidence

This checkpoint records the Rust-shaped C++ package and underscore-name move.
See [the implementation notes](../../../docs/workspace_move.md) and
[the full path ledger](../../../docs/module_path_migration.json).

The 776 owned-file moves include 315 production files. Their non-include content
fingerprints match the pre-move worktree, and their exact non-include bytes
match `0.0.1`; generated tables and moved data fixtures
retain exact bytes. The Rust source inventory and all 451 test dispositions are
preserved. Production C++20 module interfaces remain a separate M1b milestone.

Native build and verification processes completed successfully. All 388 CTest
contracts pass in 71.30 seconds, and all 427 active compiled objects have
nonempty dependency records. The pinned formatter checks all 682 handwritten
C++ files. Root exports and all 451 source-test dispositions/registrations remain
valid. See [results.json](results.json), [CTest output](native_ctest.txt),
[registration](native_registration.json) and [build summary](native_build_summary.txt). ASan/UBSan and MIPS/QEMU
are not repeated for this directory-only change. Historical toolchain and parity
measurements continue to describe their original directory layout and binaries.
