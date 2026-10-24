# SQLite provider root source review: Rust 5.0.6

Status: pre-acceptance responsibility review. The whole source remains
`pending_review`. Passing the focused lease repair or the root tests does not
establish complete provider acceptance.

The reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
`teaql-provider-sqlite/src/lib.rs` has 3,970 lines: definitions and production
helpers through line 1,858, followed by fixtures and 42 root tests. All original
production and test bodies have been read for this review. The independent
transaction/dynamic-storage modules and external integration test files retain
their own source dispositions. Historical 5.0.5 acceptance remains historical.

## Complete production responsibilities

| ID | Source responsibility and required native disposition |
| --- | --- |
| SP01 | Four public constants preserve two distinct ID tables and two 64-entry cache capacities. `SqliteTransaction` remains a separate transaction protocol, not an executor clone used as an ungoverned transaction. |
| SP02 | Stateless Dialect Debug/Default/Clone/Copy and six overrides retain SQLite kind, quoting, question-mark bindings, small-parent probes, all ten physical types and SQLite ADD COLUMN without generic fallback defaults. All inherited SQL hooks remain separate. |
| SP03 | Foreign-key discovery resolves first named target, skips missing/cross-service targets, chooses direction by `many`, validates source key before referenced key, and sorts/deduplicates physical triples. |
| SP04 | Physical CREATE TABLE validates each property through the dialect, then appends complete foreign-key clauses; it does not run generated bootstrap graphs. |
| SP05 | Six provider error alternatives, both conversions, raw Debug, exact Display and empty outer source remain distinct from SQL executor errors. Driver payload and SQL compile error contracts remain independent. |
| SP06 | Executor constructors initialize prepared caches, independent layout cache, shared transaction lease and nonreused storage identity. Same-owner C++ clones borrow identical state; connection/provider/output owners are separately checked. Explicit Context resources replace Arc ownership without extending UserContext lifetime. |
| SP07 | Private test schema helpers and production physical schema share the same responsibilities. C++ does not restore a public executor-owned legacy schema/bootstrap API. |
| SP08 | Physical setup registers SOUNDEX and foreign keys before BEGIN IMMEDIATE. ID table, table/column evolution, indexes and final foreign keys retain order. Structured failures roll back; commit failure attempts rollback; success clears both query caches after releasing the native guard. |
| SP09 | Production schema emits created/field-added/verified events at the original pre-commit stages and ignores returned event errors. Fatal sink exceptions, lock poisoning, resource cleanup and broader publication/privacy require their independent controls. |
| SP10 | Foreign-key inspection validates the whole installed key: ID/sequence/cardinality and both restrict actions. A matching first part of a composite key or CASCADE cannot satisfy the source. Missing and incompatible keys retain different diagnostics. |
| SP11 | Table existence and column inspection use actual SQLite. Column names receive source Unicode lowercase; primary-key and NOT NULL flags determine nullability. Required-column additions reject populated tables before ALTER. |
| SP12 | Type compatibility preserves all ten families and checks type, exact nullability, text width and numeric containment in order. Declared arguments filter individual invalid numbers; quote stripping here also supports brackets. |
| SP13 | SOUNDEX is a deterministic one-argument UTF-8 function with the original ASCII code table, duplicate/vowel rules and `?000` fallback. Nontext/invalid inputs do not become implicit application strings. |
| SP14 | Synchronous execute/fetch converts values before native locking. Affected counts, exact compact rows, comments, statement preparation and errors remain real provider operations. Root asynchronous entries await their separate lease first; the scoped repair below qualifies this previously missing boundary. |
| SP15 | Repeated fetch locks/prepares once, resolves one layout, then replaces and binds the indexed value for every input. Preserve order/duplicates, prepare errors on empty input, missing-index preconditions and native-lock-before-bind precedence. |
| SP16 | Buffered fetch_stream always adds its terminal chunk, including empty/exact-boundary results. Native stream emits only a nonempty tail and holds its cursor/lease across yields. Zero size retains each interface's original behavior. |
| SP17 | Native cursor buffering allocates nothing for empty results, reserves an initial `min(chunk_size, 4)` prefix, then grows to the known bound rather than geometric excess. The SC01-SC06 source checkpoint now qualifies the shared-loop repair and complete allocation test source; it does not accept the other provider responsibilities. |
| SP18 | Dynamic stream admission validates the store before chunk size and opening a cursor. Each private chunk loads owner IDs on the same connection and merges before publication; stable errors omit stored values. Root and transaction stores retain different write capabilities. |
| SP19 | Query-stream compilation uses the dummy entity descriptor and returns one compile error followed by EOF. It does not invent an observed-stream diagnostic override or bypass Context-level validation. |
| SP20 | The runtime schema provider resolves typed dialect then executor, performs physical DDL, and only then rejects legacy root/initial graphs at the typed bootstrap boundary. Provider installation is passive; explicit Context schema invocation owns initialization. |
| SP21 | Internal IDs retain default/custom table, same-owner clone, canonical entity names, strict native INTEGER reads, first ID, checked increment, optimistic compare-and-set and exact overflow/negative diagnostics. |
| SP22 | ID table/next-ID retries handle only source busy/locked cases, at most 100 attempts with one-millisecond sleeps. Floor conversion follows table setup; floor updates retain their different optimistic retry/error behavior rather than inheriting next-ID retry rules. Runtime error adapters preserve the ID-generation category. |
| SP23 | Business-ID allocation retains explicit schema, independent configured connection, scope canonicalization and range validation before I/O. BEGIN IMMEDIATE, sequence/version compare-and-set, first sequence, exhaustion and negative values retain their exact branches. |
| SP24 | Business-ID operation errors roll back; successful/empty attempts commit; commit failures preserve their separate recovery responsibility. Source message-based locked/busy retries and allocation-category adaptation remain distinct from internal IDs. C++ registered cleanup must run before arena reclamation. |
| SP25 | Declared indexes check installed table, uniqueness/partial flags, ordered full-column keys, ascending direction and BINARY collation. Collect all pragma payloads before deciding shape errors; preserve actual malformed-driver extraction failures. |
| SP26 | Binding covers every Value variant: bare/typed NULL, Boolean, checked U64, F64, scaled Decimal text, JSON text, Date and timestamp integers. Object/List remain unsupported; parameter cardinality and driver errors are not rewritten as model errors. |
| SP27 | Immutable ColumnInfo/decode kinds/ColumnLayout preserve names and shared compact-row layout. Layout cache keys un-commented parameterized SQL, returns same cached layout and clears on capacity before inserting. Explicit destination relocation preserves row ownership. |
| SP28 | Decoding preserves native NULL/INTEGER/REAL/TEXT/BLOB behavior. Declared Boolean integers are nonzero-based; legacy inferred integers remain integers for typed binary-Boolean validation. UTF-8, Decimal, JSON, Date and timestamp failures retain distinct messages. Text does not infer JSON/time. |
| SP29 | Infer tries date, RFC3339 and naive datetime before retaining text. Timestamp fast/fallback paths preserve fractions, offsets, calendar bounds, leap-second behavior and exact errors. Checked-Debug eager unsigned-subtraction panics map to the existing C++ exception; release behavior requires its separate evidence. |
| SP30 | Every native resource, pending operation, cursor, cached statement, layout and retained row obeys the two explicit Context domains. Cancellation and actual Context deletion quiesce work before reclaiming storage. Synchronous owner-thread/leased-connection adaptations remain explicit, not claims of Rust scheduler equivalence. |

## Current focused evidence and remaining work

The [lease/binding review](sqlite_provider_lease_binding_review_5_0_6.md)
reproduces twelve actual compiled-provider differences, records 87 exact original
outcomes and qualifies the shared sequencing repair. Its gates retain the
existing 144 valid-binding lease observations and actual transaction Context
deletion. This addresses SP14/SP15 and the corresponding stream error order;
it does not accept every row above.

The unchanged original root passes
[42/42](parity/sqlite_provider_root_tests_current_5_0_6.json.gz).
Its complete source test bodies map to four retained C++ ports: storage (15),
schema (17), query/rows (7) and transaction/collision (3). The current native
selection executes all four. Parametrized required-column and buffered-stream
helpers preserve both/three original tests, and the query port preserves the
four-connection, 25-ID-per-worker test rather than substituting the separate
larger ID stress test. The old raw transaction/TOPN fixtures now use the exact
original intent literals. Their remaining bodies are unchanged, and all four
ports also execute through canonical import consumers.

The SP17 comparison inspects actual public chunk capacities, not merely row
counts. The original external
`stream_chunk_allocations.rs` explicitly checks 223 rows with chunk size 73:
three capacities of 73 and a final capacity of 4, early-drop reuse, seven measured
row/chunk configurations and zero-size compatibility. The
[SC01-SC06 checkpoint](sqlite_chunk_capacity_source_review_5_0_6.md) reproduces
250/858 actual differences, repairs the shared loop and qualifies all three
original bodies plus native Context deletion. All 858 exact comparisons,
native 7/imported 6/targeted sanitizer 6 pass, retaining the independent 87
binding controls. Complete provider source acceptance remains pending.

## Complete-root replay and module registration repair

The [fresh frozen execution](parity/sqlite_provider_source_repaired_5_0_6.json.gz)
passes **native 27/27 and imported 27/27**, with **15,676 exact source outcomes
per frontend** and final selected no-work builds. The unchanged original root
passes all **42 tests**. The independent observer workspace uses **129 registry
identities** whose versions/checksums match the reference lock. Original drivers
and immutable corpora remain unchanged; no expected outcome is normalized.

| Responsibility family | Exact observations per frontend |
| --- | ---: |
| Physical schema | 735 |
| Native value decoding | 6,029 |
| Parameter binding | 1,084 |
| Provider error model | 563 |
| Native driver diagnostics | 5,810 |
| Simple error alternatives | 62 |
| FromSql error Display | 36 |
| FromSql raw Debug | 36 |
| Transport, buffered/native stream and units | 798 |
| Internal IDs | 261 |
| Business IDs | 198 |
| Checked timestamp short-circuit controls | 64 |

Raw driver Debug is native-64-bit evidence. Timestamp subtraction failures use
the explicit checked-Debug source panic/C++ exception observation; this gate
does not relabel unchecked or MIPS evidence. The retained stream-capacity,
binding/poison and asynchronous-lease comparisons run as additional checks.
Native transaction, dynamic storage/query, error ownership and actual Context
deletion tests preserve their independent protocol/lifetime responsibilities.
There is no new production change or sanitizer claim in this checkpoint.

The [initial registration gate](parity/sqlite_provider_source_registration_gap_5_0_6.json.gz)
rejects an otherwise passing 26-test imported selection because the expected
asynchronous-lease comparison is absent from its actual test names. Registering
the existing comparator in modules/CMakeLists.txt closes that omission. The
unsuccessful frozen epoch and its JUnit remain preserved. The fresh epoch above
requires all 27 exact names, rather than accepting CTest's zero exit alone.

The [preservation proof](parity/sqlite_provider_source_preservation_5_0_6.json.gz)
protects 10,650 preexisting execution inputs. It reconstructs the previous files
exactly by removing thirteen canonical module registrations/targets, the single
lease-comparator registration and the two corrected intent literals. Three new
files provide the verifier and independent Rust manifest/lock. Production
functions/headers, independent producer, generated SDKs and both references
are unchanged.

The [static gate](parity/sqlite_provider_source_static_5_0_6.json.gz) verifies
1,708 formatted files, 1,516 exports, 1,027 canonical consumer/fixture bodies,
module paths, the locked generator ledger and diff whitespace. The subsequent
[registry check](parity/sqlite_provider_source_registry_5_0_6.json.gz) discovers
782 native and 587 imported cases after the registration repair. These counts
are inventories, not full-suite results.

## Caller and lifecycle dispositions

The [read-only caller review](parity/sqlite_provider_source_caller_lifecycle_review_5_0_6.json.gz)
binds fifteen source/test excerpts to the completed 27/27 native and imported
gate. All 10,653 execution inputs remain unchanged. These are reviewed ownership
and caller dispositions, not another runtime or sanitizer execution.

| Boundary | Reviewed disposition and executed controls |
| --- | --- |
| Connection, provider and clone ownership | The connection owns SQLite and its statement cache; the provider separately owns layouts and storage identity. A same-domain clone borrows identical state. A distinct provider on the same connection gets a different identity. Transport and dynamic-storage tests exercise both identities and reject an implicit cross-domain clone. |
| Identity issuance | The provider uses the arena's nonreused namespace/slot token. The two 32-bit parts form a 64-bit comparison identity; exhaustion fails explicitly rather than wrapping. The process scalar issuer owns no Context payload. This is the bounded C++ allocation-identity adaptation, not a pointer resolver or implicit lifetime extension. |
| Deferred operation retention | Async entries check both connection and provider owners before retaining them. User-owned resources cannot escape into Global or a sibling UserContext. Global resources can serve a bound User operation. Synchronous stack-local work returns destination-owned values and does not retain a provider after returning. Transport tests disable default PMR and check rejected destinations. |
| Layout and statement caches | Prepared SQL includes its comment; layout keys omit it. Their independent 64-entry eviction/clear rules remain distinct. Rows relocate layout/string storage into the requested domain; cache invalidation changes future decoding without invalidating previously returned rows. The transport cache tests verify stale-schema decoding, explicit clear and retained old rows. |
| Schema event and failure ordering | Physical DDL emits schema/field/verified events before COMMIT and ignores returned sink errors. A later DDL failure rolls back SQL without retracting an event already delivered. A sink exception propagates, preserves the active transaction and poisons subsequent provider locking. Structured failures and failed COMMIT attempt rollback; successful physical setup releases the lease and clears both caches before rejecting untyped bootstrap graphs. All branches have direct schema controls in the completed gate. |
| Transaction and allocator commit failure | The independent consumed native transaction rolls back on failed commit/drop before releasing its lease. Business-ID allocation instead preserves a failed COMMIT's native transaction for explicit caller recovery. Its registered recovery identity only claims transactions it started; old Context cleanup cannot roll back a newer save. Transport and business-ID tests exercise both protocols rather than merging them. |
| Quiescence and native teardown | Stream cancellation/EOF and Context close remove waiters, finalize cursors and wake surviving operations before arena reclamation. The connection's native cleanup hook observes live Context blocks during cleanup, then no blocks after close. The independent capacity gate additionally performs actual Context deletion with a live cursor and a waiting transaction. |
| Service and privacy boundaries | Provider transport remains beneath the independent SQL executor and runtime. The executed integration body checks observed query/mutation, authoritative readback, stream diagnostics and transaction rollback through those layers. Physical schema events preserve their original Context dispatch; this source review does not qualify every governed save, delivery sink or protected trace caller. |

These dispositions preserve the explicit owner-thread C++ scheduler boundary.
Synchronous work on an occupied connection returns a lock error; pollable work
queues and wakes. They do not claim that sharing a Rust Mutex connection between
threads is equivalent to sharing a C++ Context resource. Independent connections
retain the original concurrent-ID test. Raw native pointers and ordinary C++
references remain borrows, invalid after their owner closes.

Complete root qualification still requires the current full integration gate
before promoting this file. Adjacent runtime/event/graph/transaction sources
and protected publication callers retain their separate acceptance requirements.
The six partial capabilities, 56 pending source files, original behavior
decisions and every P0-P7 exit remain required. No new MIPS/physical-device,
external-provider, GPU, generator or complete migration acceptance is claimed.

Reproduce this scoped gate with a fresh receipt path:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_sqlite_provider_source_5_0_6.py \
    --record /tmp/sqlite_provider_source_fresh_5_0_6.json.gz
```

## Current native epoch follow-up

The running `native_provider_closure` epoch preserves frozen executable inputs.
Its build reports `-Wreturn-type` in the success paths of
`tests/parity/generated_constant_variants_probe.cpp` and
`tests/parity/document_round_trip_probe.cpp` when included by other probes with
`main` renamed through a macro. The implicit return rule applies to the real
main function, not these ordinary renamed functions. Add explicit success
returns after the epoch reaches a terminal state, then format and check the
affected standalone and including consumers. Do not change the files during
the frozen epoch or call an unfinished build successful. This is a fixture
follow-up, not evidence that provider source acceptance is complete.

## Frozen native integration result (2026-10-21)

The full native build completed all 1,394 build steps. The pre-test and post-test
incremental builds reported no work. CTest executed all 869 registered cases:
866 passed, three failed, none skipped. Generated API positive/negative compiler
contracts completed successfully. All 11,367 frozen runtime/producer inputs and
both locked reference revisions remained unchanged throughout this epoch.

The immutable receipt is
`parity/native_provider_closure_terminal_5_0_6.json.gz` (130,345 bytes), with
input snapshot `parity/native_provider_closure_inputs_5_0_6.json.gz`.
The task-owned temporary log/JUnit directory was automatically removed after
terminal capture. This is a failed integration checkpoint, not acceptance.

The failures are:

- `provider_relation_membership_contracts`: nested projected count/load state;
  the exact original Rust test requires these results. Preserve its assertions
  and diagnose the candidate's actual graph state.
- `original_runtime_context_contracts`: the exhaustion fixture assumed two new
  graph-slot identities; the current runtime lazily creates one shared slot.
  Exercise actual exhaustion by filling the configured quota before calling the
  factory, preserving the requirement that previously published references stay
  usable and Context destruction returns all memory.
- `entity_service_contracts`: an old assertion required absent runtime state on
  ordinary entity reads. Locked `resolved.rs::fetch_entities_internal` delegates
  to `registry::decode_compact_rows_with_read_metadata`, which installs read
  context. Verify the installed state and original loaded row instead.

Follow-up edits and focused verification are separate from this immutable epoch.
No provider/source/capability disposition is promoted by this result.

### Compact relation fetch follow-up

The focused reproduction separates parent presence, count presence, count value
and projection load-state assertions. The first failure is the noncyclic,
logging-disabled case: the parent exists but contains only `id`, `version` and
`__teaql_partition_rank`; `selected_count` is missing. This is not solely a
cyclic identity overwrite.

Locked `resolved.rs::fetch_all_internal` deliberately allows a direct provider
read when the only enhancement is a relation aggregate. Its separate
`fetch_compact_all_internal` includes `relation_aggregates.is_empty()` in that
fast-path condition. Compact relation hydration calls the latter. C++ had used
one ordinary `fetch_all` callback for both modes, losing this caller distinction.

The candidate adds the independent infrastructure entry
`EntityDataServiceAccess::fetch_compact_all` and routes compact enhanced relation
fetches through it. A compact-entry flag lets the existing row execution state
machine consume the child query's relation aggregates. Ordinary `fetch_all`
retains its source-shaped direct-read behavior; SQL/provider/transport and
Context ownership responsibilities remain separate. No representation opt-in
was added to the handwritten fixture to hide this execution-path defect.

Focused native validation now passes `provider_relation_membership_contracts`,
including its original count, projected load-state, cyclic relation, physical
SQL count and trace-route assertions. The two corrected stale integration
fixtures separately passed `original_runtime_context_contracts` and
`entity_service_contracts` before this shared-header repair. Format/check passes
1,800 files. This is focused evidence only: current-header ordinary-query and
relation regressions, imported-module qualification and any relevant lifetime
checks remain required before accepting the repair or superseding the failed
full-native checkpoint. The original immutable receipt is retained unchanged.

The follow-up native regression receipt
`parity/compact_relation_fetch_native_5_0_6.json.gz` records seven passing tests
built against the repaired headers: the three integration failures, public and
private aggregate scope comparisons, flat-predecode comparison, and complete
relation-enhancement comparison. Thus the ordinary-query and predecode controls
continue to pass alongside restored compact nested counts. Six already
registered imported counterparts are being rebuilt and replayed separately;
`entity_service_contracts` has no registered imported counterpart in this gate
and is not counted as imported evidence. No complete-suite acceptance is claimed.

The imported follow-up has now completed: all six equivalent commands pass in
`parity/compact_relation_fetch_modules_5_0_6.json.gz`. These include public/private
aggregate scope (8/8 each), flat predecode (20/20), complete relation enhancement
(754/754), the original Context fixture and the full provider relation-membership
fixture. Canonical module verification passes all 1,136 consumer/fixture bodies.
The three focused sanitizer contracts are being rebuilt separately; their status
must be read from the terminal sanitizer receipt before claiming qualification.

The focused ASan/UBSan follow-up also completes successfully: 3/3 contracts in
`parity/compact_relation_fetch_sanitize_5_0_6.json.gz`, with leak scanning disabled
by the committed preset. Native, imported and sanitizer receipts bind the same
six changed runtime/fixture input hashes. Stable preset directories
were reused, with no copied binaries or disposable build directories created.
This closes the focused repair gates, not the whole provider/resolved source or
P0-P7 acceptance. The original full-suite 866/869 result remains historical
failed evidence until a later complete integration checkpoint supersedes it.

## Current complete imported-provider replay

The [current imported receipt](parity/sqlite_provider_source_current_module_closure_5_0_6.json.gz)
rebuilds the canonical provider consumers and matches **15,676/15,676** complete
outputs across the eleven original matrices and 64 checked timestamp controls.
It also passes the exact **27/27** CTest selections: all 22 unit/ownership
selections and five chunk/binding/lease controls, with no failures/errors/skips.
The source root's four C++ ports retain the 15/17/7/3 mapping to the original
42 test bodies. This is a current candidate replay against retained actual
reference outcomes, not a claim that Rust was recompiled in this continuation.

The earlier complete source gate's original source hashes still match the locked
reference, as do every retained corpus and declared original driver. The
controller records those bindings before compiling the current C++ consumers.
It retains the checked decoding-profile assertion, full outputs, exact command
paths, binary/corpus hashes, current registry and JUnit, and successful no-work
builds before/after execution. Canonical verification passes 1,163
consumer/fixture bodies. All 11,412 execution inputs and both locked references
remain identical to the concurrent full native epoch.

The owned controller/JUnit directory (13,623 bytes) was removed after all child
processes exited. Stable module and native build directories are reused. No
production function, C++ test assertion, generated artifact or original source
changes in this replay; no new sanitizer, MIPS or physical-device execution is
claimed. The complete native build succeeded and its CTest phase finished at
**876/877**, with only the obsolete stream comparator failing. The
[strict-source readback](protected_stream_shapes_5_0_6.md) records that failure
and its repair. This source remains pending until complete integration has a
passing terminal result and the complete SP01–SP30 acceptance is recorded.
