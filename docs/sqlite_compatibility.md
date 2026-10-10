# Retirement of the early SQLite compatibility adapter

After tag `0.0.2` (commit `63e4d50526e0525aa02d92e6367b49237b26796e`),
the extra early C++ adapter is removed at the user's request. This retires
`SqliteSession`, `LegacySqliteCompiler`, the legacy log types, both old
`teaql/runtime` forwarding headers, and the `teaql::legacy_sqlite` CMake target.
These were C++ compatibility APIs, not pinned Rust v5.0.5 responsibilities.
Existing callers must use the production layers; no forwarding shim remains.

The only supported database path is UserContext / EntityDataService, the
independent data-service and SQL executor/transport interfaces, and
`teaql::provider_sqlite`. Query, mutation, graph/ledger save, transaction,
provenance and privacy responsibilities remain distinct. The two handwritten
SQLite examples already use this path. No generator or production function
implementation changes are needed for this removal.

## Regression coverage disposition

`sqlite_contracts` now executes
[production provider regressions](../teaql_provider_sqlite/tests/provider_sqlite_regression_tests.cpp).
It uses the real dialect, executor, runtime service and native provider rather
than another combined adapter. Application result allocation remains explicit.
The runtime logging test drops only its obsolete legacy-type distinction and
links runtime alone; its source-aligned models/privacy/ownership checks remain.

| Old adapter coverage | Current production coverage / disposition |
| --- | --- |
| Schema and mapped columns; insert/query/update/delete/recover/hard delete | New provider regressions create the schema through SqliteDialect, query the mapped name, exercise runtime mutation and `1 -> 2 -> -3 -> 4`, reject stale versions, and hard delete. Existing schema, mutation and example contracts remain. |
| Duplicate identity, SQL failure outcome, credential parameters and audit intent | New provider regressions exercise real duplicate writes and retained success/failure privacy. Existing runtime log and Context logging contracts retain recursive credential and provenance checks. |
| Literal SQL injection text and typed NULL | New regressions use actual runtime commands/predicates and the production binder. Text remains data; typed NULL survives a committed transaction. |
| Failed transaction rolls back earlier work; successful commit | New regressions use the production transaction SPI and explicit rollback after the duplicate write. Native transaction/TransactionScope and graph rollback tests retain cancellation, nesting and complete graph behavior. |
| Nested Object/List/JSON privacy and unsupported bindings | JSON is supported by the production provider. New regressions verify nested JSON round trip and privacy on success, plus raw Object rejection/privacy without input mutation. Binding and runtime log contracts retain List and recursive credential checks. |
| Null default PMR, nested result allocators, Global retention after User deletion | New regressions disable default PMR, inspect actual row/value allocators, and explicitly copy a row into Global before deleting UserContext. Existing binding/decoding/runtime example contracts retain native cleanup, nested copies and resource lifetime coverage. |
| SQL text golden outputs, placeholder/version order, compiler field filtering | Independent dialect/mutation/query contracts and recorded Rust comparisons remain authoritative. The retired compiler's quoting/text is not a new production golden. |
| Unsigned SQLite overflow, compiler errors and arithmetic boundaries | New regressions require the typed provider binding error for UINT64_MAX. Existing binding, dialect/mutation/query and runtime version arithmetic contracts remain. Old exception classes are removed. |
| Purpose/audit validation and logging controls | Source-aligned purposed/audited runtime APIs and existing runtime query/mutation/governance tests remain authoritative. New regressions exercise purposed queries, audited commands and disabled logging. Extra validation rules specific to SqliteSession are retired. |
| Rejection of 21 query features; arbitrary 10,000-row cap; unknown write-field policy | Adapter-specific restrictions are removed, not imposed on the production path. Aggregates, relations, streaming, paging and raw/safe queries keep their accepted Rust contracts and tests. Field filtering, zero limits, pagination bounds, invalid recovery and version overflow follow the existing source-aligned compiler/runtime checks. |

The old SQLite test target name remains for CTest continuity; its source and
link dependencies now point to the production provider. No old implementation
is compiled. Old measurement logs are immutable historical evidence and can
still mention the removed names.

## Audit continuity

[The retirement ledger](retired_compatibility.json) copies the seven original
move records exactly. The M1a path ledger and original hashes remain unchanged.
`verify_module_paths.py` accepts only these specifically reviewed removed
adapter destinations and rejects retained copies or changes to any retained
canonical production body. The removed package CMake file was not a move-ledger
entry. Current Rust responsibility evidence locators are updated to the
production regression and this review; the 451 pinned source-test dispositions
and their accepted source responsibilities are unchanged.

The original isolation repair and module release checks are historical records
under `measurements/`; tag `0.0.2` retains the complete old code if inspection is
needed. Removal verification uses the current configured target graph, not
stale compatibility archives left in earlier build directories.

## Validation

Current results are recorded in
[the removal evidence](../measurements/maintenance/2026_10_10_sqlite_retirement/README.md).
Native integration, module consumers and targeted ASan/UBSan checks validate
this maintenance change. MIPS/QEMU is not repeated because no production
function, ABI, native provider, architecture or toolchain implementation changes.
No physical-router execution is claimed. Sanitizer leak scanning stays disabled.
