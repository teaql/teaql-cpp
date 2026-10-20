# SQLite provider API and lifecycle acceptance

## Source acceptance and Stage 3 completion, 2026-10-08

The complete provider source group is accepted after full production/public/
default/trait/protocol/ownership review and final current consumer evidence.
All eight Stage 3 source groups are now accepted, closing the separate
data-service, SQL executor/transport and SQLite provider stage. The later
runtime, graph/governance/privacy and final integration stages remain open.

The complete current native suite passes 315/315 in 73.69 seconds and reports
no pending build work. Fresh 28-target builds precede provider selections:
ASan/UBSan passes 32/32 in 32.73 seconds with leak scanning disabled; OpenWrt
MIPS/QEMU passes 31/31 in 147.93 seconds. Selected builds afterward report no
work, dependency records are nonempty (227 sanitizer and 143 MIPS), and source
fingerprints remain unchanged throughout verification.

Live native comparisons match all 798 transport/unit/declaration, 735 schema,
563 driver/provider error, 1,084 binding and 6,029 release-decoding observations.
MIPS transport/error models also match live Rust. The final MIPS profile uses
SQLite 3.53.2, verified through the target native-library API, with four-byte
pointers and eight-byte long double. The two ID diagnostic and two REAL-to-TEXT
engine cases additionally match current live Rust there. Existing 3.40.1 MIPS
numeric failures remain recorded; this acceptance does not change inputs or
claim identical native-engine diagnostics/conversion on every SQLite version.
The raw 64-bit FromSql Debug profile runs natively; its portable conversion and
ownership counterparts run on 32-bit MIPS without rewriting raw diagnostics.

The responsibility review below preserves source-owned connection constructors
through explicit scoped resources, same-owner clone identity, distinct provider/
SQL executor interfaces, all source schema/ID/stream/transaction paths and actual
cleanup before reclamation. All 42 root source tests have individually reviewed
ports; external test-file and final Stage 6 acceptance remain independent.
Physical WNDR4300 execution is unverified. Exact checks, fingerprints, engine
profile and logs are retained in
[the checkpoint](../measurements/parity/2026-10-08-sqlite-provider-acceptance/README.md).
Earlier pending statements below record the preceding review checkpoints.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-provider-sqlite/src/lib.rs`, and its locked rusqlite 0.40.1 dependency.
This mapping records concrete responsibilities and remaining verification. The
provider source group and Stage 3 are now accepted. SQLite is the only database.

| Original responsibility | C++ implementation and acceptance requirement |
| --- | --- |
| Four public default constants | ID/business-ID table names remain separate. Both 64-entry cache constants are now public and drive their respective caches. Direct original declaration observations are included in the transport corpus. |
| SqliteDialect | Preserve six provider overrides and all inherited SqlDialect defaults. Its original unit Debug/Default/Clone/Copy contracts require a direct public protocol review; compiler corpus success alone cannot establish them. |
| MutationExecutorError | Preserve all six outer alternatives, exact diagnostics, conversions and the empty outer cause. See [the error contract](provider_error_api_contract.md) for every enabled driver payload and conversion, explicit retention and original-call evidence. |
| SqliteMutationExecutor construction/clone/connection | Source constructors own or share a native connection and clones share its layout cache. C++ uses a separately scoped SqliteConnection resource and same-owner provider clones. No clone extends Context lifetime; constructor aliases must be reviewed as explicit ownership adaptations. |
| DataServiceExecutor declaration | Preserve all eight source capability flags, including schema and ID generation. Declarations remain independent of implementing the corresponding service and of the SQL adapter's capabilities. |
| Ordinary execute/fetch and repeated fetch | Preserve binding/prepare/error precedence, affected counts, exact row values and layouts, repeated-input order/duplicates, comments, empty repeated input and statement error payloads. Existing probes exercise the production transport and provider. |
| Buffered fetch_stream / native stream_sql | Keep distinct terminal-chunk rules and zero-size behavior. Native streams retain their guarded cursor across yields; cancellation, EOF and Context close release it. Earlier chunks remain Context borrows. |
| StreamQueryExecutor / diagnostic default | Preserve dummy-descriptor compilation and one compile failure followed by EOF. The inherited observed-stream default forwards to the provider without inventing diagnostics. Complete observer/runtime privacy composition needs acceptance. |
| SqlTransaction / SqlTransactionTransport | Preserve begin and consuming commit/rollback through the same connection. C++ tracks pending work and cancellation explicitly; abandoned scoped transactions roll back before reclamation. These cleanup/terminal adaptations require separate original-semantic and native-resource evidence. |
| Physical schema and SqliteSchemaProvider | Preserve typed resource lookup, SOUNDEX/foreign keys, BEGIN IMMEDIATE, table/column/index/foreign-key checks, granular Context events, rollback and cache clearing, followed by the typed bootstrap boundary. Unit Debug/Default/Clone/Copy and complete event/privacy composition remain review gates. |
| SqliteProviderExt | use_sqlite_provider installs dialect, executor and schema provider through UserContext. The C++ free-function counterpart requires explicit origin authority and keeps resources distinct; it cannot retain a shorter or sibling Context. |
| SqliteIdSpaceGenerator | Keep default/custom table, clone, canonical entity name, ensure-table/next-ID/floor responsibilities, retries, exact failures and the independent InternalIdGenerator adapter. Source constructors using owned connections map to explicit Context resource construction. |
| SqliteBusinessIdAllocator | Keep a dedicated connection, default/custom table, clone, explicit schema, plan validation, transactional optimistic allocation, retries, exact BusinessIdError classification and schema-contributor responsibility. It must not share an aggregate-save transaction. |
| Column-layout cache | Source clones share cache identity; statement keys include comments while layout keys do not. LRU statement eviction and whole-layout-cache clearing are separate policies. Borrowed result layouts and explicit destination copies must remain valid until their owning Context closes. |

## Complete production-body review, 2026-10-08

The review reads the full provider production body through line 1,753, before
its root test module, including every public constructor/method/constant,
trait implementation and private binding/decoding/schema helper. The two
cfg(test) physical-schema methods remain separately identified test adapters;
application DDL uses UserContext::ensure_schema. All 42 root source tests have
reviewed direct ports, and the two external test files retain their own final
Stage 6 gates. Source inventory and matching export names are not the review.

The original Arc-owned connection constructors map to explicit
make_resource<SqliteConnection> followed by make<SqliteMutationExecutor>. A
provider may share its connection's domain, or belong to a UserContext beneath
the connection's Global domain. Connection and provider state have separate
borrow checks; User provider state cannot back asynchronous work retained by
another User or Global. Provider Clone remains in its own domain and borrows
the same connection/layout state;
connection() returns that scoped resource, never a new owner. ID/business-ID
constructors select an explicit provider dependency and copy configured table
names into their destination. A destination may borrow the connection only
when its lifetime permits it; retained resources never silently extend User.
The SQL associated transaction is a distinct Context carrier which consumes
terminal access and cancels pending work before rollback/cleanup.

The six dialect overrides and all inherited generic defaults remain independent.
Actual unit observations now cover Debug/Default/Clone/Copy of both dialect and
schema provider in nine formatter modes. Native scope tests copy them through
the User Global proxy, delete User and exercise retained provider dispatch.
All eight capability flags and four public constants have actual source evidence.
The transport corpus contains 798 cases, preserving its earlier prefixes.

Ordinary calls bind before acquiring/preparing; provider repeated-fetch prepares
once even for empty values and replaces the selected parameter before binding.
Buffered fetch_stream always appends its final chunk, including an empty one;
native stream_sql retains its connection lease/cursor across yields and has
separate zero-size/final-chunk rules. The inherited observed-stream default
forwards without invented diagnostics. Direct query_stream compiles with its
dummy descriptor and returns one compile error followed by EOF.

Statement cache identity includes comments and uses LRU eviction. Layout cache
identity excludes comments and clears all entries at capacity. Previously
borrowed row layouts remain allocated in their Context. Physical schema follows
typed resource lookup, SOUNDEX/foreign-key setup, BEGIN IMMEDIATE, per-entity
column/index checks and ordered raw events, complete foreign-key validation,
commit/rollback, cache clearing and post-commit typed-bootstrap rejection.
Schema failure diagnostics retain original type/nullability/shape/index/FK data.

ID-space next/floor calls preserve canonical entity names, signed INTEGER bounds,
optimistic CAS, insertion/lock retries and RuntimeError conversion. Business IDs
preserve explicit schema, range validation, canonical scope, dedicated short
transactions, version CAS, failed-commit state and exact BusinessIdError mapping.
Cleanup only rolls back a transaction actually started by its own carrier.
Error review preserves all six provider alternatives and every one of the 25
locked enabled driver tags, conditional equality/copy capabilities, all five
FromSql conversions and real borrowed cause chains.

Final verification is running in independent native-sanitizer and MIPS build
directories. The MIPS provider acceptance uses SQLite 3.53.2, matching the locked
Rust engine; the existing 3.40.1 profile retains its documented numeric failures.
No comparison tolerance, missing input or substituted expected value is used to
hide that engine difference. The current native 315/315 checkpoint includes
these sources, but final provider target/platform tests must finish before this
group or Stage 3 is accepted. Runtime privacy and later-stage saving remain
independent responsibilities.

## Resource evidence and limits

The production transport contract disables default PMR allocation. It verifies
lazy operations; independent connection/provider identity; same-owner clones;
rejection of shorter/sibling connection retention; owner-thread confinement;
queued cancellation and wakeup; cursor cleanup at EOF/cancel/Context close;
transaction consumption, failed commit and teardown rollback; both cache policies;
and actual SQL executor query/mutation/stream/transaction composition.

A sqlite3_create_function_v2 destructor callback observes live arena blocks while
the native connection is closing. The test then observes zero blocks, proving
native cleanup occurs before arena reclamation. A failed native open survives
unwinding the creating Context. These are concrete resource checks, not proof of
all supported callbacks, external native-handle use or cross-thread execution.

The Context model intentionally owns graph/resource storage until close. Native
raw handles and cause/result views remain borrows; checked object IDs do not
make an escaped raw pointer safe after teardown. No shared owner is introduced
by provider clone, transaction handle, row layout or diagnostic cause.

## Current verification and next gates

The independent-field/capability follow-up preserves the earlier 467 driver and
796 transport inputs and observes their additions through actual original calls.
The stable-source native consumer selection passes 8/8 in 7.78 seconds and a
subsequent selected build reports no work. All 131 compiled-object dependency
records are nonempty. The sanitizer selection with leak detection passes 8/8 in 17.91 seconds;
MIPS/QEMU passes 7/7 in 78.67 seconds. Both subsequent selected builds report no
work and pass dependency-record checks. Native/MIPS match all 563 driver and
797 transport observations against live original Rust. See
[the recorded checkpoint](../measurements/parity/2026-10-08-sqlite-provider-declarations/README.md). Builds that compiled an older probe or assertion while sources changed
are failed checkpoints and must be refreshed before acceptance.

Next: finish stateless provider protocols and constructor/clone adaptations;
review the complete source body/defaults and schema/ID/observer/cancellation
composition; verify final coherent consumers. All provider, runtime, save,
privacy/example and stages 3–6 gates remain open. Physical-router execution is
unverified; current MIPS execution uses the OpenWrt toolchain and QEMU 74Kf.
