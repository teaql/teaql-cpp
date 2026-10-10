# Runtime root source acceptance review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Accepted on 2026-10-10 after complete production/fixture/assertion review,
public module repair and final native/sanitizer/MIPS integration. This closes
the runtime-root source group within the documented ownership and supported
protocol contract; the full Stage 6 requirement audit is recorded separately.

## Complete production boundary

The original lib.rs production body contains crate/module declarations, public
and private reexports at lines 1–117, and checker reexports at lines 4292–4297
after the test module. It contains no additional production algorithm, object
field, serialization implementation or trait default. Review therefore includes
both regions, the generated_support wildcard and their implementing leaf modules.
All 54 implementing source groups are now accepted within explicit C++ Context/
owner-thread and supported-protocol contracts. Export names are only locators;
each leaf acceptance records complete bodies, members/defaults/traits/protocols,
ownership and composition evidence.

The runtime umbrella exposes the complete supported source export set, including
business IDs, Context/schema/data store/cursors/logs/resources, entity service/
relations/cache, both reference protocols, immutable identity graph and mutable
ledger, audited graph saving, entity status/errors, raw/safe events, generated
support, graph models, language/catalog, internal IDs, in-memory query engine,
mutation governance/approval, registry/module and telemetry. The late checker
block retains status markers, rules/results, registries, object locations,
naming/wire models, typed checker and mark/clear helpers. Private local ID and
mutation-governance exports retain their roles in their accepted implementation
modules, with explicit Context dispatch replacing process/task-local ownership.
The optional concrete OpenTelemetry adapter remains excluded with its source
group; the standard telemetry SPI and fail-open/Noop capabilities remain supported.

Core annotated input graphs and runtime identity graphs remain separate namespace
exports. Runtime SchemaProvider and data-service schema SPI remain distinct.
Umbrellas compile independently, together and in both include orders. The root
adds no facade default, public data layout, serde/Hash wire protocol, provider,
singleton or Context owner. Model diagnostics and safe publication stay separate.
C++ field/builder/keyword renamings and allocator/context parameters are the
already documented ownership/API adaptations rather than deleted responsibilities.

## All 75 original root contracts

Each original test has an individually reviewed source line, input/assertion
contract, direct C++ evidence and named check in source_test_dispositions.json.
The complete records are retained in the final-review measurement directory.

| Direct C++ check | Original tests | Reviewed responsibilities |
| --- | --- | --- |
| original_runtime_registration_contracts | 8 | Physical-column/relation collision, detached reverse relation, metadata/module/typed behavior registration, macro expansion and resource/local indexes |
| original_runtime_service_boundary_contracts | 8 | Optimistic error type, Context telemetry/service dispatch, entity binding, behavior-before-policy, all public query rejection routes, exact ID/version/resources |
| original_runtime_validation_contracts | 15 | Generic/typed/fixing checkers, required versus omitted/null, sparse/nested payload paths, loaded versions/composition rejection and fifteen-language/custom catalog |
| original_runtime_events_relations_contracts | 8 | Four mutation event kinds/raw-safe snapshots, FK dependencies/cycles, relation plans/identity/hydration/telemetry and TOPN order/rank/query count |
| original_runtime_typed_query_contracts | 6 | Scalar codecs, inverse List relations, identity-graph getters, local copy-on-write and restored identity/version projection |
| original_runtime_aggregate_cache_contracts | 6 | Dynamic typed values/flattened JSON, U64 missing-count default, physical/logical FK mapping, cache namespace/invalidation and child propagation |
| original_runtime_pagination_contracts | 10 | Cursor seeking, ordered IDs/exact counts, limit/store/shape fallback, miss coalescing, real TTL, principal isolation, empty and disappearing entities |
| original_runtime_memory_service_contracts | 9 | Exact private test-oracle U64/Decimal aggregates, filters, grouping/having, Soundex, property comparisons, mutations and optimistic versions |
| original_runtime_context_tail_contracts | 4 | Missing schema, user identity/options, local lease/ownership and remote provider delegation |
| original_runtime_optimistic_conflict_output_contracts | 1 | Isolated-process absence of unintended optimistic-conflict output |

Private Stub/Queue/capturing/ID-set providers supply only controlled SPI results;
actual kernel components perform policy, projection, relation, aggregate, cache,
pagination, ledger and event behavior. The source synthetic PostgreSQL dialect
is adapted to SQLite numbered placeholders, with no PostgreSQL provider. Handwritten
models retain source metadata/scalar/relation/root attachment roles while the
entity generator stays untouched. The cfg(test)-only memory service remains a
reference helper, separate from the public InMemoryQueryEngine.

Fixture review covers exact Value variants, empty/absent states, counts, paths,
call ordering, structured error kinds and messages, principal/namespace separation,
clone/mutation boundaries and complete model/JSON outputs. C++ source fixtures
use explicit User/Global ownership. The real 25ms ID-query wake fixture shares
its counter/store through Global, forwards completion, and joins its native
thread before Context cleanup. Shared-provider ownership has explicit destination
borrows. Raw/safe event fixtures remain independent. Native thread IDs replace
Rust process-local ThreadId, within the documented owner-thread adaptation.
The TOPN fixture has two nonempty parents; its name does not prove an empty case.

The unchanged original runtime unit suite passes 224 tests, including all 75
root tests; one ignored file_log_child entry is the subprocess fixture invoked
by its parent privacy gate. This is source evidence rather than C++ parity by
itself. The complete native 388-check snapshot contains every direct root check and
the published-module namespace contract.
Final complete native, sanitizer and MIPS suites include every direct root
check. Live semantic examples match on all three profiles.

## Final independent validation

Final code-state native/sanitizer/MIPS integration and matched-engine SQLite
diagnostics/numeric/provider behavior pass. All supported source tests, complete
root composition and runnable handwritten examples are reconciled in the final
requirement audit.
The original example oracle invokes actual locked SQLite runtime APIs. Comparisons
retain complete result rows and ordered SQL audit/purpose provenance. Physical
WNDR4300 execution is explicitly deferred; current target evidence uses the
OpenWrt musl MIPS 74K profile and QEMU. The entity generator, dependency scope and
external exclusions remain unchanged.


## Published module namespace correction

The complete root review finds four public module namespaces absent from C++,
although their root-level members already exist: entity_save, generated_support,
inmemory_engine and log_formatter. A concrete public umbrella compilation fails
for all four qualified paths; before.cpp/log evidence is retained in the final
measurement directory. This was outside the earlier verifier's root-name check.

The runtime umbrella now preserves all four namespaces with their complete
supported public member bindings. The formatter module includes its SqlLogEntry/
SqlLogOperation reexports and all seven formatter/config/manager types. Existing
implementations, object layouts, defaults and lifetimes are unchanged; no new
service or dependency is introduced. The verifier now scans public runtime
modules and their public declarations/reexports individually. A compiled namespace
contract requires exact type/function identity and dispatches the actual human
formatter through its published namespace. All three profiles pass the named contract. Complete native/sanitizer/MIPS
consumers are rebuilt before final acceptance.


The complete namespace consumer refresh finishes all 44 tasks. Current native
CTest passes 388/388 in 187.25 seconds with four workers while other profile
compilers run; all 427 compiled object dependency records are nonempty and
pre/post-test builds report no work. This includes the namespace contract and
all ten direct root groups covering 75 original tests. Native semantic examples
and their SQL audit/purpose provenance match live original Rust. Complete sanitizer and matched-engine MIPS validation also pass as recorded
below.

## Complete matched-engine target validation

The final MIPS build completes all 351 tasks with exit zero. QEMU passes the
complete applicable suite, 387/387 in 173.24 seconds; live original examples
match 2/2. All 427 object dependency records are nonempty, the compiled kernel
layers remain independent and pre/post builds report no work. One raw FromSql
Debug comparison is registered only on native 64-bit by the existing CMake
gate; structured FromSql errors and provider contracts remain covered on MIPS.
The registration difference and complete logs are recorded in the final-review
directory. Physical-router execution remains deferred.

## Complete sanitizer validation and acceptance

All 411 final sanitizer build tasks complete with exit zero. ASan/UBSan passes
388/388 in 171.81 seconds, including every root/namespace/composition contract.
Live original examples match 2/2. All 427 dependency records are nonempty,
compiled layer checks pass and pre/post builds report no work. Leak scanning is
disabled (`ASAN_OPTIONS=detect_leaks=0`); UBSan halts on any error. This is not
proof against arbitrary later dereferencing of escaped native pointers.

All three profiles preserve the same 1,266-file production/example/test snapshot.
Combined with complete leaf reviews, both root production regions, module
bindings and all 75 original fixture/assertion contracts, this accepts the root
group. Source-test dispositions total 451: 441 covered in scope, five explicitly
non-SQLite backend cases and five external cases. Generator changes, external
concrete adapters, physical-device execution and GPU adapters remain outside
this accepted scope. See the [final evidence](../measurements/parity/2026-10-10-stage6-final-review/README.md).
