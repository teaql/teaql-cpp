# Complete formatter and log-publication source review

Status: the complete source is accepted under LF01-LF18 with explicit Context,
owner-thread and informational-header adaptations. Current native15/imported15
and all four original diagnostic tests pass. This is source qualification, not
complete governed execution, G14/G17 or P0-P7 acceptance.

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This reviews every responsibility in the 633-line
`teaql-runtime/src/log_formatter.rs`, including its four original tests and
the included startup header. The source combines formatting, configuration,
factory and manager responsibilities; C++ keeps those responsibilities in
`log_formatter.hpp/.cpp`, `log_manager.hpp/.cpp` and `log_header.hpp`.
Their separation does not remove the source publication order or privacy gates.

## Source responsibilities and adaptations

| ID | Original responsibility | Current C++ mapping and boundary |
| --- | --- | --- |
| LF01 | Formatter SPI | SQL and ordinary audit methods remain required. The sensitive-audit method defaults to the ordinary formatter, so implementing the existing SPI does not implicitly opt a custom formatter into payload disclosure. Built-in overrides are deliberate. |
| LF02 | Human trace and audit formatting | Preserve level/kind/entity/comment ordering, optional trace punctuation, mutation kind and the field-change sequence. Exclude fields starting with underscore. Ordinary output lists field names; sensitive output renders new values, with absent new values written as `null`. |
| LF03 | Human SQL formatting | Preserve UTC milliseconds, rounded and saturating microseconds, minimum duration width, optional intent Debug, outcome, result summary and the supplied trace. Missing projected debug SQL produces the non-replayable omission text. |
| LF04 | Debug formatting | Preserve the empty/nonempty trace forms and complete SQL Debug. Ordinary audit output includes entity, display ID, kind and non-internal field names. Explicit sensitive audit output uses complete raw-event Debug. Direct formatter calls do not replace the manager's privacy projection. |
| LF05 | Display identity | Prefer an id entry in new_values, otherwise current values, otherwise `Unknown`. A present Null id is still present; this formatting rule is separate from positive numeric SafeAuditEvent identity extraction. |
| LF06 | Factory and configuration ownership | Source process-wide lazy state becomes application Global-owned state accessed through UserContext. Formatter selection and configuration are separately lazy and remain cached after first use. Independent applications select independently. Returned formatters are borrows. |
| LF07 | Levels and defaults | Four exact parse tokens with caller-selected fallback. Defaults remain audit Full, SQL Summary and tool Full. LogLevel retains its model diagnostics; no source Debug/Clone derive is invented for LogConfig. C++ destination copies exist for explicit ownership transfer. |
| LF08 | Filters | Preserve unset versus present-empty lists, comma splitting, Unicode whitespace trimming and ASCII-insensitive entity/module equality. SQL filtering uses ASCII-insensitive substring matching. An empty table token matches all text; an actual empty table vector matches none. These are diagnostics filters, not authorization. |
| LF09 | Sensitive-audit level | FullWithPayload plus the ordinary entity filter is required. Summary and Full do not grant raw audit output. SQL's non-Silent levels share the original manager filter behavior; a payload level is not plaintext permission. Tool configuration is retained without inventing a tool publication path in this source. |
| LF10 | Ordinary endpoint | Preserve exact trace-off acknowledgement, then nonempty explicit endpoint, nonempty domain plus `.log`, executable filename plus `.log`, and `teaql` fallback. Blank ordinary paths remain paths. Linux executable-name decoding preserves lossy UTF-8 behavior. |
| LF11 | Separate debug endpoints | Resolve SQL and audit destinations independently and lazily. Reject unset or whitespace-only values while preserving the original untrimmed accepted filename. The token `stdout` selects stdout; `off` is special only on the ordinary endpoint path. Cached absence remains absent after later environment changes. |
| LF12 | Header and writes | Write the ordinary header once per application, append files and terminate records with a newline. Ordinary `off` suppresses both ordinary and sensitive output. File open/write errors remain fail-open. Failed stdout/stderr writes surface errors; once-only header/warning state remains poisoned after failure. |
| LF13 | Ordinary SQL ordering | Clone the source, replace trace_path with the supplied trace and project safe before applying the SQL filter and endpoint gate. Format and publish only the projected entry. Never mutate the business/execution input. |
| LF14 | Sensitive SQL ordering | Check current exact plaintext permission first, then clone/replace trace/project, apply SQL and ordinary-off gates, require the separate SQL endpoint, format, bound and publish. A retained safe entry cannot be upgraded by this path. |
| LF15 | Audit ordering | Apply entity/level and ordinary-off gates; publish a safe ordinary event before considering sensitive output. Sensitive output additionally requires FullWithPayload, current exact permission and its audit endpoint, followed by a separate projection and formatter call. |
| LF16 | Sensitive bounds | Keep at most the requested prefix byte count without splitting a UTF-8 scalar, then append the source annotation. SQL truncation explicitly says NOT EXECUTABLE; audit truncation has its distinct notice. The final annotation can make the complete output longer than the prefix bound. Manager paths use 64 KiB. |
| LF17 | C++ lifetime and cleanup | Require output scratch storage to belong to the manager's application. Configuration/endpoints live in Global; per-call events, projections and strings stay in the supplied scope. Actual User close returns its blocks; manager state retains no request payload. Quiescence completes diagnostics before reclamation. File handles use real RAII cleanup. |
| LF18 | Informational header disposition | Source `json` and `debug` both select DebugReaderFormatter, not a JSON wire format. The C++ header accurately documents that behavior. The single reviewed informational line is an explicit output adaptation; remaining header and publication text must match. |

The source's Send/Sync and process-static storage are mapped through the
approved Context ownership and owner-thread rules. This does not add arbitrary
concurrent mutation of one application's lazy configuration. Plaintext
permission is checked on each call; its warning state is cached separately.
No singleton, thread-local owner or shared pointer retains Context payloads.

## Actual source and imported verification

The [replay receipt](parity/log_publication_source_replay_5_0_6.json.gz)
records the original four diagnostic tests and complete formatter/publication
observations. Its `complete` flag and terminal exit statuses must be checked
before treating it as successful. The unit-test view retains original
source/test/fixture bytes; the only source append is the independently recorded
commit-audit observer in another module. Optional exporter test dependencies
remain absent with their feature disabled; no formatter test body is removed.

The existing actual-source observers and their dependency registry identities
are checked against the frozen Cargo.lock. Their unchanged bodies include the
actual formatter, event and privacy modules. The formatter matrix contains
468 configuration, 315 SQL-format and 144 audit-format observations, for 927
total. These compare fresh original execution, retained expectations and
current imported C++ output. Native audit-count parsing is also checked at the
actual 64-bit target width; this is not new MIPS qualification.

The 94 manager fixtures each run in a fresh process and compare stdout,
stderr and the complete synthetic log-file map. They cover endpoint/format/
level/filter gates, lazy state after environment changes, warning reuse and
revocation, file-open recovery, retained safe input and multi-byte payload
truncation. Only the volatile human UTC prefix, executable basename in the
default log filename and the single informational header correction normalize.
Other text, filenames and warning stderr remain exact.

The replay is now terminal and successful: **1,021/1,021** outcomes match fresh
original and imported execution (927 formatter/configuration plus 94 isolated
manager fixtures). Every one of its 12 recorded commands exits zero; the
manager comparison additionally completes 188 isolated processes, one original
and one C++ process per fixture. All four original tests and both imported
ownership/publication contracts pass. The
[provisional join](parity/log_publication_source_provisional_5_0_6.json.gz)
verifies that all 10,329 frozen input hashes and both fixed references remain
unchanged. That provisional checkpoint did not promote a source-ledger row
while the native epoch's inputs were frozen.

Both complete permanent imported ownership contracts also run. The formatter
body checks conservative custom-formatter defaults, lazy selection, destination
copies of nested filters, actual User deletion and disabled default PMR.
The manager body checks independent application warning state, permission
revocation, foreign/closed output scopes, repeated request reclamation,
quiescence, fail-open files and poisoned stdout/stderr initialization.

## Historical integration boundary

The provisional review ran independently while the full 736-test native epoch
was frozen. That epoch is now terminal at 735/736; the failed whole-suite outcome
remains in the [integration record](native_integration_5_0_6.md). These imported
results did not alter that epoch or establish a whole-suite pass.

The historical [logging API review](runtime_log_api_contract.md) and
[5.0.6 family replay](logging_baselines_5_0_6.md) retain their own scope.
The [complete privacy review](runtime_privacy_source_review_5_0_6.md) covers
safe alternatives, provenance consumption and render failure independently.
The current source continuation uses a repository-relative reference project,
the complete unchanged private unit-test view and canonical current consumers
through [the permanent driver](../scripts/verify_log_publication_source_5_0_6.py).
Its terminal result is required for promotion. Complete Context assembly,
graph/save lineage, protected execution, G14/G17 and P0-P7 acceptance remain
independent. No production, generator, SDK, sanitizer or architecture change is
made by this replay.

## Durable source and frontend qualification

The [permanent source project](../tests/parity/rust_log_publication_reference_5_0_6/Cargo.toml)
links the actual locked reference and reuses all seven retained observer bodies.
Its dependency registry identities match the reference versions/sources/checksums.
Unlike the earlier temporary unit view, the current complete private runtime
view appends **no observer** and preserves all 67 source/test/fixture files.
Only disabled optional exporter dev dependencies are omitted. The actual
diagnostic_log_tests module passes **4/4**; both original Context logging vector
tests pass separately.

The [terminal execution receipt](parity/log_publication_durable_execution_final_5_0_6.json.gz)
matches all seven source families against current native and imported execution:
327 Context, 425 SQL-privacy, 1,427 audit/model, 172 complete log-model, one buffer
Clone, 927 formatter/configuration and 94 manager observations. These total
**3,373 per frontend**, including overlapping adjacent model responsibilities.
The 927 formatter cases retain their distinct 468 configuration, 315 SQL-format
and 144 audit-format observations. No existing expected output is changed.

The direct manager comparisons complete 94 original, 94 native and 94 imported
fresh processes with actual stdout/stderr and complete file maps. The existing
normalizer still permits only human UTC, executable basename and the reviewed
informational-header correction. Source and C++ execution operands are unchanged.
Selected CTest comparisons independently replay their installed corpora.

Both frontends pass **15/15** checks with successful builds and final no-work
builds. Canonical registration now also includes the complete log-model ownership
body/probe and the original-vector contract, bringing the body verifier to
**925**. All 32 prior implementation/test/corpus/header files remain byte-identical
to the [before archive](parity/log_publication_durable_before_5_0_6.json.gz) and
[model-header archive](parity/log_publication_model_headers_before_5_0_6.json.gz).
No production algorithm, model, formatter or manager body changes.

Three source-gate development failures remain separate: the first
[native configuration](parity/log_publication_initial_configure_failure_5_0_6.json.gz)
used a path relative to the included tests file instead of the project root;
the [initial driver](parity/log_publication_initial_driver_failure_5_0_6.json.gz)
correctly rejected an empty assumed test-module selection; and the
[first imported vector check](parity/log_publication_module_fixture_failure_5_0_6.json.gz)
received an unexpanded CMake variable from JSON. The final configuration uses
the existing repository-relative argument convention and the actual four-test
module; all required assertions remain active.
The later [inventory postprocessing failure](parity/log_publication_inventory_driver_failure_5_0_6.json.gz)
assumed every test's `ok` appeared on its name line. One original test prints
its twelve vectors first. The acceptance inventory now joins all six executed
names with each successful zero-failure/zero-ignored selection summary; no
source test is omitted or relabeled.

The [acceptance receipt](parity/log_publication_source_acceptance_5_0_6.json.gz)
joins LF01-LF18 with the independent complete Context/vector requirements.
Exactly three source rows advance to **39 accepted / 83 pending /
2 excluded_external**. All seventeen capability statuses are unchanged.
Native registry741 does not establish a full 741-test pass; both historical
failed full epochs remain failed.

Reproduce from the repository root with a fresh receipt path:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --preset modules_native
python3 scripts/verify_log_publication_source_5_0_6.py \
    --record /tmp/teaql_log_publication_source_fresh_5_0_6.json.gz
```

Arbitrary application sinks, complete actor/tenant/runtime assembly, governed
query/save callers, actual commit/delivery and full G14/G17/P0-P7 integration
remain independent. No generator/SDK, runtime dependency, sanitizer/MIPS,
full-suite or physical-router change/run is claimed.
