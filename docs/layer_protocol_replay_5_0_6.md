# Layered request, SQL and audit protocol replay

Status: ten original-call families reproduce 4,398 observations against actual
Rust 5.0.6. Seven complete families match native C++; three diagnosed families
remain failing. Current full CTest inputs are unchanged while its last compiler
gate runs. No complete G13/G14/G17 or P7 acceptance is claimed.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

| Family | Original inputs | Native result |
| --- | ---: | --- |
| Request model API | 174 | Pass |
| Request helpers and facets | 872 | 15 row-format differences |
| SQL types | 1,538 | Pass |
| SQL executor errors | 144 | Pass |
| SQL log models | 1,008 | Pass |
| Data-service models | 40 | Invalid-intent fixture abort |
| Runtime graph models | 128 | Pass |
| Checker models | 220 | Pass |
| Audit event models | 112 | Missing source fields/projection |
| Relation models | 162 | Pass |

Seven complete families match **3,374/3,374** actual original/native calls; the
[terminal family record](parity/layer_protocol_replay_5_0_6.json) preserves all
outcomes. Every input/order is unchanged, outputs are unnormalized and old
corpora/drivers remain intact. New data reproduces in a second live Rust run.
Nine drivers only adapt their shared fixture import. The data-service observer
needs the explicit source-interface migration below. Enum/model fixtures do not
add PostgreSQL/MySQL services or any production dependency.

## Request constructor migration

The old data-service driver does not compile against 5.0.6: QueryRequest's
comment field becomes an immutable QueryIntent, and MutationRequest becomes a
distinct envelope over MutationCommand. Nine initial compilation errors are
retained as diagnostic history, not successful qualification.

The new observer constructs those authentic source types and uses the explicit
intent labels already present in the maintained C++ fixture. It retains each
original input's comment/text selection. Blank effective comments return the
actual RequestIntentError code/field/message: **27/40** inputs reject before model
construction; the remaining **13** produce full structural observations. It
does not invent a valid comment for invalid input or weaken runtime validation.

The C++ fixture unconditionally calls intent.value() and aborts on the first
invalid input. This is a fixture error, not missing runtime rejection. After full
CTest terminates, report the actual typed error and continue valid observations;
also qualify new request-envelope and row formatting rather than substituting
the old field layout. The [source/error record](parity/data_service_model_source_gap_5_0_6.json)
preserves all rejected indexes and the first expected error.

## Row formatter propagation

Request helpers match all but **15/872** facet cases. All differences are in the
enclosing list's row diagnostic text, from the existing CompactRow formatter
gap. Its [difference record](parity/request_helpers_source_gap_5_0_6.json) retains
the exact paths. This shares the runtime correction required by
[core_protocol_replay_5_0_6.md](core_protocol_replay_5_0_6.md).

## Audit concepts missing from the C++ model

The event comparison fails **112/112**, and code inspection confirms missing
responsibilities rather than a formatting-only discrepancy:

- SafeAuditEvent lacks the positive integer target entity_id, independent of
  parent trace identity and old snapshots. Source projects only positive U64/I64
  from the event's current values; arbitrary Value coercion is not equivalent.
- SafeAuditEvent lacks bootstrap_audit attribution. Source copies it and scrubs
  actor/reason with the same collected privacy operands used for safe lineage.
- RawAuditEvent lacks diagnostic_redactions. Source carries invocation-local
  SQL intent provenance into safe event/log projection, including protected
  operands that need not appear in the event's changed fields.

The [difference record](parity/event_model_source_gap_5_0_6.json) retains every
failed formatting path and a complete compact example. Correct the event model,
destination-scope copy/equality behavior and shared projection; then update its
formatters. Do not append placeholder text while omitting actual metadata.

The C++ implementation must use its existing SqlIntentRedactions mechanism and
explicit scoped ownership/copies. Source Arc ownership does not authorize
extending a UserContext lifetime. Preserve independent event, logging,
data-service and SQL layers. Thread provenance from actual audited mutation
dispatch into event projection, preserve credential masking even in plaintext
mode, and test operands absent from changed fields. Add targeted sanitizer gates
for new nested retention and actual Context destruction.

After full CTest exits, fix these shared responsibilities before switching active
comparators. Preserve the earlier complete-run evidence and all current failing
source records. No sanitizer/module/MIPS or physical-device result is inferred
from this source replay.
