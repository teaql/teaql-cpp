# Runtime log model and responsibility review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Context logging (391 production lines), formatter/manager (through line 566)
and log_privacy are one mapped acceptance group, with independent responsibilities.
This review records source mappings and the model gap; complete privacy-body,
call-site/composition and installed target acceptance remain open.

| Responsibility | Native mapping and source boundary |
| --- | --- |
| SQL operations/options | Five distinct operation variants; select versus mutation gating; default all, disabled/select-only/mutation-only factories. Source Copy/Clone/equality/Debug is preserved. |
| SqlLogEntry | Full SqlLogContext/provenance, operation, comment/purpose/audit reason, trace, parameterized SQL, parameter Values, debug/pretty SQL, start/end SystemTime, Duration, result count/type/affected rows and summary. Parameter count is independent of visibility. |
| Unified models | Timestamp, optional user, trace and distinct Sql/Info payload remain separate. Full Clone/PartialEq/Debug retain all nested fields and source NaN behavior. |
| Unified buffer | Default empty buffer, explicit-domain entry copy and checked registered borrow. Source Arc/Mutex ownership becomes Context confinement; no implicit owner keeps User data alive. No Debug derive is invented for the source buffer. |
| Context controls | Independent select/mutation enable/disable/set/fluent options; disable-all clears SQL entries. Retrieval copies into the result domain; clear affects logical entries. |
| Metadata recording | Recurse into leaf statements in order, gate by source operation mapping, normalize trace, choose latest intent comments, populate database fallback and result summary, derive elapsed from start/end with zero fallback. |
| Safe storage/publication | Construct sensitive source entry first, derive safe projection for retained SQL/unified entries, then send ordinary safe and explicitly sensitive views independently. Raw executor bindings remain unchanged. |
| Canonical trace | Preserve an already canonical Operation/Provider/Sql path while removing intent nodes; otherwise derive operation/request-or-entity, relation nodes, provider and SQL family with source fallback names. |
| SQL display preparation | Source keyword/newline replacements and AND indentation remain exact; do not substitute a SQL parser or alter execution SQL. |
| Formatter SPI | Independent SQL/audit and default sensitive-audit method delegating conservatively to ordinary audit. Third-party formatters are not forced to disclose raw audit values. |
| Human formatter | Exact trace formatting, elapsed microsecond conversion, outcome/intent optionals, summary/debug SQL fallback, and ordinary value-free audit field lists versus explicitly sensitive values. |
| Debug formatter | Structured SQL model view, ordinary audit names-only and separate raw-sensitive audit output. Raw model Debug is not itself a safe publication API. |
| LogLevel/config | Four exact levels and parse tokens with caller fallback; source environment defaults, optional comma-split/Unicode-trim filters, ASCII-insensitive matching and sensitive-audit gating. LogConfig has no source Debug/Clone derive to invent. |
| Factory/manager lifetime | Source static Once/OnceLock states become explicit application Global-owned factory/config/endpoints/header state, accessed through User Context. No process cache owns Context payloads. |
| Endpoint selection | Exact trace-off acknowledgement, endpoint/domain/executable fallback, distinct sensitive SQL/audit endpoints, header once and append/stdout behavior. Empty/blank endpoint rules preserve the source differences. |
| Ordinary/sensitive dispatch | Always project before formatter/sink. Sensitive output additionally requires exact acknowledgement and configured endpoint; SQL/audit level filters remain independent. IO failures preserve source fail-open behavior. |
| Sensitive truncation | Enforce source 64 KiB bounds on UTF-8 boundaries, with explicit non-executable SQL truncation annotation and separate audit annotation. |

## Privacy/provenance acceptance boundaries

Safe/default entries retain projected parameter values at original positions.
Credentials, policy-masked business values, target IDs and inherited intent have
different source rules. Projection state and intent provenance survive explicit
retention without reinterpreting one statement's positions against another.
Neither these mappings nor raw diagnostic parity accepts complete log_privacy
or runtime query/save call sites. Those require their own complete body review,
controlled environment/sink tests and statement/descendant failure evidence.

All Context-owned strings, Values, traces, provenance and optional fields copy
explicitly into selected domains. Async observers and sinks run under owner-thread
rules, stop before reclamation and cannot reclaim their active Context. External
log files/formatters are RAII/application resources; retained views are borrows.
Raw Debug contains raw fields by source design and must never replace the existing
safe publication projection.

## Complete privacy projection body review, 2026-10-08

The complete production body of pinned `log_privacy.rs` (lines 1–410,
before its tests) was compared with `src/runtime/audit_privacy.cpp`,
`src/runtime/logging.cpp`, and the acknowledgement path in
`src/runtime/log_manager.cpp`. This is a body review; installed regression,
formatter/manager composition and every query/save publication call site remain
separate acceptance requirements.

| Source responsibility | Native body and preserved behavior |
| --- | --- |
| Acknowledgement and warning | `LogManager::plaintext_enabled` compares the entire acknowledgement exactly. The source process-wide warning becomes application-owned Global state under the explicit ownership contract. |
| Credential recursion | Both Value and JSON objects inspect field names recursively; lists/arrays recurse. The already accepted data-service credential-name predicate is used. |
| Secret collection | Null and TypedNull contribute nothing; empty strings contribute nothing; scalar display, timestamp milliseconds, nested lists/objects and JSON scalar serialization preserve source distinctions. |
| Scrubbing | Secrets are sorted by descending byte length, preserving equal-length insertion order. Replacement is literal. Trace comments and entity names are scrubbed; trace entity IDs remain typed fields. |
| Business masking | Null variants are preserved, lists recurse, objects/JSON become the redaction sentinel, and other scalars use the source Unicode-aware mask. |
| Projection reuse | Safe mode cannot acquire plaintext permission. A prior debug entry reuses its safe alternative only when SHA-256 of its complete compact Debug representation matches. Opaque state contributes only its private diagnostic marker. |
| Safe alternative ownership | Only the safe entry is retained, with no raw source bindings or recursive state. C++ explicit destination copying replaces source shared ownership without extending User lifetime. |
| Parameter decisions | Invalid policy/mask lengths imply Unknown. Credential policy, nested credentials and unsafe credential SQL force protection. Unknown never permits plaintext. Already-masked positions retain protection. |
| Intent and provenance | Target-ID/binding secrets are added before scrubbing optional intent fields, result summary and trace. Intent provenance is then cleared; positional mask flags and safe/debug mode are preserved. |
| Rendering and failure | SQLite rendering validates policy/mask lengths and rejects untrusted literal/comment templates. Failure revokes all bindings to Null, replaces SQL with the non-replayable sentinel, and scrubs all original parameter secrets. The exact previous-omission allowlist is retained. Other database implementations remain explicitly excluded. |
| Lost alternative | A missing or modified debug-state fingerprint hides intent and trace. Recognized singular/plural count-only summaries remain execution facts; otherwise the source count fallback is used. Result count takes precedence over affected rows, including zero. |
| Raw audit projection | Values, old/new snapshots and changes are projected independently. Credentials remain redacted with opt-in; ordinary business values are redacted without it, except id/version. Null variants survive. Target IDs scrub trace and bootstrap actor/reason without mutating the source event. |

The existing SQL fingerprint writer includes all fields in the staged derived
model writer, in the same source order: full log context, operation, intent,
trace, SQL/parameters, rendered SQL, times/duration and result metadata.
The recorded projection corpus currently contains 425 cases; Context logging
contains 327, formatter configuration/output 927, and isolated manager output 94.
These are corpus sizes, not a claim that a fresh installed run has passed.
The indexed privacy tests additionally cover exact opt-in, unchanged raw events,
credential exclusion, four mutation traces, retained safe downgrade and isolated
file publication. Complete log-source acceptance remains open until the installed
tests and remaining composition/call-site review finish.

## Reproduced model omissions and original observations

A public runtime umbrella compile reproduces missing source-derived diagnostics
for SQL operation/options/entry, Info entry, payload, unified entry and LogLevel.
The staged repair preserves complete field order and delegates provenance,
SystemTime and Duration to already reviewed model writers. It changes neither
projection nor formatter/manager routing.

The actual original oracle records 172 twice-agreeing inputs in nine modes:
all operation/option gates, default options, level parsing/fallback, full SQL
provenance/params/times/result fields, both payload variants, Clone/PartialEq,
parameter count, Unicode/embedded-zero text, NaNs and full unsigned Duration.
LogLevel lives in the original private formatter module, so the existing fixture
includes unchanged formatter/event/privacy source; its algorithm is not copied.
The staged native probe matches all 172 live original cases.

A default-PMR-disabled ownership test copies a complete unified buffer with SQL
and Info entries into Global, destroys its source User, verifies nested parameter
ownership, formats through a new User and rejects expired SQL references.
Provenance redaction Debug stays source count-only, while other raw fields remain
visible. Cross-target and production/CMake integration remain independent;
physical-router execution is unverified.

## Current standalone model evidence

Native, ASan/UBSan and MIPS/QEMU finish successfully. Each probe matches
172/172 live original model observations and each default-PMR-disabled nested
log-buffer ownership test passes. ASan/UBSan disables leak scanning. Complete
privacy-body/formatter/manager/call-site acceptance and installed integration
remain open; no full logging source acceptance follows from model parity alone.

## Shared buffer Clone capability gap, 2026-10-09

An independent original Clone script reveals a semantic gap absent from the
172 raw diagnostic cases: UnifiedLogBuffer clones share mutable entries in
Rust. C++ same-domain construction previously copied the vector. The native
reproducer disagrees on storage identity and both mutation observations.

Same-domain native construction now borrows one registered arena vector;
explicit cross-domain construction deep-copies into destination-owned storage.
This preserves shared mutations without a shared owner extending User lifetime.
The original object can be destroyed while another same-Context handle remains
valid. All vector objects and nested entries are destroyed before arena blocks
are reclaimed. Each new vector state consumes a managed object slot.

The new clone/retention tests are installed, with focused native and selected
sanitizer/MIPS consumers building. Current full native351 validation and full
log-source/publication acceptance remain open.

## Complete Context logging and formatter/manager body comparison

The complete Context logging production body (1–391) and formatter/manager
body (1–566) were compared with context_logging.cpp, log_formatter.cpp and
log_manager.cpp. Leaf statements recurse in order; source operation mapping,
latest intent selection, canonical trace branches, backend fallback, elapsed
zero fallback and count precedence are preserved. Safe SQL entries are retained
before ordinary and explicitly sensitive dispatch, without changing raw bindings.

Human and Debug formatting retain separate ordinary/sensitive audit views,
source ID preference, underscore-field filtering, exact trace strings, duration
rounding and missing-safe-SQL text. Configuration preserves defaults, exact
level tokens, comma-split Unicode trim and ASCII-only matching. Explicit Global
ownership replaces process statics for factories, config, endpoints, headers and
warnings. Endpoint and acknowledgement distinctions, fail-open file IO,
poisoned once-state behavior and UTF-8 truncation annotations remain separate.

The buffer Clone capability is repaired independently of these display and
projection bodies. Corrected native focus passes10/10, and its clone observation
matches live original Rust. Full native351, fresh broader target selections and
remaining query/save publication/call-site acceptance are still required.

The installed corrected provider/buffer selection now passes42/42 on both
ASan/UBSan (31.10 seconds, leak scanning disabled) and MIPS/QEMU (22.51 seconds),
following no-work refreshes. The independent Clone observation matches live
original Rust on native and MIPS. Complete native351 integration is pending.

## Publication boundary follow-up

The complete metadata-to-Context projection bridge and event publication
ordering are compared with source data_service/types.rs and Context send_event.
Query intent belongs to a private query tree, is accumulated leaf by leaf with
normalized statement provenance, and never holds a lock across callbacks.
Context metadata uses independent select/mutation capture gates. Raw sinks run
first, safe sinks apply descriptor mask/length rules, and ordinary/sensitive
LogManager publication follows successful sinks. Governance and generated
bootstrap provenance attach before telemetry and sink dispatch.

Base query/mutation and resolved prepared-query publication entries preserve
their separate observer and successful-result recording paths; their existence
does not accept complete service/save call sites. A returned-work review finds
a C++ lifetime gap before these normal paths: foreign work was rejected without
cancellation at several factories. The cleanup repair is installed after
native351 and documented in the runtime-foreign-work measurements. Initial
composition failures and their corrected assertions/emulator invocation are
recorded there; refreshed integration is required.
Full service/transaction/graph publication and final privacy acceptance remain
independent requirements.

## Base service and transaction publication ordering, 2026-10-09

The base service publication paths were compared directly with pinned base.rs.
Plain fetch_all returns rows without recording successful metadata; smart-list
and entity fetch record it before decoding. All use the metadata-selected
diagnostic observer for executor failure. Mutation paths record each successful
statement before optimistic-conflict classification or batch accumulation;
earlier successful statement logs remain when a later statement fails. These
differences are deliberate source behavior, not a uniform publication policy.

TransactionScope query/mutate select observers independently from their Context
select/mutation flags, convert executor errors to Transaction, and record
successful metadata before returning the provider result. The transaction
facade validates and tracks returned work; PendingOperations cancels rejected
foreign work even before ScopeRequest stores a pointer. Callback completion
commits or rolls back through the consuming native transaction; remembered
ledger intent clears only after successful commit. This review adds no
post-commit SQL publication rule. Wider graph/audited-save publication and
complete installed privacy acceptance remain separate gates.

The resolved entity mutation publication bodies (resolved.rs, insert/update/
delete/recover and prepared batch entries) were also compared directly with
EntityMutationOperation. Successful base writes precede event construction and
dispatch. Update merges supplied complete old values with written values, adds
the original id/version fields, and preserves the explicit changed-field list;
the pinned current-event-row helper deliberately returns no database snapshot.
Delete attaches its selected lineage while recover deliberately leaves its
event lineage empty. Batch writes finish before emitting per-row events in
input order, stopping on the first sink error. Empty batches emit no event.
These events remain within the invoking transaction when a transaction-bound
repository is used; no uniform after-commit event queue is introduced.

## Current full-body reconciliation, 2026-10-09

All production lines of context/logging.rs1–391, log_privacy.rs1–410 and
log_formatter.rs1–566 were compared again with the installed log models,
context_logging.cpp, audit_privacy.cpp, log_formatter.cpp and log_manager.cpp.
Control factories/defaults, operation mapping (including Recover/Batch/Schema
as Update), leaf-statement recursion, latest-present intent, canonical trace
branches, database fallback, zero elapsed fallback and count precedence match.
Safe records are retained before separate ordinary/sensitive publication.
SqlLogEntry/unified model Clone/equality/Debug preserve all fields and NaNs;
no source serde/Hash protocol is invented. Same-domain UnifiedLogBuffer clones
share a registered vector, while destination copies own nested storage.

Projection reuses a safe alternative only under the complete source SHA-256
Debug fingerprint. Opaque state is private; a missing/tampered alternative hides
unknown inherited intent. Safe labels never upgrade. Credentials/nested secrets,
Unknown/malformed provenance, business masks, existing masks and target IDs
retain independent decisions. Render failure clears bindings to Null, revokes
untrusted SQL and scrubs all source values before the exact omission allowlist
is applied. Singular/plural count-only summaries retain zero and source result-
count precedence. Raw audit snapshots/changes project independently; ordinary
business data and credentials retain distinct opt-in rules, id/version handling,
trace scrub and bootstrap actor/reason behavior. PostgreSQL/MySQL rendering
branches remain outside the agreed SQLite scope, rather than guessed support.

Formatter SPI defaults stay conservative. Human/Debug ordinary audit output is
value-free, sensitive audit is separate, new-record ID precedes request ID,
underscore fields are skipped and trace/time/duration/optional text remains
source-shaped. Config retains exact tokens/default levels, comma-split Unicode
trim and ASCII-only comparisons. Factory config/endpoints/header/warning state
belongs to application Global; no process cache owns User payload. Built-in
factory selection retains source lazy behavior and offers no custom formatter
registration to assume. Low-level formatters remain pure explicit-resource APIs.

Endpoint/acknowledgement distinctions, stdout exceptions/once poisoning, fail-
open file errors and UTF-8 64KiB truncation annotations are unchanged. The one
header-text correction explicitly documents that json selects Debug text; it
is an informational header adaptation, not a data/provenance protocol change.
Oracle comparisons normalize only that documented line and volatile Human UTC
prefixes; other stdout/stderr/file bytes remain exact.

Base/context/resolved/TransactionScope publication paths were reconciled with
their current adapters. Successful mutation metadata precedes optimistic-lock
classification and raw events. Prepared batch writes finish before ordered
per-row events; first sink failure stops later publication. Graph/ledger executors
reach these same mutation adapters and do not introduce a post-commit event
queue. Query/authoritative read capture retains its independent select option.
Broader graph-plan/execution semantics remain a separate source group.

Current native/ASan-UBSan/MIPS selections each pass16/16 after successful
consumer builds, with post-test no-work; sanitizer leak scanning is disabled.
Complete native375/375 is unchanged. Fresh original library runs pass39 privacy
and4 formatter tests; the one ignored privacy child is explicitly executed by
the passing isolated-process parent for all three acknowledgement cases.
Live native comparisons match172 model,1 shared-buffer Clone,425 projection,
327 Context,927 formatter and94 isolated-manager observations. MIPS matches
172/1/425/327/921/94; six native-width formatter counts reject before execution
and are checked against actual original Rust u32 parsing.

The complete three-file logging source group is accepted. Evidence is recorded
in measurements/parity/2026-10-09-log-source-acceptance. This accepts the source
responsibility, not complete Context/service/graph/runtime-root composition or
stages4–6. Physical WNDR4300 execution remains explicitly deferred.
