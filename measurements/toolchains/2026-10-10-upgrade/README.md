# Local toolchain upgrade checkpoint

Baseline: C++0.0.1 at `04fe05bc07c899e089bcbfd7550b5449639fad91`;
Rust remains pinned to v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The user authorized upgrading local tools and dropping old-version compatibility.

Installed and version-checked tools: LLVM/Clang/scanner23.1.3, native GCC/libstdc++
16.2.0, CMake 4.4.4, Ninja 1.13.2, OpenWrt 25.12.5 GCC 14.3.0/musl. Native GCC is
provided by pinned, SHA256-verified conda-forge compiler/runtime packages;
no Conda installation is required. Archive/package details are in the repository
`toolchains.lock.json`. Binaries, archives and BMIs stay under ignored `.tools`.

A CMake FILE_SET CXX_MODULES probe compiles, links and runs on native Clang 23,
native GCC 16 and MIPS GCC 14/QEMU, one check each. The importer instantiates the
existing Ref<int> template, checks its eight-byte layout, exercises original
allocation-limit defaults and the inline Timestamp interface. This proves basic
scanner/import/target feasibility, not a complete library module surface.

Reproduce using the recorded fixture and new tools:

```sh
source scripts/toolchains/activate.sh
cmake -S measurements/toolchains/2026-10-10-upgrade/module-probe -B build-toolchain-probe -G Ninja -DTEAQL_SOURCE_DIR="$PWD" -DCMAKE_TOOLCHAIN_FILE="$PWD/scripts/toolchains/clang-native.cmake"
cmake --build build-toolchain-probe -j4 && ctest --test-dir build-toolchain-probe --output-on-failure
```

For GCC use the pinned native g++ compiler instead of the Clang toolchain file.
For MIPS use scripts/toolchains/openwrt-wndr4300.cmake with the installed
TEAQL_OPENWRT_TOOLCHAIN_DIR; CTest then uses the configured QEMU 74Kf emulator.

Initial failures are retained: libc++ requires a broader allocator-container
compatibility refactor, so this upgrade keeps the existing implementation and
uses new libstdc++. The original-source GCC build was explicitly stopped with
exit130 after verified prebuilt compiler/library probes succeeded. A MIPS test
using sqlite3.h lacked an explicit SQLite usage dependency; the test target now
links SQLite3::SQLite3. A missing <memory> include is supplied explicitly in
memory/clone.hpp and a missing <algorithm> include in core/expr.hpp. No
production function body is changed by these repairs.

CMake 4.4 CMP0189 expands generator-expression link properties transitively.
The first native regression passes387 semantic checks but rejects that expanded
metadata in the unchanged direct-layer verifier. Capturing direct properties at
configuration time restores the intended check, which passes independently.
The complete native regression is rerun after repair; no old-policy fallback or
weakened layer expectation is used.

## Complete project results

| Profile | Full CTest | Seconds | Live original examples | Compiled dependencies |
| --- | ---: | ---: | ---: | ---: |
| Clang 23 / libstdc++ 16 native Debug | 388/388 | 155.59 | 2/2 | 427 |
| Clang 23 ASan/UBSan Debug | 388/388 | 429.58 | 2/2 | 427 |
| OpenWrt GCC 14.3 MIPS MinSizeRel / QEMU 74Kf | 387/387 | 90.87 | 2/2 | 427 |

All compiler/linker commands terminate successfully before their outputs are
used. The final consumer builds finish 725 tasks per profile after header repairs.
All 427 object dependency records are nonempty; independent compiled-layer checks
pass and pre/post builds report no work after direct-metadata regeneration.
Actual example business rows and original SQL provenance match live locked Rust;
full runtime example contracts additionally verify actual C++ SQL provenance.

Native/target registration differs only in the existing native64-only raw
FromSql Debug comparison. Structured error/provider contracts remain on MIPS.
All 451 source-test dispositions remain reviewed and registered. Compiler-body
semantics, context lifetime/allocation rules, generator and dependency scope
are unchanged. `results.json` and profile result files contain terminal statuses.

The three module smoke probes demonstrate basic named-module build and imports,
not a complete public module surface. This upgrade does not implement the
separate modules migration. Native full ASan/UBSan leak scanning is disabled.
Physical WNDR4300 execution remains deferred; target execution uses QEMU.
