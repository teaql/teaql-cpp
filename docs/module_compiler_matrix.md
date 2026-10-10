# Module release compiler and target matrix

## Accepted configurations and explicit scope

| Configuration | Current evidence | Release gate |
| --- | --- | --- |
| Native Clang23.1.3/libstdc++16.2, CMake4.4.4/Ninja1.13.2, Debug | Seven production modules, all exports, runtime ABI, 72 imported checks, 388 original checks, independent installed consumers, complete incremental/dependency edits/restoration and isolated measurements pass. | Native module release accepted. |
| Same native compiler with ASan/UBSan | Separate module build; 72/72 imported checks and complete 460/460 original/imported integration, 502 dependencies and final no-work rebuild. Leak scanning disabled. | Sanitizer release gate accepted. |
| OpenWrt25.12.5/GCC14.3/musl, MIPS74K soft-float, SQLite3.53.2 | Current header release passes 387/387 in 91.47 seconds under QEMU74Kf, with 427 dependencies and no-work rebuild. | Header fallback accepted; production MIPS modules are not adopted. |
| Native GCC16.2/libstdc++16.2 with host libc | Independent installed-header consumer compiles, links and runs against the native SDK. | This scopes consumer interoperability; a full GNU producer or GNU module surface is unqualified. |

Older compiler and build-tool versions remain historical. The supported runtime
dependency set is standard C++20 facilities and SQLite. Physical WNDR4300
execution is deferred. A basic module probe on a compiler is distinct from
qualifying all seven production interfaces, their templates and runtime contracts.

The source default enables modules only for the exact accepted native Linux
x86_64/Clang23.1.3/libstdc++16.2/Ninja/Debug configuration. The standard library
is identified through the configured implicit include directories. Other
configurations default to headers. Explicit header presets/overrides and
unsupported-module diagnostics remain available. All M5 release gates are
accepted within this matrix; see [the final audit](module_release_audit.json).

Source and installed-package configurations explicitly reject cross compilation
when modules are requested, including Clang cross configurations. This closes a
version-only eligibility gap without adopting a production MIPS module toolchain.
See [the configuration evidence](../measurements/modules/2026_10_10_module_configuration/README.md).

## Current compiler observations

The GNU compiler distribution includes an older libc sysroot. Linking its
default sysroot with the host SQLite shared library fails on missing newer
GLIBC symbol versions. `gcc_native.cmake` selects the host libc with
`CMAKE_SYSROOT=/`, retaining the locked GNU compiler/libstdc++16. The installed
header consumer then passes. This is an actual sysroot compatibility correction,
not evidence for a GNU production-module surface.

Clang23 warns at `MutationRequest::comment()` about a returned stack address.
The reviewed source uses a local `std::span<const TraceNode>` over request-owned
trace storage, then returns a view into the last node's allocator-aware string.
Destroying the span does not destroy its referenced nodes or their strings.
The result remains a borrow, invalid when its request storage or owning Context
ends. Existing executor/data-service contracts exercise final comments and
explicit destination copies. The original production body remains unchanged;
the warning and complete sanitizer release evidence are recorded separately.

BMI caches are separate for ordinary and instrumented builds. Installed SDKs
ship interface sources, so consumers regenerate matching BMIs. Archive ABI,
standard library, architecture and instrumentation compatibility remain required.
