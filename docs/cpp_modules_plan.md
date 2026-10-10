# C++20 modules assessment and migration plan

Checkpoint: C++ `0.0.1`, commit
`04fe05bc07c899e089bcbfd7550b5449639fad91`. Rust v5.0.5 remains pinned at
`62940ae2b88e7acc32763853b8b4685a01c7e90a`.

This plan concerns standard C++20 named modules, rather than namespaces,
RuntimeModule objects or compiler-specific Clang header modules. Production
implementation remains header-based; the completed toolchain upgrade is separate
from the structural migration planned here. An isolated
feasibility probe is retained in [assessment evidence](../measurements/modules/2026-10-10-assessment/README.md).

## Toolchain upgrade authorization

The user has authorized upgrading the local toolchain and dropping old-version
compatibility requirements. The selected new baseline is documented in
[the toolchain guide](development_toolchain.md). Earlier tool/version rows below
record the `0.0.1` assessment snapshot, not the required future configuration.
The new OpenWrt GCC14.3 toolchain has already built and run a real CMake module
probe under QEMU. Complete new-toolchain regressions pass native 388/388, sanitizer 388/388 and
MIPS/QEMU 387/387; all three real CMake module probes also pass. This qualifies
the tools, not the complete library module surface.
Maintain a header frontend only as an implementation/migration boundary while
adding modules; it does not imply maintaining GCC11/GCC13 or old CMake/Ninja.

## Recommendation

Begin with optional module facades over the existing implementation. Preserve
one definition of each production function and the accepted header build,
while qualifying the upgraded modules toolchain.

Separate three milestones: consumers can import the library; internal
dependencies form an explicit acyclic module graph; selected implementations
can compile as module units. A facade achieves the first milestone. Moving
existing declarations to native named-module ownership requires a separate ABI
decision before proceeding to the third.

Modules change compilation and visibility, not Context lifetime safety or
runtime ownership. Query/mutation/save/log algorithms, core concepts and
allocation semantics must remain intact.

## Rust directory structure and unified names

The selected layout follows the in-scope Rust workspace crates at the locked
commit. Convert hyphens in Rust crate names to underscores for C++ project-owned
names. Use the same package name for the directory, CMake target and primary
named module: `teaql_core`, not `teaql-core` or `teaql.core`. Existing C++
namespaces such as `teaql::core` and CMake aliases such as `teaql::core` remain
unchanged; they are distinct mechanisms, not alternative module names.

| Pinned Rust crate | C++ package directory / target / primary module | Pre-move implementation at `0.0.1` |
| --- | --- | --- |
| `teaql-core` | `teaql_core` | `include/teaql/core*`, `src/core` |
| `teaql-data-service` | `teaql_data_service` | `include/teaql/data_service*`, `src/data_service` |
| `teaql-sql` | `teaql_sql` | `include/teaql/sql*`, `src/sql` |
| `teaql-runtime` | `teaql_runtime` | `include/teaql/runtime*`, `src/runtime`, excluding legacy SQLite |
| `teaql-provider-sqlite` | `teaql_provider_sqlite` | `include/teaql/provider_sqlite`, `src/provider_sqlite` |
| C++ lifetime adaptation | `teaql_memory` | `include/teaql/memory`, `src/memory` |
| C++ asynchronous protocol adaptation | `teaql_async` | `include/teaql/async`; initially an interface target |

The two C++ adaptation packages do not replace or remove Rust concepts. Keep
the legacy SQLite adapter isolated under `compatibility/teaql_legacy_sqlite`;
it is not a Rust crate or part of the production import surface. Do not add
workspace directories for excluded databases, cloud components, integrations
or the deferred generator.

Each package owns its declarations, implementation, module entry and tests.
Rust `lib.rs` maps to the C++ primary module interface `src/lib.cppm`;
Rust `mod.rs` maps to the corresponding internal directory entry when needed.
Directories organize responsibilities; they do not force one named module or
partition for every Rust file.

```text
CMakeLists.txt                       # workspace wiring and shared configuration
teaql_memory/
teaql_async/
teaql_core/
    CMakeLists.txt
    include/teaql/core.hpp            # canonical header frontend during migration
    include/teaql/core/...
    src/lib.cppm                     # export module teaql_core;
    src/query.cpp
    src/expr.cpp
    src/...
    tests/...
teaql_data_service/
teaql_sql/
teaql_runtime/
    CMakeLists.txt
    include/teaql/runtime.hpp
    include/teaql/runtime/...
    src/lib.cppm                     # export module teaql_runtime;
    src/context/                     # locking, logging, pagination, transaction
    src/data_service/                # base, context, executor, graph, helpers,
                                     # relation, resolved, types, core cache
    src/entity_save.cpp
    src/entity_runtime.cpp
    src/mutation_policy.cpp
    src/log_formatter.cpp
    src/...
    tests/...
teaql_provider_sqlite/
examples/
tests/parity/                        # shared Rust oracle and comparison harness
compatibility/teaql_legacy_sqlite/
scripts/
docs/
measurements/                        # historical evidence retains original paths
```

The tree specifies destination responsibilities, not existing files or new
algorithms. A Rust responsibility implemented by several C++ sources retains
those sources and bodies. For example, route `context_logging.cpp` to
`teaql_runtime/src/context/logging.cpp` and `context_pagination.cpp` to
`teaql_runtime/src/context/pagination.cpp`. Place the implementation of each
Rust data-service responsibility under the matching `data_service/` subtree;
keep query/mutation helper splits explicit instead of combining their types.
Package-local internal headers should follow the same responsibility structure
where feasible. A file that spans responsibilities must be recorded in the
mapping before structural splitting; do not infer placement from its basename
alone. The existing five-header service cycle stays within the coarse runtime
boundary until its declarations and definitions can be separated safely.

During facade adoption, package-local `include/` directories retain the existing
`<teaql/...>` include spelling. CMake propagates the required include directories
through each target, so moving files does not require copying header bodies or
changing downstream namespaces. The final module implementation can later
change declaration placement after the separate ownership/ABI gate.

### Naming scope

- Use `lower_snake_case` for project-owned package directories, file stems,
  scripts, fixture paths, targets and new internal identifiers. Preserve public
  C++ type/API spellings; this refactor does not rename established APIs.
- Primary module names match packages, for example `import teaql_data_service;`.
  If a justified partition is introduced, use an underscore-separated component,
  for example `teaql_runtime:context_logging`. Do not introduce parallel dotted
  spellings for the same package.
- Active owned paths use `tests/parity/teaql_runtime`, `scripts/format_cpp.py`
  and underscore-separated documentation/toolchain file stems. Update these in
  a dedicated path change, including callers, relative imports, links, CMake sources,
  preset paths, formatter discovery/exclusions and current evidence ledgers
  together. Preserve CTest contract names when already valid.
- Upstream crate names, Rust source IDs, URLs, downloaded tool names, command
  options, required external filenames such as `.clang-format`, and immutable
  historical measurements retain their authentic spelling. Do not globally
  replace `-` in source text or rewrite pinned-reference provenance. Move owned
  fixture filenames while preserving their contents and Rust source identifiers.

Before moving sources, create a checked old-to-new path ledger that identifies
the owning Rust responsibility, C++ package and whether the file is public,
internal, test support or historical evidence. Reject destination collisions
and dangling live references. Update the existing parity ledgers' C++ paths
without changing their Rust source IDs, acceptance statuses or test dispositions.
The inventory/export scripts currently assume root `include/` and `src/`, and
the formatter does not discover `.cppm`; adapting these tools is a prerequisite
for accepting moved production files and module interfaces.

## Current repository and tools

There are 211 production headers containing 37,771 lines and 102 production
`.cpp` files containing 26,698 lines. Templates, inline methods, concepts,
allocator construction and private execution helpers are substantial parts of
the header implementation; they cannot all be hidden in ordinary `.cpp` files.

| Entry header | Transitive project headers | Project-header lines |
| --- | ---: | ---: |
| `teaql/core.hpp` | 53 | 9,555 |
| `teaql/data_service.hpp` | 43 | 8,050 |
| `teaql/sql.hpp` | 53 | 10,157 |
| `teaql/runtime.hpp` | 150 | 28,518 |
| `teaql/runtime/context.hpp` | 64 | 12,320 |
| `teaql/runtime/data_service.hpp` | 112 | 23,383 |

These are literal project-include counts, excluding standard/system headers,
not preprocessed input or measured parsing time. They suggest possible consumer
build benefits. Interface compilation, scanning and BMI loading add work, so
speedup is not guaranteed.

The include scan identifies a five-header strongly connected component:

- `runtime/detail/data_service.hpp`
- `runtime/detail/base_query.hpp`
- `runtime/detail/context_query.hpp`
- `runtime/detail/base_mutation.hpp`
- `runtime/detail/context_mutation.hpp`

The service header includes method-definition headers that include it again.
Include guards permit this today. Mechanical conversion to mutually importing
modules would create an invalid import cycle. Separate shared declarations from
method definitions structurally, retaining classes, friend access and bodies.

| Profile | Observed configuration | Modules assessment |
| --- | --- | --- |
| Accepted native / sanitizer | GCC 11.4, CMake 3.22.1, Ninja 1.10.1 | Preserve header support; below the CMake modules integration requirements. |
| Available local Clang | Clang 21.0.0; `clang-scan-deps` absent from PATH | Manual small probe succeeds; a complete matching scanner/tool distribution is still needed. |
| Accepted MIPS | OpenWrt GCC 13.3.0, musl, MIPS74K soft float | Header/QEMU profile is accepted; target modules are unverified. |

CMake introduced named-module support in 3.28. Its scanning support includes
Clang 16+ and GCC 14+; Ninja module builds require Ninja 1.11+. Header units are
not supported by this integration. Treat these as eligibility floors and pin
an exact tested combination in M0. See [CMake documentation](https://cmake.org/cmake/help/v3.28/manual/cmake-cxxmodules.7.html).

The GCC 13-series manual documents partial modules support, extra compiler
options and missing private-module-fragment/partition behavior. See
[GCC 13 documentation](https://gcc.gnu.org/onlinedocs/gcc-13.4.0/gcc/C_002b_002b-Modules.html).
The adjacent release's documentation does not qualify our 13.3 cross compiler.
Selecting C++20 or adding an experimental flag does not establish integration.

A newer host compiler does not upgrade MIPS. The original assessment kept MIPS as a header consumer
during adoption; the newly authorized GCC14.3 profile can now be qualified for modules. Mandatory target modules require independent qualification of
a cross compiler/scanner and OpenWrt sysroot, including musl, libstdc++, SQLite,
exception/unwind support and target flags. This assessment performs no toolchain
upgrade or physical-device test.

Modules are a build-time mechanism. The router runs the linked machine-code
artifact and does not load compiler BMIs. This does not by itself require a new
CPU feature or modules runtime library. Verify target instructions, linkage,
initialization, artifact size and QEMU behavior under any new cross toolchain.

## Risks and controls

| Risk | Consequence here | Control / required evidence |
| --- | --- | --- |
| Module ownership and ABI | Module-attached declarations may no longer match global declarations/definitions in existing `.cpp` files. | Initially include declarations in the global module fragment and explicitly export existing names. Verify linking, symbols, type identity, RTTI and virtual dispatch. Ownership changes need a separate ABI gate. |
| Export completeness | Root aliases alone omit usable constructors, factories, constants, overloads and auxiliary C++ APIs. | Maintain an export/reachability ledger against accepted contracts; test import-only consumers. Preserve namespaces and separate graph/schema types. |
| Templates, concepts and customization | Entity concepts, allocator construction, ADL, friend helpers and hash specializations may fail for downstream importers. | Instantiate handwritten entities and custom checkers/providers; verify defaults, constraints, overload resolution, hash/allocator customization and destination copies. Keep template bodies reachable. |
| Cycles and private access | A file-by-file conversion creates import cycles or loses private/friend access. | Begin with a coarse runtime facade. Later separate declarations/definitions within existing responsibilities and validate the import DAG. |
| Shared declarations / mixed consumers | Core/runtime facades share global headers; compiler merging can fail with includes and imports. | Test combined modules and mixed translation units. Prefer standard includes before imports initially; do not promise every ordering without evidence. |
| BMI mismatch or stale state | Compiler/target/library/macros/flags can make an interface unusable or inconsistent. | Separate caches by compiler/version, ABI/sysroot, target, configuration, sanitizer and relevant flags. Rebuild matching interfaces; BMIs are not portable release binaries. |
| Macros | Named imports do not provide textual macro behavior. Existing private mutation macros and model-registration fixtures need handling. | Audit macros; retain necessary textual shims and implementation-private expansion. Preserve braces. Keep standard/SQLite headers textual initially; do not alter the generator. |
| Build correctness | Object dependency records do not prove interface ordering or invalidation. | Use supported scanner/CMake file sets; check clean parallel builds, interface/body edits, dependency changes and no-work rebuilds. Extend BMI checks while retaining object/layer checks. |
| Compile time / RAM | Large BMIs, serialization and dual support may offset parsing savings. | Measure clean, incremental and consumer-only builds, peak compiler RAM and BMI size. Partition according to actual dependencies and results. |
| Packaging / tools | Install consumers, clangd, formatter and parity scripts currently assume headers/`.cpp`. | Ship interface sources and usage metadata; test an external consumer. Add `.cppm` discovery/formatting and module-aware export checks; preserve generated-table exclusions. |
| Semantic drift | Structural edits could alter memory domains, cleanup, save ordering or audit privacy. | Keep one canonical function implementation. Separate correctness repairs from structural changes and retain source differential evidence. |

Clang documents a compatible export-using migration style, linkage implications
and mixed include/import limitations. This motivates facade-first adoption and
explicit compatibility checks. See [Clang migration guidance](https://clang.llvm.org/docs/StandardCPlusPlusModules.html#transitioning-to-modules).

## Protect the function implementations

For the first implementation milestone, preserve production bodies, layouts,
namespaces, signatures, default arguments and ownership. A facade includes
existing declaration/definition sources in its global module fragment, explicitly
publishes intended names and links the existing library. It does not compile a
second copy of those functions.

The isolated Clang time facade calls existing `Date::from_ymd`, `Date::epoch_days`
and `Timestamp::from_u64` functions from the accepted archives, plus existing
inline access and `std::hash<Timestamp>`. An initial BMI without `-pthread` is
rejected by a consumer compiled with it; matching flags fixes the failure.
This demonstrates limited feasibility, not whole-library ABI, CMake, template/
runtime or MIPS modules acceptance.

Later edits may change include/import prologues, export annotations and
declaration/definition placement. Keep each body in one canonical source used by
the supported frontends. Do not duplicate algorithms into a module tree, repeat
default arguments or create equivalent global and module-owned types. Template
definitions remain available for downstream instantiation.

Review each affected function's constraints, member access, errors, ordering,
allocator domain and cancellation/cleanup. Any unavoidable change to these is
a correctness change with its own review and tests. Moving genuinely out-of-line
bodies to implementation units is allowed only after ownership/linkage is resolved.

## Proposed boundaries

| Named module | Existing responsibility |
| --- | --- |
| `teaql_memory` | Arena, allocation types, limits, references and callable/storage support |
| `teaql_async` | Existing operation/stream/wake interfaces; no new executor or owner |
| `teaql_core` | Core models, expressions, queries, entities and related protocols |
| `teaql_data_service` | Independent requests/results, metadata, logs and executor SPI |
| `teaql_sql` | Dialect/compiler/bindings, executor, transport and transactions |
| `teaql_runtime` | Context, registration, entity/query/graph/ledger, checking/governance, logs and services |
| `teaql_provider_sqlite` | Sole database provider, including runtime ID/schema implementations |

The async facade is a compilation boundary over current headers, not a new
runtime layer; qualify its dependency placement before adding it. Module names
do not rename namespaces or move Context ownership.

Follow actual signatures and existing library dependencies: core uses memory;
service/SQL use their lower layers; runtime uses SQL/service/core; SQLite
implements SQL and runtime SPIs. Provider-to-runtime is expected and must not
become runtime-to-provider. No production module imports legacy compatibility.
An aggregate import can reexport modules without merging libraries, but is not
required initially.

Begin with coarse boundaries, not one module per header/Rust file. Add internal
partitions only after dependency and generic reachability review. Record
transitive name visibility explicitly rather than losing it during splitting.

## Work stages

| Stage | Work | Implementation change limit | Exit gate |
| --- | --- | --- | --- |
| M0: qualify tools / design | Pin tools; complete the Rust-to-C++ path/export ledger, naming inventory, customization/macro/cycle review and deeper Context/template probes. | Development probes and documentation. | Exact configurations, collision-free destinations, complete responsibilities, symbol/type evidence and target strategy. |
| M1a: workspace layout / names | Move packages in dependency order: memory/async, core, data-service, SQL, runtime, then SQLite. Introduce package CMake files; normalize active owned paths and update tooling/ledgers with each move. | Paths, include directives, build and development-tool wiring; function bodies unchanged. | Native header consumers, existing contracts/examples and export/layer checks remain valid; no stale live paths or duplicated bodies. |
| M1b: memory/core module pilot | Add `src/lib.cppm` facades for memory, async and core with a guarded CMake path; retain the canonical header frontend. | New interfaces and wiring; function bodies unchanged. | Import-only consumers, complete exports, out-of-line linking, shared-header combinations, references/allocator/lifetime contracts. |
| M2: service/SQL/SQLite | Add package facades following existing targets; keep standard/SQLite headers textual. Introduce the SQLite facade after its runtime imports are qualified in M3. | Facades and wiring; no algorithms/layout changes. | Replaceable SPI, defaults/overloads, real SQLite/transactions/streams, independent compiled layers. |
| M3: runtime/examples | Preserve Rust context/data-service subtrees; add a coarse runtime facade, complete the SQLite facade, then switch the two examples to imports; qualify downstream models/checkers/providers. | Consumer imports and exports; mappings/bodies preserved. | Public namespaces/root APIs/all75 root contracts, actual original example rows and C++ SQL provenance, lifetime/graph/governance/privacy composition. |
| M4: internal organization | Resolve service cycle structurally; partition justified areas; consider implementation units for non-template functions. | Placement/prologues and one canonical body; ownership moves need an explicit ABI decision. | Acyclic graph, complete template/default/customization behavior, no duplicate symbols/types, compatible consumption or documented versioned ABI transition. |
| M5: release qualification | Test packaging, compiler matrix, incremental correctness and performance; enable modules by default only on qualified profiles. | Distribution/build defaults; no unrelated semantic refactor. | Full relevant native/module sanitizer suites, accepted MIPS header suite; separate MIPS module suite if adopting a qualified cross toolchain; external-consumer and clean/no-work evidence. |

M0 tool installation and basic compiler/scanner qualification are complete
under the upgraded profiles. Export/reachability inventory and deeper imported
The M1a path/naming ledger is recorded in
[module_path_migration.json](module_path_migration.json). Package directories,
package-local CMake definitions, active underscore paths and the Rust-shaped
runtime context/data-service subtrees have been moved. M1a is accepted after
the successful native build, 388/388 CTest contracts, 427 active dependency
records, unchanged production contents and source/export/format checks. See
[workspace move evidence](../measurements/modules/2026_10_10_workspace_move/README.md).
Deeper Context/template import qualification and
production module interfaces remain M1b work. Directory organization does not
prove a usable named-module surface.

## Verification, fallback and completion

Use `0.0.1` and the final parity evidence as the comparison checkpoint. Preserve
all 451 source-test dispositions and accepted source contracts. Existing totals
are native 388, sanitizer 388 and MIPS 387; record meaningful added module checks
honestly. Header-only test success does not establish module exports. Require
import-only callers and downstream generic instantiation as well.

Build interfaces/importers through the supported scanner/build system and wait
for terminal compiler/linker success before tests. Default to relevant native
checks during routine development. Run ASan/UBSan for memory safety, lifetime or
asynchronous cleanup changes, and MIPS/QEMU for target toolchain or architecture
changes; either profile may also be explicitly requested. Do not repeat all
profiles after every edit. Run relevant original-source matrices, full integration
at boundary milestones, both examples and compiled-layer checks. Preserve
Global/User limits, teardown, explicit copies
and stop/join behavior. Leak scanning was disabled in final parity sanitizer
validation; physical WNDR4300 execution remains deferred.

Record BMI configuration fingerprints. Test parallel clean/no-op builds,
declaration edits, ordinary body edits, template body edits and dependency
changes. Keep compiler, host/target and sanitizer BMIs separate. Package interface
sources with usage requirements so consumers build matching interfaces. Measure
the same compiler/profile before and after; distinguish compiler-upgrade effects.

Review each structural patch separately: declaration/export/build wiring in one
change, implementation relocation in another if required. For unchanged bodies,
compare signatures and body tokens against the checkpoint; use the same compiler
for any AST comparison and retain semantic regression as the acceptance gate.
Record approved differences explicitly instead of treating a clean compilation
or a text move as sufficient proof.

The proposed `TEAQL_ENABLE_MODULES` option starts OFF. When ON, unsupported
configuration fails clearly rather than silently building headers and calling it
modules. Separate build directories keep a working header fallback. Structural
stages can be reverted without weakening semantic tests or rewriting functions.

Completion requires a usable named-module surface, one canonical implementation,
supported acyclic dependencies and reproducible external consumers. Performance
gain and deeper module-owned implementation are independent measured/ABI
milestones, not assumptions behind the initial import surface.
