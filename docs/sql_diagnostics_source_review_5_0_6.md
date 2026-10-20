# SQL statement, journal and stream diagnostic source review

Status: both complete mapped diagnostic sources are accepted under DX01-DX16,
including targeted sanitizer, formatting and preservation checks. Higher
executor, runtime, privacy and generator-phase exits remain separate.

Reference: Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, complete
`teaql-sql/src/diagnostic_execution.rs` (91 lines) and `diagnostic_stream.rs`
(245 lines, including seven original tests).

## Complete responsibilities

| ID | Responsibility | Mapping and evidence |
| --- | --- | --- |
| DX01 | Independent optional metadata and observer | StatementDiagnostic carries both independently; absent metadata never invents a statement or notification. The 352-input guard matrix crosses both options and compiled/absent SQL. |
| DX02 | Successful result ownership | success takes metadata once, updates end time/outcome and returns it to the normal result path. It does not notify the failure observer; later success/fail/drop calls cannot duplicate it. |
| DX03 | Failure and cancellation | fail consumes metadata as Failure. Ordinary Drop becomes Cancelled; unwinding Drop becomes Failure. Remaining metadata fields and parameters stay intact. |
| DX04 | Callback isolation | Metadata is removed before notification. Sink panics/exceptions are contained and do not escape Drop or replace the provider result. Repeated terminal actions remain no-ops. |
| DX05 | Optional fallback journal | FailureJournal creates an entry vector and recorder only with an observer. C++ keeps the vector in the same Context and registers quiescence explicitly. |
| DX06 | Recorder lifetime and ordering | Recording retains the exact executed metadata in invocation order. A recorder can outlive its journal; C++ confines that borrow to its Context instead of extending Context lifetime through Arc. Late entries after journal destruction are not published. |
| DX07 | Journal disarm | Disarm removes the observer, preserving recorder availability and stored data until their owner ends. Success discards fallback publication. Disarm before/after recording and late recording have direct original observations. |
| DX08 | Journal drain and callback failure | Drop takes the observer and drains the accumulated entries once, preserving order and incoming outcomes/times. Each callback is isolated so a failed sink does not prevent later entries from being attempted. |
| DX09 | Stream constructor and pending | DiagnosticStream owns the source cursor, metadata, observer and delivered count. Pending does not finish or increment rows. Native forwarding/cancel/join retains its explicit Context adaptation. |
| DX10 | Delivered row counts | Successful chunks add their actual row count. The final chunk finishes successfully before being returned; no additional source poll is needed. Buffered but undelivered rows are not counted. |
| DX11 | Error and EOF terminals | A source error finishes as Failure and remains the returned error. EOF finishes as Success. Once terminal, the wrapper does not poll the source again. Native move-only error contracts preserve the absence of a Clone requirement. |
| DX12 | Release before observation | Source.take/drop precedes the observer. Native cancel/stop/join releases the provider cursor first. Original seven tests and real native cursor/transaction contracts verify this order. |
| DX13 | Stream Drop and sink exceptions | Ordinary Drop is Cancelled, unwinding Drop is Failure. Already consumed metadata cannot be published twice, including a throwing sink. All seven original tests execute unchanged. |
| DX14 | C++ ownership and exception adaptations | Managed guard destruction, explicit borrowed-stream cancellation and actual UserContext deletion map the source owned Drop boundary. Native poll exceptions/count overflow finish before propagation; source panic safety is verified through owned Drop during unwinding. No simultaneous cross-thread Context mutation or post-panic stream-reuse guarantee is added. |
| DX15 | Actual request and executor integration | Existing query, mutation, transaction, stream and backend-kind drivers run unchanged against the actual locked SQL crate. Historical missing-intent inputs remain constructor rejections; explicit-intent controls still execute. No result/error normalization hides differences. |
| DX16 | Layer and privacy boundary | These guards carry metadata, never execute SQL, render provider error text or authorize request data. Raw metadata remains subject to higher runtime privacy publication. Success results, failure journals, transport cursors and runtime sinks remain distinct. |

## Exact dependency repair

The historical `rust_sql_protocol_5_0_6` workspace has **44 registry identity
differences** from the reference, including futures 0.3.34 versus reference
0.3.32. Its outputs and lock are retained. Its baseline label and successful
execution alone do not establish exact-reference source qualification.

The independent `rust_sql_diagnostics_source_5_0_6` project pins the reference
versions/checksums and introduces no C++ dependency. A disposable SQL source view
preserves all **12 original source files**, including the complete lib.rs prefix.
Only an access/test observer is appended. Its sole supplemental dependency is
the already referenced serde_json 1.0.150 for observation serialization; no
original dependency is omitted or replaced. Both the view and observer locks
must contain only reference registry identities.

The old mutation observer uses include!(CARGO_MANIFEST_DIR/../...). Three helper
files are copied byte-for-byte into a disposable sibling layout so the original
driver and included body need no edits. Actual helper, manifest, lock, source
and binary hashes are recorded. The original seven stream tests are derived
from their source test attributes and checked against the compiled registry.

| Family | Actual original/native/imported outcomes |
| --- | ---: |
| New StatementDiagnostic / FailureJournal lifecycle matrix | 352 |
| Query protocol | 767 |
| Stream protocol | 1,765 |
| Mutation execution protocol | 2,029 |
| Transaction execution protocol | 2,029 |
| Executor backend-kind protocol | 93 |
| **Total** | **7,035** |

All outcomes reproduce the retained corpora on the exact reference dependency
graph. The new guard corpus is written only after two original runs agree and
native output matches. Observation compares fixed start times and end-time
update predicates; it does not claim that independently sampled wall clocks are
equal. Other metadata, ordering, outcomes and returned/observed paths are exact.

## Lifecycle coverage

The guard matrix crosses metadata/observer presence, sink failure, compiled SQL
and repeated terminal calls. Journal cases include three ordered statements,
disarm before/after recording, destruction during unwinding, recorder use after
journal destruction, and Context close with live managed guards. Parameters and
comments survive publication; journals do not replace incoming metadata with
fabricated failure outcomes. Each native case actually deletes UserContext and
asserts zero retained User bytes, with default PMR allocation disabled.

The original DiagnosticStream tests independently cover partial transport
failure, a terminal chunk without extra polling, unpolled/pending Drop, consumer
unwinding and sink panic on provider failure/Drop. Their existing C++ port uses
the production wrapper and an explicit cancellation owner for borrowed streams.
Real cursor, transaction and noncopyable-error contracts supplement that port.

The [exact-source execution](parity/sql_diagnostics_source_all_frontends_5_0_6.json.gz)
passes **native11/imported11** across all six matrices and five integration
contracts. No production function/header or generated artifact has changed.
The original observer project and historical full-suite outcomes remain intact.

Two collector failures are preserved: the
[initial disposable lock failure](parity/sql_diagnostics_source_initial_execution_5_0_6.json.gz)
needed the explicit observer-only dependency edge to be resolved before locking;
the [second attempt](parity/sql_diagnostics_source_retry_execution_5_0_6.json.gz)
completed the source replay but requested a nonexistent native target. The
[corrected native run](parity/sql_diagnostics_source_native_execution_5_0_6.json.gz)
passes 6,268 observations/native10 before the final query-inclusive cohort.
Neither failure is reported as a production mismatch or erased from the record.

The [targeted sanitizer record](parity/sql_diagnostics_source_sanitize_5_0_6.json.gz)
passes 2/2 checks: all 352 guard/closing/unwind observations and the original
seven cursor contracts. Leak scanning is disabled. The
[final qualification](parity/sql_diagnostics_source_qualification_5_0_6.json.gz)
verifies these receipts, exact-source replay and final no-work builds before
accepting the two files.

Pinned formatting/check covers 1,704 C++ files. Structural checks preserve
999 canonical imported bodies, 1,516 exports and seven acyclic packages. The
[preservation receipt](parity/sql_diagnostics_source_preservation_5_0_6.json.gz)
checks all 78 existing SDKs / 3,457 files and 5,741 protected artifacts against
their starting bytes. Production diagnostic headers/functions, the historical
protocol project and all retained corpora remain unchanged.

Only diagnostic_execution.rs and diagnostic_stream.rs advance: **59 accepted /
63 pending / 2 excluded_external**. All seventeen capability verdicts remain
unchanged. Current native discovery is 775 and imported-only discovery is 559;
these are registration counts, not complete-suite results. The historical failed
native epochs keep their recorded outcomes.

## Reproduction

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_sql_diagnostics_source_5_0_6.py \
    --record /tmp/teaql_sql_diagnostics_fresh.json.gz --modules
```

Use a fresh execution-receipt path. Current sanitizer verification is targeted
at the new closing/unwind matrix and the original cursor contracts, with leak
scanning disabled. Complete SQL executor/mutation orchestration, runtime governed
publication and G13/G17/P7 integration require their own evidence. No complete
suite, generator reproduction, MIPS/QEMU or physical-device result is inferred.
