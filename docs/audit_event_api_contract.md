# Raw and safe audit event production review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Complete event.rs production source is reviewed, lines 1–572. RawAuditEvent,
SafeAuditEvent, their sinks and source privacy projection are distinct concepts.
The complete event source responsibility is accepted after current installed
validation. Broader Context/save/log-privacy composition remains independent.

| Responsibility | C++ mapping and semantic boundary |
| --- | --- |
| RawAuditEventKind | Preserve Created, Updated, Deleted, Recovered and four schema/bootstrap kinds, with source Copy/Clone/equality and derived Debug. |
| EntityPropertyChange | Field plus independent optional old/new Value; None differs from Some(Null). Constructor/Clone/PartialEq and raw Debug preserve NaNs and nested values. |
| RawAuditEvent | Preserve kind/entity/values/updated fields/old/new records/changes/trace/bootstrap/governance as separate fields. Full source Clone/PartialEq/Debug is independent of privacy projection. |
| Created/updated factories | Created changes contain absent old and each full new value. Updated derives ordered fields and changes; updated-with-old preserves supplied fields, duplicates, missing-value optionals and independent records. |
| Deleted factories | ID and optional expected version remain request values. Ordinary delete has no changes/old/new; delete-with-old supplies old-to-None changes and an old snapshot. |
| Recovered factories | Ordinary recover retains expected version as request values. With-old creates a new record with ID and `1 - expected_version`, and only the version change. Debug overflow versus release wrapping retains the documented source build profile. |
| Bootstrap identity | Actor/category/reason, optional resulting version and complete u64 occurrence milliseconds; Clone/equality/Debug preserve all fields. |
| Schema factories | Table/field/count values remain source records and changes. Counts use the source native-width cast profile; no new/new_values record is invented. Field-added and seeded inserted/updated counts remain independent. |
| SafeAuditField | Preserve optional value, mask/truncation flags, raw/output Unicode lengths and independent optional reasons. No raw Value or old-value payload is added to the safe type. |
| SafeAuditEvent | Preserve projected fields/trace/governance and source kind/entity, without bootstrap or raw snapshot fields absent from the source safe type. |
| Safe projection | Collect strings from credential/masked/credential-containing old and new values, longest first; skip underscore-prefixed fields. New values stringify Text and integers directly, other variants through raw Debug before projection. |
| Credential precedence | Credentials and nested credentials remain redacted even under business plaintext opt-in. Credential-containing values force complete REDACTED output, reset truncation and keep source metadata behavior. |
| Business masking/truncation | Exact field matching, source acknowledgement gate, Unicode scalar counts, numeric/short masking, head/tail masking, then max-length truncation. Reasons and raw/output lengths retain source order. |
| Trace scrubbing | Include collected sensitive strings and request ID strings, longest first; scrub trace text and remove source identifiers under the existing privacy implementation. Governance remains the source copied model, without invented stripping. |
| Unicode helpers | limit_audit_value returns unchanged within limit, stars for limits <=3, otherwise source head/ellipsis/tail split. Mask helper counts Unicode scalar values and ASCII numeric digits, not UTF-8 bytes. |
| Raw/safe sinks | Separate SPIs. Default/new raw fan-out registry is empty, registration order is preserved, Clone borrows checked original sinks without prolonging their owners, and first Result error stops later sinks. |

## Ownership and scheduling adaptation

Raw/safe models and all nested records, Values, traces, fields and governance
copy explicitly into the destination Context. Source Arc-owned sink entries
become checked owner borrows with scoped registry copies, rather than shared
ownership of Context data. Registration, projection, dispatch and close follow
owner-thread Context rules. Actual provider/sink lifetimes must outlive borrowed
use; stale references reject before dispatch. Cross-domain retention owns copied
payloads and never retains the source User arena implicitly.

Projection and full raw Debug are different APIs. A raw formatter must not
silently redact a source field, while a safe formatter must not reconstruct raw
payload from its safe model. This review preserves original provenance and
privacy boundaries; complete log/formatter/environment/save call-site acceptance
remains a separate Stage 5 gate.

## Reproduced model gaps and independent evidence

A public runtime umbrella compile reproduces missing source-derived diagnostics
for the six public event types. Staged complete writers preserve field order,
optional records/values, traces, bootstrap/governance metadata and inherited
formatting options, using the separately staged policy diagnostic dependency.
No event factory or privacy implementation changes are required by this repair.

The actual public original oracle records 112 twice-agreeing inputs in nine
modes, invoking every raw factory, Clone/PartialEq and actual build_safe_event.
Cases include empty/duplicate/missing fields, full-width values, NaNs, nested
credentials, Unicode text, source acknowledgement on/off, several truncation
limits, complete bootstrap/trace/governance data and common native-width schema
counts. Existing broader audit/projection/environment/width observations remain
independent evidence; this diagnostic matrix does not replace them.

The staged native model probe matches all 112 live original cases. A lifecycle
test disables default PMR allocation, explicitly retains raw and safe nested
models in Global, destroys the original User, checks destination-owned payloads,
formats safe retained values in the new caller's User and rejects expired raw
references before access. It verifies raw diagnostics preserve actual credential
data while safe diagnostics do not reintroduce it. Native/sanitizer/MIPS and
production/CMake validation remain separate; physical-router execution is
unverified.

## Current standalone target evidence

Native, ASan/UBSan and MIPS/QEMU builds finish successfully. All three model
probes match 112/112 live original cases; all three default-PMR-disabled
retention/privacy executables pass. ASan/UBSan disables leak scanning. The
reviewable patch and source fingerprints are saved in the evidence directory.
Production/CMake and complete runtime/save/privacy composition remain pending.

## Full-body reconciliation follow-up, 2026-10-09

All 572 production lines were compared again with installed event models,
factory methods, diagnostic writers and source-aligned safe projection. Raw and
safe models preserve independent bootstrap/governance/trace/record fields;
there is no source serialization derive to invent. Factory ordering, absent vs
present-null changes, sparse updates, schema native-width counts and the
recover-version Debug/Release boundary remain explicit. Projection collects old
and new credentials, orders secrets longest first, applies credential/business
masking before truncation, preserves reasons/Unicode lengths and scrubs trace
intent while copying governance unchanged.

The concrete InMemoryRawAuditEventSink fan-out preserves registration order and
first Result failure, but its direct public callback dispatch needs independent
lifecycle validation. Context.send_event_inner guards its larger event pipeline;
that protection alone does not prove direct fan-out calls safe against reentrant
Context close. This remains a verification item, not a reproduced failure or an
accepted event-source verdict. Current native/sanitizer/MIPS consumer validation
for the separate checker repair is active; production inputs remain stable.
Physical-router testing is explicitly deferred by the user.

## Controlled direct fan-out reproduction, 2026-10-09

The separately formatted/compiled MIPS fixture now confirms the pending gate:
InMemoryRawAuditEventSink.on_event directly borrows UserContext while its fan-out,
callback, raw event and result domain are Global-owned. The callback closes the
User and throws before accessing reclaimed payload. QEMU exits 2 with the exact
controlled diagnostic, proving close returned. Context.send_event already has
an outer synchronous guard, but direct public fan-out bypasses it. The concrete
entry requires equivalent Context-borrow protection, unchanged first Result
failure/ordinary exception behavior, and guard release before actual User
reclamation. Full native 371/371 is preserved before that production repair.
See measurements/parity/2026-10-09-event-direct-callback-review.

## Installed direct fan-out protection, 2026-10-09

Four separate controlled native calls reproduce close returning: the supplied
Context, fan-out owner, raw-event owner and requested result arena can each be
closed by a callback. Each closes and throws before any reclaimed payload is
accessed, exiting2 before repair. The concrete entry now registers a stack-owned
quiescence record for each distinct borrowed arena; repeated domains are guarded
once. No heap allocation, persistent registry or lifetime extension is introduced.
This rejects reentrant arena reclamation using the existing synchronous-call
termination contract. The ordinary callback exception propagates exactly once,
detaches all guards and permits actual deletion of all four Users with zero live
User bytes. Existing success, duplicate registration and first Result-failure
contracts remain unchanged.

Four named CTest children verify context/owner/payload/result close paths. Current
native375 consumer integration and sanitizer/MIPS event/save/privacy selections
are building; no new passing result or source acceptance is claimed yet. The
accepted371 snapshot and controlled source/argv/results remain retained.

## Final event source acceptance, 2026-10-09

The complete572-line production body, every public/private factory, inherited
model default/trait/diagnostic, privacy helper and sink responsibility are
reviewed above. Final current native regression passes375/375 in236.70 seconds
with418 nonempty compiler dependency records and no-work before/after testing.
The current sanitizer and MIPS selections each pass20/20 after28-task consumer
builds finish; sanitizer leak scanning is disabled. Four independently borrowed
arena close paths reject before reclamation, ordinary exceptions detach all
records and actual four-User deletion returns all blocks. Existing Result-first
failure, duplicate registration and successful dispatch preserve source order.

Live original models/projection/Context outputs match native112/1427/1088 and
MIPS112/1421/1088. Six MIPS native-width count inputs explicitly reject before
execution and match Rust u32 parsing. No fixture is removed or rewritten to
match. Source event models derive no serde/Hash/Default beyond the documented
empty sink; no such protocol is invented. Send/Sync adapts to Context-owner
execution without implicit shared lifetime. This accepts the event source
responsibility, not complete Context/services, graph saving, log privacy or Stage5.
Full evidence remains in measurements/parity/2026-10-09-event-direct-callback-review.
