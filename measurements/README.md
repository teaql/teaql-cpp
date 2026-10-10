# Runtime measurements

This directory stores measured build and runtime observations, their source
checkpoints, configuration, and supporting evidence. Each dated record is a
snapshot of a particular experiment. It is not an automatic claim about the
latest source revision or every device in the same architecture family.

| Target | Experiment | Recorded date | Evidence |
| --- | --- | --- | --- |
| Local native workspace | Rust-shaped package moves, underscore names and unchanged production contents | 2026-10-10 | [M1a move checkpoint](modules/2026_10_10_workspace_move/README.md) |
| Local native, sanitizer and MIPS tools | Locked toolchain upgrade and real named-module probes | 2026-10-10 | [Upgrade checkpoint](toolchains/2026-10-10-upgrade/README.md) |
| Local Clang and repository inventory | C++20 modules risk assessment and isolated compatible time facade | 2026-10-10 | [Assessment and limited probe](modules/2026-10-10-assessment/README.md) |
| Native, ASan/UBSan and OpenWrt MIPS/QEMU | Six-stage final acceptance: complete regressions, live examples and requirement audit | 2026-10-10 | [Final report and evidence](parity/2026-10-10-stage6-final-review/README.md) |
| Netgear WNDR4300, MIPS 74Kc | Static and dynamic executable size comparison | 2026-10-05 | [Report and data](mips-wndr4300/2026-10-05-linkage/README.md) |
| Netgear WNDR4300, MIPS 74Kc | SQLite numeric conversion and emulator configuration | 2026-10-08 | [Report and reproduction](mips-wndr4300/2026-10-08-sqlite-numeric/README.md) |

Reports use English, record exact bytes before rounded units, and distinguish
executable size from shared-library requirements. Emulator results and physical
device results must be identified separately. Do not infer latency, RAM use, or
compressed firmware size from executable file size.

Store structured results and enough configuration to interpret or reproduce an
experiment. Keep compiled executables, shared libraries, SDK archives, and build
trees in their ignored build locations; record their hashes and measured sizes
here. New experiments should use a new dated directory rather than overwrite
an earlier source checkpoint's results.
