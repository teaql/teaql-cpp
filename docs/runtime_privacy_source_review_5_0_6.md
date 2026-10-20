# Complete runtime privacy and masking source review

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This reviews the complete `teaql-runtime/src/log_privacy.rs` (663 lines) and
its `masking_contract_tests.rs` (634 lines). The earlier
[logging contract](runtime_log_api_contract.md) retains its historical 5.0.5
review and evidence. Both complete sources are now accepted under LP01-LP16
and all 34 SQLite-applicable masking contracts, with the explicit PostgreSQL
test exclusion and Context ownership adaptations below. This is scoped source
acceptance; complete G14/G17 and P0-P7 remain open.

## Production responsibilities

| ID | Original responsibility | Current C++ mapping and boundary |
| --- | --- | --- |
| LP01 | Exact plaintext acknowledgement | `LogManager::plaintext_enabled` reads the environment on each call and compares the complete acknowledgement. Only warning initialization is remembered. The source process-owned warning becomes application Global-owned state; endpoint/configuration caching does not cache plaintext permission. |
| LP02 | Credential detection | Delegate field-name classification to the data-service contract. Recursively inspect Value objects/lists and JSON objects/arrays; a nested credential key forces protection of the containing value. |
| LP03 | Secret collection | Null variants and empty strings contribute nothing. Preserve typed scalar display, Decimal/date spelling, timestamp milliseconds, recursive order and JSON scalar serialization. Collection is not JSON normalization of typed Values. |
| LP04 | Literal scrubbing | Sort by descending byte length, retaining insertion order for equal lengths. Replace literal substrings in intent, summaries, trace comments and entity names. Typed trace IDs remain separate fields. |
| LP05 | Business masks | Preserve Null/TypedNull; recurse through lists; replace objects/JSON with the sentinel. Scalar masks use the existing Unicode code-point algorithm and its digit-only rule. |
| LP06 | Safe-mode monotonicity | A safe entry cannot regain debug permission through a later `allow=true`. Debug mode is recognized from the mode or the exact diagnostic header. The boolean is a trusted projection input, not a request-data authorization field. |
| LP07 | Retained safe alternatives | Reuse only a concrete private SafeSqlProjection whose SHA-256 matches the complete compact Debug of the current entry. Tampering with entry fields or losing private state invalidates reuse. The fingerprint checks retained projection integrity; it is not an authenticated authorization token. |
| LP08 | Alternative ownership | Store only the projected safe entry and fingerprint, never the raw source entry or a recursive projection-state graph. C++ allocator-aware destination copies replace source shared ownership. Ordinary copies remain Context borrows; explicit Global retention does not keep the source User alive. |
| LP09 | Parameter decisions | Nonempty policy/mask arrays with invalid lengths make policies Unknown. Unsafe credential SQL, Credential policy and nested credentials force redaction. Unknown never permits plaintext. Already-masked values retain their flags without repeating the mask. |
| LP10 | Inherited intent | Extend binding secrets with inherited redactions, including forced target identities. Scrub optional comment/purpose/audit reason/result type, result summary and trace; consume private intent provenance in the published projection. Preserve the original entry and its execution operands. |
| LP11 | SQL rendering | Render SQLite bindings through the existing SQL contracts. Untrusted templates containing literals, comments or digits fail closed. Generated provenance can permit those templates; SQL text cannot grant Plain policy. Non-SQLite rendering remains outside the edge-provider scope. |
| LP12 | Render failure | Scrub every original binding, including previously Plain values; replace SQL with the non-replayable sentinel and each parameter with Null. Preserve only the exact source omission-reason allowlist, otherwise use the generic prior-omission reason. Pretty SQL follows the safe diagnostic SQL. |
| LP13 | Missing or invalid debug state | Hide all optional intent fields, trace names/comments and unrecognized summary text. Preserve recognized singular/plural count summaries as execution facts; result count takes precedence over affected rows, including zero. Use the source count fallback when needed. |
| LP14 | Raw audit projection | Independently project current, old and new records and both sides of each change. Without permission, protect ordinary fields except id/version. Credentials and nested credentials stay protected with permission; Null variants survive. Drop private provenance from projected output. |
| LP15 | Audit identity and bootstrap | Current target-ID text scrubs trace and bootstrap actor/reason without changing typed identity/version or the original business event. This remains separate from SafeAuditEvent's positive-ID extraction and configured field-mask rules. |
| LP16 | Publication | Ordinary sinks always receive safe projections. Sensitive output additionally needs exact permission and its separate configured endpoint. These source-owned checks do not authorize arbitrary application/custom sinks. File output and retained debug downgrade are tested through the actual manager, not a substitute formatter. |

The C++ implementation is primarily `teaql_runtime/src/audit_privacy.cpp`,
with permission and publication in `log_manager.cpp`. The
[SQL-log source review](sql_log_source_review_5_0_6.md) separately covers
compiler-owned binding/intent provenance. Context capture, query/save lineage,
transaction commit and complete G14/G17 remain independent responsibilities.

## Complete source tests and imported bodies

The unchanged original source defines five ordinary privacy tests, one child
fixture and 35 masking tests after macro expansion. The replay runs all **40
ordinary tests**, including the PostgreSQL array source test. The test harness
reports the child as ignored by design; the parent explicitly executes it in
**three fresh processes** for unset, invalid and exact acknowledgement. It is
not silently omitted from protected-output verification.

The C++ masking port covers all **34 SQLite-applicable contracts**, retaining
the ten-row golden TSV, six macro inputs for both SQL/audit helpers, inherited
intent, numeric display, target IDs, count facts, idempotence, tampered/lost
alternatives, malformed policies/masks, omission channels and nested credentials.
The PostgreSQL ARRAY/ANY rendering contract is explicitly excluded from C++;
running its original Rust test does not add provider support.

The complete original C++ privacy and masking bodies are adapted with the
repository's canonical import preparation and body comparison. No test
assertion or function body is replaced. The privacy body checks unchanged raw
events and explicit Global safe-event retention after actual User deletion.
The masking body retains all sixteen clone workers; each has worker-local
storage and joins before the source owner can be reclaimed.

The full imported runtime-log ownership body additionally tests copied and
assigned retained alternatives, unified-buffer deep copies, tamper rejection,
lost state, stable binding positions and disabled default PMR allocation.
The imported audit-event body and its four reentrant-close cases preserve
separate callback/resource cleanup coverage.

## Actual-source replay

The [current replay receipt](parity/runtime_privacy_source_replay_5_0_6.json.gz)
preserves every command, exit status, original unit output, canonical consumer
body and complete comparison output. Its `complete` flag is required before
interpreting the run as successful. The unchanged original observers execute
the actual private event/privacy source and public runtime models.

The retained matrices contain 425 SQL projection outcomes, 1,427 audit/model
outcomes and 104 audit-provenance/identity outcomes. The wider audit/model
family also exercises event responsibilities outside this source; its size is
not a count of independent privacy rules. Historical expected outputs must
match fresh original execution as well as current imported C++ execution.
Existing formatter adapters remove only the volatile human-time prefix.

The replay is now terminal and successful: **1,956/1,956** outcomes match fresh
original and imported execution, and every one of its 18 commands exits zero.
All 40 ordinary original tests, six permanent imported CTest contracts, both
complete temporary imported privacy/masking bodies and the three C++ isolated
file cases pass. The [provisional join](parity/runtime_privacy_source_provisional_5_0_6.json.gz)
also verifies that all 10,329 frozen input hashes and both reference repositories
are unchanged. These are focused results, not a current whole-native pass.

All observer registry dependencies are checked against the frozen Cargo.lock
by name, version, registry source and checksum. The full original unit-test
view retains every original source/test/fixture byte; its only source append
is the previously recorded, unrelated commit-audit observer. Optional exporter
test dependencies are absent with that feature disabled. No privacy source or
test is edited to compile.

The [initial fixture failure](parity/runtime_privacy_source_initial_fixture_failure_5_0_6.json.gz)
is preserved separately. The temporary audit observer manifest initially
omitted direct `teaql-sql` and `sha2` dependencies required by the unchanged
included privacy module. The retry supplies the reference-locked dependencies;
this is a fixture repair, not a runtime or original-source change.

## Historical integration boundary

The provisional review used independent module/temporary projects while the
736-test native epoch was frozen. That epoch is now terminal at 735/736; the
[integration record](native_integration_5_0_6.md) preserves its failed outcome.
The exact earlier review remains in the
[before archive](parity/log_privacy_durable_before_5_0_6.json.gz).

## Durable source and frontend qualification

The [repository-relative observer project](../tests/parity/rust_log_privacy_reference_5_0_6/Cargo.toml)
reuses the unchanged original SQL/audit/privacy observers with exact-reference
registry identities. The permanent
[driver](../scripts/verify_log_privacy_source_5_0_6.py) also creates a byte-checked
view of the actual complete original runtime for private tests. Its 67 original
source/test/fixture files are preserved and **no observer is appended** in this
view. Only the disabled OpenTelemetry exporter dev dependencies are omitted;
that source-view adaptation adds no deployed C++ dependency.

The [terminal execution receipt](parity/log_privacy_durable_execution_5_0_6.json.gz)
records **40 passed / 0 failed / 1 ignored** for the complete original privacy
and masking selection. Its passing parent runs the ignored child in three fresh
processes. The original PostgreSQL ARRAY/ANY test runs here; C++ retains its
explicit exclusion and all 34 SQLite-applicable contracts. Both C++ frontends
run the unchanged isolated file runner in three fresh processes each.

All seven retained source matrices match actual original and both current C++
frontends: 1,008 binding/intent, eight opaque-state, 40 SQL-model, 79 query-source,
425 SQL-privacy, 1,427 audit/model and 104 audit-provenance observations. These
total **3,091 per frontend**, including overlapping adjacent model contracts.
The complete source families remain distinct from a count of privacy rules.

Native **19/19** and imported **19/19** checks pass after successful builds;
each final consumer build reports no work. The permanent module registry now
includes the complete canonical privacy/masking bodies and isolated file runner,
plus SQL/query/audit projection probes. All **922** canonical consumer/fixture
bodies verify. Immutable byte checks preserve all 27 archived C++ implementation,
test and corpus files; the relocated observers change only source/include paths.

The first module configure failed on duplicate registration of an existing
projection comparator. Its exact
[failure](parity/log_privacy_initial_module_configure_failure_5_0_6.json.gz)
remains separate. Removing the extra registration allows the new configured
build and complete scoped execution to pass; the old comparator remains active.

The [source acceptance receipt](parity/log_privacy_source_acceptance_5_0_6.json.gz)
joins the source bodies, all LP01-LP16 responsibilities, masking contracts,
immutable identities and terminal native/module evidence. Exactly these two
runtime files and `sql_log.rs` advance the ledger to **36 accepted / 86 pending /
2 excluded_external**. All seventeen capability statuses are unchanged. Native
registry **740** does not claim a new whole-suite pass.

Reproduce using a fresh receipt path:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --preset modules_native
python3 scripts/verify_log_privacy_source_5_0_6.py \
    --record /tmp/teaql_log_privacy_source_fresh_5_0_6.json.gz
```

Whole Context logging, formatter/manager source qualification, arbitrary custom
sinks, actor/tenant assembly, commit/delivery and governed query/save lineage
remain separate. No generator/SDK, production algorithm, runtime dependency,
sanitizer/MIPS/full-suite or physical-router change/run is claimed.
