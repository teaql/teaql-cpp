# TEAQL C++

TEAQL for edge runtimes, following the core design of Rust **v5.0.5** with SQLite as the only database implementation. The project connects TEAQL to the C++ systems programming and GPU ecosystem. Memory has two explicit lifetimes: application-owned `GlobalContext` and `UserContext`. Development is kernel first, using C++20 standard facilities and few dependencies.

Pinned commit: `62940ae2b88e7acc32763853b8b4685a01c7e90a`. See the [baseline lock](upstream.lock.json) and [migration plan](docs/migration-plan.md).

Two runnable examples reproduce the business semantics of the corresponding Rust examples:

- [sqlite_schema_crud](examples/sqlite_schema_crud.cpp): schema creation, insert, update, soft delete, and query; versions change from `1` to `2` to `-3`.
- [sqlite_relations_graph](examples/sqlite_relations_graph.cpp): atomic persistence of an order and two items, followed by queries and product-reference assembly.

These handwritten examples remain regression baselines. Their local relationship assembly is not the generic entity graph runtime. They will be connected to that runtime after its contracts are implemented. The model generator has not been changed.

## Current implementation

The kernel includes explicit PMR allocation; every Value variant, including Decimal, JSON, dates, and timestamps; metadata and collection foundations; expression/query models; distinct mutation commands; request and dynamic-search models; Entity/Audited contracts; SafeExpression evaluation; annotated save-input graphs; and SQLite parameter binding, optimistic locking, restore, transactions, and safe SQL logs. Production dependencies are the C++20 standard library and SQLite. See the [lifetime contract](docs/context-lifetime.md).

The independent `teaql::data_service` target provides requests, results, execution metadata, log provenance, and asynchronous executor interfaces. Context-owned operations and streams stop and join native work before object destruction. The legacy SQLite example path still uses its transitional session/compiler implementation.

Runtime resource registration by type/name, locals, and ContextError have 410 fixed Rust comparisons. Registries borrow resources from explicit Global/User scopes. Check results, object paths, separate wire models, all RuntimeError variants, and DataServiceError have 1,134 comparisons; nested errors support explicit copies into a destination Context. Runtime MetadataStore/EntityRegistry lookups remain separate from data-service descriptor snapshots.

Identity graphs, three-state relation handles, change-set stacks, and the mutation ledger have 241 fixed-source comparisons, plus tests for same-scope shared state and explicit cross-scope copies of cyclic graphs. Four graph decoder forms, behavior/RequestPolicy interfaces, and RuntimeModule entity registration have 182 public API comparisons covering installation, failures, and replacement. Native SQLite rows are decoded into handwritten entity graphs in integration tests.

EntityDataService, ContextDataService, RuntimeDataService, and Context metadata views are separate abstractions. Basic query adapters have 256 comparisons, and relation planning/query construction has 112. Query preparation preserves behavior, policy, and relation-key projection order, including the bootstrap exception. Context query telemetry has 468 comparisons; relation window/per-parent probe execution has 432, with real SQLite tests for per-parent offset/limit, stable ID ordering, and metadata-controlled routing.

Recursive embedded-row and ordinary/compact graph hydration has 356 fixed-method comparisons. Relation aggregates, object grouping, and child-query enhancement have 266 comparisons and native SQLite tests. These kernels compose through an independent entity-fetch boundary; complete public entity query orchestration remains pending.

Independent aggregation-cache, pagination-cursor, and retained-ID-set stores, their in-memory implementations, Context registration, and observations have 259 comparisons. Scoped ID-set build coordination has another 144 comparisons against the original registry and actual asynchronous mutex. Its directory belongs to GlobalContext; leases, waiters, and guards belong to UserContext. Complete cache/pagination execution and example wiring remain pending.

Business IDs preserve legacy daily-sequence and default daily-permuted-v1 strategies, encoding keys, and the static key-provider interface. V1 HMAC-SHA256/permutation has 2,150 comparisons. Injectable clocks, nanosecond UTC time, in-memory allocation, BusinessIdService, and Context installation have 859. Internal ID interfaces, AtomicCounter/Snowflake, SQLite adaptation, Context generation, and fixed bootstrap initialization have 171; the fallback counter belongs to an explicit GlobalContext. Controlled schema execution preserves provider, business-ID schema, and bootstrap order, with 384 comparisons.

Raw/safe audit models, governance snapshots, sink interfaces, and privacy projection have 1,427 comparisons. SQL/unified log models and safe reprojection have 425, including private safe snapshots and tampering fallback. Human/Debug formatters, Global factories, and log configuration have 927. LogManager output, endpoint caching, and application-scoped one-time warnings have 94 isolated-process comparisons. Context metadata recording, SQL log options/snapshots, and unified buffering have 315. Telemetry interfaces, failure isolation, and Context propagation have 1,032. Governance dynamic scopes, event delivery, and module sink registration have 1,088. EntityValues/GraphNode, mutation plans/batches, and trace scopes have 432, including initial/root graph registration and cross-scope copying. Business save/governance call sites remain pending.

The independent SqlDialect supports expressions, subqueries, aggregates, partition pagination, and basic schema compilation, with 2,206 comparisons. Single/batch/guarded mutation compilation has 2,505, including real SQLite execution. Index/name compilation and validation have 1,305.

The independent `teaql::provider_sqlite` target provides its dialect, native binding, and row decoding, with 1,084 binding and 6,029 decoding/error comparisons. Production fetch/mutation, repeated execution, native streams, and statement/layout caches have 796 live provider comparisons. The old static compiler is now `LegacySqliteCompiler` in runtime; SqliteSession still uses this transitional path.

SQL transport/transaction interfaces and errors are separate. Default repeated fetch has 344 comparisons. Query executor caching, probes, and diagnostics have 381; mutation/guarded execution, batch diagnostics, and rereads have 1,012. Transaction begin/query/mutation/guard/commit/rollback adaptation has another 1,012. Native/transaction-buffered streams and final diagnostics have 880. Native SQLite transactions have 366. SQLite ID-space generation has 261 cross-version comparisons, with two native diagnostic differences documented separately. Independent business-ID allocation has 198; BusinessIdSchemaContributor and its SQLite implementation are present.

The production physical SQLite schema provider has 735 public-entry comparisons. It preserves audit events inside the DDL transaction, rollback on failure, cache clearing on success, and legacy-graph rejection after commit. Unicode column behavior is checked against the pinned Rust standard library. Complete schema data backfill and entity save orchestration remain pending.

## Scope and limitations

Allocations explicitly select `global.make<T>()` or `user_context.make<T>()`; external RAII wrappers use the corresponding `make_resource<T>()`. Context results are borrows that become invalid on close. Retention across domains requires an explicit deep copy.

Query models preserve functions, subqueries, aggregates, relations, and pagination/stream configuration. The legacy SQLite path explicitly rejects advanced execution that it does not implement; constructing a model does not establish execution support. Generic graph save, mutation policy/approval, business-entry audit calls, runtime relation streaming, and complete log dispatch remain migration work.

The legacy session uses separate LegacySqlLogEntry/LegacySqlLogOperation types. It fully hides Masked/Unknown/Credential parameters, including nested credentials, and has no plaintext debug mode. This remains more conservative than Rust's partial masking of business values.

The API is still under migration. Unsupported features are recorded as incomplete rather than advertised through placeholders. Follow the [six-stage plan](docs/migration-plan.md), [design parity contract](docs/design-parity.md), and [module responsibility map](docs/parity/cpp-module-map.md).

## Build

Requirements: CMake 3.20+, a C++20 compiler, and SQLite 3.37+ development headers/libraries, such as `libsqlite3-dev`. Core and memory targets depend only on the standard library. Tests require Python 3. Ordinary CTest uses recorded Rust observations and does not require Rust.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/teaql_context_lifetime
./build/teaql_sqlite_schema_crud
./build/teaql_sqlite_relations_graph
```

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
python3 scripts/format-cpp.py --install
python3 scripts/format-cpp.py
python3 scripts/format-cpp.py --check
```

The CMake targets `format` and `format-check` run the same commands. An existing
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
python3 scripts/verify-upstream.py
```

## Validation

Both original Rust examples were run from the pinned reference to verify version transitions and order/item/product relationships. C++ examples include runtime assertions and CTest entries. Native tests cover core models, arena lifetimes, SQLite rollback/conflicts, privacy, and binding. Tests disable the default PMR resource to detect hidden allocation fallback.

The Value corpus contains 722 recorded Rust cases for conversions, precision, overflow, date boundaries, and invalid JSON; dynamic search has 491 for validation, warnings, and type boundaries. Optional live runners are documented in [parity testing](tests/parity/README.md). Decimal/JSON/Date/Timestamp database binding and metadata decoding are implemented in the independent provider; the old SqliteSession example path retains type limitations.

```sh
python3 scripts/verify-upstream.py
python3 scripts/inventory-upstream.py --check
```
