# Provider and SQL source-test acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
This checkpoint migrates named original test contracts into executable C++
acceptance tests. The original reference remains unchanged. It does not replace
individual public API, serialization, privacy or complete-stage acceptance.

## Complete binding and permutation source-test acceptance, 2026-10-09

The complete SQL binding-policy (ten tests) and runtime Business ID permutation
(four tests) source-file groups are accepted after direct review of every
original body, helper input and assertion against the C++ ports. Fresh unchanged
Rust passes 10/10 and 4/4. Current complete native 353/353 passes in 88.45 seconds
with post-test no-work and 416 nonempty compiler dependency records. Fresh
source/ID selections pass 4/4 under ASan/UBSan (77.73 seconds, leak scanning
disabled) and MIPS/QEMU (21.73 seconds), after successful builds.

All 64 compiler workers, every exact policy vector, all ten byte-identical golden
vectors, and both encodings of every one of 20,000 sequences remain intact.
These test-only files add no public production API or separate serialized
model. Their production compiler/privacy and ID/clock/service source contracts
remain independent. The evidence is retained under
measurements/parity/2026-10-09-sql-binding-source-review and
measurements/parity/2026-10-09-business-permutation-source-review. Complete
Stage 6 and physical-router execution remain unverified.

## SQLite provider tests

The original `teaql-provider-sqlite/tests/dynamic_search.rs` test uses physical
schema setup through UserContext, two platform tenants and six school rows.
The C++ port retains every fixture, the trusted field/order binding callbacks,
four drift warnings, outer and related tenant filters, order, limit and purpose
provenance. Actual executor/provider execution must return IDs `[3, 2]`; the
wrong outer tenant, wrong related tenant and other name stay excluded. Structured
warnings omit ignored input values. It uses no locally emulated query execution.

The original `guarded_mutation.rs` test retains three rows and all nine guarded
actions: denied and successful update, soft delete, recover and hard delete,
plus a denied transaction update followed by rollback. Final tenant, version,
name and physical deletion assertions use actual SQLite rereads.

An additional C++ contract changes the tenant at the transport write boundary.
The guarded update must affect zero rows, preserve version/name and return no
foreign persisted snapshot. A pre-read authorization check alone cannot pass
this contract; the guard must be present in the mutation statement itself.

## Transaction capabilities and trace ownership

Rust's associated concrete SQL transaction preserves GuardedMutationExecutor and
streaming interfaces. The C++ generic TransactionHandle retains a Context borrow
but its `get()` returns the base query/mutation/terminal interface. Callers can
now explicitly recover an implemented interface with `get_as<Capability>()`.
Dynamic capability lookup rejects unsupported interfaces rather than adding a
guard method to every generic transaction or emulating a guard with a read.
The SQL interface still performs active/domain checks and tracks pending work;
commit, rollback and native cleanup remain separate responsibilities.

The provider port executes guarded mutation through this handle capability.
The existing executor SPI fixture also verifies that an ordinary transaction
rejects guarded access without being consumed. The interface recovery adds no
owner, heap allocation or lifetime extension. Retrieved native references remain
temporary Context borrows, as with `get()`.

The same port exposed missing TraceNode move assignment through a standard PMR
vector. TraceNode now supports allocator-aware moving and assignment preserving
its destination domain. A native contract assigns long User strings into a
Global trace, verifies both string allocators, actually deletes the User, and
reads the retained kind/entity/id/comment through a new User. Standard trace
vector move assignment is also instantiated. No trace field or enum is removed.

## Binding privacy tests

`teaql-sql/src/binding_tests.rs` has ten original tests. The C++ port preserves
their descriptors, logical/physical name difference, canaries, expected policy
sequences and 64 independent compiler threads. It covers legacy undeclared
policy, explicitly empty policy, masked/plain/credential precedence, scalar
function/BETWEEN/IN propagation, sibling isolation, subquery descriptor scope,
per-property search-text classification, unknown projection literals, whole-
statement raw SQL taint, guarded and batch mutation policies, and exception
scope restoration. Parameters remain unchanged and have one policy per value.
Each worker owns its own arena; the shared dialect is stateless. No Context
data is owned by thread-local storage or a hidden shared compiler cache.

## SQLite renderer acceptance and excluded backend cases

The source renderer test file contains six original tests spanning all three
DatabaseKind discriminators. The selected migration supplies SQLite only.
The C++ source-test port validates SQLite's shared debug/projected rendering,
quoted text/backticks/brackets/Unicode/comments, strict binding accounting,
numbered/named binding rejection, unterminated input, unchanged parameters,
callback failure propagation and omission of failed private SQL. Null and
nonfinite scalar/nested values are tested against the SQLite renderer.

The PostgreSQL-only repeated-numbered/dollar-quote/array cases and MySQL-only
backslash/hash/executable-comment behavior are outside the selected backend
scope. They are not counted as successful C++ semantic comparisons. The enum
discriminators and renderer interfaces are retained, and C++ explicitly rejects
these unsupported kinds. Running all six original Rust tests establishes the
reference checkpoint, not a claim of PostgreSQL/MySQL support in this project.

## Runtime business-clock and permutation tests

The complete source-only `teaql-runtime/tests/business_clock.rs` body was reviewed
again against its handwritten port on 2026-10-08. Both independent tests retain
the exact inputs/assertions described below, allocate the clock/override through
UserContext and run with default PMR disabled. Its source-test dispositions are
complete. This prepares the test-file group for acceptance after fresh integrated
verification; it does not accept the production clock/business-service/runtime
API group. No extra clock responsibility is inferred from these two tests.
The fresh unchanged-original Rust integration run passes 2/2 using the local
Cargo target. A newly compiled native C++ port passes both contracts with
default PMR disabled. The named CTest check also passes on native (0.01 seconds),
fresh ASan/UBSan with leak detection (0.03 seconds) and MIPS/QEMU (0.05 seconds).
The native
standalone compile links the current production archives and does not mutate
the active CMake consumer build or substitute local clock behavior.
This accepts the entire two-test source-file responsibility group. It contains
no additional public models, trait defaults or serialized protocol to implement;
the imported production runtime clock APIs remain in their separate partial
module. Source-test acceptance does not establish full-stage or physical-router
acceptance.

Two original business-clock tests use the exact leap-date fixture
`2032-02-29 10:15:30 UTC`. The second installs a `2040-01-02` BusinessDate
override and requires the original time to remain unchanged. Their C++ port
allocates the clock and override in UserContext and retains the original
independent time/date responsibilities.

Four original business-ID permutation tests consume all ten unchanged
cross-language CSV vectors, check the complete retained range of 20,000
sequences twice with reconstructed scope/key inputs, verify six-character
canonical uppercase/digit codes and uniqueness, reject domain exhaustion,
version zero and a blank scope namespace, and reject an invalid allocation
range before consuming a sequence. The C++ port copies the original CSV fixture
byte for byte into the repository. It does not reduce the retained range or
replace original observations with newly generated C++ expectations.

## Validation evidence

After rebuilding all native consumers, the complete Debug suite passed 184/184
checks. The six provider/SQL/trace/executor checks below passed under ASan/UBSan
with leak detection and OpenWrt MIPS/QEMU. The two runtime business-clock and
permutation checks also passed in both configurations, retaining the complete
20,000-sequence range. Physical-router execution remains unverified.

Unchanged original Rust tests passed at this checkpoint: one provider dynamic
search, one provider guarded mutation, ten SQL binding-policy tests and six
shared renderer tests, two business-clock tests and four permutation tests.
C++ acceptance is registered separately in CTest so the
selected backend scope is visible:

```sh
ctest --test-dir build --output-on-failure \
  -R '^(original_provider_dynamic_search_contracts|original_provider_guarded_mutation_contracts|original_sql_binding_policy_contracts|original_sql_log_rendering_contracts|core_model_contracts|executor_spi_contracts)$'
```

Relevant source ports live under `tests/parity/teaql-provider-sqlite/` and
`tests/parity/teaql-sql/`. The shared fixture keeps runtime metadata, SQL executor,
transport, provider and connection objects separate, with explicit User ownership
and actual resource cleanup before arena release. Generator development,
external adapters, other original source-test groups, and complete privacy/API
audits remain separate migration gates.

## Binding-policy disposition review

All ten original binding_tests.rs contracts were reviewed individually on
2026-10-07. The ledger records each C++ helper and exact original policy vector,
raw-fragment variant, input canary and 64-worker scope check. Original Rust
binding_tests again passes 10/10. The scoped C++ binding-policy check passes on
native, ASan/UBSan with leak detection and MIPS/QEMU. Every compiler result
retains one policy per unchanged input parameter. This per-test review does not
claim complete SQL API or privacy acceptance.

## Search, tenant and business-service disposition review

On 2026-10-07 the eight original core dynamic-search tests, both original
SQLite provider integration tests, two business-clock tests and four business-ID
permutation tests received individual covered_in_scope records after review of
the original fixtures and assertions. The separate canary subprocess retains
its own privacy check and explicit MIPS emulator forwarding. The real provider
keeps all original outer/related tenant predicates, four warnings, exact row
order, limit/purpose provenance and all nine guarded actions plus rollback.

SQLite result assertions now compare the original I64 values for row IDs,
tenant/version and count, instead of permitting cross-variant integer
conversion. The existing transport-boundary tenant-change contract still checks
zero writes, unchanged data and no foreign persisted snapshot. Production code
is unchanged. Rebuilt native, ASan/UBSan with leak detection and MIPS/QEMU each
pass all four core/provider checks; original Rust passes eight core and two
provider tests.

The clock and permutation selection was rebuilt and passes both checks in each
configuration. Original Rust passes two clock and four permutation tests. The
CSV remains byte-identical to the reference; all ten golden rows and the full
20000-sequence range with reconstructed inputs are retained. Fixture parsing
now requires exactly ten columns and complete hexadecimal byte consumption.
Controlled missing/extra-column CSV inputs are rejected at the exact-column
check. No source, generator or production dependency was changed.

The last complete native regression is the preceding 240-check checkpoint;
these subsequent assertion repairs have scoped acceptance, not another claimed
complete regression. Stage/API/privacy completion remains separate.


## SQLite storage source-test contracts

Fifteen root tests in `teaql-provider-sqlite/src/lib.rs` now have individually
reviewed direct ports in `tests/parity/teaql_provider_sqlite/storage.cpp`.
The shared fixture owns real SQLite connections/provider state in UserContext;
its convenience helpers only execute fixture SQL and submit compiled commands.
Default PMR allocation is disabled. No local code reproduces provider outcomes.

The ports retain every original input and assertion for this subset:

- Two empty schema checks followed by exact SOUNDEX Robert/Rupert/NULL results.
- Ordered five-row native streaming with complete 2/2/1 chunk sizes, early
  cancellation after two rows, and successful synchronous count after release.
- Decimal 123.450 text binding with scale and actual numeric CAST comparison.
- Leap-date/millisecond timestamp prepared versus executable debug-SQL storage,
  including the original replacement of inserted identity and SQLite type check.
- Complete mutation/table DDL, native Boolean result decoding and a single layout
  cache entry across different purpose comments on identical parameterized SQL.
- Identical text/JSON content decoded into distinct original Value variants.
- Order ID sequence 1/2 and original overflow table/MAX input with every safe,
  actionable diagnostic substring and no connection URL.
- Buffered streams with all 25-row chunk sizes/indices/flags/endpoint names,
  empty-result final chunk and exact 20-row boundary's empty final chunk.
- All five original general timestamp inputs; direct fast-parser exact epoch
  results for three offset forms and absent results for invalid month/offset;
  all four declared-text inputs and six exact declared/absent decode kinds.

C++ uses the existing controlled `UserContext::ensure_schema` boundary for the
empty schema test. It does not add an unrestricted provider schema entry.
Explicit stream cancellation adapts Rust's drop to Context-owned native work.
The original fixture's execute_batch CREATE/INSERT becomes two actual provider
statements. Result-column aliases adapt positional rusqlite reads to CompactRow
access without changing expression values or storage.

The existing timestamp fast-path body is unchanged and now named
`detail::parse_fixed_sqlite_timestamp` in the internal decoding header. Both
the real general decoder and direct private-contract tests call it. This avoids
substituting a different general parser for the source's private fast-path
assertions, and does not add a public root export or a dependency.

On 2026-10-07 the unchanged original provider library passes 42/42 tests, with
all root names matched to the pinned disposition ledger. The fifteen-contract
C++ group passes its registered native Debug check. The remaining 27 provider
source tests still require individual review; passing the original module or
existing broad integration tests alone does not close their dispositions.


After rebuilding, the storage group and existing decoding/binding ownership
checks pass 3/3 under ASan/UBSan with leak detection and 3/3 under OpenWrt
MIPS/QEMU. These selections do not imply full-platform regression acceptance
or execution on the physical router. The source ledger is 312 covered, 129
pending, five backend and five external exclusions; strict completion remains
open. Reproduce the selected checks with the appropriate build directory:

```sh
cmake --build build --target teaql_original_provider_storage_tests \
  teaql_provider_sqlite_decoding_tests teaql_provider_sqlite_bindings_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_provider_storage_contracts|provider_sqlite_decoding_contracts|provider_sqlite_binding_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-provider-sqlite --lib --locked --offline
```

Use `build-sanitize` with
`ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1`, or
`build-wndr4300-memory` with its configured QEMU emulator, for the selected
C++ tests. No dependency or generator changes are introduced.


The complete native consumer rebuild then finished successfully, including all
44 affected link/compile steps. The full Debug CTest suite passes 260/260 in
88.64 seconds, including the new group and both existing decoding/binding checks.
Rust lock verification, 58-module inventory, public-export locators, compiled
runtime layer separation, 534-file clang-format 18.1.8 checks and whitespace
checks pass. These locators/checks do not replace individual remaining source
reviews or prove complete-stage acceptance.


## SQLite schema and boundary source-test contracts

Seventeen further root provider tests now have individually reviewed direct
ports in `tests/parity/teaql_provider_sqlite/schema.cpp`. All original models,
actual table/index/FK declarations and successful/failing inputs are retained.
The ports assert every original diagnostic substring and exact DDL/storage
value rather than treating any error or an empty result as interchangeable.

The direct schema entry in the Rust test module is explicitly `cfg(test)`.
Its C++ counterpart invokes the existing private physical schema kernel on an
idle Context-owned connection, preserving ordered descriptor inputs, actual
BEGIN/COMMIT/rollback, native constraint enforcement and successful cache reset.
The fixture contains no schema algorithm. It does not expose another production
DDL entry: business code still uses `UserContext::ensure_schema`. Direct column
compatibility tests call the production private helper. Unexpected scoped errors
are converted to independent standard exception strings while their owner is
alive. Default PMR is disabled.

The individually reviewed subset preserves:

- All nine Boolean/date/timestamp column-compatibility outcomes; exact existing
  type/nullability diagnostics; wider shape acceptance and all narrower shape
  diagnostics; the complete shaped CREATE TABLE statement.
- Required-column addition to an empty table twice, including exact declaration
  and nullable flag; populated-table rejection with all original diagnostics
  and no partially added code column.
- Eight barrier-synchronized independent connections adding name/code to the
  original temporary file, all worker outcomes, exact final column count two
  and successful file removal. Every worker owns its own Global/User arenas.
- Both original parent/child relations and child-first/parent-first install
  order, exact native FK triple and orphan rejection; missing FK diagnostics;
  composite-key rejection, explicit single-key rebuild/idempotence, and CASCADE
  rejection using the original physical schemas.
- First declared composite index count and all five incompatible index forms:
  reversed/nonunique, partial, descending, alternate collation and expression.
- Passive OrderLine module/provider installation, absence of entity/ID tables,
  explicit Context schema creation and idempotence; exact Order row roundtrip.
- Private physical schema retaining root graph input without saving it, and
  public schema rejecting the separate legacy initial graph after physical
  installation with zero inserted rows.

The public/bootstrap tests remain separate. Calling public schema cannot stand
in for the private physical-only source test, and ordinary inserts cannot stand
in for graph save. The original provider library passes 42/42, including these
17 names; the direct C++ group passes native Debug. The remaining ten provider
source dispositions and complete stage/API/default/privacy/target gates remain
open. This brings the ledger to 329 covered, 112 pending, five backend and five
external exclusions. No production behavior or dependency changed.


The full native Debug suite passes 261/261 (73.17 seconds). After the test-only
slow-target deadline refinement, the original schema, existing physical schema
and original storage selection passes 3/3 on native, ASan/UBSan with leak
detection and OpenWrt MIPS/QEMU. The pinned provider still passes 42/42 unchanged.

MIPS concurrency uses a documented fixture timing adaptation: eight simultaneous
writers exceeded the source provider's five-second contention budget, both in
the grouped run and in isolation. The MIPS-only test fixture allows 30 seconds;
its seventeen-contract group then completes in 7.32 seconds. Every original
writer, barrier, schema outcome, diagnostic and database assertion remains.
The production provider retains its original 5000 ms default, with no retry,
locking or arena changes. Standalone SQLite-only diagnostics helped isolate
contention from TEAQL ownership; this is not a claim of a SQLite defect or a
physical-router measurement. The observed failures and successful adapted run
are retained in
[the measurement record](../measurements/mips-wndr4300/2026-10-07-schema-concurrency/results.json).

```sh
cmake --build build --target teaql_original_provider_schema_tests \
  teaql_provider_sqlite_schema_tests teaql_original_provider_storage_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_provider_schema_contracts|provider_sqlite_schema_contracts|original_provider_storage_contracts)$'
```

Use `build-sanitize` with leak detection/UBSan halt-on-error, or
`build-wndr4300-memory` with its configured QEMU emulator. Formatting and checking
535 project C++ files with pinned clang-format 18.1.8 passes; the standalone
measurement source is formatted/checked separately with the same tool/config.
Pinned inventory, registered dispositions and layer/export locators remain
separate from remaining source/API/default/privacy and complete-stage acceptance.


## SQLite rows, queries and ID source-test contracts

Seven further root provider tests have individually reviewed ports in
`tests/parity/teaql_provider_sqlite/query_rows.cpp`. Real SqliteDialect/provider
queries retain the original descriptors, SQL seed values, ordered result vectors
and strict Value variants. The TOPN fixture uses the actual generic SQL executor,
not a local implementation of probing/window selection.

The scalar test retains all three Alpha/Beta/Gamma rows and every numeric,
text, date, time, timestamp, null and Boolean value. All seventeen original
predicates compare complete ordered I64 identity vectors. Eight original
relation-subquery cases preserve positive/negative reverse matching, known
versus NULL foreign keys, forward nested matching and the deliberate exclusion
of orphan13 from forward NOT IN. Reverse NOT IN still preserves the source
non-null subquery key behavior instead of letting an orphan suppress every row.

The partition test retains two parents and five children each, all original
line names, six total rows and both complete descending [5,4,3] identity suffix
vectors. The TOPN test retains all fifteen rows across three parents, the excluded
fourth child, visible/parent predicates and the original request comment with
both metadata/debug capture flags disabled. It compares complete optional
identity/name tuples between default probes and threshold-zero window, then to
both exact [5,3,2] visible expectations; neither path may return parent13.

`feature_flag.hpp` is a handwritten model counterpart of the pinned private
FeatureFlagRow derive fixture. Provider decoding and entity mapping remain
separate: new BOOLEAN columns return Bool, while legacy INTEGER columns retain
I64 values. The typed model accepts only Bool or signed/unsigned integer0/1,
retains missing/null optional fields, and emits the original field diagnostics
for all nonbinary inputs. All original DDL, affected-row, raw variant and typed
field assertions are preserved. The model follows the pinned macro mapping
rules; the generator and general Value conversion methods are unchanged. This
is model-level mapping evidence, not generator implementation acceptance.

The ID test retains a real temporary file, four independent connections,
twenty-five allocations per worker, exact original five-second contention
configuration and the complete sorted U64 set1..100. Each worker owns its own
Global/User resources. It does not use the earlier schema test's target-specific
extended deadline or replace file-backed concurrency with a shared memory pool.
Default PMR is disabled throughout these C++ fixtures.

On 2026-10-07 unchanged Rust again passes 42/42 provider tests. After rebuilding,
`original_provider_query_contracts` passes 1/1 on native Debug, ASan/UBSan with
leak detection and OpenWrt MIPS/QEMU, including the original ID deadline. No
production behavior, dependency or generator changed. The ledger is 336 covered,
105 pending, five backend and five external exclusions. Three provider source
tests remain: TransactionScope, typed audited transaction boundaries and
relation-name collision hydration/update. Complete source/API/default/privacy/
composition/target and six-stage acceptance remain open.

```sh
cmake --build build --target teaql_original_provider_query_tests
ctest --test-dir build --output-on-failure \
  -R '^original_provider_query_contracts$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-provider-sqlite --lib --locked --offline
```

Use `build-sanitize` with leak detection/UBSan halt-on-error, or
`build-wndr4300-memory` with its registered QEMU emulator, for the selected C++
check. Formatting/checking 537 project C++ files uses pinned clang-format 18.1.8.
Physical-router execution remains unverified.


The complete native Debug CTest regression then passes 262/262 in 55.10 seconds,
including the new seven-contract group and prior schema/storage/runtime checks.
Rust lock, 58-module inventory, registered dispositions, public-export locators,
compiled runtime layer separation, 537-file formatting and whitespace checks
pass. The private handwritten model additions and test registration change no
production behavior. These checks do not close the three remaining provider
source dispositions or the broader all-stage audit.


## Final provider transaction and collision source contracts

The final three root provider tests have direct ports in
`tests/parity/teaql_provider_sqlite/transactions.cpp`. Every original data value,
audit reason and success/failure assertion is retained. The actual UserContext
TransactionScope, SQL executor and SQLite provider own transaction and mutation
behavior. Callback body states only sequence the source operations, forward
wake signals and cancel their real children; they contain no transaction/saver
implementation or invented database results.

The raw scope test commits Order1/first and Order2/second, then attempts
Order3/must roll back followed by duplicate Order1. Failure leaves the complete
ordered native identity vector [1,2]. The typed scope uses a handwritten
TransactionSchool with the existing allocator-aware model/ledger hooks. First
and second have separate source-style new ledgers and both authoritative
versions equal one. Their original audit reasons precede successful enclosing
commit. The third school and its aliased ledger are retained outside the failed
callback; after its save and duplicate failure, new_keys remains nonempty and
native committed rows are exactly (1,first),(2,second).

C++ keeps the third model in User storage and captures a non-owning reference
before moving it once into the callback state. This obeys the existing one-shot
callback allocator contract instead of relaxing it for an opaque owned capture.
Returned typed models and pending intent remain Context-bound; no owner prolongs
the arena. The handwritten models reuse actual public field/ledger helpers and
leave the generator unchanged.

The collision fixture retains the original payment tables, physical column
channel for logical channel_id and the separate channel relation. Compiled SQL
must contain channel AS channel_id. Decoding the original Order9 row gives
FK1002, no synthesized child and version1. Updating name through expected
version1 affects one existing parent; reread retains FK1002/no child with
version2/name after. The source test checks scalar mapping and update, not an
additional eager entity-graph query, so its name does not broaden the claim.

Unchanged Rust again passes 42/42 provider tests with names matched to the pinned
inventory. Native Debug and MIPS/QEMU pass the direct three-contract target;
the complete native suite passes 263/263 in 103.79 seconds. This closes all 42
individual root provider source dispositions, not complete provider/public API
or six-stage acceptance. The ledger is now 339 covered, 102 pending, five backend
and five external exclusions. Remaining source reviews are runtime roots,
round-trip references, entity save and two graph-reference lifetime adaptations.
No production behavior or dependency changed.


After rebuilding, the final source target and existing transaction ownership
check pass 2/2 under ASan/UBSan with leak detection and 2/2 under MIPS/QEMU.
Native full regression includes both. Pinned source, 58-module inventory,
registered dispositions, public-export locators, compiled runtime layer boundary,
539-file clang-format 18.1.8 and whitespace checks remain required.

```sh
cmake --build build --target teaql_original_provider_transaction_tests \
  teaql_transaction_scope_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_provider_transaction_contracts|transaction_scope_context_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-provider-sqlite --lib --locked --offline
```

Use `build-sanitize` with leak detection/UBSan halt-on-error, or
`build-wndr4300-memory` with its configured QEMU emulator. Physical-device
execution and remaining runtime/API/default/privacy/composition/lifecycle
acceptance are separate from this complete per-provider-root-test review.
