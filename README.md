# TEAQL C++

TEAQL for edge runtimes, following the core design of Rust **v5.0.5** with SQLite as the only database implementation. The project connects TEAQL to the C++ systems programming and GPU ecosystem. Memory has two explicit lifetimes: application-owned `GlobalContext` and `UserContext`. Development is kernel first, using C++20 standard facilities and few dependencies.

Pinned commit: `62940ae2b88e7acc32763853b8b4685a01c7e90a`. See the [baseline lock](upstream.lock.json) and [migration plan](docs/migration_plan.md).

All six migration stages are accepted within the documented C++ ownership,
owner-thread and supported-protocol adaptations. The source ledger is **55
accepted / 0 partial / 2 external exclusions / 1 reference-only helper**.

| Final profile | Complete CTest | Live original examples |
| --- | --- | --- |
| Native Debug | 388/388 | 2/2 |
| ASan/UBSan Debug | 388/388 | 2/2 |
| OpenWrt musl MIPS/QEMU, SQLite 3.53.2 | 387/387 | 2/2 |

All profiles have 427 nonempty object dependency records and no-work builds
before/after testing. Final sanitizer leak scanning is disabled. One raw FromSql
Debug comparison is native64-only; structured errors and provider contracts
remain covered on MIPS. Physical WNDR4300 execution is deferred by the user.

All 451 original source tests have reviewed dispositions: 441 covered, five
non-SQLite backend exclusions and five external exclusions. Complete leaf and
root responsibility reviews accompany the tests; names and counts alone are
not acceptance. Handwritten examples use the actual entity/query/graph runtime.
The generator is unchanged. C++20 standard facilities and SQLite remain the
production dependency scope; GPU adapters and external integrations are excluded.

See [the final requirement audit](measurements/parity/2026-10-10-stage6-final-review/README.md)
and [the migration plan](docs/migration_plan.md).

The C++20 named-modules refactor follows [the modules plan](docs/cpp_modules_plan.md).
The workspace now uses Rust-shaped `teaql_core`, `teaql_data_service`, `teaql_sql`,
`teaql_runtime` and `teaql_provider_sqlite` package directories, plus the C++
`teaql_memory` and `teaql_async` adaptations. Each package owns its CMake definition,
headers and implementation; runtime implementations preserve `context/` and
`data_service/` subtrees. Shared test fixtures live in `tests/support`, while the
Rust comparison harness stays in `tests/parity`. Public `<teaql/...>` include
spelling and C++ APIs remain stable. Production `lib.cppm` interfaces and import
consumers remain the next milestone; directory moves alone do not establish
named-module support. See [the workspace move](docs/workspace_move.md).

Two runnable examples reproduce the business semantics of the corresponding Rust examples:

- [sqlite_schema_crud](examples/sqlite_schema_crud.cpp): provider schema creation, audited entity graph saves, soft delete, and typed query; versions change from `1` to `2` to `-3`.
- [sqlite_relations_graph](examples/sqlite_relations_graph.cpp): one runtime graph save of an order and two items, followed by enhanced entity queries loading `lines.product`.

Both examples now use the entity/query/graph runtime and independent SQLite provider. Handwritten models reproduce the locked Rust example mappings; runtime code owns traversal, planning, child writes, transactions and relationship queries. The model generator has not been changed.

## Implementation and ownership

The C++ kernel retains the Rust 5.0.5 core models, distinct mutation/snapshot/
generated values, expression/query/request models, Entity/Audited contracts,
annotated input graphs, identity graphs and mutation ledgers. Data-service, SQL
executor, dialect, transport, SQLite provider and runtime remain independent
layers. The opt-in legacy SQLite compatibility target does not supply the
production runtime path.

Runtime responsibilities include entity decoding and relations, pagination/
aggregation caches, schema/bootstrap, internal/business IDs, reference codecs,
checkers and translation, policy/approval, graph/ledger saves, TransactionScope,
raw/safe audit events, source log provenance/privacy and telemetry interfaces.
The public in-memory query engine retains its original supported behavior.
Complete contracts, source ordering, failure paths and differential evidence
are linked from [the module map](docs/parity/cpp_module_map.md).

Application code receives UserContext and delegates Global allocation through it.
Managed objects explicitly select `global.make<T>()` or
`user_context.make<T>()`; native RAII resources use scoped `make_resource<T>()`.
Growing multi-block arenas run registered destruction and native cleanup before
reclaiming all blocks. Context results are borrows, invalid after close. Nested
containers inherit the allocator; retention across domains requires an explicit
deep copy. Asynchronous work stops and joins before referenced storage is freed.

The [memory implementation](docs/context_memory_implementation.md) provides
64-bit object references, separate Global/User live bitmaps, non-reused IDs,
object/byte limits, aggregate User budgets, shutdown reserves, stable byte/string
storage and bounded explicit interning. Checked resolution validates identity
and liveness. Arbitrary escaped native pointers and unmanaged PMR stack objects
still require the [lifetime contract](docs/context_lifetime.md).

## Scope and limitations

Production dependencies are C++20 standard facilities and SQLite. Generator
changes, GPU adapters and concrete non-SQLite/cloud/external integrations remain
outside this migration. Their retained core interfaces are not claims of an
external backend implementation. Legacy compatibility limitations are documented
separately in [the compatibility contract](docs/sqlite_compatibility.md).

The accepted contracts describe explicit C++ ownership, owner-thread dispatch,
protocol and native-width diagnostic adaptations. Final sanitizer leak scanning
is disabled. Physical WNDR4300 execution is deferred; target results use QEMU.
A passing test suite cannot establish safety for raw pointers used after close.

## Build

Use the locked local development tools described in [the toolchain guide](docs/development_toolchain.md). The current baseline is CMake4.4, Ninja1.13, Clang23/GCC16 and OpenWrt GCC14.3. SQLite3.37+ development headers/libraries are required for native builds. Core and memory targets depend only on standard facilities. Tests require Python3; recorded parity tests do not require Rust.

```sh
python3 scripts/toolchains/setup.py --check
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native
ctest --preset native
./build_native_current/teaql_context_lifetime
./build_native_current/teaql_sqlite_schema_crud
./build_native_current/teaql_sqlite_relations_graph
```

## Measurements

The [runtime measurements](measurements/README.md) directory stores dated results
with source checkpoints and supporting data. The
[WNDR4300 linkage comparison](measurements/mips-wndr4300/2026-10-05-linkage/README.md)
records static/dynamic MIPS executable sizes, shared-library costs, command
arguments, ELF evidence, hashes, and emulator output. Physical-device validation
remains pending; these are measurements of the recorded checkpoint.

## C++ formatting

Use the repository's clang-format **18.1.8** configuration for handwritten headers,
sources, examples, and tests. The style uses four spaces, a 100-column target, and
expanded function/control-flow blocks. `InsertBraces: true` automatically adds braces
to `if`/`else`, `for`, `while`, and `do` bodies, including single statements.
Macro definitions and bodies spanning preprocessor directives require explicit
braces in the source because clang-format skips automatic insertion there.
Formatting preserves include order, string literals, and comments. Long literals
may exceed the column target. Generated
Unicode fact tables are excluded through `.clang-format-ignore`.

Install the development tool in an ignored repository-local virtual environment:

```sh
python3 scripts/format_cpp.py --install
python3 scripts/format_cpp.py
python3 scripts/format_cpp.py --check
```

The CMake targets `format` and `format_check` run the same commands. An existing
clang-format 18.1.8 can be supplied with `--clang-format PATH`; mismatched versions
are rejected. Editors can read `.clang-format` and `.editorconfig` directly.
Git selects tracked and new, nonignored files, keeping build artifacts out of the
formatting scope. Run the check before submitting C++ changes. Python/LLVM formatting
tools are development-only and add no runtime dependency. The available options are documented
in the [LLVM 18.1.8 style reference](https://releases.llvm.org/18.1.8/tools/clang/docs/ClangFormatStyleOptions.html).

## Obtain the pinned reference

Reference sources support development comparisons and are not part of the C++ build.

```sh
git clone --branch v5.0.5 https://github.com/teaql/teaql-rs.git .reference/teaql-rs
git -C .reference/teaql-rs checkout --detach 62940ae2b88e7acc32763853b8b4685a01c7e90a
python3 scripts/verify_upstream.py
```

## Validation

Both original Rust examples were run from the pinned reference to verify version transitions and order/item/product relationships. C++ examples include runtime assertions and CTest entries. Native tests cover core models, arena lifetimes, SQLite rollback/conflicts, privacy, and binding. Tests disable the default PMR resource to detect hidden allocation fallback.

The Value corpus contains 722 recorded Rust cases for conversions, precision, overflow, date boundaries, and invalid JSON; dynamic search has 491 for validation, warnings, and type boundaries. Optional live runners are documented in [parity testing](tests/parity/README.md). Decimal/JSON/Date/Timestamp database binding and metadata decoding are implemented in the independent provider; the retained legacy SqliteSession compatibility path has separate type limitations.

```sh
python3 scripts/verify_upstream.py
python3 scripts/inventory_upstream.py --check
```
