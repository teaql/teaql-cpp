# Resolved mutation execution and event assembly

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The complete original runtime and both C++ frontends match all 3,518 cases.
Selected native checks pass 6/6 and module checks pass 5/5, with canonical
imported consumers and final no-work builds. No production or generator
implementation changes in this continuation.

## Close the early-exit evidence gap

The earlier 1,502-input mutation corpus is retained byte-for-byte, including its
expected outcomes. Most of those callers lacked trusted mutation intent and
therefore returned before provider execution. They remain useful rejection
controls; they do not establish successful resolved-service writes or events.

The added 2,016 inputs bind a validated mutation intent through trusted fixture
assembly. The C++ probe has one optional `bound_intent` switch; absent switches
leave every retained input unchanged. This is a test driver capability, not a
request-JSON authorization feature or a new production API.

The Rust observer appends an access-only forwarding bridge to the complete
locked `resolved.rs`, using `ReferenceRuntimeView`. It calls the real prepared
and unprepared methods and binds intent with the actual
`EntityDataService::with_mutation_intent`. The retained driver supplies the same
executor, behavior, policy, Checker, event sink and observations. Only imports,
fixture path, the additional binding argument and test entry are adapted.
The receipt retains the rendered observer and input hashes. No preparation,
mutation, event assembly or persistence algorithm is reconstructed.

| Additional inputs | Controls |
| --- | --- |
| 864 | Direct insert/update/delete/recover and prepared insert/update; zero/success/failing provider results; event failure; absent/empty/complete/partial old snapshots; command/service lineage; optional expected versions |
| 288 | Behavior/policy set and rejection, Checker fix, metadata availability and repository versus physical command entity |
| 864 | Empty/nonempty prepared batches, ID cardinality, missing/partial versions and per-row traces, zero counts, provider failure and event failure at either row |

Of these added inputs, 1,472 reach the provider, with 1,648 provider mutation
calls. Of the added inputs, 882 enter event delivery, with 982 delivered or attempted
events; 836 inputs return successful row counts. These counts come from actual
source observations and do not infer execution from signatures or inventory.

The 100 caught panics all concern malformed batch-update ID cardinality.
The original base implementation indexes `batch_ids[i]`; C++ uses checked
`at(index)` and the retained driver reports a caught exception as `panic`.
Exact preceding provider/event observations are compared too. These cases do
not establish successful batch execution or a recoverable validation error.

## Reconciled orchestration

The comparisons observe complete physical mutation requests, scalar values,
old snapshots, trace frames, event kinds, old/new event values, updated fields,
callback/cache notes, row counts, errors and ID allocator position. In
particular, they retain these source distinctions:

- Unprepared insert/update run their preparation; prepared entries do not run
  behavior, policy or Checker again. Single prepared operations use supplied
  service lineage rather than silently retaining command trace.
- Delete selects command lineage when present, otherwise repository lineage.
  Recover uses repository lineage for its command and emits its source-defined
  event without adding that lineage to the event.
- The audit readback helper performs no query. Incomplete or missing update
  snapshots therefore remain absent rather than triggering an invented SELECT.
  The fixture query executor throws if called.
- Update events preserve updated fields, overlay mutation values on a retained
  complete snapshot, and add the source identity/next-version fields. Prepared
  batch updates use their own per-row snapshots, update-field list and traces.
- Empty batches return zero without writing or emitting events. Provider errors
  prevent successful events. Event delivery errors occur after the provider has
  returned and stop further event delivery; they do not undo already observed
  provider calls.

The providers are controlled executors. Their successful return is not evidence
of a real SQLite commit, rollback, transaction atomicity or a durable audit
outbox. Mutation privacy classification and protected log publication retain
separate observers and remaining G17 obligations. This witness does not accept
whole graph Save, all resolved-service bodies or the entire capability.

## Qualification and reproduction

The [installed receipt](parity/resolved_mutation_execution_installed_5_0_6.json.gz)
binds the actual source replay, unchanged 1,502-case prefix, both complete
matrices, exact CTest selections, consumer checks and no-work builds.
It reruns the preceding 1,036-case select preparation and 1,690-case mutation
preparation controls. The
[initial source attempt](parity/resolved_mutation_execution_source_5_0_6.json.gz)
retains a fixture compilation failure: RuntimeDataService is exported from
`crate::data_service`, not the crate root. Correcting that import changes no
runtime body or expected outcome.

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_resolved_mutation_execution_5_0_6.py \
    --record /tmp/resolved_mutation_execution_fresh.json.gz \
    --cpp build_native_current/teaql_runtime_mutation_probe \
    --module build_modules_native/modules/teaql_module_runtime_mutation_probe \
    --check-gates
```

Pinned format apply/check passes for 1,722 handwritten C++ files. Source
compilation disables incremental caching after the preceding disk-full incident.
These fixture-only changes do not repeat sanitizer or MIPS profiles. Source
totals remain 84 accepted / 38 pending / 2 excluded; capability totals remain
11 qualified / 6 partial.
