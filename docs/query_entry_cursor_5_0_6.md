# Query entry and cursor publication protocols

Reference: Rust 5.0.6,
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
G09 is **requalified** by the [supplemental acceptance](parity/query_entry_cursor_requalified_5_0_6.json.gz) after its recorded counterexample/reopening. The corrected runtime matches **8,000/8,000** actual-source outcomes on both installed C++ frontends. Earlier pending paragraphs retain their checkpoint scope.

## Original-source execution and preserved controls

The complete byte-preserved original runtime executes ten distinct query entries:
plain/owned rows, rows with aggregates, SmartList with/without aggregates,
typed entities with/without aggregates, enhanced entities and the borrowed/owned
enhanced aggregate entries. Fixture setup does not replace their implementations.
Exact reference dependency versions and registry checksums are retained.

The 8,000 controls retain the earlier 320 aggregate cases and 6,080 entry cases
without changing their inputs or expected observations. Supplemental cases
observe cursor-store reads and writes independently. Query features include
ordinary execution, explicit projection, first-page/cursor-miss continuous
pagination, ID-set pagination and dynamic-field selection on a model that cannot
load extensions. Empty results distinguish the required lazy dynamic guard.

Outputs include complete compact values, loaded/total metadata, provider query
shape, ID-set observations, continuous-page observations and cursor events.
Cursor events retain the actual boundary Value, entity, page size and next
offset. Timestamp-based cursor identifiers are neither compared nor normalized;
the fixture observes their publication through stable cursor fields.

## Reproduced differences

The first 6,080 controls reproduce 2,592 native differences. The full
[pre-repair replay](parity/query_entry_cursor_before_5_0_6.json.gz) reproduces
**3,136/8,000** differences per C++ frontend, including **504** cursor-event
differences. The earlier 320 aggregate outcomes remain unchanged.

- Ordinary row shortcuts preserve unprocessed Query-owned aggregate declarations
  in the original provider request. General enhancement paths merge them after
  query preparation; constructor-time normalization changed these distinct entry
  semantics and could skip aggregates on ordinary queries with pagination.
- Aggregate-only enhancement executes the original continuous-page preparation
  path even without a pagination hint. It records `DISABLED` and clears an older
  cursor observation. C++ retained stale diagnostic state.
- Original result enhancements precede cursor publication. A failing aggregate
  must not publish a cursor; a successful identity-overwriting aggregate supplies
  the final row boundary. C++ published the pre-aggregate row first.

## Runtime correction and Context lifetime

The shared row operation now normalizes Query-owned aggregate declarations only
after preparation, when the selected entry actually enters the enhancement path.
It reuses the existing merge/deduplication implementation. Ordinary direct
entries retain the original query declaration for the provider.

The enhancement state machine completes relationship aggregates before cursor
registration/storage. Its disabled-observation condition includes aggregate-only
queries. It retains separate provider, service, pagination store and Context
responsibilities; no generator changes or new allocation domains are introduced.

The C++ lifetime program keeps a cursor store in Global and deferred cursor work
in UserContext. The parent and child aggregate providers must complete before
the pending put begins. Explicit cancellation and actual UserContext destruction
each stop the put exactly once before arena reclamation, without completing a
late write. Default PMR is disabled. These are C++ ownership-adaptation controls,
separate from the original-source outcome matrix.

## Qualification boundary

The [initial sanitizer fixture failure](parity/query_entry_cursor_sanitize_fixture_failure_5_0_6.json.gz)
is retained with its exact body, binary identity, stack trace and JUnit. The
fixture held a PMR-backed `PurposedSelectQuery` on the stack while destroying
its UserContext, so its later destructor accessed reclaimed arena storage.
The query is now allocated with `user_context.make<PurposedSelectQuery>()`;
registered destruction runs before arena reclamation. The pending-work and
exactly-once cancellation assertions remain intact. The 8,000 outcome checks
and the existing deferred cursor lifecycle program passed in that failed run.
The first installed verification attempt was stopped at its live Ninja build
after this independent sanitizer failure; its incomplete receipt is preserved.
Fresh installed/sanitizer receipts qualify the corrected fixture separately.

The [reopening record](parity/query_entry_cursor_reopened_5_0_6.json.gz) preserves
the prior G09 ledger and its original EX01-EX14 acceptance. It does not relabel
that evidence after finding a new counterexample. The supplemental gate reuses
the exact original selection of 37 native/10 imported tests, with the 29/7
targets recovered from the accepted gate drivers. Added query/cursor and lifetime
checks run alongside that selection. New selected compiler or sanitizer evidence
will be recorded only after terminal success.

This continuation does not accept complete `data_service/resolved.rs`, every
graph/hydration/dynamic/provider caller or P0-P7. Source totals remain
84 accepted, 38 pending and two external exclusions. Membership snapshots across
object-group, child-query and relation-hydration mutation still require the
separate full-source review; this matrix does not cover every such composition.


## Installed supplemental acceptance

The [corrected installed replay](parity/query_entry_cursor_installed_repaired_5_0_6.json.gz)
passes source/native/imported8,000 with no differing observations. Native **51/51**
and imported **18/18** selected gates include every retained G09 native37/module10
check, both query protocol corpora, original aggregate320/loaded-graph cases,
complete ownership programs and the generated Q/E example. Canonical consumers
validate and final builds have no work. The initial installed attempt remains
incomplete and is not acceptance evidence.

The [corrected sanitizer replay](parity/query_entry_cursor_sanitize_requalified_5_0_6.json.gz)
passes all three selected ASan/UBSan tests: query/cursor8,000, the new pending-put
lifetime program and the existing deferred continuous-page lifecycle program.
Leak scanning is disabled. It verifies the actual instrumented compile command,
JUnit, binary hashes and final no-work build. The earlier sanitizer failure and
its fixture body remain unchanged.

The sole production delta since the counterexample is the shared row operation.
Its public entry definitions and all producer inputs are byte-identical to that
checkpoint. Prior compiler, Java and generated-artifact qualification remains
historical evidence; these commands were not rerun or relabeled by this runtime
continuation. G09 restores its preceding qualified status, with QEP01 archived
and resolved. All other capability statuses and source84/38/2 remain unchanged.
This is not complete G08/G13/G17, resolved/graph source, full native/sanitizer,
Release, MIPS, device or P0-P7 acceptance.

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_query_entry_cursor_5_0_6.py \
  --record /tmp/query_entry_cursor_replay.json.gz \
  --cpp build_native_current/teaql_enhanced_aggregate_probe \
  --module build_modules_native/modules/teaql_module_enhanced_aggregate_probe \
  --check-gates
```
