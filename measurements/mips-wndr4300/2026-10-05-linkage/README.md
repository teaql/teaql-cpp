# WNDR4300 static and dynamic linkage measurements

Recorded on 2026-10-05. The SQLite CRUD executable is 1,487,520 bytes when fully
static and 140,184 bytes when libc, the C++ runtime, and SQLite are shared. Sharing
only libc makes it slightly larger, at 1,491,924 bytes. These numbers exclude
shared dependencies and do not describe total deployment or physical RAM usage.

## Source and method

The compiled TEAQL objects belong to source checkpoint
`c9b96bb4ee6546fe7e9f71b9855f15d93d892eb7`. The dynamic variants reuse those same
objects from the original CMake build to isolate linkage changes. This is not
a rebuild of the current HEAD. Rust remains pinned to v5.0.5 at
`62940ae2b88e7acc32763853b8b4685a01c7e90a`.

The build uses `-Os`, `-ffunction-sections`, `-fdata-sections`,
`-Wl,--gc-sections`, and `--strip-all`. File sizes are measured after stripping.
One KiB is 1024 bytes; one MiB is 1,048,576 bytes. The C++ runtime category includes
libstdc++ and libgcc. TEAQL's own libraries remain embedded in every executable.

| Environment item | Recorded value |
| --- | --- |
| Reported hardware | Netgear WNDR4300, Atheros AR9344 rev 2, MIPS 74Kc V4.12 |
| Generated code and ABI | MIPS32r2, O32, big endian, soft float; `-march=74kc` |
| Toolchain distribution | OpenWrt 24.10.4 ath79/nand |
| C++ compiler | OpenWrt GCC 13.3.0, revision r28959-29397011cc |
| libc | musl 1.2.5 |
| Binutils strip | 2.42 |
| SQLite | 3.40.1 |
| Build tools | CMake 3.22.1, Ninja 1.10.1 |
| Emulator | QEMU 6.2.0, user mode, CPU model `74Kf` |
| Actual router software | OpenWrt release and installed library versions not supplied or verified |

The toolchain release is the build environment, not a confirmed router release.
[Reported CPU information](target-cpuinfo.txt) and
[observed tool versions](tool-versions.txt) are stored separately.

## Executable file sizes

All entries are exact bytes. Shared libraries are excluded.

| Example | Fully static | Shared libc only | Shared libc and C++ runtime | Shared libc, C++ runtime and SQLite |
| --- | ---: | ---: | ---: | ---: |
| teaql_context_lifetime | 571,208 | 572,180 | 17,240 | 17,240 |
| teaql_sqlite_schema_crud | 1,487,520 | 1,491,924 | 794,840 | 140,184 |
| teaql_sqlite_relations_graph | 1,491,792 | 1,496,196 | 798,936 | 143,704 |

The fully shared CRUD executable is approximately 136.9 KiB, a 90.6% reduction
in executable bytes relative to its static counterpart. The Context-only
executable is approximately 16.8 KiB. Sharing libc alone is not an effective
size reduction for these measured artifacts. These observations do not prove
a general result for other examples, optimization settings, or library builds.

| Record variant | External dependencies directly recorded by ELF |
| --- | --- |
| fully-static | None; no dynamic interpreter |
| libc-dynamic | libc.so |
| runtime-dynamic | libc.so, libstdc++.so.6, libgcc_s.so.1; SQLite stays embedded where used |
| fully-dynamic | libc.so, libstdc++.so.6, libgcc_s.so.1, and libsqlite3.so.0 for SQLite examples |

The Context-only executable does not require SQLite even in the fully dynamic
variant. All dynamic executables request `/lib/ld-musl-mips-sf.so.1`.
See [machine-readable results](results.json) and [CSV sizes](sizes.csv).

## Shared libraries and deployment accounting

The emulator's stripped shared libraries were also measured:

| Library | Exact file bytes | Approximate KiB |
| --- | ---: | ---: |
| libc.so | 791,836 | 773.3 |
| libstdc++.so.6 | 2,114,812 | 2,065.2 |
| libgcc_s.so.1 | 139,716 | 136.4 |
| libsqlite3.so.0 | 872,012 | 851.6 |

These are test copies from the toolchain and the rebuilt SQLite library, not
measurements of installed router packages. Reusing compatible installed libraries
can make the executable the only additional file. If libc is present but all
three other libraries are absent, the fully dynamic CRUD executable plus those
dependencies totals 3,266,724 uncompressed file bytes. That is larger than this
single static executable; shared dependencies can instead be amortized across
multiple applications. Actual installed libraries and compressed firmware
footprint remain unmeasured.

[Shared-library data](shared-libraries.csv) and `results.json` retain individual
sizes, hashes, and dependency closures. The conditional deployment totals assume
all non-libc dependencies are missing and count each library once. They are
arithmetic derived from these records, not observed target installation sizes.

## SQLite build and validation

The original SQLite object is not position independent and cannot be used to
create a shared library. SQLite 3.40.1 was rebuilt from the same amalgamation with
`-fPIC`, `-Os`, `-DNDEBUG`, and the same MIPS CPU and floating-point flags.
Its soname is `libsqlite3.so.0`. The static and shared implementations report
matching [SQLite compile options](sqlite-compile-options.txt). ELF inspection
found no TEXTREL tag in the shared library.

All nine dynamic executable variants completed under QEMU 74Kf with exit code 0.
Their stdout and stderr matched the corresponding original static example.
Saved stdout is under `outputs/`; the earlier
[static build validation](static-build-validation.txt) records the original
CMake build and emulator results. ELF evidence under `elf/` confirms headers,
interpreter, dependencies, and soft-float ABI. GOT entry listings are omitted
from the architecture-specific dump because they are not needed for these checks.

The 24Kc emulator model failed on the original 74Kc DSP/DSP2 output; the successful
runs use 74Kf. This is an emulator compatibility observation, not permission to
change the target ABI. Physical WNDR4300 execution is still pending. Querying
tool versions and inspecting ELF files do not verify the router's kernel or
installed libraries.

These examples validate their current handwritten behavior. They do not prove
generic graph-save completion or introduce new memory-safety guarantees.
Execution time, RSS, maximum memory, and compressed package size were not measured.

## Files and reproducibility

| File or directory | Contents |
| --- | --- |
| results.json | Source checkpoint, toolchain archive hash, SQLite source hash, sizes, dependencies, artifact hashes, and validation status |
| sizes.csv | Twelve executable records: three examples across four linkage variants |
| shared-libraries.csv | Four stripped shared-library measurements |
| commands.json | Linker, strip, SQLite PIC compilation, and emulator command arguments with normalized path tokens |
| artifact-sha256.txt | Hashes identifying the measured binaries and shared libraries; the binaries themselves remain in ignored build locations |
| record-sha256.txt | Hashes of the saved evidence and reports in this directory |
| elf/ | ELF headers, program headers, dynamic sections, and ABI attributes |
| outputs/ | Saved stdout for each example and linkage variant |
| target-cpuinfo.txt | CPU information supplied by the user |
| tool-versions.txt | Observed compiler, libc, strip, emulator, CMake, and Ninja versions |
| sqlite-compile-options.txt | Compile-option report verified equal for static and shared SQLite |
| static-build-validation.txt | Original static build and emulator validation record |

Command arrays use `@REPO_ROOT@` and `@TOOLCHAIN_ROOT@` rather than a developer's
absolute workspace path. Set `STAGING_DIR` to the toolchain staging directory.
Link commands run in `@REPO_ROOT@/build-wndr4300`; the SQLite PIC compile command
uses its explicitly recorded working directory. Replace `@BINARY@` with the
selected executable for strip and emulator commands. The original static link
command names the unstripped build output; the measured copy was stripped into
the original `dist/` directory.

Reproduction requires the recorded source checkpoint, the matching OpenWrt SDK,
SQLite amalgamation, and the CMake-compiled object files. The saved commands are
observed build recipes, not a one-command build of a fresh checkout. Rebuild those
inputs in a separate checkout before relinking; do not treat current HEAD as
the recorded source. See `results.json` for the SDK archive's SHA-256 and the
SQLite source file's SHA-256.

Dynamic local binaries remain under
`build-wndr4300/link-comparison/<variant>/`; static binaries are under
`build-wndr4300/dist/`. The local emulator sysroot is
`build-wndr4300/link-comparison/target-root/`. The compatibility of these libraries
with an actual router must be checked before deployment.

Validate this saved record from this directory with:

```sh
sha256sum -c record-sha256.txt
```
