# Complete runtime-root review (Rust 5.0.6)

Status: **pending current integration qualification**. This review concerns
`teaql-runtime/src/lib.rs` at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
SHA-256 `8afefa70e4dfc9b2564038f00b4f2f66aad91f59c32ef27a6fcb9f25fc6242e8`.
The file has 4,513 lines. Historical 5.0.5 acceptance is retained separately;
neither that acceptance nor a refreshed registration ledger accepts this source
against the current baseline automatically.

## Production and fixture boundaries

The production regions are lines 1–129 and 4508–4513. They declare modules and
reexport their members; the later region reexports Checker types and status
helpers. There is no additional production object layout, field default,
serialization implementation or execution algorithm in the root itself. Public
exports, private exports and the `generated_support` wildcard all belong to this
review. The implementing leaves retain independent source dispositions.

The C++ umbrella preserves the supported root types and functions, with five
public module namespaces: `entity_save`, `generated_support`, `inmemory_engine`,
`log_formatter` and `dynamic_fields`. They refer to existing implementations;
they do not introduce another Context, service, executor or provider. The runtime
module facade and isolated lookup catalog retain these names through imports.
The new 5.0.6 shared read-metadata decoder and `DynamicFieldChanges` remain
distinct exports. The private ID and mutation-governance bindings retain their
implementation responsibilities without becoming application-owned globals.
The optional concrete OpenTelemetry implementation keeps its external exclusion;
its runtime SPI remains in scope.

Lines 130–4506 contain the test module. Its `fixture_select` supplies explicit
comment/purpose for trusted test queries. The model descriptors, typed entity
fixtures, fixed/sequential ID generators, Checker implementations, queue/capture
executors, policy hooks, audit/telemetry sinks and lock fixtures support the
assertions below; they are not additional production providers. The synthetic
PostgreSQL dialect remains a fixture: the C++ ports use SQLite placeholders and
retain the reviewed SQL/metadata assertions without adding a PostgreSQL provider.
The imported `cfg(test)` memory service is separately
[accepted at 5.0.6](memory_source_acceptance_5_0_6.md), rather than reimplemented
inside the root fixtures.

## All original root assertions

The [current locator ledger](parity/v5_0_6/runtime_root_contracts.json) maps all
75 unchanged test identities to current source bodies, called C++ functions and
registered native/imported checks. The
[upgrade review](parity/runtime_roots_5_0_6_source_review.json) retains the exact
45 changed bodies and 30 unchanged bodies, including intent setup changes.
Historical per-test assertion reviews remain attached to the current mappings.

| Direct native selection | Original tests | Responsibility retained |
| --- | ---: | --- |
| `original_runtime_registration_contracts` | 8 | Column/relation identity, detached relations, descriptors, typed behavior/module registration and Context resource/local indexing |
| `original_runtime_service_boundary_contracts` | 8 | Optimistic errors, service/telemetry resolution, entity binding, behavior-before-policy, public rejection routes and ID/version preparation |
| `original_runtime_validation_contracts` | 15 | Generic/typed Checker and Fix, required/omitted/null boundaries, sparse/nested locations, loaded versions, composition and language handling |
| `original_runtime_events_relations_contracts` | 8 | Mutation events, ledger foreign-key ordering/cycles, relation plans/queries/enhancement and stable, empty-safe, count-free top-N loading |
| `original_runtime_typed_query_contracts` | 6 | Inverse-many list shape, runtime identity-graph access, mutation copy-on-write, typed results and ID/version restoration in reduced projections |
| `original_runtime_aggregate_cache_contracts` | 6 | Dynamic aggregate properties, relation aggregate storage/property keys, registered cache use, namespaces/write invalidation and relation-cache propagation |
| `original_runtime_pagination_contracts` | 10 | ID-set and continuous pagination, retained request state, expiration, principal boundaries and concurrency |
| `original_runtime_memory_service_contracts` | 9 | The separate private memory service's typed selection, relation/scalar/grouped aggregation, filters, comparisons and optimistic mutations |
| `original_runtime_optimistic_conflict_output_contracts` | 1 | The isolated-process optimistic-conflict output contract, also checked independently of its shared fixture executable |
| `original_runtime_context_tail_contracts` | 4 | Missing schema-provider errors, Context user identity and local/remote lock SPI ownership, timeout, lease and release behavior |

The [retained original execution](parity/runtime_roots_5_0_6_source_execution.json)
passes exactly 75/75 root tests, with no source rewrite or private access bridge.
Its source hash is identical to the current locked file. The separate
[complete original library receipt](parity/runtime_library_original_5_0_6.json.gz)
passes 272 tests with one original ignored subprocess fixture; that child is
exercised by its parent privacy test. These scopes must remain distinct: neither
272 nor the number of CTest entries is the number of root responsibilities.

## Current consumer qualification

A new root-consumer gate checks the ten registered imported selections and the
published namespace contract, then applies three rejection controls: an old
baseline, a changed C++ body hash and an omitted registration. It binds the same
11,412 execution inputs and locked references as the graph-model native epoch.

The [initial setup receipt](parity/runtime_root_source_current_module_closure_5_0_6.json.gz)
records a controller failure before compilation or test execution. Nine selected
CTest entries had no `command` field because their executables were not yet
built in the current module directory. This is not a passing test result or a
runtime semantic failure. The corrected controller derives targets from the
committed canonical contract catalog, builds them first, and only then checks
each exact registered command and its terminal result. It retains the initial
failure and writes a separate receipt.

The [corrected current gate](parity/runtime_root_source_current_module_built_closure_5_0_6.json.gz)
passes **11/11** imported CTests with no failures/errors/skips. All 75 current
source/fixture records verify, and each of the three altered inputs is actually
rejected. Terminal build success, exact registered command paths, complete JUnit
and no-work checks before/after execution are retained. Canonical verification
passes 1,163 consumer/fixture bodies; the export catalog verifies 1,524 names and
their isolated lookup coverage across seven modules. Catalog checks establish
declaration coverage; the called fixtures establish their stated behavior.

The corrected controller changes only build/command discovery order. It does not
change a C++ assertion, production implementation, generated file or locked
source. Its owned controller/JUnit/negative-input directory (206,844 bytes) was
removed after all child processes exited. The initial setup directory (4,411
bytes) was also removed; its compressed failure evidence remains.

The frozen native epoch recorded by the graph-model continuation finished at
**876/877**, with the obsolete stream comparator as its only failure. Its
[repair](protected_stream_shapes_5_0_6.md) is qualified separately and does not
relabel the full epoch. No pending source is promoted by this review. A
successful full native result, current imported execution, exact input
and source hashes, and source-specific acceptance reconciliation remain required.
Source totals remain **118 accepted / 4 pending_review / 2 excluded_external**;
the six partial capabilities and P0–P7 remain open.
