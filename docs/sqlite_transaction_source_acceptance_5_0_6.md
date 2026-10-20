# Complete native SQLite transaction source: Rust 5.0.6

This review covers the complete 94-line
`teaql-provider-sqlite/src/transaction.rs` and complete 160-line
`teaql-provider-sqlite/tests/transaction_lease.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. It preserves native SQL transactions
as a provider responsibility, separate from data-service transactions and graph
Save. The missing native streaming interface is repaired before qualification.
Source dispositions now stand at **27 accepted / 95 pending / 2 excluded_external**;
the seventeen capability statuses remain unchanged.

## Complete responsibility mapping

| ID | Locked source responsibility | C++ implementation and evidence |
| --- | --- | --- |
| ST01 | Associated transaction type and exclusive begin | SqliteMutationExecutor's begin operation allocates the private SqliteNativeTransaction in its explicit Context, acquires the existing connection lease, begins SQLite work and activates its identity. A second begin and root operations wait. Native scheduling remains lazy and Context confined. |
| ST02 | Executor clone and non-Clone transaction ownership | The private transaction borrows the Global-owned provider state; its SqlTransaction base deletes copying. Context ownership replaces Rust's shared executor carrier and operation-owned mutex guard. No additional owner or allocator domain is introduced. |
| ST03 | Compact fetch, repeated prepared fetch and execute | Public transaction entry points retain source result/error types and delegate to existing SqliteCall/SqliteRepeatedCall with the transaction's actual identity. Repeated probes preserve value order and reuse the held lease. The unchanged 144 lease and 366 ordered historical transaction inputs match actual Rust and both installed frontends. |
| ST04 | Optional dynamic-field store exposure | dynamic_field_store returns this transaction's DynamicFieldStore interface. The full existing SQLite dynamic-storage body passes native, imported and targeted sanitizer gates, including transaction-bound storage and cleanup. This preserves the other interface across the added streaming base; it does not qualify every dynamic-field source file. |
| ST05 | StreamingSqlTransport and unleased delegation | The private native transaction now implements the existing optional streaming protocol. stream_sql passes its actual identity into the existing SqliteStream/SqliteCursor. All three source lease bodies run in C++, including three transaction-visible rows, repeated probes, root-stream lease retention and early close. Buffered data-service streams do not substitute for this interface. |
| ST06 | Consuming commit/rollback and active state | Existing SqlTransaction terminal entry points validate the domain and reject pending operations before consuming the borrow. The existing Terminal operation executes COMMIT/ROLLBACK and abandons on failure/cancellation. Historical transaction schedules exercise successful and failed terminal paths and reuse. A stream prevents terminal consumption until EOF or explicit cancellation. |
| ST07 | Drop rollback before lease release | Existing native transaction abandonment rolls back active work before clearing its connection identity and waking waiters. Source test two is ported using actual transaction UserContext deletion. The additional open-cursor deletion body checks a waiting root read sees rollback, rather than uncommitted data. |
| ST08 | Root I/O waits outside the rolled-back transaction | The first complete original body and its C++ port hold a root write pending, roll back, then verify the write occurs afterward and survives. This checks physical SQLite contents, not only scheduling flags. |
| ST09 | Cancelled begin and dropped transaction allow reuse | The second complete body cancels the waiting begin, deletes the owning Context, confirms rollback, then begins and commits again on the same connection. |
| ST10 | Stream and repeated probes share the transaction lease | The third complete body checks ordered IDs 1 and 3, streams all three rows, commits, then holds a root cursor while another begin waits. Closing that cursor releases the waiter. |
| ST11 | C++ borrow and cleanup adaptations | Four additional bodies reject unpolled/partial terminal use, foreign output Context and consumed transaction access; cancel the transaction with a live stream; delete its actual Context with an open cursor; and close a native prepare-error cursor before rollback/reuse. Default PMR allocation is disabled throughout. |
| ST12 | Public imported interface and exact source provenance | The canonical module port retains the entire test body. Both current frontends match fresh reference-source observations through unchanged observer bodies and ordered corpora. The dedicated repository-relative oracle project verifies every registry version/checksum and fixed source identity before building. |

## Executed evidence and limits

The [current original test run](parity/sqlite_transaction_source_tests_current_5_0_6.json.gz)
executes the unchanged reference crate's entire three-test transaction_lease
target. The [installed native gate](parity/sqlite_transaction_streaming_installed_native_5_0_6.json.gz)
passes **8/8**; the [installed imported gate](parity/sqlite_transaction_streaming_installed_modules_followup_5_0_6.json.gz)
passes **4/4**. Both include the full seven-body source/lifecycle port and the
existing complete dynamic-store, SQL transaction and stream bodies.

The [current actual-source replay](parity/sqlite_transaction_streaming_installed_source_replay_5_0_6.json.gz)
matches **510/510 per frontend**: 144 lease, 183 asynchronous and 183 synchronous
transaction observations. Corpus bytes and source observers remain unchanged.
These older observers do not call the missing transaction-stream interface;
ST05/ST10/ST11 use the complete source port and actual lifecycle assertions.
No comparison normalizes SQL, errors or result payloads.

The [targeted sanitizer gate](parity/sqlite_transaction_streaming_sanitize_runtime_5_0_6.json.gz)
passes both full lifetime executables against coherently instrumented project
libraries. The reviewed prototype production files are byte-identical to the
installed repair. This is scoped ASan/UBSan evidence with leak scanning disabled
and an uninstrumented SQLite archive; it is not a full installed sanitizer run.

The [repair history](sqlite_transaction_streaming_5_0_6.md) keeps the missing
interface witness, proposal, coherent prototypes, installation and failed first
module configure separate. The configure failure was three missing package
fields in new probe registrations. Its successful follow-up preserves that
failure and compiles the actual installed headers and module interfaces.

Rust compile-time borrow/consuming rules map to the existing explicit Context
domain checks, pending-operation tracking and consumed-state guards. A stream
wrapper does not extend Context lifetime; returned rows remain in their owning
arena until close. Thread-safe sharing of arbitrary C++ Context borrows is not
promised. Transaction cancellation and Context quiescence use existing native
cursor cleanup before arena reclamation.

This accepts only the two complete source files under ST01-ST12. It does not
accept the complete provider, data-service transactions, graph-save governance,
generated assembly, G13/G17 or P0-P7. The failed historical full native 735/736
epoch is not relabeled. No generator/SDK, MIPS, GPU, external provider or physical
router execution is claimed.

The [terminal acceptance receipt](parity/sqlite_transaction_source_acceptance_5_0_6.json.gz)
joins the complete source mapping, current installed gates, exact prototype/code
identity checks, retained before-state and repository-relative reproduction files.

## Reproduction

Configure the existing native/module presets first, then follow the
[installed repair commands](sqlite_transaction_streaming_5_0_6.md#installed-regression-gates).
The original complete source tests can also be run directly:

```bash
set -e
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_state_baselines_506 \
    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
    TEAQL_LOG_ENDPOINT=off cargo test --offline --locked --jobs 2 \
    --manifest-path .reference/teaql-rs/teaql-provider-sqlite/Cargo.toml \
    --test transaction_lease
```
