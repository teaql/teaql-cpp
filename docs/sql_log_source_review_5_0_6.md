# Complete SQL-log provenance source review

Status: the complete source is accepted under SL01-SL16 with explicit Context
ownership adaptations. The durable source/native/module qualification below
supersedes the provisional integration status, whose exact earlier text is
retained in the before archive. Complete G14/G17 and P0-P7 remain open.

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This covers all 457 lines of `teaql-data-service/src/sql_log.rs`, including its
two original query-intent tests. The earlier
[data-service API review](data_service_api_contract.md) retains its historical
5.0.5 acceptance. Compiler-owned log provenance, runtime safe projection,
operator publication and transaction commit remain separate responsibilities.

## Complete source responsibilities

| ID | Original responsibility | Current C++ mapping and boundary |
| --- | --- | --- |
| SL01 | Parameter policy enum | Unknown, Plain, Masked and Credential remain distinct; Unknown is the source default. Debug/Clone/Copy/equality responsibilities retain their original values. Compiler-owned policy is not a remote caller opt-out. |
| SL02 | Execution outcome | Success, Failure and Cancelled preserve exact as_str and Debug spellings. These describe statement/cursor completion, not transaction commit or business approval. No Rust Default implementation is invented. |
| SL03 | Complete SqlLogContext model | Preserve all nine fields: projection state, inherited redactions, optional outcome/database, parameter policies, generated-SQL flag, masked positions, optional mode and omission reason. Defaults remain empty/None/false; allocator-aware destination copies preserve nested data. |
| SL04 | Opaque projection state | Actual concrete-type lookup, private Debug and allocation-identity equality remain independent. Same-domain copies can borrow noncopyable state. Explicit cross-domain retention requires an allocator-aware copyable payload and preserves logical origin identity without retaining the original Context. |
| SL05 | Redaction value model | Ordered private (text, forced) pairs retain Clone/equality and count-only Debug. Debug must not expose the collected strings. An empty value and clear remain distinct from publication, which has its own clearing responsibilities. |
| SL06 | Metadata field policy | Unknown property yields Unknown. Credential matching on either property or physical-column name wins. An undeclared mask list yields Unknown; a declared exact property-name mask yields Masked, otherwise Plain. Column spelling and case-folded mask guesses do not replace canonical property identity. |
| SL07 | Query fallback roots and traversal | Iteratively visit filter, HAVING, expression projection, expression ordering and search text. Traverse actual SQL subqueries but exclude separately executed relation siblings, child enhancements and object groups. Raw SQL/comment text is not fabricated into a typed binding. |
| SL08 | Every expression variant and ordering | Value and LikePattern contribute actual values; columns contribute none. Functions/And/Or push children, Binary pushes left then right, SubQuery pushes left then query, Between pushes expression/lower/upper and unary operators push their child. LIFO visitation order is retained. |
| SL09 | Reconstructed LIKE provenance | Include the original operand only for Like/NotLike with a genuine LikePattern whose current pattern equals one of the three lowered forms. Rewritten operators or unrelated patterns must not retain stale original text. The current pattern is still visited normally. |
| SL10 | Binding ancestry and invalid-policy handling | Start with inherited redactions. A policy-count mismatch makes every binding Unknown. Credential-looking SQL forces protection when SQL is ungenerated or policy count is invalid. Neither condition changes execution operands. |
| SL11 | Scalar fast paths | Ordinary Plain scalars can be omitted from intent redactions only when statement provenance permits. Empty text and NULL/typed NULL contribute no strings. Masked, Credential and Unknown preserve their separate forced-versus-optional behavior. |
| SL12 | Recursive object/list/JSON collection | Inspect nested credential keys even beneath a Plain outer field. A credential anywhere in the value forces all collected strings from that value. Preserve iterative ordering, JSON scalar formatting, dates, timestamps and numeric spellings through existing core contracts. |
| SL13 | Extend and target IDs | Extend deduplicates exact (text, forced) pairs while preserving order. Target IDs add forced free-text redactions, including promotion beside an existing unforced pair, without changing SQL policy or the actual binding value. |
| SL14 | Projection SPI, is_empty and clear | extend_secrets appends to the caller's list; forced entries remain protected even when plaintext is allowed. Non-forced entries depend on that trusted projection flag. Clear removes logical entries without promising arena reclamation. |
| SL15 | Credential classifier | Thirteen exact substrings after ASCII alphanumeric filtering and lowercase conversion. UTF-8 bytes/punctuation are ignored; no Unicode case-folding or word-boundary behavior is invented. The bounded C++ sliding window adds no allocation or external dependency. |
| SL16 | Source tests, ownership and inherited provider default | Run both original tests against the actual crate. The default provider fallback must not invoke query I/O or grant debug plaintext. Existing complete data-service/executor/projection ownership bodies retain default-PMR-disabled checks, noncopyable state, failure rollback and actual User deletion. |

The source's Arc ownership is adapted to the approved two Context lifetimes.
Projection identity is scalar comparison metadata, not a resolution capability.
Ordinary projection wrappers and raw payload pointers remain borrows; they are
invalid after their owning Context closes. Checked Context references and
explicit destination copies provide their separately documented guarantees.
The static type/clone dispatch table owns no Context data.

## Verification against actual source

The [source replay receipt](parity/sql_log_source_replay_5_0_6.json.gz) records
the two unchanged original unit tests, reference-locked observer project,
complete original/imported outputs, canonical consumer preparation and terminal
commands. Its `complete` field distinguishes an ongoing run from success.
No runtime algorithm, original observer or historical corpus is changed.

The retained families are 1,008 binding/ancestry/classifier observations, eight
non-Clone projection observations and forty complete SQL model observations.
The last family exercises all SqlLogContext fields, equality/copies and nine
Debug option modes through actual compiled-query models. It also retains SQL
model errors; those extra results do not independently qualify another source
file or add a non-SQLite provider.

A separate 79-case observer calls the actual default QueryExecutor protocol and
field-policy helper. It covers every expression node family, all query entry
positions, excluded sibling scopes, three original text shapes with seven LIKE
rewrites, and declared/undeclared masks with canonical/column/credential/missing
fields. It compares complete ordered safe/debug secrets and private diagnostics.
Both observers fail if fallback discovery tries to execute a query.

Canonical imported consumers preserve the complete existing data-service,
executor and projection ownership test bodies. The three retained probes and
the new query observer also use imports with their original fixture include
positions intact. The new observer has default PMR allocation disabled; all
query/descriptor/result data uses its UserContext. These controls exercise
runtime behavior, not merely exported names or corpus labels.

The replay is now terminal and successful: **1,135/1,135** complete outcomes
match actual original and imported C++ execution (1,008 + 8 + 40 + 79).
All two original unit tests and three complete imported ownership/SPI bodies
pass. Every recorded command exits zero. Original generated/source helpers,
runtime implementation and historical corpora remain unchanged. The new query
observer was temporary at this checkpoint; the durable integration below now
replaces that reproduction dependency.

## Historical integration boundary

The original review ran while the full 736-test native epoch was frozen. That
epoch is now terminal at 735/736; its failed result is preserved in the
[integration record](native_integration_5_0_6.md). The provisional review did not
consume unfinished native outputs or alter that epoch's inputs.

## Durable source and frontend qualification

The permanent [Rust observer project](../tests/parity/rust_log_privacy_reference_5_0_6/Cargo.toml)
links the actual locked source and reuses five unchanged observers. Its query
observer changes only three source-path annotations from its successful temporary
version. Every registry version/source/checksum is a subset of the reference
lock. The complete original SQL-log test selection passes **2/2**.

The [canonical C++ query observer](../tests/parity/sql_log_query_source_probe.cpp)
preserves its earlier body, changing only a fixture include. Its immutable
[79-case corpus](../tests/parity/sql_log_query_source_5_0_6_cases.json.gz) retains
actual original outputs. All seven source matrices match fresh original,
current native and current imported execution: 1,008 intent, eight opaque-state,
40 SQL-model, 79 query-source, 425 safe-SQL, 1,427 audit/model and 104 audit-identity
observations. The **3,091 per frontend** include overlapping and adjacent model
responsibilities; they are not 3,091 independent SQL-log requirements.

The [execution receipt](parity/log_privacy_durable_execution_5_0_6.json.gz)
records **19/19 native and 19/19 imported checks**, terminal successful builds
and no-work builds after each frontend. The complete data-service, executor,
projection, runtime-log, audit-event and reentrant-close bodies retain their
original assertions. Six additional canonical module registrations bring the
body identity check to **922**. All 27 archived implementation/test/corpus files
are byte-identical to their
[before state](parity/log_privacy_durable_before_5_0_6.json.gz).

The [scoped acceptance](parity/log_privacy_source_acceptance_5_0_6.json.gz)
joins SL01-SL16 with complete runtime privacy/masking source evidence. Exactly
three dispositions advance: **36 accepted / 86 pending / 2 excluded_external**.
All seventeen capability statuses are unchanged. Native registry **740** is
a discovered test count, not a full-suite pass. The initial module configure
[failure](parity/log_privacy_initial_module_configure_failure_5_0_6.json.gz)
records duplicate projection-test registration; removing that duplicate preserves
the existing registration, and the fresh configure/build/test sequence passes.

Reproduce from the repository root with a fresh receipt path:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --preset modules_native
python3 scripts/verify_log_privacy_source_5_0_6.py \
    --record /tmp/teaql_log_privacy_source_fresh_5_0_6.json.gz
```

Context capture, application/custom sinks, whole formatter/manager configuration,
query/save lineage, complete G14/G17 and P0-P7 retain their independent scope.
No production C++ algorithm, generator, SDK, runtime dependency, sanitizer/MIPS,
full-suite or physical-router change/run is claimed.
