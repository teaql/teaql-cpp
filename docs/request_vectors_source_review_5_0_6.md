# Complete data-service request vector and privacy test review

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This reviews both complete original integration test files:
`teaql-data-service/tests/request_intent_vectors.rs` (167 lines) and
`teaql-data-service/tests/request_privacy.rs` (45 lines), together with the
unchanged `tests/fixtures/request-intent-v1.json` fixture.
These are request construction/projection contracts, not provider execution.

## Complete responsibility mapping

| ID | Original body/input/assertion | C++ semantic port |
| --- | --- | --- |
| RV01 | Shared fixture gate | Require the authentic contract identifier and all twenty original cases. Preserve missing, Null, empty, ASCII/Unicode blank and valid untrimmed input. Query and mutation remain separate request types. |
| RV02 | Query envelope vectors | Construct SelectQuery with its own optional comment/purpose and authored trace, then call QueryRequest::from_query. Compare exact owned comment/purpose or structured code/field. The metadata-capture flag is changed only after successful construction; disabling logs cannot supply missing intent. |
| RV03 | Mutation envelope vectors | Construct an intent-free standalone command or actual Batch containing individually validated child requests. Validate the outer optional MutationIntent independently and create the request with that owned reason. Child reasons and legacy traces cannot substitute for missing root intent. Compare exact code/field or preserved comment. |
| RV04 | Query execution trace | Preserve the original Comment/Purpose/Relation input, bind a different owned QueryIntent, then mutate the query builder comment. Execution projects the owned comment/purpose into the inherited route without changing the request's original trace. Exact node kinds, entity names, optional IDs, order and text are checked. |
| RV05 | Native mutation trace | An insert with id 100 and no source trace materializes exactly one owned root AuditReason with the correct entity/ID/reason. The original request trace remains empty before and after projection. |
| RV06 | Complete parent/child lineage | Update Payment 301 under Order 100 retains the complete two-node ancestor/child path without duplicating the root. Compare the actual execution trace to the original command path. |
| RV07 | Request-owned privacy snapshot | Bind a Masked operand's private provenance, then extend the mutable source. The request keeps only the original value. Request Debug exposes none of it; a new request cloned from the original command inherits no private state; the allowed debug view omits non-forced redactions. |

The original Arc::make_mut control creates an independent mutable source after
the request receives its shared snapshot. The C++ request explicitly copies
SqlIntentRedactions into its Context at binding time. This preserves the
observable immutable snapshot without shared ownership or an extension of the
source Context lifetime. The new request is built from the first request's
actual command, rather than using a separate placeholder command.

The C++ port retains all five original test responsibilities and the complete
twenty-case fixture. Its extra assertions identify malformed fixture/setup
failures; they do not replace original expected errors or relax equality.
Default PMR allocation is disabled, and all request/trace/provenance objects use
the explicit UserContext. Ordinary C++ wrappers remain borrows.

## Verification and evidence boundary

The [current source replay](parity/request_vectors_source_replay_5_0_6.json.gz)
records actual original crate tests, the complete formatted C++ port, unchanged
fixture, canonical module body, helper provenance and terminal commands. Its
`complete` flag and every command's exit status must be checked before claiming
success. Original tests are executed from the frozen crate with its own exact
Cargo.lock; no copied validator, shell, edited Rust body or expanded dependency
scope supplies the results.

The C++ consumer uses actual imported QueryRequest, MutationRequest,
MutationCommand, typed intents, trace projection, private provenance and Debug
APIs. Its quoted JSON fixture helper is adapted with the canonical module
preparer; standard includes remain before imports and the function body must
compare equal. The test fixture remains pure authored request input, with no
database or substitute data service involved.

Two complete existing imported bodies also execute: the data-service kernel
and request/trace scope contracts. Their separate controls cover Context-owned
intent and command/request separation, independent nested batch binding,
default-PMR-disabled nested copies, explicit Global retention after User
deletion, moved request refusal, private service snapshots and governed plan
provenance. They complement these five source tests rather than turning the
pure vectors into end-to-end generated/provider evidence.

The [request-intent source acceptance](request_intent_acceptance_5_0_6.md)
retains its separately qualified 4,655 lexical/envelope/token observations.
The [Context logging vector review](context_logging_source_review_5_0_6.md)
covers canonical statement paths and executed metadata folds independently.
Neither is relabeled as a newly executed broad run here.

The replay is terminal and successful: all **five original tests**, **twenty
shared vectors**, **three execution-trace contracts**, the **request privacy
snapshot contract** and **two complete imported lifecycle bodies** pass. Every
one of its eight recorded commands exits zero. The
[provisional join](parity/request_vectors_source_provisional_5_0_6.json.gz)
verifies all 10,329 frozen input hashes and both fixed references unchanged.

The [initial constructor failure](parity/request_vectors_source_initial_constructor_failure_5_0_6.json.gz)
remains separate evidence. The temporary port initially used an unsupported
two-argument SelectQuery constructor. Its corrected explicit allocator constructor
matches the existing C++ ownership API; no production constructor or original
source changes. The complete port is pinned-formatted and its imported body
matches before compilation and execution.

## Remaining integration

Production, producer, generated SDK, permanent test/module registration and
JSON-ledger inputs remain frozen during the running full native epoch. This
review adds a temporary formatted consumer and compressed evidence plus
narrative documentation. It does not consume unfinished native outputs.

After that epoch is terminal, retain this complete consumer and fixture in the
permanent test/module structure, install a durable reference-locked reproducer
and review both source dispositions against the terminal evidence. Wider
QueryRequest/MutationRequest APIs, executor/provider publication, G13/G14/G17
and complete P0-P7 qualification retain their independent exits. No additional
sanitizer, MIPS or physical-router run is claimed for these unchanged bodies.

## Permanent registration after the frozen epoch

The earlier temporary-only boundary above is historical. The complete semantic
port is now installed as `teaql_data_service/tests/request_vector_tests.cpp`,
with its exact fixture at
`tests/parity/teaql_data_service/data/request_intent_v1.json`. Apart from its
repository-relative include, the complete earlier assertion body is unchanged.
Both native and named-module contracts execute it; neither substitutes an
in-process fixture or a locally implemented request algorithm.

The [complete data-service review](data_service_source_review_5_0_6.md) joins
all five unchanged original request tests, the three root tests, two existing
SQL-log tests and 1,308 actual original/native/imported outcomes per frontend.
Native 12/12 and imported 12/12 pass with no skips and stable builds. The
[terminal record](parity/data_service_source_acceptance_5_0_6.json.gz) promotes
the complete public data-service source and both request test files. Source
status becomes 43 accepted/79 pending/2 external; all capability statuses remain
unchanged. Actual protected provider/runtime publication and complete
G13/G14/G17/P0-P7 acceptance remain separate obligations. No additional full-suite,
sanitizer, MIPS or physical-device result is inferred.
