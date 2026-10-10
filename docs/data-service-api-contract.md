# Data-service API and lifecycle review

## Data-service source acceptance, 2026-10-08

The complete lib.rs source group is accepted after reviewing production lines
1–310, all 29 root names, every model field/variant/derive and all executor
methods and inherited defaults. The distinct responsibilities in the tables
below are retained. The SQL-log module has its own accepted source contract;
provider execution and runtime privacy do not follow from API acceptance.

The latest complete native suite passes 314/314 in 74.66 seconds, and a subsequent
build reports no pending work. Fresh selected builds precede nine named checks
on ASan/UBSan (9/9, 5.23 seconds, leak scanning disabled) and OpenWrt MIPS/QEMU
(9/9, 3.56 seconds). These checks cover model contracts, scoped retention,
original model/Clone observations, inherited executor defaults and shared
full-range Unix SystemTime. The async lifetime check additionally passes on
both configurations, exercising native stop/join before Context reclamation.

Live native comparisons match all forty model inputs, forty scoped-model inputs,
144 inherited-default inputs and 254 full-range time inputs against actual
pinned Rust methods/derives. The existing arithmetic/error/time-consumer reviews
supply the wider SystemTime evidence; the early nanosecond-only fixtures do not
stand in for the full representation. No handwritten source algorithm is used
as a substitute oracle.

Rust owned Clone and Arc borrows become explicit allocator-aware destination
copies and Context-scoped handles. The generic Error parameter stays distinct
from SQL/provider errors; it need not inherit a C++ exception class. Rust
Send/Sync scheduling becomes owner-thread dispatch: native completion threads
only wake operations, and Context values/errors/observers are delivered on the
owner thread. This changes memory and scheduling ownership, without deleting
an error, stream, observer or executor responsibility. Transaction terminal
methods consume access before polling, reject pending or foreign-domain work,
and expose implemented optional capabilities explicitly. Descriptor lookup
returns a provider-scope borrow; retention requires a destination copy.

This closes the data-service API source contract. Actual transport/provider
cleanup, guarded-statement atomicity, cancellation metadata, runtime safe-log
projection and stages 3–6 retain their separate acceptance gates. Earlier
partial/pending paragraphs below describe their historical checkpoints.

## SQL-log source acceptance, 2026-10-08

The separate sql_log.rs source group is accepted after complete production/
public/default/trait/ownership review, current native 314/314 in 74.66 seconds,
and a stable post-test no-work build with 314 registered checks. Current named
sanitizer/MIPS lifecycle/projection selections pass 13/13, including noncopyable
state, quota/unsupported-copy rollback, actual User deletion, allocator rebinding,
origin identity and private shutdown diagnostics. All eight projection and 1,008
intent observations match the actual original. This accepts compiler-owned log
models/intent collection, not complete runtime safe-log privacy or Stage 3.
The containing data-service lib.rs group is now also accepted. Earlier paragraphs
below retain the repair and verification history.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-data-service/src/lib.rs`. The current acceptance is above. Model,
executor, SQL, transport, provider and runtime responsibilities remain separate.
The review reads the complete production source and the C++ types/executor
headers, rather than treating exported names as acceptance.

## SQL-log module follow-up and reproduced projection gap

The separate sql_log.rs body review preserves four policy variants, three
outcome variants, all nine SqlLogContext fields, opaque projection state,
execution-local intent redactions and conservative credential classification.
Unknown is the only policy default; outcomes have no source Default. Projection
state Debug excludes all payloads and equality uses selected allocation identity.
Intent Debug reveals only count; ordered duplicate/forced entries remain private
and are cleared by the safe runtime projection at its own call sites.

The iterative query fallback includes filter/having/expression projection/order/
search and SQL subqueries, excluding separately executed relation siblings.
Binding collection preserves ancestry, invalid-policy fallback, SQL credential
detection, nested credential keys, scalar spellings and forced versus masked
values. extend deduplicates exact (text, forced) pairs; target-ID capture forces
free-text redaction without changing the execution binding. Credential matching
uses thirteen substrings after ASCII-only alphanumeric lowercase normalization.

Eight independently named original projection_scope inputs now exercise a
payload with no Clone implementation. Original state Clone preserves identity,
typed lookup, private Debug and readability after dropping the first wrapper;
separately allocated identical payloads compare unequal, and empty states compare
equal. Existing 1,008 SQL intent observations remain unchanged and the eight
new source observations are revalidated twice. They are stored separately in
sql_projection_scope_cases.json.

The two reproduced native gaps are now repaired. The clone dispatch checks the
actual allocator-aware copy capability before instantiating a payload copy.
Same-domain copies support noncopyable state; explicit cross-domain retention of
such a payload throws before publishing a destination object. Copyable state is
deep-copied and uses the destination allocator while preserving its origin
identity. Assignment prepares the replacement first, so unsupported copies and
quota failures preserve the existing destination and source.

Identity is a scalar arena namespace plus its non-reused object slot, not the
payload address or shared ownership. Arena namespaces are issued by a bounded
32-bit atomic counter without Context pointers or caches; exhaustion throws
rather than wrapping. The counter is 32-bit on MIPS and needs no additional
atomic runtime. Every arena carries four bytes of namespace and four bytes of shutdown-token
accounting, subject to alignment;
each projection wrapper carries an eight-byte origin token. Explicit copies
retain that token without keeping the original arena or payload alive. The token
is comparison metadata, not a checked reference or resolution authority.

The registered native counterpart matches all eight live original observations;
all 1,008 existing SQL intent observations also match live Rust. Four selected
checks pass native, ASan/UBSan with leak scanning disabled, and MIPS/QEMU. Direct
ownership tests cover noncopyable borrows, unsupported assignment, actual User
deletion, destination string allocation, independent equal-value allocations,
empty states, wrong-type lookup, and same-address arena reconstruction. Quota-failure and closing-callback tests have been added and are undergoing
the subsequent memory/consumer refresh. Private shutdown diagnostics use a
disjoint token range above the public object-slot encoding, preserve the existing
shutdown allocation reserve, and never publish checked references. Their tokens
do not consume public object quota and do not imply a new ownership lifetime. The previous 303/303 native result predates the arena/header change; a
complete fresh native build and regression is required. The latest ownership test, including quota failure and private closing diagnostics,
also passes standalone native, ASan/UBSan with leak scanning disabled, and the
OpenWrt MIPS32r2/O32 toolchain under QEMU 74Kf. The standalone MIPS executable
links the memory implementation and test with the standard exception-support
libraries but without `libatomic`; its 32-bit namespace issuer requires no new
atomic dependency. These focused builds do not replace the in-progress CMake
consumer and lifetime refresh. The SQL-log group and Stage 3 remain partial
pending their full acceptance gates.

## Final-header cross-platform consumer checkpoint

After the closing-diagnostic header change, the OpenWrt MIPS selected consumer
refresh finishes, and a repeated build reports no work. All thirteen named
checks pass in 2.71 seconds: Context memory/reference/early Global close,
async Context, resource cleanup, Context logging, the lifetime example,
projection ownership and its eight source comparisons, the 1,008 SQL intent
comparisons, data-service model lifetime integration, and executor SPI defaults.
This includes the corrected cross-domain metadata identity assertion. Tests run
only after the final selected build completes. The sanitizer final-header refresh
also completes, reports no pending work, and passes the same thirteen checks in
2.03 seconds with ASan/UBSan and leak scanning disabled. The final MIPS opaque
probe matches all eight live original observations; all 1,008 intent observations also match live original Rust. The complete native consumer
build remains running; the previous 303/303 native checkpoint cannot stand in
for its final headers.

## Models and responsibilities

| Original item | C++ contract |
| --- | --- |
| DataServiceCapabilities | Eight independent booleans, all false by default; Clone and derived Debug. Capability declaration does not implement an operation. |
| QueryRequest | Independent SelectQuery, trace chain, optional comment and two required capture flags. Debug/Clone preserve request metadata separately from query metadata. No source Default or serde derive. |
| QueryResult | Ordered CompactRows and complete ExecutionMetadata; allocator-aware Clone and derived Debug. |
| MutationRequest | Five alternatives: Insert, Update, Delete, Recover and recursive Batch. trace_chain borrows the concrete command; Batch has an empty top-level trace. comment observes only the final trace node, including Some empty text; Batch has None. |
| MutationResult | Distinct affected-row count, GeneratedValues, optional EntitySnapshot and ExecutionMetadata. Do not replace these models with an undifferentiated map. |
| DataServiceOperation | Seven distinct Copy/Clone/PartialEq/Eq/Debug variants. No source Hash/Default/serde derive is invented. |
| ExecutionMetadata | All fourteen original fields, including recursive statements and compiler-owned sql_log provenance. unrecorded_query has empty data, Query operation, epoch times and Some count; it must not invent execution facts. |
| StreamChunk | Independent ordered rows, chunk index and terminal flag; Context borrows do not extend their lifetime implicitly. |
| GuardedMutationRequest | Mutation and trusted Expr remain separate. Constructor/Clone/Debug do not enforce authorization; a guarded executor must use one datastore statement. |
| SchemaRequest / SchemaResult | Named entity request and changed flag remain separate from provider schema implementation. |

Source model Debug was missing in C++. A direct compile check reproduces this
for QueryRequest and ExecutionMetadata; the new explicit model_debug.hpp header
now supplies complete structural writers for every model and operation. It uses
existing query/mutation/row and log-state writers. Nested private projection
state remains a placeholder, and pending intent shows only its count. Raw model
Debug is not a safe-log projection or a new serialization protocol.

The initial Linux SystemTime diagnostic audit covers signed i64 nanoseconds,
including both endpoints, negative fractions and epoch boundaries. A subsequent
[full-range carrier repair](system-time-contract.md) replaces the narrowed
system_clock::time_point storage with the existing signed-seconds/nanoseconds
carrier, now shared from core. Its 254-input standalone native comparison passes
against actual Rust; metadata/log/pagination and ABI integration are running.
The earlier nanosecond-domain checks do not establish that later integration or
Windows diagnostic formatting.

## Executor defaults and composition

| Original SPI | Required behavior |
| --- | --- |
| DataServiceExecutor | Separate associated Error responsibility and mandatory capabilities declaration. C++ typed error adapters remain separate from transport/provider errors. |
| QueryExecutor | Mandatory query; default query_log_intent is pure conservative collection without I/O/log emission. query_observed forwards the unpolled query and does not invent observer diagnostics. |
| StreamQueryExecutor | Mandatory stream and caller chunk size; observed default delegates without inventing statement metadata. The stream borrow is bound to its Context. |
| MutationExecutor | Mandatory mutate; observed default forwards exactly once. |
| GuardedMutationExecutor | Separate datastore-enforced guard; observed default delegates to guarded mutation. A preliminary read cannot satisfy its atomicity contract. |
| TransactionExecutor / Transaction | Associated transaction retains query, mutation and terminal responsibilities. C++ consume/cancel/pending-operation guards adapt owned Rust terminal calls to Context borrows. Additional associated SPIs remain recoverable explicitly, not advertised on every transaction. |
| SchemaExecutor / IdGeneratorExecutor | Separate ensure_schema and next_id operations. Neither is folded into query execution. |
| SchemaProvider | Returned descriptor borrows its explicit provider scope in C++; any retention beyond that scope needs destination copying. Rust Arc ownership does not silently extend User lifetime. |
| QueryStream / ExecutionObserver | Distinct stream and runtime diagnostic callback. Context-thread invocation/cancellation/cleanup adaptations require explicit lifecycle evidence; callbacks are not serialized request fields. |

Existing executor_spi_contracts verifies unpolled forwarding, ignored default
observers, chunk-size preservation, conservative no-I/O intent, separate guarded
dispatch, associated transaction capabilities and pending/terminal handling.
The source default bodies are direct delegations. Complete transaction,
cancellation, provider/observer and runtime composition still need acceptance.

## Current verification

Forty actual original model inputs record all model fields, operation/capability
variants, nested metadata, all mutation alternatives, comments/traces,
unrecorded defaults, actual Clone and nine Debug formatter modes, for 11,520
complete diagnostic observations. The oracle
uses original types/methods/derives; it does not copy model behavior algorithms.
The deterministic corpus is compressed to approximately 265 KB.

Native lifetime checks disable default PMR, explicitly copy every scoped model
into Global, close and delete User, reject a closed metadata reference, assert
all User blocks returned, and verify identical complete retained diagnostics.
Nested queries, rows/layouts, mutation snapshots, statements and private
projection strings are checked against the destination allocator. Local owning
objects are destroyed before manual close.

The fresh native selection passes 3/3 in 6.63 seconds: model contracts,
original-call comparison and existing executor SPI contracts. ASan/UBSan with
leak detection passes 3/3 in 8.31 seconds, and MIPS/QEMU passes 3/3 in 35.47
seconds. All selected targets finish building before their respective tests run.
Native and MIPS probes also match all forty inputs against live original Rust.
The builds use independent directories to avoid shared-output races.
An initial fixture compilation uses the wrong TraceNode argument
order; that is corrected and rebuilt, and the failed build is not evidence of
passing verification.

Remaining acceptance includes full supported SystemTime consumer integration,
complete generic async/transaction/descriptor/callback lifetime composition,
current whole-project consumers and the independent sql_log.rs privacy surface.
This review does not accept Stage 3 or complete the six-stage migration.
