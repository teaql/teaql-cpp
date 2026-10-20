# SQLite request trace source review: Rust 5.0.6

Status: accepted for this complete named source. Original9 and installed
native1/module1/focused-ASan+UBSan1 pass; each C++ executable runs all nine
counterparts. The existing owner-thread scheduling adaptation is explicit below.
Earlier count-only evidence retains its historical scope.

Locked source: `teaql-provider-sqlite/tests/request_trace_log.rs`, 770 lines at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`7c7bd3cd32fe5b2d9de9cc20b4192fbe728ca761ab5910da25dd8c03148e6b71`.

## Complete source checklist

| Original test | Required behavior | Current C++ evidence in this continuation |
| --- | --- | --- |
| derived_count_retains_removed_relation_privacy_without_executing_it | Borrowed/owned count removes relation and slice, hides diagnostic source in Debug, preserves descendant privacy without extra SQL, keeps ordinary bindings and isolates later requests; legacy extraction is a leaking negative control | Installed native/module pass |
| failed_count_retains_removed_relation_privacy_at_the_error_sink | Real missing-table error records one failure with removed password-descendant intent redacted | Installed native/module pass |
| count_logging_off_does_not_disable_required_intent | Missing comment still rejected; valid count returns 2 with no logs | Installed native/module pass |
| native_recover_request_preserves_operation_and_owned_reason_at_safe_log_sink | Transactional insert/delete/recover, version 3 readback, Recover operation and owned reason, safe route and subsequent query intent | Installed native/module/sanitizer pass |
| blank_typed_route_tails_keep_owned_intent_at_sqlite_logging_on | Entity/Provider/Sql blank tails preserve exact root reason, physical insert/readback metadata and committed-row visibility with two logs | Installed native/module/sanitizer pass |
| blank_typed_route_tails_keep_owned_intent_at_sqlite_logging_off | Same three route-tail cases and physical metadata, with no logs | Installed native/module/sanitizer pass |
| native_nested_batch_inherits_root_intent_and_preserves_local_lineage | Nested root/local audit reasons, four physical leaves, unchanged sensitive bindings, safe logs, persisted update and later request isolation | Installed native/module/sanitizer pass |
| failed_native_batch_keeps_root_intent_and_masks_future_sibling_on_prior_statement | Duplicate-key failure, earlier-statement masking using future sibling values, root intent, readback purpose and actual rollback | Installed native/module/sanitizer pass |
| independent_native_batches_on_one_context_keep_operation_local_execution_intent | Two overlapping operation-local batches, exact per-leaf lineage, eight logs, protected values and six committed rows | Installed native/module/sanitizer pass with actual pending lease overlap |

## Count fixture and assertions

The count checkpoint was originally installed as
`tests/parity/teaql_provider_sqlite/request_trace_count.cpp`; it is now part of
`tests/parity/teaql_provider_sqlite/request_trace_log.cpp`, which uses the real
SQLite provider, SQL executor and runtime repository. It creates the exact
`BatchProbe` descriptor/table: I64 id/version, Text name/password/status, and a
declared mask list containing name. A real transaction inserts rows 9 and 10,
version 1, name `before`, password `BEFORE-CREDENTIAL` and status `ACTIVE`, using
the original seed reason. Schema setup and commit complete before queries run.
The fixture's native transaction handle supplies the seed; runtime repository
execution supplies the count logs being asserted.

The successful count preserves the original `COUNT-PRIVATE-DESCENDANT` canary
in a diagnostic source while removing `unused_relation` from executable SQL.
Both borrowed and owned calls return a U64 count of 2 and one safe log. The
exact physical parameter is `ACTIVE`; ordinary intent remains visible. The next
query through the same repository retains the canary as ordinary text. A
separate legacy extraction deliberately discards provenance and must expose the
canary, demonstrating that a passing positive check depends on source retention.

The failure test actually drops the table before execution; it asserts a single
Failure outcome and absence of `FAILED-COUNT-PRIVATE` from the complete safe-log
Debug representation. The logging-off test separately checks missing-intent
rejection and a valid count result, with no log publication in either case.

## Evidence

- `parity/request_trace_log_original_5_0_6.json.gz`: all nine original tests pass,
  zero failed, ignored or filtered; exact source and manifest/lock hashes retained.
- `parity/request_trace_count_candidate_5_0_6.json.gz`: preserved candidate link
  failure caused by omitting the public `purposed_query_debug.hpp` inline
  definition. This was a fixture include problem, not a production behavior gap.
- `parity/request_trace_count_candidate_v2_5_0_6.json.gz`: corrected candidate
  passes all three derived-count counterparts; no production change.
- `parity/request_trace_count_installed_5_0_6.json.gz`: native1/module1 pass,
  each covering the three count tests. Canonical consumer/fixture bodies,
  no-work builds, module paths and source/reference guards pass. The pinned
  formatter and check pass for all 1,767 handwritten C++ files.

Source and capability acceptance totals remain unchanged until the complete
source responsibilities are qualified. P0-P7 remains open.


## Complete transaction continuation

The complete counterpart retains all three count functions and their assertions.
It adds the six remaining source tests without modifying production execution.
The earlier count-only target and path are replaced by
`teaql_provider_request_trace_tests` and `request_trace_log.cpp`; historical
receipts keep their original paths and scope.

### Recovery and owned native query intent

A separate `RecoveryProbe` descriptor and schema retain I64 id/version and Text
name. One real runtime transaction inserts ID 7/version 1 with the original
private name and seed reason, soft-deletes it with the archive reason, and
recovers expected version -2 with `restore archived object`. Assertions require
one affected row, Recover metadata and a persisted I64 version 3. The safe log
must remain Recover (not Update), retain its owned reason, have the original
root/operation identities, exclude intent kinds from its route, and hide the
private name throughout the complete safe Debug output. A subsequent real
transaction reads the committed row and checks the exact owned query comment,
purpose and route endpoints. C++ uses the distinct `EntitySnapshot::fields`
representation for persisted values.

### Blank typed route tails

Both logging modes run all Entity, Provider and Sql tails. Each tail has its
original empty comment and the corresponding ID 100, 101 or 102. The request
and a retained copy preserve the exact whitespace-bearing root reason and
unchanged tail. The runtime mutation must return the real persisted identity
and two physical leaves: Insert then Query. Each leaf retains the root reason,
Success outcome and nonempty parameterized SQL. Safe logs number two or zero
according to logging; every published entry has the exact audit reason and
comment. A new runtime transaction checks the committed row independently of
the mutation's readback.

### Nested success and failure

The request has the original delete of row 9 followed by a nested update of row
10, or a duplicate-key insert of row 10 for the failure branch. Root and both
levels of local audit reasons retain their exact original text. Success checks
two affected rows, four alternating physical write/readback leaves, one-row
readbacks, exact per-leaf audit lineage, unchanged Riverside/password physical
bindings and unchanged caller-owned child reasons. Four safe logs, including
two SELECTs, retain ordinary ACTIVE intent, exclude both private canaries and
end at SQL without exposed intent frames. A subsequent runtime transaction
checks persisted values/version 2 and request-local privacy isolation.

Failure must be a real duplicate-key error. Logs must include the earlier
Delete and the failed Insert, root intent on every entry, both secrets hidden
also from earlier statements, and the original readback purpose for SELECT.
A separate transaction requires both original rows and version 1 after rollback.

### Independent operations on one Context

The Rust source starts two worker threads together, while the SQLite connection
serializes their native transactions. The C++ counterpart preserves the existing
Context owner-thread contract: it constructs both operations, polls the first
until its real transaction is held at an explicit callback gate, then polls the
second. Both must be pending; exactly one callback has entered and no SQL log
has been published. The second operation actually waits for the native
connection lease, rather than using a fake executor or a completed first call.
After release both callbacks execute exactly once. Each batch affects two rows,
and every physical leaf retains its own exact root and child audit reason.
There are exactly eight safe logs: four SELECTs and four entries per operation;
none exposes either private name. A new transaction sees all six committed rows.

The gate and deferred work are explicitly Context-owned. No thread-local state,
shared ownership or new scheduling domain is introduced. This checks actual
cooperative overlap and native lease waiting; it does not promise arbitrary
cross-thread Context access. Original source threading and C++ scheduling are
not reported as identical mechanisms.

### Continuation evidence

- `parity/request_trace_transaction_candidate_5_0_6.json.gz` preserves the
  initial eight-test compile failure: the fixture incorrectly used a row getter
  on `EntitySnapshot`. The correction uses its actual fields map.
- `parity/request_trace_complete_candidate_5_0_6.json.gz` passes all nine test
  counterparts, including the pending native transaction/lease overlap.
- `parity/request_trace_complete_installed_5_0_6.json.gz`: installed native1/
  module1/focused-ASan+UBSan1 pass, each with all nine counterparts. Canonical
  consumer and fixture preservation, no-work builds, module paths and final
  source/reference guards pass. Leak scanning is disabled per project policy.
  The pinned formatter and check pass for 1,767 handwritten C++ files.

Only this complete named source is accepted. Other pending sources, remaining
generator capabilities and P0-P7 completion stay open. No physical-router or
unrestricted cross-thread Context guarantee is claimed.
