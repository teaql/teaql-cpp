# Development toolchain

The current upgrade replaces the supported local build-tool baseline. Earlier
toolchain measurements remain historical records, not compatibility requirements.
The runtime remains C++20 with SQLite as the sole database dependency.

## Locked configuration

| Component | Selected version | Role |
| --- | --- | --- |
| LLVM / Clang / clang-scan-deps | 23.1.3 | Native compiler, sanitizer runtime and module dependency scanner |
| GCC / libstdc++ | 16.2.0 | Native GNU compiler and the shared native C++ standard library |
| CMake | 4.4.4 | Configuration, module scanning coordination and CTest |
| Ninja | 1.13.2 | Build execution and dynamic module dependencies |
| OpenWrt ath79/nand | 25.12.5, GCC 14.3.0, musl | MIPS74K cross compilation |
| SQLite MIPS archive | 3.53.2, rebuilt using GCC 14.3 | Existing matched-engine provider source |
| clang-format | 18.1.8 | Independently pinned project formatting tool |

The selection and archive hashes are in `toolchains.lock.json`. Downloads and
installed tools live under ignored `.tools` directories. LLVM is installed with
its matching scanner. Clang uses GCC 16's libstdc++, rather than relying on an
older system standard library. This avoids making an unrelated libc++ container
compatibility refactor part of toolchain installation.

The GNU compiler/runtime are pinned conda-forge prebuilt GCC 16.2 packages,
including development headers, binutils and the distribution sysroot. Their
publisher hashes are verified before local extraction. No Conda environment or
package manager is required. An official-source bootstrap was explicitly stopped
after the verified prebuilt compiler and Clang/libstdc++ consumer probes succeeded;
its partial build is not an installed or accepted toolchain.

Clang's local driver configuration selects the exact GCC installation and embeds
its runtime-library path. Shell activation also supplies the local GNU runtime
path. This keeps standalone compiler commands and preset builds on the same
standard library. Compiler packages are development dependencies; the production
library still uses only standard facilities and SQLite.

The optional `scripts/toolchains/gcc_native.cmake` selects the same locked GNU
compiler/libstdc++ with the host libc (`CMAKE_SYSROOT=/`), matching native SQLite
and the Clang profile. Its installed-header consumer passes; this is not yet
complete GNU producer/modules qualification. The bundled compiler sysroot has
an older libc and must not be combined with the host SQLite shared library.
See [the installed-package checkpoint](module_packaging.md).

## Install and use

The native module frontend is qualified through the `modules_native`
configure/build/test presets; see [the release matrix](module_compiler_matrix.md).
The ordinary `native` preset explicitly selects the header frontend.
The `modules_sanitize` presets select the same imported contract targets with
the existing ASan/UBSan flags in `build_modules_sanitize`. Their complete
460/460 acceptance is recorded independently in
[the build-validation contract](module_build_validation.md).

On Linux x86_64 with Python3, network access, tar, zstd and system libc development
headers:

```sh
python3 scripts/toolchains/setup.py
python3 scripts/toolchains/setup.py --check
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native
ctest --preset native
```

Installation verifies archive hashes before extraction. The MIPS SQLite
compilation and archiving run sequentially with checked exit statuses. LLVM's zstd archive
needs long-window decompression, supplied by the installer. All compiler/linker
commands must finish before their outputs are executed or inspected.

Activation changes the current shell's tool paths, compiler and library variables. It
does not replace system package-manager binaries or the separately pinned
formatter. The committed presets select absolute repository-local compiler,
Ninja and toolchain paths, avoiding accidental use of the Swift Clang shim.

Routine validation uses the `native` profile and tests relevant to the change
(select tests with `ctest --preset native -R <pattern>`). Full-suite runs are
appropriate for broad changes and integration/release milestones. Documentation-only
changes do not require compiler or runtime checks.

The `sanitize` profile runs instrumented host x86_64 binaries. AddressSanitizer
(ASan) detects memory errors such as use-after-free and buffer overruns;
UndefinedBehaviorSanitizer (UBSan) detects instrumented undefined operations.
Run it for changes affecting memory safety, lifetimes or asynchronous cleanup,
or on explicit request.

The `mips` profile cross-compiles target binaries and executes them through QEMU
on the host. Run it for changes affecting the target toolchain or architecture
support, or on explicit request. These profiles remain available but are not
part of every routine verification. All three profiles use separate build
directories. When a specialized profile is needed:

```sh
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize

cmake --preset mips
cmake --build --preset mips
ctest --preset mips
```

The native profiles use Debug semantics with `-g1`, which reduces debug artifact
size without defining NDEBUG. Sanitizer presets use ASan/UBSan, halt on undefined
behavior and currently disable leak scanning. MIPS uses MinSizeRel, MIPS74K soft
float, static linkage and QEMU74Kf. QEMU is not a physical-router result; physical
WNDR4300 execution remains deferred.

SQLite source and its target archive are installed by setup. Its version matches
the existing supported target source, while the archive is regenerated with the
new cross compiler. The native provider continues using system SQLite. Runtime,
SQL transport/executor and concrete SQLite provider remain independent layers.

## Modules qualification

The installed versions are sufficient candidates for CMake named-module builds.
A separate probe uses `FILE_SET CXX_MODULES`, dependency scanning, a real import
consumer and existing checked-reference/quota/time types. It must compile, link
and run on each qualified profile. Installation or a compiler version alone is
not acceptance of the complete library's module surface.

Existing production files contain no imports; scanning is disabled for that
header-based implementation. Module file sets and importing consumers explicitly
enable their required scanning. This keeps the toolchain upgrade separate from
the planned function-preserving migration in [the modules plan](cpp_modules_plan.md).

Upgrade results and any compiler/library incompatibilities are recorded under
`measurements/toolchains/2026-10-10-upgrade`. Complete validation passes native 388/388, ASan/UBSan 388/388 and MIPS/QEMU 387/387.
Each profile matches both live original examples, retains independent compiled
layers and has 427 nonempty object dependency records. Pre/post builds report no
work after regenerating direct layer metadata. Final sanitizer leak scanning is
disabled; physical-device execution remains deferred. Historical 0.0.1 parity
results remain separate from this new-toolchain checkpoint.
