# UserContext lifetime contract

Contexts use growing multi-block arenas. Every result belongs to its Context and becomes invalid on close. Retention across lifetimes requires explicit copying; shared_ptr must not defer reclamation.

## Two ownership domains

| Owner | Typical contents | Destruction |
| --- | --- | --- |
| Application GlobalContext | Immutable metadata, service factories, long-lived GPU/system services | After every dependent UserContext ends |
| UserContext | Query/entity/results, mutation work, log buffers, temporary resources | Explicit close() or Context destruction |

Global objects must not retain User-owned data; copy it into Global explicitly. Users may borrow longer-lived Global objects. Temporary activation/registered-wait borrows described below must detach before User memory is reclaimed. TLS/static caches are not a third ownership domain.

## Arena implementation

Arena uses std::pmr::monotonic_buffer_resource, growing on demand with new_delete_resource as the default upstream. Each Context exclusively owns its blocks; no hidden global pool retains them. Injected upstream resources must outlive the Context; their own caching policies govern returned blocks.

Allocation sites expose lifetime:

```cpp
teaql::GlobalContext global;
auto& metadata = global.make<std::pmr::string>("EdgeDevice");
auto user_context = global.make_user_context();
auto& rows = user_context->make<std::pmr::vector<std::pmr::string>>();
```

GlobalContext is application-owned, not a singleton. UserContext requires an explicit Global and has no default constructor. Global destruction with live Users fails fast rather than leaving dangling metadata. Both scopes currently use owner-thread access.

make returns borrows and registers nontrivial destruction. Constraints reject ordinary std::string/vector; make_resource explicitly admits foreign RAII wrappers without claiming arena-backed internals. PMR strings/vectors and nested allocator-aware containers receive the selected resource. Ordinary containers inside foreign wrappers retain their own allocations and release them through registered destruction. Implemented core/SQL/result/log dynamic storage uses explicit PMR; tests disable default PMR while running real SQLite paths.

Close order:

1. Enter closing and reject new asynchronous registration.
2. Stop/join registered work while ordinary objects remain alive. Final diagnostics may allocate in the original scope.
3. After host-memory access stops, reject allocation and destroy successfully constructed objects in reverse order.
4. Return every block; live_blocks/live_bytes become zero.

Contexts cannot copy/move. close is idempotent/noexcept and destruction follows the same path. Failed construction registers no destructor; unused bytes remain until close. Context reset/reuse is not provided because old request borrows must never become valid in a new request.

## Usage

```cpp
teaql::GlobalContext global;
auto ctx = global.make_user_context();
auto& rows = ctx->make<std::pmr::vector<std::pmr::string>>();
rows.emplace_back("edge-device-001");
// Explicit deep copy; nested strings also use the Global allocator.
auto& retained = global.make<std::pmr::vector<std::pmr::string>>(rows);
ctx.reset(); // Destroy the Context and return all its blocks.
// rows is invalid; retained remains valid until Global destruction.
```

Never delete/manually destroy make results or transfer them to owning smart pointers. Stack PMR containers using a Context resource must be destroyed before close; prefer Context-managed construction when their cleanup belongs to the Context.

## Native resources and GPU work

Arena reclamation does not replace SQLite finalize/rollback, file close, or GPU-buffer release. Explicit scoped resource factories register noexcept RAII destruction in reverse dependency order. Destructors cannot allocate in a closing Context.

Allocation/poll/cancel/close run on the owner thread. AsyncOperation/AsyncStream and quiescence already exist; native completion threads only notify WakeSignal, never allocate Context values/run observers. There is no scheduler/pool/GPU synchronization implementation. Hosts must finish unregistered work first. Future GPU adapters must await completion/synchronization so devices stop touching host memory; requesting cancel alone is insufficient. Device/driver/SQLite heap allocations are accounted separately and released by resource destructors.

Reclamation releases allocation ownership, not necessarily immediate OS RSS. Long-lived Context temporary bytes are not returned individually; edge workloads should use short-lived Users. Future explicit budgets must not silently become cross-request caching.

## Acceptance tests

- Multi-block growth and matching upstream sizes/alignments on deallocation.
- 256-byte alignment and nested PMR propagation.
- Reverse resource destruction before block release.
- Construction/allocation failure, repeated close, and post-close allocation rejection.
- Independent Users and 1,000 create/destroy cycles without retained blocks.
- Separate Global/User allocation, explicit deep copies, and Global survival after User close.
- Delayed native completion, cancellation/join, and buffers allocated after operations destroyed only after join.
- Native-state cleanup on operation-allocation failure; closing rejects new tasks but permits final diagnostics; idempotent/reentrant close handling.
- ASan/UBSan.

## Evaluation and entity models

SafeExpression roots/evaluator/mapper states are registered in one Context. Expression/ContextFunction handles borrow; same-domain copies share state like Arc clones without extending Context lifetime. eval executes each time without caching; allocator-aware outputs belong to the expression domain.

Global copies explicitly relocate root/callback graphs. Captureless callbacks relocate; captured callbacks need allocator-aware state/deep-copy constructors. Uninspectable captured lambdas work only in their original Context and reject cross-domain copying. Built-in ID/list adapters use relocatable scalar state. Custom copies must preserve nested allocation/borrow boundaries; std::function/shared_ptr do not own Context data.

Entity/BaseEntity traits map to concepts/CRTP defaults. EntityError/Result are scoped values whose errors must also be copied when retained. AnyRef is a read-only erased borrow for on_loaded, not an owner.

Audited validates a nonempty Unicode reason; into_entity applies it. It preserves the typed save gate, whose runtime remains pending. Annotated EntityGraph save input carries relation names/per-node reasons/Save-Delete separately from identity graphs. Input graphs allow absent and empty comments; do not apply Audited validation to them.

Naming adaptations: EntityGraph::new becomes from_entity; delete becomes delete_; BaseEntityData.dynamic storage becomes dynamic_values while dynamic(key) remains accessor. BoxedRelations delegates scoped allocation. Invalid BaseEntity fields retain entity/field/value diagnostics; value text currently uses JSON projection rather than complete Rust Debug, so acceptance remains partial.

## Asynchronous handles and interfaces

make_operation/make_stream allocate allocator-aware state/handles in explicit Contexts. Native states use resource factories. Providers may pass same-Context Memory without depending on UserContext. Handles are non-owning borrows; abandoned work should cancel explicitly, and close cancels remaining work.

poll/wait and poll_next/wait_next retain pending/item/end distinctions. Typed Result failures are not EOF/cancellation. Delivered results/chunks survive later polls until close. Long streams accumulate arena allocations; fixed-memory infinite streaming or per-chunk reclamation is not claimed.

States implement poll/poll_next and noexcept finish_and_wait/cancel_and_wait. Terminal hooks detach native callbacks and join all host-memory access before ordinary destruction. Constructors do not start I/O; first poll does. Native threads may notify, but poll/close do not race. Closing from an active poll/terminal callback fails fast; repeated close during existing shutdown is idempotent. Cleanup handles diagnostic allocation failure internally.

ExecutionObserver is a Context callback, not a request field. Default observed interfaces forward without fabricated SQL diagnostics. Query/mutation/guard/transaction/stream diagnostics and FailureJournal have execution tests; complete provider/runtime integration needs separate acceptance.

Commit/rollback consume transaction access even on failure. Transaction handles/work cannot escape their Context. SQL/production SQLite commit/rollback/cancel/close are tested; runtime governance/ledger cleanup remain pending.

## Operation composition and transaction borrows

WakeSignal::Forward borrows child/parent signals. Owner installs/removes topology; native threads only notify. One pending child has one awaiting parent; nested forwarding is allowed and cycles reject. Detach synchronizes with in-flight notification. Composition resets forwarding and stops/joins children before releasing their referenced query state. Forward must not outlive either signal.

Repeated fetch snapshots templates/values into the selected result domain and executes sequentially/lazily. Transport outlives pending borrows. Child operations/results share the Context; operations are not manually destroyed before close.

SQL/data-service transactions track pending work. Commit/rollback reject while work is not Ready/Failed/Cancelled, preserving the handle on rejection. Actual termination consumes immediately even if its future fails. This runtime precondition replaces Rust's consuming-self borrow check without secretly completing business work. Provider transaction extensions must use protected track registration.

## SQL cache and diagnostic lifetimes

SqlExecutorState is explicitly Global/User allocated. Executor/state share a domain; same-domain copies borrow one state and cross-domain executor copies reject. Dialect/schema/transport outlive pending work. Cache access remains owner-thread-bound; concurrent execution is not claimed.

Descriptors/query shapes copy nested fields into state. Requests/results/diagnostics propagate operation allocators, allowing explicit copies from sibling Users or retention in Global. The 256-entry FIFO is a logical entry ceiling; evicted arena bytes remain until owning Context close. Choose cache domain accordingly; entry count is not a resident-memory ceiling.

StatementDiagnostic/FailureJournal register quiescence. Close first joins later transport work, then delivers failure/cancel diagnostics, then destroys objects/blocks. Successful metadata returns with results and is not repeated to observers. Sink exceptions do not replace transport errors; debug never directly interpolates original bindings.

Each ObservedSqlTransport mutation leaf owns intent. Rereads inherit that write's privacy/target IDs without shared-cache storage. Outer journals collect executed nested-batch statements; success disarms, failure/exception/close delivers in order. Close stops active children before journal/request cleanup. Resume after a completed write only rereads; it never repeats the write.

Ordinary executor mutations do not reread. Guards apply in the same SQL statement and one-row success rereads with ID plus the same guard. Transactions provide ordinary rereads; production SQLite adapters now exist. Snapshots come from database rows; generated values remain separate. Input/synthetic versions are not authoritative rereads.

## SQL transaction adaptation

Executor implements TransactionExecutor only when its transport implements SqlTransactionTransport; ordinary erased transports do not advertise begin. Preserve the source capability flag's false transaction value rather than substituting it for the type constraint.

Begin/results/adapters belong to caller-selected Context. Underlying transactions/adapters share a domain; shared caches/dialect/schema are longer-lived explicit borrows. Transaction queries retain one partitioned SQL query instead of ordinary per-value probes; mutations reread.

Pending work blocks commit/rollback until completion/cancel. Actual termination consumes both adapter/native handle before its lazy native poll. Close joins later work, delivers diagnostics, then provider RAII/quiescence rolls back outstanding transactions. Generic adapters must not guess SQLite cleanup or start new unawaitable work.

Pure query_log_intent is a copyable diagnostic value, not transaction ownership. Work/handles remain scoped. Stack objects using arena memory must be destroyed before explicit close; Context-owned containers/requests/callbacks and provider resource wrappers register cleanup before reclamation.

## Streaming SQL

Native streaming exists only for StreamingSqlTransport. Entry-time compilation/cursor creation passes chunk_size unchanged. Transaction streams buffer: first poll executes one query, then chunks by max(chunk_size,1). Without observers compilation is lazy; observed metadata-enabled streams compile diagnostics before first query poll. Preserve these source timing differences.

Chunks belong to caller Context and survive later polls. Buffered rows borrow same-domain query results. Open streams hold transaction borrows until EOF or explicit cancel; after is_last callers can fetch EOF or cancel to release access.

DiagnosticStream joins native streams before final observer notification at last chunk/EOF/error/cancel. Count delivered rows only. Notify immediately, not at Context destruction; close supplies cancellation only for unfinished streams. Exceptions classify Failure and explicit cancellation Cancelled. Isolate sink exceptions; callbacks run on owner. Typed error items remain distinct; without diagnostics do not force stream termination merely from provider is_last/error when its interface allows continuation.

## Production SQLite transport

Create `SqliteConnection` with its owner's `make_resource`, and create the executor
with the same domain's `make<SqliteMutationExecutor>(connection)`. Connection,
caches, and executor clones retain that explicit lifetime. A Global connection can
be borrowed by its own UserContexts. Asynchronous operations and streams on a
User connection must use that User domain; Global and sibling destinations are
rejected before operation creation. Synchronous calls may explicitly copy results
to another domain because those results do not retain the connection.

SQLite executes on the Context owner thread. A stream holding a native cursor
queues other asynchronous operations. Cancellation and close remove waiters;
releasing the cursor wakes the next operation. Callers must advance or cancel the
current holder while waiting for queued work. Synchronous contention returns a
Lock error to avoid blocking the only execution thread. This scheduling adaptation
does not reproduce Rust's blocking synchronous mutex. `native_handle()` is a
same-thread native borrow; external statements must finish and be released before
the connection is destroyed.

Parameters, stream state, and delivered chunks belong to the operation domain.
Closing a cursor does not invalidate delivered rows. Connection cleanup finalizes
cached statements, closes SQLite, and then permits arena reclamation. Construction
failures use independent standard C++ exceptions so exception messages cannot
borrow a Context destroyed during unwinding. Execution failures use scoped
`MutationExecutorError`. Unwinding while holding the connection poisons its lock;
ordinary typed errors do not.

The statement cache is a 64-entry LRU keyed by SQL including comments. The column
layout cache uses raw SQL and clears its index at 64 entries. Evicted layouts
remain in their arena for existing row borrows: entry capacity is not a bound on
resident Global arena memory. Cross-domain results copy their layouts. Native
stream `chunk_size=0`, no final chunk for evenly divisible streams, and the
buffered helper's empty final chunk preserve source behavior. Production
transactions, both ID allocators, runtime ID registration, controlled schema
entry, and physical schema are connected; schema leases cover the complete
transaction and event dispatch.

## Production SQLite transaction ownership

`begin_sql` returns a scoped `SqlTransportTransaction` carrier corresponding to
the Rust provider clone's associated transaction. It borrows the same connection
and caches. Its first poll executes `BEGIN IMMEDIATE`; a pending begin can be
canceled. Calling commit or rollback consumes the handle, while SQL executes on
poll. Pending transaction operations prevent consumption. Failed termination
preserves the native error and current SQLite transaction state. The consumed
handle cannot retry; synchronous provider primitives or Context close can clean
up. Synchronous begin/commit/rollback do not create a new Context transaction owner.

Canceling or closing an asynchronous transaction rolls back the native transaction
it still owns. Restored autocommit clears ownership, preventing an old Context
from rolling back a later transaction. Stop transaction operations before rollback
and reclamation. Cleanup can use SQLite's supported rollback with active statements
even when another User's cursor holds the connection, without blocking the owner
thread. That stream retains its own host memory and subsequently reports SQLite
results or abort errors. Do not change transaction boundaries through
`native_handle()` behind the provider's ownership tracker.

## SQLite ID-space generator

Create `SqliteIdSpaceGenerator` through an explicit Context's `make`; it borrows
the provider. Its table name belongs to that domain. Cross-domain copies duplicate
the name and validate that the destination may borrow the provider connection.
Temporary parameters and errors from `next_id` and `ensure_floor` use the caller's
explicit result domain. There is no hidden counter. Allocation participates in
the connection's current transaction, allowing reuse after rollback.

Concurrent callers use independent Contexts and connections per thread; database
CAS coordinates allocation. They do not share arenas or native connections.
`ensure_table` retries BUSY/LOCKED up to 100 times. `next_id` retries locking and
optimistic conflicts up to 100 times at 1 ms intervals. `ensure_floor` retries only
row-count and insertion-constraint races as in the source; other native errors
remain errors. Busy timeout follows the source provider and can be configured
through the native handle. Business IDs use the separate rules below.

## SQLite business ID allocator

`SqliteBusinessIdAllocator` implements the core `BusinessIdAllocator` SPI and
remains separate from internal ID-space generation. The service and table name
have an explicit owner. Returned scope and errors are copied into the requested
domain, which must be able to borrow the connection for failed-commit cleanup.
`ensure_schema` is explicit: constructing or copying an allocator never performs
DDL. The schema-contributor SPI is wired, but installing its standalone table is
not complete runtime bootstrap.

Each allocation uses a short `BEGIN IMMEDIATE`/`COMMIT` transaction. A separate
connection is appropriate when allocation must survive aggregate rollback. A
failed nested begin must not roll back an existing transaction. Validate ranges
before begin; read level and version as strict INTEGER values. Operation failures
roll back. Exhaustion rolls back counter/version changes. Commit failure retains
transaction state for Context cleanup; ownership identity prevents affecting a
later transaction.

An optimistic update count other than one commits before retrying. Lowercase
`locked`/`busy` error messages retry up to 100 times at 1 ms intervals. The source
does not check insertion row count: a trigger using IGNORE can return the initial
sequence without inserting. This behavior is retained.

## Runtime resources and locals

Typed and named resource indexes remain independent. Resources require explicit
origin and may borrow self or the associated Global domain; siblings and foreign
Globals are rejected. Validate an actual allocator when available. For external
RAII wrappers, declared origin is the caller's ownership contract. C++ RTTI names
are compiler names, not Rust paths. Named keys preserve complete strings including
NUL. Replacing a named resource with another type produces MissingResource;
typed absence produces MissingTypedResource. Preserve all three ContextError
prefixes. Const lookup returns a const borrow.

Registries are indexes rather than second owners. Replacing an entry leaves its
old object alive until arena cleanup, unlike immediate Rust Box destruction; this
is a deliberate ownership adaptation. Use `ResourceBorrow` for domain checks;
raw-pointer getters remain caller-managed borrows. User registries are lazy.
During quiescence lookup is allowed, but writes and local removal are blocked until
work stops. Closed lookup rejects before touching the index. Locals copy keys and
nested values into the User domain. Removal invalidates old references; returned
values still belong to the User domain.

## Check models and error ownership

Allocate every nested location, optional Value, JSON tree, SQL error, and concrete
executor error in the explicit result domain. Cross-domain assignment keeps the
destination allocator. Preserve RuntimeError's 14 variants and DataServiceError's
three forwarding categories without adding prefixes. Custom formatters are
independent. These models do not prove complete checker execution or mutation
governance propagation.

Model paths, display paths, native paths, and JSON Pointer remain distinct. Keep
empty components, Unicode uppercase expansion, and escaping order. Wire fields
retain model names; configured naming changes instance paths only. Emit every
nullable wire field. Absent and explicit Null can share a JSON representation
while remaining distinct model states.

## Metadata and data-service registries

MetadataStore and DataServiceRegistry retain separate responsibilities. Explicit
origin validation precedes replacement. Replaced objects remain in their arena;
copying descriptors, properties, relations, and audit metadata is recursive.
`entity` and `all_entities` return borrows; allocating a pointer array in Global
does not extend a User descriptor's lifetime. `require_entity` checks actual
ownership. Descriptor replacement invalidates its old semantic borrow.

Service availability is registry OR metadata presence; it does not imply full
runtime execution. A data-service metadata snapshot belongs to its store domain
and survives later replacement until close. Default query observers depend on
metadata capture independently of debug; mutation observers remain available.
Captured store borrows are checked. Context log methods are implemented; complete
business orchestration is still pending.

## Business ID keys, services, and clocks

Key version and 32-byte secret are value copies. Never format the secret. Plans,
scopes, encoded results, and errors use the requested domain. V1 temporary buffers
belong to the current call; HMAC uses fixed stack storage without TLS, caches, or
new dependencies. Preserve length prefixes, cycle walking, version handling, and
error order; this implementation is not a general cryptographic API.

Schema contributors are independent SPIs. DDL requires an explicit call, never
construction or copying. Context schema ordering is wired. BusinessClock is an
explicit borrow; the default system clock is stateless. `UtcDateTime` preserves
seconds, nanoseconds, chrono's complete range, and leap-second behavior rather
than narrowing to Timestamp. BusinessDate overrides only the date. Clock reads
are permitted during quiescence; replacement is blocked and closed reads reject.

BusinessIdService has explicit origin and borrows allocator and key provider.
Rust shared-service construction maps to explicit allocation and registration,
without lifetime extension. Clones share service state only where the borrow is
valid. Global-retained generated results and errors require copies; User services
cannot escape to Global. Key-provider copies retain independent configuration
snapshots when a provider is replaced.

The in-memory allocator owns its scoped table; callers own results. Several Users
can borrow one owner on its owner thread, but cannot concurrently write its arena.
Unwinding poisons the mutex state. Validate ranges before checking poison. A new
scope that overflows is not inserted. Formatting failure after allocation does
not restore the consumed sequence. Passive infrastructure installation registers
service and contributor without DDL. The SQLite service pipeline is connected.

## Internal IDs and bootstrap initialization

Global/User generators own scalar AtomicCounter or Snowflake state. Worker results
must use each caller's arena, and workers must finish before resource close. This
does not make Context arenas thread-safe; SQLite remains owner-thread-only.

`generate_id` may return None. `next_id` then lazily uses one Global fallback
counter shared across entities; different Globals are independent. Configured
generator errors never fall back. Bootstrap requires an InternalIdGenerator,
ensures its floor, then marks new; it neither assigns an ID nor performs the first
mutation. SQLite error display maps to RuntimeIdGeneration without extra text.
A nonvirtual `ensure_floor` delegates to virtual `do_ensure_floor`, retaining the
same SPI for the SQLite implementation.

AtomicCounter starts at 1000 and fetch-adds before adding one to the result. Debug
throws after state wraps; Release wraps. Snowflake retains its epoch, 5/5/12-bit
layout, strict clock-rollback waiting, sequence exhaustion, and error/state-update
order. It uses the system clock, not BusinessDate.

## Controlled schema invocation and bootstrap scope

SchemaProvider remains separate from the data-service schema interface.
`SchemaInvocation` is private, nondefault-constructible, noncopyable, and a temporary
borrow. User `ensure_schema` is lazy: first poll gets the provider, awaits it, calls
the business schema service synchronously, then runs bootstraps sequentially.
The first failure stops execution; business errors display as SchemaError.

A pending invocation holds an immutable Context borrow. Resource setters, locals,
and module replacement reject until success, failure, or cancellation releases
it. Const `make` may mutate allocator internals without changing configuration.
Close is allowed. Bootstrap mode uses a transient Global guard around synchronous
callbacks, poll, and cancel. Nested work in the same Global inherits it; another
Global does not. There is no TLS authority retained across suspension.

Re-register the parent's quiescence callback after a Pending child so cancellation
runs inside the outer scope, matching Tokio drop ordering. Ordinary interleaved
work sees normal mode. RuntimeModule copies metadata, entity names, and function
pointers into the destination; closing the source or changing it later does not
change that snapshot. Behavior and decoder registration are implemented; complete
checker execution and language integration remain pending.

## Audit and SQL log ownership

Audit policy, EntityKey cached identity, and all nested snapshots use their owner
allocator. Global retention requires explicit copies; assignment preserves the
destination. Default PMR can remain disabled. Float Debug follows Rust.
Sink SPIs stay independent. Registration validates origin; Global collections
reject User and sibling borrows. Collected events are copied and checked. External
RAII wrappers retain explicit resource ownership. Projection copies without
executing mutation. Environment is read per call, with no Context-owned process
cache; LogManager owns the warning state. Event dispatch is wired, while complete
business-save audit timing is pending.

SQL log payloads and UnifiedLogBuffer recursively propagate allocators. Registration
checks borrows; insertion copies entries. Private safe projection stores SHA-256
and protected state, not original bindings or ownership cycles. Same-domain
cloning is a borrow; cross-domain cloning rebuilds state. A Global copy remains
safe after User close. Tampering falls back to safe projection. Private Debug
state and intent hashing use fixed stack storage without default PMR.

Formatters take explicit Memory for every string. The Global factory is lazy and
reads configuration once per application; independent Globals choose independently.
Factory output must belong to that Global, never a User or foreign Global.
Configuration vectors/strings copy recursively; assignment retains its destination.
Formatting does not retain inputs. Global-retained output uses a Global target or
an explicit copy. Native tests disable default PMR and read copies after close.

LogManager is Global-lazy and owns configuration, endpoints, headers, and warning
state without retaining User payloads. Scratch belongs to the same Global family
and is validated. Files close before return. Repeated large User requests reclaim
all User blocks without growing Global response-body storage. The memory-layer
ContextArena borrows Global state without owning runtime log registries or TLS
caches; direct application projection state can exist independently of an arena.

Global destruction closes its arena while fields remain valid. Derived arena
destructors close while their dynamic type is still available to callbacks.
Ordering is quiesce, object destruction, block reclamation. Native tests cover
stdout poisoning and close-time output.

## Context identity and log recording

Capture identity at construction in the Context allocator. Preserve empty explicit
user identity; invalid or missing environment USER/USERNAME falls back to native
thread ID, a documented C++ adaptation rather than Rust ThreadId equality.
User SQL history is lazy. `sql_logs` creates independent snapshots; Global retention
requires an explicit vector copy. Global unified buffering validates actual
allocators and never retains User string views. Clear and disable affect history,
not entries already in the unified buffer.

Recording is allowed during quiescence. Save metadata before emitting output so
output failure retains records. Configuration setters require an open Context
without a schema borrow. Reads and final diagnostic recording are permitted while
stopping work.

## ContextBox, telemetry, and native work

ContextBox allocates payload bytes in the arena and may destroy its payload early.
Reset clears the pointer before destruction to prevent double cleanup. Ownership
claims cannot be copied; consumed boxes reject reuse. Bytes remain until arena
close. External resources still need actual RAII destruction.

Default telemetry belongs to User; injected telemetry may borrow self or Global.
Start/extract creates a fresh scoped box. Factories reject the wrong domain and
consumed delegates fail open without stealing ownership. Inputs are borrows;
retention requires copies. Completion takes the delegate, isolates the callback,
then resets. A poll panic poisons scope and suppresses its completion callback;
close still destroys the payload. Propagation does not poison in the same way.

Run-scope delegates borrow self/Global; child operations use the same domain.
Cancellation stops and joins work, detaches wake forwarding, then permits delegates
and arena memory to be destroyed. All quiescence precedes ordinary destruction.
Native tests cover both wrappers, null default PMR, input reads, wakes, completion,
cancellation, and Context close.

## Governance activation and event dispatch

Governance snapshots belong to User. Global activation exists only during
synchronous poll/cancel and restores via RAII; it never remains active across
suspension. Parent quiescence establishes scope before child cancellation, joins
native work, then releases payloads.

Event input is a borrow and its working copy belongs to User. Safe and raw inputs
may be borrowed by callbacks; Global retention copies every nested governance,
module, and event value. Sink collection copies borrows, rather than cloning an
external resource owner. Registration validates origin. Errors and exceptions
still close telemetry. Stack PMR values must be destroyed before their Context
closes.

## Entity values, graph tokens, and trace

EntityValues, relations, dirty fields, comments, and originals propagate allocators.
Root getters return borrows. Module installation copies into an independent target
that remains valid after source close. Graph tokens are created with `Context.make`.
Parents may come from self or the same Global, never a sibling or shorter domain,
and remain immutable. Explicit recursive copies preserve shared batch borrows;
there is no owning reference count. Tokens and plans become invalid together.
Stack CommentNode may borrow a parent; trace output chooses its destination and
uses no static Context storage.

## Physical SQLite schema lifetime

`use_sqlite_provider(ctx, origin, executor)` validates self/Global origin and
installs the User dialect/provider passively. Only explicit ensure performs DDL.
Lazy operations own metadata pointers and a connection lease. Canceling a pending
operation removes its wait node before reclamation, leaving no User wake pointer
in a longer-lived connection. One lease covers soundex, foreign-key setup,
BEGIN, DDL/events, and COMMIT.

Sent events are not retracted by rollback. Sink Result errors are ignored as in
the source; thrown exceptions poison the connection. Commit failure rolls back.
Successful completion clears caches before rejecting a legacy root/initial graph,
so already committed effects remain. Temporary SQL, Unicode identifiers, maps,
foreign keys, and indexes recursively allocate in User. `table_columns` explicitly
copies to its result domain. Statements finalize via RAII. Soundex captures no
Context. Its static Unicode facts contain no Context data or ICU dependency.

## Identity graphs, mutation ledgers, and copying

State owns slot, ledger, and graph sharing within its domain. Lazy ledger lookup
for loaded reads does not allocate. Frozen state owns type-erased payload; replacement
destroys the old payload immediately. Close destroys graphs, ledgers, and payloads
before blocks. Handles are borrows. Weak graph views restrict publication authority
without extending memory lifetime.

Explicit Global copies duplicate all graph, ledger, and original state. A temporary
copy map preserves cycles, inverse edges, and aliases within that copy. The map
belongs to the target arena during synchronous copying and unwinds cleanly; no
TLS or permanent source pointers remain. Allocator-aware payloads copy recursively;
plain values copy normally. Move-only native payloads may be installed in the same
domain but cannot escape by pretending to copy.

Fresh shared graphs create a new ledger; `with_shared_graph` preserves the receiver's
ledger and loaded snapshot. Composition validates every version before merging
and leaves the source unchanged. Independent copies have independent graphs;
aliases inside one copy remain aliases.

## Behavior, policies, decoding, and entity-service borrows

Behavior, RequestPolicy, and domain-rule module installation checks all borrows
and fails atomically without a partially installed target. Decoder names and
function pointers copy into the target. Root builders must use the decoder's
domain. Fresh weak graphs and ledgers belong to the result Context.

EntityService binds to User. Executors can borrow self/Global. Metadata, behavior,
and policy cannot retain sibling or shorter borrows through cloning. ContextDataService
keeps its metadata view in User so operations survive a temporary service wrapper,
not Context close. Query snapshots and child operations retain checked borrows;
wake cancellation and native join precede memory cleanup.

Observers and intent snapshots belong to Context, not temporary repositories.
Captured callbacks cannot escape across domains. Descendant privacy snapshots do
not mutate siblings. Relation plans recursively allocate in their domain; explicit
Global copies remain independent. SmartList same-domain moves are cheap;
cross-domain assignment deep-copies.

Context observation activates scopes only during poll. Finish consumes and destroys
the delegate. Cancel joins the child before freeing scopes. Wake state belongs to
the owner. A foreign child is rejected without canceling that foreign operation.

## Relation execution, hydration, and enhancement

Relation execution owns parent rows, plans, IDs, and results in User. Snapshotting
isolates later caller plan changes. Callback borrows cannot escape. Probe children
forward wakes; reset stops work; the first error stops execution. Global result
retention requires a final copy, never a hidden owner.

Hydration uses one plan tree with stable child-repository descendant borrows,
avoiding repeated whole-tree copies. Parent-row spans and root graphs use the same
User domain and remain valid without resizing during execution. Updates are in
place, not hidden clones; even read-only foreign parents require explicit copying.
Cancel joins the active child before releasing telemetry and embedded state.
Failed `mem::take` paths leave empty state while prior ordinary/compact installation
remains. Native completion/cancel/close tests cover all three paths and read only
explicit Global copies afterward.

Enhancement snapshots query/options/trace and parent input. It copies indexes,
retains last-duplicate-target behavior, and isolates overlapping input rows.
Completed steps remain after failure; pending steps do not mutate results.
Null-default-PMR copies and native tests check all paths. Propagating cache options
does not create an external cache owner or complete cache orchestration.

## Query stores and same-key ID-set coordination

Default User stores are lazy and have no hidden Global owner. Global sharing uses
an explicit factory/setter and validated origin. Replacing a store affects future
lookups, not existing operations. Old objects remain in their arena. Invalid domains
reject. Put/get copy recursively and independently; canceling an unpolled write
does not write. Native custom stores stop and join before store destruction and
arena reclamation on completion, cancellation, and close.

ID-byte capacity is a logical limit, not arena resident memory. Erasing an entry
destroys its values; its blocks remain until the owner's close. The build registry
belongs explicitly to Global and allocates its keys and waiter slots there.
User-owned leases, wait states, and guards do not extend Context lifetime. Pending
nodes borrow User wake state and are removed on finish, cancel, or quiescence before
reclamation. Unlock wakes one occupied slot; closing a notified User transfers its
wake to the next waiter.

Native build work created after its guard registers dependencies so cleanup joins
the child, releases the guard and lease, destroys objects, then returns blocks.
Native tests read User input and confirm the guard remains held before join; only
then can another Context acquire. Guards cannot escape to Global. After release
and lease reset, the next registry lookup prunes zero-lease entries. Global blocks
are finally returned together at Global close. This validates coordination;
cache-miss rereads, ID construction, and fallback query orchestration remain pending.


## Query-key temporary and retained data

Canonical query/key helpers take explicit destination Memory. They copy the input
before normalization; sorting uses domain-owned cached strings and index vectors,
without a fallback allocation from stable_sort. Every nested subquery, descriptor,
relation, object group, and child enhancement follows the destination allocator.
A User source can be closed after an explicit Global result copy. Root/user identity
views are used only during the synchronous call and are not retained.

Key building does not extend an input's lifetime or create a hidden registry/cache.
Temporary bytes remain in their selected arena until close, just like other
monotonic allocations. Query Debug contains values and must remain internal to key
construction rather than bypassing log privacy projection. Native contracts use
null default PMR and verify source immutability, recursive Global retention, and
complete User reclamation.
