# M1b memory, async and core module pilot

This document records the original M1b checkpoint. The current catalog and test
presets are extended by [M2](module_services.md); its counts do not retroactively
change the six-check pilot evidence below.

The production interfaces are `teaql_memory/src/lib.cppm`,
`teaql_async/src/lib.cppm` and `teaql_core/src/lib.cppm`. Consumers can import
`teaql_memory`, `teaql_async` and `teaql_core` using the pinned native Clang23
toolchain. This completes M1b for that profile; service/SQL/runtime/provider
modules, packaging and the broader compiler matrix remain later milestones.

## Implementation and ownership

The interfaces include canonical declarations in their global module fragments
and export existing names with using declarations. Their module libraries link
the existing kernel archives. Production function bodies, type layouts,
namespaces, signatures and allocation semantics remain unchanged. This follows
[Clang's ABI-preserving migration guidance](https://clang.llvm.org/docs/StandardCPlusPlusModules.html#export-using-style).

Core and async reexport memory through an acyclic import graph. Named-module
targets are separate frontends: `teaql::memory_module`, `teaql::async_module`
and `teaql::core_module`. Existing `teaql::memory`, `teaql::async` and
`teaql::core` header targets retain their independent layer contracts.
No runtime owner, executor, dependency or alternate implementation is introduced.

The [export catalog](module_exports.json) records 49 memory, 17 async and 308
core namespace/name entries, including existing template-support declarations.
These 374 entries include aliases and support names; they are not a count of
Rust concepts or independently implemented behaviors. Combined and isolated
import-only translation units verify every catalog name. The catalog derives
from Clang23 declarations for the selected canonical entry headers; it is not
a replacement for public/default/trait/protocol reviews.

## Build and contracts

```sh
source scripts/toolchains/activate.sh
cmake --preset modules_native
cmake --build --preset modules_native
ctest --preset modules_native
python3 scripts/verify_module_exports.py
python3 scripts/verify_module_paths.py
```

At this M1b checkpoint, `TEAQL_ENABLE_MODULES` defaults OFF. The later M5 release
enables it only for the exact qualified native configuration; see
[the final matrix](module_compiler_matrix.md). The modules preset uses a separate build
directory and builds the qualified module consumers. Unsupported generators or
compilers fail at configuration rather than silently falling back to headers.
Native GCC and MIPS basic toolchain probes remain historical qualification of
the tools; they do not qualify these complete production interfaces.

Six module CTest contracts pass:

- `module_import_contracts`: complete name lookup, Context/template instantiation,
  custom allocator-aware copies, actual User deletion, invalid-reference checks,
  async state factories, handwritten Entity/defaults/graph construction, distinct
  mutation/snapshot/generated-value types, trace allocators, timestamp hashing,
  existing out-of-line calls and mixed-frontend RTTI/type identity.
- Five existing core contracts: Decimal, auxiliary Debug, collections, core
  models and query models. Development-only adaptation removes project includes
  and inserts imports; every original assertion and function body is retained.
  Generated consumers and their body fingerprints stay in the ignored build
  directory. This does not modify the model generator or production sources.

The test-only `teaql_context_qualification` interface exports existing Global,
User and ArenaObject declarations for downstream qualification. It is not a
production runtime facade and does not claim the M3 runtime export surface.
Global payload retention remains explicit: handles issued through a User expire
with that User, and a subsequent User must `rebind_global` before resolving a
retained Global object. Importing does not weaken that lifetime rule.

The module consumers explicitly include the standard facilities they use before
imports. Qualified combinations cover memory/async/core imports, reversed import
order, a separate header translation unit, and header-before-import in one
translation unit. Arbitrary header-after-import ordering is not promised.

## Evidence and next gates

Native module contracts pass 6/6, and the corresponding five original header
contracts plus the compiled layer check pass 6/6. The 315 existing production
files retain exact non-include contents, and formatting covers the new `.cppm`
and module test sources. No-op builds perform no compiler or linker work.
Evidence is retained under
[the M1b checkpoint](../measurements/modules/2026_10_10_module_pilot/README.md).

Sanitizer and MIPS suites were not repeated: this milestone changes exports and
build wiring, preserving ownership and target implementation bodies. Their
selective validation policy remains in effect. M2 next adds data-service and SQL
facades; the SQLite facade depends on the M3 runtime qualification. M4 and M5
retain internal organization, ownership/ABI, installation, compiler-matrix and
incremental-build release gates.
