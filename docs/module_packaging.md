# M5 installed package checkpoint

## Distribution contract

The seven canonical package targets install their libraries and production
headers. A module-enabled producer additionally installs the seven facade
archives and their `lib.cppm` source interfaces, each under its own
`share/teaql_cpp/modules/teaql_*` directory. Legacy compatibility, tests, tools
and generated models are not runtime package dependencies. The generator is
unchanged.

`find_package(teaql_cpp CONFIG REQUIRED)` exposes the existing `teaql::memory`,
`teaql::async`, `teaql::core`, `teaql::data_service`, `teaql::sql`,
`teaql::runtime` and `teaql::provider_sqlite` targets. Consumers inherit standard
C++20, Threads and SQLite usage requirements through the same independent layers.
The module component adds the corresponding `teaql::*_module` targets:

```cmake
find_package(teaql_cpp CONFIG REQUIRED COMPONENTS modules)
target_link_libraries(my_application PRIVATE teaql::provider_sqlite_module)
set_target_properties(my_application PROPERTIES
    CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
```

The installed package follows the existing CMake project version. The generated
`teaql_cppConfig.cmake` and `teaql_cppConfigVersion.cmake` spellings follow CMake's
external package-discovery convention; project-owned template and helper files
retain underscore names. A header-only installation rejects the required modules
component explicitly. Installed modules are currently qualified on native
Clang23 with Ninja and CMake4.4; unsupported module-component configurations are
rejected. GNU/MIPS module qualification remains a separate release decision.

## Relocation and BMI ownership

No precompiled BMI is installed. The consumer builds matching BMIs from the
installed interface sources with its own compiler, configuration and flags.
The facade archives provide distinct module initializer symbols and link the
same canonical libraries; implementation bodies remain single and unchanged.
Standard-library/compiler/architecture ABI compatibility of the linked archives
is still required. Rebuilding a BMI does not make a native archive portable to
another architecture or sanitizer configuration.

In the pinned CMake4.4 export, suppressing BMI installation leaves original
build-tree interface paths in automatically generated per-configuration module
metadata. The package therefore publishes the documented
`IMPORTED_CXX_MODULES_<CONFIG>` property using installed source paths instead of
that generated metadata directory. CMake still exports the public module file
sets and transitive compile/link requirements normally. This keeps the SDK
metadata free of producer source/build paths and requires no rewritten functions
or post-install text patching.

## Independent consumer verification

`scripts/verify_installed_package.py` installs a completed producer build into
a temporary prefix, moves that prefix, and copies the standalone consumer project
outside the producer repository. The original installation prefix no longer
exists. The tool rejects producer-path leaks and distributed BMIs, then configures,
builds and runs actual installed module/header consumers. Every compiler/linker
process must exit successfully before tests or artifact inspection.

The consumer exercises canonical out-of-line core functions, header/import RTTI,
actual SQLite execution and async SPI decoding, User-domain results, explicit
Global copies, actual User deletion and checked-reference expiry. Module BMIs
are rebuilt with Debug `-g0`, while the producer uses `-g1`. Both modes must
finish with no-work rebuilds. A header-only SDK is verified separately.

```sh
source scripts/toolchains/activate.sh
cmake --preset modules_native
cmake --build --preset modules_native
teaql_package_check=$(mktemp -d)
python3 scripts/verify_installed_package.py --build-dir build_modules_native \
    --output-dir "$teaql_package_check/module_package"

cmake --preset native
cmake --build --preset native
python3 scripts/verify_installed_package.py --build-dir build_native_current \
    --output-dir "$teaql_package_check/header_package" --headers-only
```

The native GNU consumer uses `scripts/toolchains/gcc_native.cmake`, selecting the
locked GCC16/libstdc++16 with the host libc sysroot. The bundled compiler sysroot
contains an older libc and cannot link the host SQLite shared library; the
initial failure is retained in measurement evidence. This consumer test does not
qualify a complete GCC-produced runtime or GNU modules.

## Release acceptance

This checkpoint covers package installation, relocation and independent
consumption. The later M5 audit also accepts complete compiler/target release
reconciliation, sanitizer validation, controlled incremental edits, parallel
clean builds and same-compiler performance/resource measurements. See
[the final release evidence](../measurements/modules/2026_10_10_module_release/README.md).
The source default enables modules only for the exact qualified native
Linux x86_64/Clang23.1.3/libstdc++16.2/Ninja/Debug configuration. Explicit
frontend selections remain available. Physical router execution is deferred.
