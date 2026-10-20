# Trace-chain request source audit: Rust 5.0.6

Status: accepted for all seven source contracts after original, native and
canonical module verification. No broader capability is promoted. The locked runtime commit is
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

Reviewed source: `teaql-runtime/tests/trace_chain_requests.rs`, all 307 lines,
SHA-256 `bd96e93a88724cdcace1145a878cf28433d697c9a522afec33d7bdbd13371e46`.
The source contains seven tests. Its no-I/O executor and rejecting Policy are
part of the contract: expected traces must never be supplied to the planner.

## Required direct coverage

| Source test | Required setup and assertions |
| --- | --- |
| `query_missing_comment_cannot_be_repaired_by_trace_or_disabled_logs` | Both logging states; comment absent, empty, ASCII whitespace and U+2003; nonempty purpose and fabricated ambient Comment frame. Reject before any Policy callback or executor call. Inspect structured Query kind, `comment` field and `REQUEST_COMMENT_REQUIRED` code. |
| `graph_planner_rejects_missing_or_blank_root_comment_before_io` | All four root-comment forms above, a create-only Order and child with a valid local reason. Call the actual service graph planner; inspect structured Mutation kind, comment field and code. No provider query or mutation is permitted. |
| `planner_retains_deleted_child_reason_and_isolates_sibling_branches` | Exactly six plan items: Order 100, inherited OrderItem 201, deleted/versioned OrderItem 202 with local reason, Payment 301 with local reason, inherited PaymentAttempt 401 and Shipment 501 with local reason. Recover each item's real scope token; every frame is AuditReason. Assert exact ordered entity/type/id/reason tuples and Delete kind for item 202. |
| `blank_descendant_comment_inherits_without_adding_a_scope` | Root reason and whitespace-only child reason. The child scope recovers exactly the root tuple; no blank frame appears. |
| `newly_allocated_ids_and_same_id_different_types_are_retained` | Order and Payment both use numeric ID 100 but distinct types; Shipment is created without an ID. Exactly three planned items. Each trace tail matches its own item type and allocated value ID, and every tail ID is positive. |
| `macro_entity_comments_are_keyed_even_when_sharing_one_ledger` | Decode two actual typed entities with id/version/name, install one shared runtime ledger on both, then set distinct entity comments. Each generated getter retains its own comment. Separate mutation ledgers are insufficient evidence for this case. |
| `child_composition_must_not_supply_a_missing_root_comment` | Decode two typed entities, set only the child's comment and compose its pending mutations into the root through the actual generated adapter. Root comment stays absent; the child comment survives. |

## Existing evidence and its limits

`teaql_runtime/tests/request_trace_scope_tests.cpp` covers request-owned intent,
ambient trace rejection, Unicode blank intent, Context lifetime copies and direct
ledger scope capture/enrichment. Its `graph_and_standalone_lineage` uses a
standalone deletion and explicit ID enrichment. That is useful implementation
coverage, but it does not replace this source's six-item service planner case,
its same-number/different-type case or its no-Policy callback assertion.

`tests/generator/ledger_composition_tests.cpp` exercises actual generated models,
independent hydration ledgers, entity comments and graph intent. Its independent
root isolation check does not prove keyed comments when both entities deliberately
share a single mutation ledger. Preserve both scenarios: independent hydration
and explicitly shared ledger composition have different contracts.

`teaql_runtime/tests/runtime_completion_tests.cpp` already calls the real
`EntityDataService::plan_graph` and tests asynchronous failure/cancellation.
The new source port should use that production entry point rather than invoking
only internal scope-capture helpers. Existing completion checks remain relevant
regressions, not a substitute for the seven source tests.

`tests/parity/rust_field_layout/src/bin/original_request_trace_5_0_6.rs` includes
the SQLite `request_trace_log.rs` suite. Despite the similar filename, it is not
an unchanged-source execution of this runtime test file.

## Qualification sequence

1. Execute all seven unchanged original tests against the locked runtime view,
   recording the exact source, dependency view, command and successful exit.
2. Add a direct C++ counterpart using the real service planner, explicit metadata
   and no-I/O/Policy sentinels. Use an actual generated carrier for the last two
   tests; reuse an equivalent generated fixture only after verifying the required
   scalar layout and runtime-state replacement/composition adapters.
3. Assert exact source outcomes rather than merely successful planning. Preserve
   the four invalid-comment variants, both logging states, every item and every
   trace tuple. Reject extra or missing observations.
4. Execute native and canonical imported-module counterparts with the relevant
   request-intent, graph planning and generated-ledger regressions. Any change to
   runtime ownership or asynchronous behavior also requires focused sanitizer
   coverage. Never change captured build inputs during an active integration run.
5. Record the source-to-counterpart review and terminal evidence before updating
   its source disposition. Passing this file alone does not qualify all of G17
   or any full P0-P7 exit criterion.

## Original execution checkpoint

The [unchanged original execution](parity/trace_chain_requests_original_5_0_6.json.gz)
passes all seven named tests with zero failures, ignored tests or filtered tests.
The receipt retains the controller, command outputs, source hash and complete
runtime-view provenance. Every original source/test body is preserved; the view
omits only its documented unrelated OpenTelemetry development dependencies and
verifies resolved package identities against the reference lock. This supplies
the original side of the gate; C++ direct coverage and source acceptance remain
pending.

## Isolated C++ counterpart checkpoint

The [C++ candidate receipt](parity/trace_chain_requests_cpp_candidate_5_0_6.json.gz)
retains a formatted, separately compiled and executed counterpart of all seven
contracts. It calls the real `EntityDataService<NoIo>::plan_graph`, checks exact
structured intent failures before a throwing Policy/executor, and recovers real
scope tokens for all six graph items. Blank inheritance and allocated same-number/
different-type IDs are exercised through the same planner entry. The existing
HydrationProbe generated carrier supplies id/version/name decoding, deliberate
shared runtime-state replacement, keyed comments and generated composition; its
additional nullable note field is omitted and irrelevant to these assertions.

The isolated native executable exits successfully with
`TRACE_CHAIN_REQUEST_SOURCE_CONTRACTS=7;passed`. Its linked library hashes are
unchanged across execution. The receipt also retains initial compile diagnostics
from adapting the fixture to C++ provider include paths, relation metadata,
resource-scoped polymorphic sentinels and the explicit PurposedSelectQuery wrapper.
No runtime or generated artifact was patched to make the counterpart pass.

The candidate remains outside captured repository build inputs during the active
SDK integration epoch. It still needs repository installation, canonical imported
execution and a final source review before disposition promotion. A single
isolated native run is not the complete source gate.

## Installed counterpart verification

The candidate is now installed as
`tests/parity/trace_chain_requests_probe.cpp`. The completed
[installed verification receipt](parity/post_hydration_candidate_verification_5_0_6.json.gz)
includes its native and canonical imported execution within native10/module10.
The same selection checks request-trace scope, generated readonly properties,
loaded-view movement, entity runtime and allocation consumers. Canonical source
bodies, no-work rebuilds, binary hashes and final captured inputs/references are
checked. The original installer discovery failure remains in its own receipt.

The earlier statement that this candidate remained outside repository inputs
describes that earlier checkpoint. Installation and imported execution are now
complete; ledger reconciliation awaits the current completion-fixture epoch so
its captured JSON inputs are not changed during validation. This file still does
not establish all runtime graph/trace behavior or complete P0-P7 acceptance.

## Final source acceptance

The complete source responsibility reviewed here is accepted under the
[current closure evidence](hydration_trace_core_source_closure_5_0_6.md).
Original and installed native/imported gates are terminal and successful; the
relevant focused concurrency sanitizer evidence is retained with its own scope.
Earlier pending/isolated-candidate statements describe historical checkpoints.
This accepts only the named source, not the enclosing runtime, provider, generator
capability or P0-P7 goal.
