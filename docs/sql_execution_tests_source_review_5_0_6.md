# SQL execution diagnostic test-source review

Reference: Rust 5.0.6, commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. This review covers the complete
214-line `teaql-sql/src/execution_tests.rs` and 846-line
`teaql-sql/src/mutation_execution_tests.rs`. It does not accept the containing
executor or mutation implementation, runtime publication, or complete G13/G17/P7.

The historical 5.0.5 port covered six query and fifteen mutation tests. The locked
5.0.6 source has six query and twenty-one mutation tests. Six mutation tests and
their shared successful-readback assertions were absent from the C++ port.
The source inventory's old fifteen-test description was therefore insufficient.

## Fixtures and adaptation boundaries

The original and native tests both call the production SQL executor through an
independent fault-injection transport, dialect and schema provider. No fixture
implements executor, cache, mutation, readback, privacy or diagnostic algorithms.
The schema has Customer ID/version/name/status and masks name. Writes retain
Riverside unchanged in driver bindings; successful reads return ID 1/name Riverside.
The read fixture intentionally does not synthesize the requested row identity.
Readback metadata must still bind the individual item ID.

The transport modes preserve success, zero affected rows, indexed write failure
or pending, read failure, empty read and pending read. Query-only modes preserve
failure, pending and provider unwind. A provider lease must be released before
each diagnostic observer; the observer can itself throw after retaining metadata.
Original `std::io::Error` becomes the existing typed native transport error with
the same relevant message. No provider exception is converted to a successful
operation.

Rust owned-future Drop maps to explicit cancellation of Context-owned native
work. Rust Arc/Mutex fixture state maps to registered owner-thread Context data;
no shared owner extends User lifetime. The native query tests enter the public
executor rather than the private precompiled-query helper. They independently
compile and check the exact masked input before that call, then exercise the
same production compiled execution and release/notification behavior.

Query purpose and mutation reasons now use the original fixture strings. The
target-ID test rebuilds the request's owned MutationIntent after adding its trace
node, matching the source's distinction between intent authority and diagnostic
frames. Batch helpers use the original root reason. Native-only assertions
add zero transport reads where the original checks no IO indirectly.

## Complete query test mapping

All source names below are in `executor::diagnostic_tests`; native entries live
in `tests/parity/teaql_sql/execution_tests.cpp`.

| ID | Original test | Native entry and retained assertions |
| --- | --- | --- |
| EQ01 | `failed_query_releases_resources_and_keeps_original_error` | `failed_query_and_observer_panic`, false branch: exact original driver error, Failure, unknown counts, Masked policy, comment, absent debug rendering and lease release. |
| EQ02 | `observer_panic_cannot_replace_driver_error` | Same entry, true branch: throwing sink cannot replace the original error or duplicate the terminal record. |
| EQ03 | `dropping_pending_query_is_cancelled_with_unknown_count` | `cancelling_pending_query`: poll Pending, cancel, one Cancelled record with unknown counts after lease release. |
| EQ04 | `provider_unwind_is_failure_and_releases_resources` | `provider_unwind`: propagate controlled provider exception, record Failure and release before observer. |
| EQ05 | `success_returns_metadata_without_duplicate_observer_notification` | `success_and_disabled_capture`, success branch: zero result count and Success in returned metadata, no fallback notification. |
| EQ06 | `disabled_capture_does_not_emit_failure_diagnostic` | Same entry, failure branch: preserve driver error and resource release with no diagnostic emission. |

The shared native `diagnostic` helper covers every original `check` assertion.
The input retains Riverside/name masking, limit 10, explicit comment/purpose,
disabled debug rendering and independent metadata capture.

## Complete mutation test mapping

All source names below are in `executor::mutation_diagnostic_tests`; native
entries live in `tests/parity/teaql_sql/mutation_execution_tests.cpp`.

| ID | Original test | Native entry and retained assertions |
| --- | --- | --- |
| EM01 | `native_batch_repeated_root_is_one_slot_with_real_item_identity` | New `native_batch_repeated_root`: nested grouping, root comment, exactly two reasons in order, repeated root retains real item ID 1, no fallback observer. |
| EM02 | `inherited_batch_provenance_covers_old_values_and_failed_readback_without_changing_binds` | New `inherited_batch_old_value_provenance`: sibling old name/password contribute inherited secrets before execution; only first write/read execute; exact raw bind vector; root comment retained; ordinary/debug credential and ID distinctions. |
| EM03 | `mutation_target_id_is_sql_intent_provenance_without_changing_plain_binding` | `mutation_target_id_provenance`: all four operations and success/failure, actual owned intent containing 1001, raw U64 bind unchanged, readable trace canary, protected target secret. |
| EM04 | `cached_query_intent_compilation_uses_current_values_without_transport_io` | `cached_query_intent_uses_current_values`: Riverside/Lakeside rebound, one cached shape, masked ordinary value, empty permitted debug secret set, no transport calls or observer. |
| EM05 | `transaction_cached_query_intent_compilation_is_io_free_and_conservative_on_unknown` | `transaction_cached_query_intent`: known masked value, unknown metadata remains protected even with debug permission, no IO, successful rollback. |
| EM06 | `direct_write_failure_preserves_driver_error` | `direct_write_failure`: one Failure, original error, unknown affected count, masked binding and audit lineage. |
| EM07 | `partial_batch_retains_execution_order_despite_sink_panics` | `partial_batch_failure_and_cancellation`, failure branch: first Success/1, second Failure/unknown, exactly two writes despite throwing sinks. |
| EM08 | `partial_batch_cancellation_retains_success_and_unknown_inflight_count` | Same entry, pending branch: cancel second write, retain first Success/1 and second Cancelled/unknown in order. |
| EM09 | `write_success_readback_failure_is_two_distinct_outcomes` | `write_success_then_readback_outcomes`, failure branch: successful write and failed Query, original readback error and unknown result count. |
| EM10 | `empty_readback_is_sql_success_but_mutation_contract_failure` | Same entry, empty branch: PersistedRecord error while read SQL remains Success/0 and write remains Success/1. |
| EM11 | `cancellation_during_readback_does_not_relabel_successful_write` | Same entry, pending branch: successful write followed by Cancelled Query, no invented count. |
| EM12 | `successful_batch_does_not_notify_fallback_observer` | `successful_batch_and_later_compile_failure`, success branch: three affected rows, three native item groups, no fallback events. |
| EM13 | `successful_transaction_readback_is_returned_once_after_write` | New `successful_transaction_readback`: observer absent/present, one write plus one successful readback, snapshot present, write count 1, no fallback events. |
| EM14 | `successful_batch_readbacks_keep_item_grouping_and_local_lineage` | New `successful_batch_readbacks`: three item groups each retaining their own physical write/read pair and ID, affected count excludes reads. |
| EM15 | `successful_guarded_readback_is_returned_with_guard_and_owned_intent` | New `successful_guarded_readback`: observer absent/present, shared pair assertions, status guard SQL and ACTIVE bind on both statements, no fallback notification. |
| EM16 | `unsuccessful_match_and_hard_delete_do_not_invent_a_readback` | New `unsuccessful_match_and_hard_delete`: zero affected insert or physical delete, no snapshot or child statements, no SELECT or fallback event. |
| EM17 | `later_compile_failure_does_not_invent_an_executed_statement` | `successful_batch_and_later_compile_failure`, missing-entity branch: Compile error after first successful write, exactly one executed write and one fallback fact. |
| EM18 | `guarded_readback_failure_retains_guard_on_both_statements` | `guarded_readback_failure`: Update Success then Query Failure, raw guard binding on both SQL statements and inherited sensitive write intent. |
| EM19 | `guarded_transaction_write_failure_has_one_outcome` | `guarded_transaction_write_failure`: erased guarded transaction access, one Update Failure with unknown affected count. |
| EM20 | `nested_batch_failure_does_not_duplicate_prior_statement` | `nested_batch_failure`: two ordered physical facts, no duplicate earlier success or later item execution. |
| EM21 | `all_mutation_kinds_keep_failure_kind_and_unknown_affected_count` | `all_mutation_kinds_preserve_failure_kind`: Insert/Update/Delete/Recover retain transport error, operation, Failure and unknown affected count. |

The shared `assert_successful_readback_pair` checks all original helper assertions:
exactly two physical leaves, write operation/count and Query outcome/count,
SELECT prefix and item-ID parameter, equal ordered typed audit reasons and
comment, the derived verification purpose, inherited Riverside privacy and
unchanged Riverside driver binding. Read success uses returned metadata;
fallback observers receive failures/cancellation and must not duplicate it.

## Reproduction and qualification boundary

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_sql_execution_tests_source_5_0_6.py \
    --record /tmp/sql_execution_tests_source_506.json.gz \
    --profiles native modules_native sanitize
```

The controller creates a disposable byte-preserved SQL source view, retaining
every original dependency and test file. Its existing access-only diagnostic
observer is inert in these original tests; the sole supplemental serde_json
dependency is already in the reference lock. All registry identities must match
the exact reference. The controller validates all registered test names before
running the six and twenty-one source tests unchanged, then builds and runs both
semantic ports in each selected profile with a no-work rebuild. Sanitizer leak
scanning is disabled. Modules import the same canonical bodies and fixture.

This is test-source acceptance only. The complete executor/transport/cache and
mutation-execution production responsibilities retain their own pending source
review. No new generator or generated SDK behavior, full-suite acceptance,
MIPS result or physical-device execution follows from these focused tests.

## Terminal evidence

The [execution receipt](parity/sql_execution_tests_source_execution_5_0_6.json.gz)
records all **27/27 unchanged original tests**, native **2/2**, imported **2/2**
and selected ASan/UBSan **2/2**, each after terminal builds and with a final
no-work rebuild. Leak scanning is disabled. No production implementation repair
was needed. The substantive change adds the six missing source tests and shared
readback assertions, strengthens owned-intent provenance, and aligns fixture
reasons; the six query contracts retain their existing assertions.

The [preservation receipt](parity/sql_execution_tests_source_preservation_5_0_6.json.gz)
verifies **6,091 protected files**, including production C++ source/header bytes
and the inherited **78 SDKs / 3,457 files**. Canonical module coverage grows to
**1,002 bodies** with **1,516 exports**. Native discovery stays **775**; the
module preset discovers **561 imported tests**. These are inventory counts,
not full-suite results. Pinned format/check passes for **1,704 files**.

The first static-validation attempt used a nonexistent verifier filename after
the three module checks passed. The
[failed receipt](parity/sql_execution_tests_source_static_5_0_6.json.gz) is retained;
the [corrected follow-up](parity/sql_execution_tests_source_static_retry_5_0_6.json.gz)
runs the actual `verify_generator_parity.py` and explicitly reuses only those
successful unchanged module checks. It verifies 42 source anchors and 17
capabilities; six capabilities remain unqualified.

Only these two test files advance in the source ledger, to **61 accepted /
61 pending / 2 excluded_external**. The
[qualification receipt](parity/sql_execution_tests_source_qualification_5_0_6.json.gz)
preserves the remaining full-goal requirements and historical failed full-suite
epochs. No generator/Java/reproduction gate is newly claimed for this test-only
continuation.
