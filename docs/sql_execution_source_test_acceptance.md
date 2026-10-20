# SQL execution source-test acceptance

This document preserves the historical 5.0.5 acceptance checkpoints below.
The active baseline is 5.0.6. The subsequent
[complete diagnostic-source review](sql_diagnostics_source_review_5_0_6.md)
accepts diagnostic_execution.rs and diagnostic_stream.rs using exact reference
dependencies and fresh original calls. The subsequent
[complete execution-test review](sql_execution_tests_source_review_5_0_6.md)
qualifies all six query and twenty-one mutation tests on 5.0.6, including the
six mutation tests missing from the historical port. The subsequent
[ME01-ME20 review](sql_mutation_execution_source_review_5_0_6.md) accepts complete
mutation_execution.rs after repairing two reproduced provenance/cache gaps.
The subsequent [SX01-SX27/XT01-XT16 review](sql_executor_source_review_5_0_6.md)
qualifies complete executor.rs, its sixteen embedded tests and conditional
transport/stream/transaction interfaces on the active baseline. Provider and
runtime/governance/publication caller requirements remain independent.
The subsequent [DC01-DC31/root review](sql_compiler_source_review_5_0_6.md)
accepts complete dialect.rs and lib.rs on the active baseline, including all
thirty root tests' explicit SQLite dispositions and three dialect tests.

## Complete diagnostic and renderer source-test acceptance, 2026-10-09

The six query diagnostic and fifteen mutation diagnostic source-test files are
accepted after complete original body/helper/input/assertion comparison, fresh
unchanged Rust 21/21 and current native 369/369. All SQLite-applicable branches of
the renderer source-test group are accepted with fresh original 6/6 and explicit
source-name/backend exclusions. No native supplementary rejection check is
counted as positive parity for an excluded backend branch.

Current consumer compilation finishes 622 tasks before CTest; no-work refreshes
precede/follow native 369/369 in 80.91 seconds, and 418 dependency records are
nonempty. Both current sanitizer/MIPS selections pass 51/51, including all three
source ports. Leak scanning is disabled. Their test-only files add no production
public interface/default/trait or separate wire model to accept. Context-owned
future cancellation and owner-thread fixture storage are explicit adaptations.

See measurements/parity/2026-10-09-sql-diagnostic-source-review and
measurements/parity/2026-10-09-sql-renderer-source-review. These accept only the
complete source-test responsibilities in the SQLite scope. Runtime publication/
privacy, graph/transaction saving and complete Stage 6 remain independent gates.
Earlier pending paragraphs below retain historical review evidence.

## Current source-test review, 2026-10-09

The complete execution_tests.rs and mutation_execution_tests.rs bodies, all
fixture/helper inputs and every assertion were compared again with their C++
ports. Fresh unchanged Rust passes all six query and fifteen mutation diagnostic
contracts (21/21). No filename filter or inventory entry substitutes for that
execution. The native fixture preserves the real executor, separate transport,
dialect/schema and scoped pending work, including release-before-observer,
source error/outcome/count/order/privacy-intent and guarded readback assertions.
Source pending-future Drop maps to explicit cancellation of Context-owned work.

The complete log_rendering_tests.rs source review retains every SQLite branch
and all eight SQLite ambiguous/incomplete cases. The solely PostgreSQL repeated-
binding test and non-SQLite branches of mixed tests remain excluded under the
explicit SQLite-only scope. Supplementary native nonfinite/nested-comment and
unsupported-kind checks are boundaries, not positive excluded-backend parity.
Fresh unchanged original Rust passes 6/6 across its backend set.

Evidence is retained in measurements/parity/2026-10-09-sql-diagnostic-source-review
and measurements/parity/2026-10-09-sql-renderer-source-review. Current consumer
builds add both diagnostic ports and the renderer to the wider cross selection
(51 checks). Current whole 369/native/sanitizer/MIPS evidence and recorded
source-group acceptance remain pending. Production runtime privacy, graph/save
and complete Stage 6 remain independent gates.

## Complete executor/transport source acceptance, 2026-10-08

The executor.rs/mutation_execution.rs source group is accepted after complete
production review: executor lines 1–576 and 1197–1892 (excluding its embedded
test body), every public trait/impl/helper, all twelve original executor-test
responsibilities, and all 317 mutation-execution lines. The latest full native
suite passes 315/315 in 73.69 seconds after a complete consumer build; a subsequent
build reports no pending work. All 384 compiler dependency records are nonempty.

Fresh selected builds precede all fifteen executor/error checks on ASan/UBSan
(15/15, 16.00 seconds, leak scanning disabled) and OpenWrt MIPS/QEMU
(15/15, 11.37 seconds). Exact check names are recorded in the module acceptance.
Live native comparisons match transport 344/344, query 381/381, mutation
1,012/1,012, transaction mutation 1,012/1,012, stream 880/880, backend hook
44/44 and executor error 144/144. Current MIPS additionally matches live
transport, query, stream and error observations; the earlier backend/mutation
live comparisons remain their documented evidence. None copies source behavior
algorithms into a substitute oracle.

| Source responsibility | Accepted contract |
| --- | --- |
| Transport and repeated-fetch default | Separate fetch/write, lazy sequential template cloning and parameter replacement, ordered rows/duplicates, first-error stop, empty input without index access and typed native errors. |
| Executor construction/Clone | Explicit owned cache state with same-domain sharing; dialect, transport and schema remain independent scoped dependencies. Clone cannot extend User lifetime. |
| Descriptor and plan caches | Name lookup and complete descriptor identity; 256-entry FIFO shape bound; complete key normalization/matching, scalar/list/array-hook binding recollection and current comment/provenance. Arena bytes remain owned until close. |
| Query and intent | Pure cached/unclassified intent; lazy compilation/execution, first differing scalar partition parameter, metadata/threshold gates, complete unrecorded defaults and no eager debug SQL. |
| Mutation tree and guard | Separate ordinary/guarded writes, ordered nested batches and statement metadata, independent GeneratedValues/snapshot, exact readback count, same-statement guard and guarded reread predicate. |
| Observed execution | Invocation backend capture, local target/intent provenance, success disarm, ordered failure journal, exactly-once cancel/unwind diagnostics and release before observer. |
| Native and buffered streams | Conditional native-stream SPI; buffered transaction query without partition shortcut; source compile timing, chunk order, EOF/error distinctions and delivered-only counts. |
| Transaction adapter | Conditional associated native transport, shared caches, ordinary mutation reread, separate begin/commit/rollback errors, pending-work rejection and consuming terminals. |
| Error protocol | Compile/Transport/PersistedRecord, complete Debug/Display and absent source; transport payload Debug/copy capability remains conditional rather than requiring Clone. |

C++ allocator-aware copies and Context borrows replace owned source clones/Arc.
Owner-thread dispatch replaces source Send/Sync scheduling; native threads only
wake operations, and stop/join precedes arena teardown. Cross-domain transaction
retention rejects, optional concrete SPIs remain explicit, and noncopyable native
errors transfer completion ownership exactly once. These are memory/resource
adaptations; no executor, transport, provider or runtime layer is merged.

This closes the complete SQL executor/transport source contract. SQLite provider
and its native-resource/error/schema/ID integration still need their own Stage 3
acceptance. Runtime graph saving, governance, audit/privacy and final Stage 6
acceptance remain open. Earlier partial/pending text below records checkpoints.

## SQL root facade acceptance, 2026-10-08

The lib.rs root source group is accepted after reading its full export body and
reviewing all 29 root test dispositions. All 22 source exports resolve directly
through teaql::sql aliases; independent C++20 umbrella compilation verifies the
thirteen type/template aliases and nine helper names, including the Unicode
quote overload. The facade introduces no state, ownership, method defaults,
serialization or execution behavior. Underlying executor/provider contracts
remain separate and are not accepted merely because their aliases compile.

The applicable original root compiler/renderer contracts pass native, ASan/UBSan
with leak scanning disabled, and OpenWrt MIPS/QEMU after selected target builds.
The sanitizer target was initially absent; that not-run attempt is superseded
by a successful build and passing rerun. The latest full native 314/314 regression
includes the same root test target, and the build still reports no pending work.
The public-export verifier confirms all 22 SQL names against pinned Rust.

The detailed root review below preserves 26 generic/SQLite test inputs and full
literal SQL/binding/log assertions. Its synthetic backend/sigil adaptation and
three wholly native PostgreSQL/MySQL renderer exclusions follow the agreed
SQLite-only implementation scope. No actual non-SQLite service is added.
This closes the root source responsibility, not the complete SQL executor,
provider, Stage 3 or final Stage 6 acceptance.

## Diagnostic source-group acceptance, 2026-10-08

The diagnostic_execution/diagnostic_stream source group is accepted after full
91/89-line production review and final current native 314/314 in 74.66 seconds,
with a stable post-test no-work build and 314 registered tests. Current named
sanitizer/MIPS diagnostic/query/mutation/noncopyable selections pass 4/4; the
subsequent backend-capture consumer selections also pass 8/8. They cover
exactly-once delivery, success disarm, ordered journal drain, cursor release
before sinks, delivered-only counts, terminal/pending/unwind/cancel paths and
exception isolation. Source Arc/Mutex and owned Drop have explicit Context-bound
owner-thread/cancel/close adaptations. This accepts these private diagnostic
helpers, not the containing executor group, Stage 3, or runtime safe privacy.
Earlier pending paragraphs below retain the implementation/review history.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The unchanged Rust diagnostic test modules pass 21/21: six from
`teaql-sql/src/execution_tests.rs` and fifteen from
`teaql-sql/src/mutation_execution_tests.rs`. Their registered Rust module names
are `executor::diagnostic_tests` and `executor::mutation_diagnostic_tests`;
filtering on the source filenames runs zero tests and is not evidence.

## Diagnostic production-body review

The complete diagnostic_execution.rs body and all DiagnosticStream production
code before its test module are reviewed against the C++ guard, journal and
stream implementations. Statement metadata is taken once before notification;
success returns it through the existing result path. Failure/cancellation update
the terminal time/outcome and isolate each observer exception without logging
provider error text. Repeated terminal paths do not notify again.

FailureJournal allocates invocation-local entries only when an observer exists,
records in execution order, disarms successful fallback delivery, and drains
before invoking observers so a throwing sink cannot duplicate entries. The
source Arc/Mutex recorder becomes a Context-bound callback and Context-owned
vector in C++; it cannot acquire a hidden shared lifetime beyond User close.
Context-owner-thread invocation replaces the source's shared locking while the
async kernel joins native work before arena teardown. Recorders retained within
their owner remain borrows, not static C++ data.

DiagnosticStream releases the source cursor before notifying, counts only chunks
actually delivered, treats is_last and source end as Success, and preserves the
original error on Failure. Pending adds neither count nor event. Explicit cancel
or stack/Context teardown maps owned Rust Drop, with Failure during unwind and
Cancelled otherwise. Observer failure is isolated on terminal and teardown paths.
Existing direct source ports and noncopyable integration exercise these bodies;
current sanitizer/MIPS selected executables finish their eight-task builds
before the four named diagnostic/query/mutation/noncopyable checks run.
ASan/UBSan with leak scanning disabled passes 4/4 in 0.39 seconds, and
MIPS/QEMU passes 4/4 in 0.77 seconds. Complete native integration and diagnostic
source-group acceptance remain pending, separate from runtime safe-log privacy
acceptance.

## Executor-error Debug capability repair

A public-umbrella compile check reproduces a missing source derive: the original
SqlExecutorError has Debug, while the native wrapper supplied Display/source and
ownership but no structural writer. The error_debug.hpp header now formats all
three variants and delegates nested compile diagnostics or the transport
payload's Debug writer. An explicit element formatter supports arbitrary native
payloads without requiring Clone, exception inheritance or a default allocator.
Raw structural diagnostics are not a runtime safe-log projection.

The new actual-source oracle calls the unchanged pinned enum's Debug, Display
and Error::source. All 144 inputs agree across two source executions: four nested
compile shapes, custom transport and persisted-record variants; signed version
endpoints; empty, escaped, control, Unicode, long and private text; and nine
formatter modes. The native registered probe matches every complete observation.
Native ownership tests also exercise the default text payload writer and a
custom noncopyable payload formatter, verifying its active state and cleanup
count are unchanged. The generic error carrier retains its absent cause chain.

The complete current native suite now passes 315/315 after the new capability
and target. The fifteen-check sanitizer/MIPS selections pass, including the new
error comparison and generic noncopyable ownership checks. All 144 complete
error observations also match live original Rust on MIPS.

## Executor production review and backend capture repair

The complete production sections of the 1,892-line executor.rs (before and after
its embedded test module) and all 317 mutation_execution.rs lines are reviewed.
The review maps transport/stream/transaction associated roles, the three error
variants and empty default cause, explicit executor/cache sharing, all plan-key/
matcher/rebinding helpers, scalar partition probes, observed query/mutation paths,
guarded readback, native/buffered streams and consuming transaction terminals.
C++ Context-owned state replaces source Arc/RwLock ownership without extending
User lifetime; cache and operation/transport/provider responsibilities remain
separate. Reading and member presence alone do not establish acceptance.

A new mandatory kind() hook fixture invokes the actual original executor.
Forty-four cases observe getter counts, failure after two reads, transport calls,
metadata and observer events across ordinary/guarded/transaction/batch paths.
Two source executions agree, and all 1,012 earlier execution observations are
revalidated unchanged. The pre-repair native counterpart matches 19/44. A simple
observed insert reads kind() three times instead of the source's two; a later
getter failure can prevent a write that the original completes.

ObservedSqlTransport now owns its Context-allocated backend string and captures
it once, matching the original adapter responsibility. Guarded capture precedes
compilation; returned guarded metadata reads the backend only after successful
write/readback, while batch metadata reads after its children. This restores
source failure timing as well as successful metadata without a singleton or
shared owner. Native and MIPS now match all 44 live original observations.
Fresh two-check selections preserve the 1,012 prior cases and pass native 2/2
(0.81 seconds), ASan/UBSan with leak scanning disabled 2/2 (2.05 seconds), and
MIPS/QEMU 2/2 (1.16 seconds). The current MIPS composed consumer build then finishes all eleven tasks and
passes eight named checks in 4.56 seconds. This adds the original executor,
query/mutation/stream diagnostic and transport contracts plus noncopyable
integration to the two comparison checks. The corresponding sanitizer consumer build also finishes all eleven tasks and
passes the same eight checks in 5.93 seconds with ASan/UBSan and leak scanning
disabled. The subsequent complete native regression passes 314/314 in 74.66
seconds, with a post-test no-work build; it includes the repaired observed
adapter layout. Full executor/transport/provider and Stage 3 acceptance remain
open, independently from the accepted diagnostic and root source contracts.

## Query diagnostics

The C++ port retains Customer's masked name, the Riverside binding, limit ten,
bounded comment, disabled eager debug rendering and explicit metadata-capture
flag. Its expected precompiled input uses the original test dialect's always-
quoted identifiers and anonymous SQLite placeholders. Actual execution enters
the production query executor and SqlQueryOperation, whose compilation remains
separate from transport I/O; no local helper simulates executor outcomes.

All six contracts assert typed driver failure, observer exception isolation,
pending cancellation with unknown counts, provider unwind and resource release,
returned success metadata without duplicate fallback notification, and disabled
capture suppressing failure diagnostics while releasing the provider resource.
The transport lease closes before any observer receives metadata. Observer
resource-order violations are recorded independently of swallowed exceptions,
so observer exception isolation cannot make that check pass accidentally.

## Mutation diagnostics

All fifteen original contracts preserve their insert/update/delete/recover,
guard, batch and transaction paths:

- All four mutation variants retain a plain target-ID binding and independent
  inherited intent in both success and failure.
- Repeated cached query-intent compilation uses current masked values, retains
  one select-plan shape, and performs no transport reads/writes or log emission.
  Transaction lookup remains conservative for unknown metadata and rolls back.
- Direct driver failures keep their original typed error. Partial batches retain
  success-before-failure order even if the observer throws; cancellation preserves
  unknown affected count for the in-flight second statement.
- A successful write and failed/empty/cancelled readback remain distinct outcomes.
  Empty SELECT is SQL success with zero rows and a persisted-record contract
  failure. Cancelling readback cannot relabel the earlier successful write.
- Successful transaction batches return three statement metadata leaves without
  fallback observer notification. Later compilation failure and nested batch
  failure cannot invent or duplicate an executed statement.
- Guards stay in both mutation and authoritative reread SQL with the ACTIVE
  binding. Readback inherits Riverside redaction intent. Guarded transaction
  write failure has exactly one failed UPDATE outcome and unknown affected count.
- Each mutation variant retains its operation kind on failure.

## C++ lifetime adaptations

The fixture uses a real SqlDataServiceExecutor, independent schema/cache state,
SqlTransport and SQL/data-service transaction adapters. Only the driver is a
controlled test transport, as in the Rust originals. It injects failure, pending
and empty results without supplying executor logic or expected diagnostics.
All dynamic runtime data uses the invoking UserContext. No third-party library,
backend or generator changes are introduced.

Rust drops an owned future to cancel it. C++ operations are Context-owned borrows;
the corresponding boundary is explicit `cancel()` before asserting diagnostics.
The async kernel joins the native child before delivering diagnostics. Existing
native SQLite and Context-close tests separately cover actual native cleanup and
User destruction. Typed guarded access on an erased transaction uses
`TransactionHandle::get_as<GuardedMutationExecutor<Error>>()`, retaining the
optional independent capability rather than adding it to every transaction.

## Reproduction

On 2026-10-07 the complete rebuilt native Debug suite passed 189/189 checks.
The two new groups passed 2/2 under ASan/UBSan with leak detection and 2/2 in
the OpenWrt MIPS cross-build under QEMU. Pinned upstream, inventory and
clang-format 18.1.8 checks passed. No production implementation change was
needed for these source-test ports.

```sh
cmake --build build --target teaql_original_query_execution_tests \
  teaql_original_mutation_execution_tests
ctest --test-dir build --output-on-failure \
  -R '^original_sql_(query|mutation)_execution_contracts$'

CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-sql --lib diagnostic_tests --locked --offline
```

Full public API/member/default/serialization and privacy/log audits remain
required for the six-stage migration. A source-test port and passing regression
suite do not establish complete stage parity. Physical-router execution remains
unverified.

## Default capabilities, schema and diagnostic streams

Twelve more source tests have direct ports and individual reviewed dispositions.
The data-service request test retains both original Entity traces, all four
mutation variants, the exact final comment, Insert+Update batch and empty-trace
None behavior. The default provider-intent test uses a NoIo QueryExecutor and
retains all six original filter/subquery/credential/projection/Having/search
secrets under both permissions, excludes the unrelated relation sibling, and
checks secret-free private Debug output. It cannot execute query I/O.

Three generic dialect tests retain field-specific TypedNull(Timestamp), all four
invalid schema-shape fixtures and the eleven length/numeric coverage inputs.
Their source dummy reports PostgreSql with dollar placeholders; the C++ dummy
reports SQLite with anonymous placeholders. This is a test-helper adaptation
for the generic compiler assertions, not PostgreSQL implementation or positive
backend acceptance. Raw quoting and the dummy TEST schema type remain; no DDL
is executed. Structured InvalidSchemaShape and original field names are checked.

Seven cursor fixtures call the production DiagnosticStream wrapper directly.
The controlled source only supplies original two-row chunks, errors or Pending
and exposes cleanup/poll counters. Complete terminal checks require one event,
Some delivered count, exact outcome and absent debug_query. Cursor-release
ordering is recorded separately from the throwing observer, so swallowed sink
exceptions cannot hide an ordering failure. The last chunk closes after one
provider poll, buffered unpolled rows never count as delivered, and both original
driver error messages survive failure/sink exceptions.

Explicit cancel adapts owned Rust stream drop. A scoped consumer invokes that
same real cancellation path during stack unwinding, preserving Failure rather
than ordinary Cancelled. Each fixture actually destroys its User while its
borrowed observer target remains alive; retained observations are explicitly
copied into Global. The final aggregate live User byte charge is zero. There is
no replacement outcome/count/redaction algorithm in the port.

After rebuilding, both named checks pass 2/2 on native, ASan/UBSan with leak
detection and OpenWrt MIPS/QEMU. Unchanged Rust passes 2/2 data-service, 3/3
dialect and 7/7 diagnostic-stream tests. Source lock, inventory and 522-file
formatting checks pass. No production dependency or generator changed. This
selection follows the full native 246-check checkpoint; it does not claim a
new full 248-check or full-platform regression.

```sh
cmake --build build --target teaql_original_sql_default_tests \
  teaql_original_diagnostic_stream_tests
ctest --test-dir build --output-on-failure \
  -R '^original_sql_(default|diagnostic_stream)_contracts$'
```

## Executor cache and partition-probe source contracts

All twelve tests in `teaql-sql/src/executor.rs` have individually reviewed direct
ports in `tests/parity/teaql_sql/executor_contracts.cpp`. The original Order
descriptor, typed values and assertions are preserved. The ports invoke the
production executor, cache compiler and scalar partition rewrite; the controlled
transport only returns empty results and counts which boundary was called.

The descriptor cache is shared by an allocator-aware executor clone. Private
name bindings retain Masked policies on cache hits; switching descriptor privacy
forces compilation with Plain policies. Batch metadata keeps two statement
leaves with independently numbered bindings. Disabled capture retains all five
original empty metadata assertions, and requested diagnostics never eagerly
expand the private canary for query or insertion.

The original id IN [7,9], descending order, limit-one partition query uses one
repeated-probe boundary only when metadata capture and the configured threshold
permit it. Enabled metadata or an explicit zero threshold uses ordinary query
execution. The scalar rewrite retains tenant, visibility and business guards,
ordering and slice while replacing only partition membership.

Cache rebinding checks retain scalar values, distinct IN-list cardinalities and
all four original complex query pairs: search, projection/filter/Having/order,
partition/page/order and subquery. Cached SQL and complete binding order match
direct uncached production compilation. The source's synthetic array-dialect
hook is retained in a test-only subclass with both complete 21-element lists.
Its SQL reaches only the controlled transport; no native array backend is
implemented. The source dummy's PostgreSql kind and dollar placeholders adapt
to SQLite and numbered question placeholders, including numbering reset for
each batch statement. This is generic executor-hook acceptance.

Runtime/cache state has an explicit Global owner; request and result storage has
an explicit User owner. The default PMR resource is disabled throughout the port.
Original Rust passes 12/12. The named C++ contract passes after building on
native Debug, ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. No production
implementation, dependency or generator change was needed.

Full native integration at this checkpoint passes 249/249 CTest checks after
requesting the complete consumer build. Compiled-runtime layer validation,
pinned source/inventory checks and 523-file clang-format checks pass. The
sanitizer and MIPS executor results remain scoped evidence; full migration
acceptance and physical-router execution remain unverified.

```sh
cmake --build build --target teaql_original_sql_executor_tests
ctest --test-dir build --output-on-failure \
  -R '^original_sql_executor_contracts$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-sql --lib executor::tests --locked --offline
```

## Per-test disposition review

The shared SQL renderer now has five reviewed SQLite-applicable dispositions.
The entirely PostgreSQL repeated-binding/dollar-quote test is excluded by name.
For mixed tests, only their MySQL/PostgreSQL branches are excluded; the original
SQLite branches remain covered. These include exact quoted/debug/projected SQL,
Unicode and bracket identifiers, all eight invalid SQLite binding inputs,
unchanged projection errors, private SQL omission and the NULL literal. Additional
C++ checks reject unsupported database kinds; this does not claim their positive
dialect behavior. The runtime's separate PostgreSQL array case is also excluded.

On 2026-10-07 unchanged Rust passed 6/6 renderer tests. The renderer and runtime
masking selection passed 2/2 after rebuilding on native, ASan/UBSan with leak
detection and MIPS/QEMU. See the runtime acceptance note for the paired
reproduction commands. SQLite remains the only production database dependency.

On 2026-10-07 all six query and fifteen mutation diagnostic source tests were
reviewed individually against their concrete C++ functions and shared transport
fixture. Each record now identifies the corresponding helper, original inputs
and outcome/error/order/resource assertions in source_test_dispositions.json.
The unchanged original registered Rust modules again pass 21/21; the two scoped
C++ test groups pass on native, ASan/UBSan with leak detection and MIPS/QEMU.
Rust future-drop cancellation remains the explicit C++ cancel boundary.
No production change was needed and no unreviewed module is marked complete.


## Root SQL compiler and renderer contracts

All 29 indexed tests in `teaql-sql/src/lib.rs` have been individually reviewed.
The direct C++ port in `tests/parity/teaql_sql/compiler.cpp` retains 26 applicable
tests. The three wholly native-backend renderer tests below are excluded by
explicit source name; generic tests are not excluded merely because an input,
variable name or test name mentions another backend.

The original TestDialect is a synthetic compiler adapter with always-quoted
identifiers, numbered PostgreSQL placeholders, and a GBK-function override.
The C++ fixture keeps the generic adapter and its GBK override, adapting only
its reported backend to SQLite and numbered placeholders from `$n` to `?n`.
Literal expected SQL comes from the source assertions. The helper changes the
sigil in expected strings only; it does not derive expectations from compiler
output or normalize away quoting, aliases, predicates, ordering or projections.
The two whole-CompiledQuery comparisons preserve every default log field,
absent comment, Text binding, Plain parameter policy and generated-SQL flag;
only the synthetic backend/sigil fields use their SQLite counterparts.

The port retains these source responsibilities and assertions:

- Identifier quoting, deterministic bounded names, full ordered index specs
  and DDL, byte-based table/column rejection, and generic foreign-key bounds.
- Filter/order/pagination SQL and typed bindings, partition window rank and
  projection deduplication, aggregate-only projections, grouped predicates,
  all six string-match builders, text search, and the empty schema-setup default.
- All four mutations and guarded update/soft-delete/hard-delete/recovery,
  including every original same-statement tenant predicate and typed binding.
- Explicit empty/large IN forms, property-to-property filters, raw projections,
  dynamic properties, comments, raw SQL overrides, subquery binding order,
  expression/extended aggregate/HAVING compilation and the dummy GBK hook.
- Both original SQLite debug-renderer tests, including embedded quote escaping,
  untouched quoted question marks, complete line/block/purpose comments,
  leap-date storage text and millisecond timestamp storage.

The source test named
`deduplicates_partition_projection_for_mysql_derived_tables` uses the generic
TestDialect and remains covered. The raw `payload @>` fragment remains an opaque
pass-through assertion; the dummy `convert_to` override and extended aggregates
exercise abstract compiler responsibilities. These tests never execute such SQL
against SQLite and do not claim native backend/function support.

| Original source test | Scope decision |
| --- | --- |
| `renders_postgres_debug_sql_with_inlined_params` | PostgreSQL-only numbered/ANY-array renderer, no SQLite branch. |
| `debug_sql_renders_copy_paste_statement_with_shared_semantics` | Entire test selects PostgreSQL and its TRUE/repeated-numbered binding syntax despite the generic name. |
| `postgres_and_mysql_debug_sql_use_typed_temporal_literals` | Both branches are native PostgreSQL/MySQL typed temporal renderers, including the negative timestamp. |

The supplemental C++ boundary check rejects unsupported native render kinds.
It is negative evidence only, not an implementation of the excluded tests.
Their original Rust bodies still pass; the pinned reference is unchanged.

On 2026-10-07 the original SQL library passes 88/88 tests, including all 29 root
names verified against the disposition ledger. The direct C++ compiler check
passes 1/1 after rebuilding on native Debug, ASan/UBSan with leak detection,
and OpenWrt MIPS/QEMU. The latest complete native regression passed 259/259
before the test-only unexpected-error handling refinement; the refined compiler
fixture was then rebuilt and passed in all three configurations. Scoped SQL
compile errors are caught while the fixture's allocator is still alive.
No production compiler, generator or dependency changed.

```sh
cmake --build build --target teaql_original_sql_compiler_tests
ctest --test-dir build --output-on-failure \
  -R '^original_sql_compiler_contracts$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-sql --lib --locked --offline
```

Use `build-sanitize` with leak detection and UBSan halt-on-error, or
`build-wndr4300-memory` with its registered QEMU emulator, for the selected
C++ check. Complete source/API/default/privacy/lifecycle acceptance remains
separate; the source ledger is now 295 covered, 146 pending, five backend and
five external exclusions. The strict completion gate remains open.
