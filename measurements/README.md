# Runtime measurements

This directory stores measured build and runtime observations, their source
checkpoints, configuration, and supporting evidence. Each dated record is a
snapshot of a particular experiment. It is not an automatic claim about the
latest source revision or every device in the same architecture family.

| Target | Experiment | Recorded date | Evidence |
| --- | --- | --- | --- |
| Native C++ and Java generator | Typed generated model/query/expression frontend, compiler rejections, real SQLite and selective sanitizer checks | 2026-10-11 | [Generator evidence](generator/2026_10_11_cpp_frontend/README.md) |
| Native Clang23 and targeted ASan/UBSan | Early SQLite adapter retirement, production regression coverage and active target graphs | 2026-10-10 | [Retirement evidence](maintenance/2026_10_10_sqlite_retirement/README.md) |
| Native Clang23, sanitizer and MIPS headers | M0-M5 final module release audit, qualified defaults and complete evidence | 2026-10-10 | [Final module release](modules/2026_10_10_module_release/README.md) |
| Native Clang23, matched Debug profile | Sequential complete/consumer build timings, child RSS and BMI sizes | 2026-10-10 | [Isolated measurements](modules/2026_10_10_module_performance/README.md) |
| OpenWrt GCC14.3/musl/MIPS74K under QEMU | Current header release 387/387, original examples, 427 dependencies | 2026-10-10 | [MIPS release](modules/2026_10_10_module_mips/README.md) |
| Native Clang23 source snapshot | Complete header/module clean and controlled incremental edit/restoration contracts | 2026-10-10 | [Build qualification](modules/2026_10_10_module_builds/README.md) |
| Native Clang23 ASan/UBSan | Complete imported selection, separate BMI configuration, leak scanning disabled | 2026-10-10 | [Module sanitizer checkpoint](modules/2026_10_10_module_sanitizer/README.md) |
| Native Clang23 and GCC16 header consumer | M5 installed SDKs, prefix relocation and independent consumers | 2026-10-10 | [Packaging checkpoint](modules/2026_10_10_module_packaging/README.md) |
| Local native Clang23 | M4 service cycle removal, canonical bodies, dependency graph and runtime ABI | 2026-10-10 | [Structural checkpoint](modules/2026_10_10_module_structure/README.md) |
| Local native Clang23 | M3 runtime/SQLite imports, all75 root contracts and canonical import examples | 2026-10-10 | [Runtime/provider checkpoint](modules/2026_10_10_module_runtime/README.md) |
| Local native Clang23 | M2 data-service/SQL interfaces, SPI and real SQLite import contracts | 2026-10-10 | [Service/SQL checkpoint](modules/2026_10_10_module_services/README.md) |
| Local native Clang23 | M1b memory/async/core production interfaces and import-only contracts | 2026-10-10 | [Module pilot checkpoint](modules/2026_10_10_module_pilot/README.md) |
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
