# M0-M5 module migration release acceptance

All module migration stages are accepted within the explicitly supported
compiler/target matrix. See [the requirement audit](../../../docs/module_release_audit.json),
[release results](results.json) and [the module plan](../../../docs/cpp_modules_plan.md).
The Rust baseline remains v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

Seven named interfaces retain all 1284 catalog exports and one canonical
implementation. All 315 original production contents remain unchanged, accounting
for one explicit declaration relocation. The 208-header/552-edge graph and seven
imports are acyclic and follow the original layers. Runtime metadata RTTI/layout/
virtual dispatch and facade symbols are qualified without moving existing types
to a new module ABI.

| Release configuration | Acceptance |
| --- | --- |
| Native Clang23/libstdc++16 Debug header frontend | 388 original checks pass; 427 dependency records; final [no-work build](header_no_work.txt). |
| Native production imports | Final [72/72](module_ctest.txt), 177 dependency records and [no-work](module_no_work.txt). |
| Separate ASan/UBSan original plus imports | 460/460, 502 dependency records, leak scanning disabled and final [no-work](sanitizer_no_work.txt). |
| OpenWrt GCC14.3/musl/MIPS74K soft-float headers under QEMU74Kf | 387/387, 427 dependency records and final [no-work](mips_no_work.txt). |

Detailed [sanitizer](../2026_10_10_module_sanitizer/README.md) and
[MIPS](../2026_10_10_module_mips/README.md) results retain their full registrations,
timings and platform exclusions. Physical router execution is deferred;
production MIPS/GNU modules and GPU adapters are not claimed.

Independent [SDK consumers](../2026_10_10_module_packaging/README.md) work after
prefix relocation without producer-path leaks or precompiled BMIs. Three Clang
consumers and one GNU16/host-libc header consumer pass. The complete
[incremental experiment](../2026_10_10_module_builds/README.md) passes all 68
commands and restores all 472 package-file hashes. The isolated
[performance measurements](../2026_10_10_module_performance/README.md) retain the
same compiler/profile: clean build 110.321s headers versus 138.311s modules;
consumer rebuild 9.375s versus 0.605s. Single-run/host-load/RSS limits are explicit.

The source default enables modules only for the exact qualified native Linux
x86_64/Clang23.1.3/libstdc++16.2/Ninja/Debug configuration. Explicit header
overrides and the native header/MIPS presets retain their frontend selection.
[Configuration evidence](../2026_10_10_module_configuration/README.md) covers
defaults and unsupported cross-module rejection. All 451 source dispositions,
75 runtime roots and 75 canonical consumer/fixture bodies remain verified.
Pinned formatting covers 706 handwritten C++ files. No generator or production
function algorithm changes are introduced by this migration.
