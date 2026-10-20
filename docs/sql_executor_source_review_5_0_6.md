# Complete SQL executor source review

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This review covers the complete **2,252-line executor.rs**, including all public
traits/types/defaults, private cache/query/stream/transaction helpers, conditional
implementations and sixteen embedded tests. The separate mutation implementation
is qualified by [ME01-ME20](sql_mutation_execution_source_review_5_0_6.md);
diagnostic helpers and test files retain their own linked acceptance records.

## Reproduced dynamic-stream difference

The dynamic entry accepts an optional observer independently of its capture
flag. Rust constructs diagnostic metadata after successful compilation whenever
capture is enabled, then opens the provider stream, and wraps only if metadata
and observer both exist. C++ incorrectly required the observer for metadata
construction. This skipped a dialect hook and could suppress its exception
before the provider call.

A new 720-input public-source fixture covers owner/selection gates, unknown
entity and invalid function compilation, capture/observer combinations, default
unsupported and supported transport SPIs, hook failures, chunk sizes and
poll-versus-drop. It reproduces **24 actual runtime differences**. The native
dynamic entry now captures on the request flag alone and wraps only when both
optional values exist. Native ordinary and buffered stream paths retain their
different source-specific capture timing.

The provider fixture returns an empty tracked cursor or uses the actual default
unsupported SPI. It does not implement SQL, dynamic-field hydration, diagnostics
or execution algorithms. The observer records capabilities, store forwarding,
schema/dialect calls, provider entry/chunk size, returned errors, delivered items,
cursor close and release-before-observer. Opaque store identities are not emitted.

## Complete production responsibility reconciliation

Native files remain separated among `executor.hpp`, `transport.hpp`,
`transaction.hpp`, `detail/query_execution.hpp`, `detail/stream_execution.hpp`,
`detail/query_plan.hpp`, `src/query_plan.cpp` and existing error/diagnostic types.
No provider implementation is merged into the executor.

| ID | Locked source responsibility | Native reconciliation |
| --- | --- | --- |
| SX01 | SqlTransport, lines 27–61 | Separate fetch/write and optional store SPI. Default repeated fetch lazily clones a template per value, replaces one parameter, appends rows in order and stops at first error. Empty input never indexes the template. |
| SX02 | StreamingSqlTransport, lines 63–85 | Required native stream plus the distinct dynamic-plan entry. The actual default dynamic entry returns the source unsupported code/field rather than synthesizing hydration. |
| SX03 | SqlTransactionTransport/SqlTransaction, lines 87–107 | Associated native transaction supports transport and consuming terminals. C++ uses a scoped native transaction handle and terminal guards; no stream/transaction fallback capability is invented. |
| SX04 | SqlExecutorError, lines 109–130 | Compile/Transport/PersistedRecord, complete Debug/Display and absent source stay distinct. Native transport payload copying remains conditional; move-only completion transfer is separately exercised. |
| SX05 | Executor fields/new/Clone, lines 132–168 | Independent dialect, transport and schema contracts plus descriptor/plan state. Explicitly installed Context-owned state replaces Arc ownership; same-domain facade copies borrow the installed components/cache and cannot extend their lifetime. |
| SX06 | `compile_select_with_cache`, lines 175–212 | Full descriptor equality plus shape match, current binding recollection/comment, 256-entry FIFO eviction before duplicate check, normalized key and cached log context with cleared intent provenance. Arena backing storage remains owned until Context close. |
| SX07 | Plan matching, lines 214–343 | Preserve every compared field and recursive Expr shape; distinguish scalar/list and list length, function/operator/column identity, nested descriptors/queries, optional expression presence and ordered children. Preserve source omissions rather than broadening cache identity. |
| SX08 | Plan key/normalization, lines 345–402 | Clone into the cache owner; clear comment/trace, normalize search text and literal/LIKE values while retaining list shape; recurse through projection, filter, HAVING, ordering and subqueries. Other source key fields remain intact. |
| SX09 | Binding collection, lines 404–524 | Preserve raw-SQL early return, projection/window-order/filter/search/HAVING/ordinary-order positions, descriptor text-field search expansion, LIKE original provenance, scalar/list/large-array hooks and nested subquery order. |
| SX10 | Partition helpers, lines 526–568 | Select the first matching IN/InLarge membership in an AND path, require a partition field/predicate, replace only that membership with equality and remove partitioning while preserving policy/business predicates, ordering and slice. |
| SX11 | Descriptor lookup and capabilities, lines 570–615 | Cache by requested name, copy successful descriptors into the explicit state owner, do not cache misses. Query/mutation/batch and small-parent preference are independent from available transaction/stream interfaces; source capability flags remain exact. |
| SX12 | Query/store/intent facade, lines 1460–1491 | Forward store to native transport. I/O-free intent uses cached compilation and current bindings; missing metadata/compile errors fall back conservatively to unclassified query provenance. Ordinary query delegates to observed query with no observer. |
| SX13 | Observed query, lines 1493–1533 | Resolve metadata lazily. Optional partition probes require disabled capture, provider preference, no explicit threshold and at least two values; otherwise compile normally and execute through the shared helper. |
| SX14 | Repeated probe execution, lines 1535–1579 | Build two scalar queries, compare collected parameter vectors and use the first differing parameter. If neither rewrite nor a difference exists, fall back. Successful repeated fetch returns source unrecorded metadata. |
| SX15 | Compiled query execution, lines 1581–1602 | Optional diagnostic guard, preserve original transport error, one terminal notification on failure, returned success metadata or exact unrecorded defaults and actual result count. |
| SX16 | Query diagnostic construction, lines 1604–1627 | Capture invocation backend, current intent/trace, SQL/policies/raw parameters and start/end; leave outcome/count/debug expansion unset until execution. Debug capture never authorizes eager plaintext rendering. |
| SX17 | Ordinary mutation facade, lines 1628–1660 | Delegate to qualified mutation orchestration using the shared cached lookup, with ordinary readback disabled. |
| SX18 | Guarded validation/helper/facade, lines 1662–1814 | Accept Update/Delete/Recover only; reject Insert/Batch before entity lookup. Compile guard into writes and derived `id AND guard` reads; preserve actual single-row readback and distinct errors. Delegate observed facts to qualified mutation orchestration. |
| SX19 | Transaction state/caches, lines 1816–1856 | Borrow dialect/schema, own a scoped native transaction and share the same explicit descriptor/plan state. Lookup and compilation retain executor semantics. |
| SX20 | Transaction capabilities/guarded forwarding, lines 1858–1917 | Exact source flags and guarded validation; native transaction guard additionally rejects foreign domains, pending/consumed misuse and unsafe retention. |
| SX21 | Transaction query/intent/store, lines 1919–1973 | Forward transaction store; same pure intent compilation/fallback. Query uses compiled execution without the ordinary executor's partition shortcut. |
| SX22 | Buffered transaction stream, lines 1975–2042 | Clamp chunk size to at least one, lazily buffer one query, preserve chunk indices/order/last and empty result EOF. Observed capture compiles early, suppresses inner capture, and counts delivered rows rather than buffered rows. |
| SX23 | Transaction mutation, lines 2044–2077 | Delegate through shared mutation mechanics with readback enabled. Source snapshots, generated values, physical facts and native item grouping remain separate. |
| SX24 | Commit/rollback/begin, lines 2079–2134 | Forward consuming native terminals and preserve Transport failures. Begin is lazy, wraps only a successful native transaction and borrows the existing caches. Stop/join and explicit-domain checks protect C++ cleanup before arena reclamation. |
| SX25 | Dynamic stream, lines 2136–2191 | Check owner and exact selection before SQL lookup; compilation failure becomes one error stream item without invented SQL diagnostics. Capture metadata on the flag even without observer, then call the distinct provider SPI. Map transport errors, wrap only with both metadata and observer, and preserve provider errors/chunk-size handling. |
| SX26 | Observed native stream, lines 2193–2223 | Disabled capture delegates to the unobserved entry; enabled capture compiles immediately, returns compile errors as stream items, constructs metadata before opening the cursor and wraps delivered-row diagnostics. |
| SX27 | Unobserved native stream, lines 2225–2252 | Compile at stream creation, map provider item errors, and return compile failures as stream items. Its behavior differs intentionally from lazy buffered transaction streaming and from the dynamic entry's optional-observer signature. |

Rust owned fields, Arc/RwLock/Mutex, Send/Sync and future Drop have the established
C++ adaptations: registered Global/User ownership, same-Context borrowed facade
copies, owner-thread dispatch, explicit child cancellation and stop/join before
reclamation. Native wrappers enforce active/domain/terminal checks that replace
Rust moved-value/lifetime enforcement. Pure intent results may explicitly copy
to another destination; native transaction work remains in its owner. Concrete
transport type parameters preserve conditional interfaces; choosing the erased
base transport intentionally erases those optional interfaces. These boundaries
do not promise arbitrary user-defined Rust Clone side effects in a C++ facade copy.

## Complete embedded test reconciliation

The original 843-line embedded test block contains sixteen tests. The historical
port had twelve. Native ports live in
`tests/parity/teaql_sql/executor_contracts.cpp`, with the four missing entries
now added. All source helpers, inputs and assertions were reviewed.

| ID | Original test / native entry | Preserved assertions |
| --- | --- | --- |
| XT01 | `caches_entity_descriptors_across_executor_clones` | One schema lookup across same-domain executor copies and no eager debug expansion. |
| XT02 | `cached_query_keeps_binding_policies_and_checks_descriptor_identity` | Rebind first/second values, one compilation, generated flag/Masked policies; a changed descriptor separates Plain policy and causes a second compilation. |
| XT03 | `batch_metadata_keeps_each_statement_and_its_numbered_bindings` | Two physical statement groups, per-item values, independently restarted numbered binds, Plain policy and absent debug SQL. |
| XT04 | `skips_execution_metadata_when_caller_will_discard_it` | Empty backend/SQL/params/trace and epoch start for unrecorded output. |
| XT05 | `sql_executor_never_eagerly_expands_private_values_even_when_diagnostics_requested` | Query and insert keep raw private binds while parameterized SQL/debug output never eagerly expose the canary. |
| XT06 | New `ordinary_sql_query_future_keeps_partition_probe_state_out_of_frame` | Deferred schema/transport access, one ordinary call, no repeated probe, plus the 4,608-byte bound on the actual native arena-resident operation state. Rust measures its stack future; C++ measures its Context-owned state. These are not identical ABI layouts. |
| XT07 | New `partition_probe_fallback_keeps_identical_single_and_explicit_policy_queries` | Identical values, singleton membership and explicit threshold 32 each use one ordinary query and no repeated transport. |
| XT08 | `topn_004_011_sqlite_reuses_one_repeated_probe_boundary` | Exactly one repeated boundary for values 7/9, no ordinary call, empty rows and unrecorded epoch metadata. |
| XT09 | `keeps_partition_query_observable_when_metadata_is_enabled` | Capture forces one ordinary call rather than a probe shortcut. |
| XT10 | `topn_001_explicit_zero_threshold_forces_window_for_probe_provider` | Explicit zero threshold disables probe optimization. |
| XT11 | `topn_007_probe_rewrites_only_partition_membership_filter` | Retain tenant/version/business predicates, ordering and slice; rewrite matching membership only and remove partitioning. |
| XT12 | `cached_select_plan_rebinds_values_and_separates_in_list_lengths` | Current values reuse SQL; short/long IN shapes keep distinct SQL and parameter counts. |
| XT13 | New `cached_like_plan_rebinds_originals_across_all_expression_positions` | Projection/filter/subquery/HAVING/order bindings equal fresh compilation, current original secrets survive, prior secrets do not, one normalized plan and cleared cached provenance. |
| XT14 | New `cached_partition_having_rebinds_numeric_and_private_like_operands` | Ranked SQL retains GROUP BY/HAVING; exact projection/window/WHERE/search/HAVING parameter order; one cache compilation, current private original and normalized secret-free key. |
| XT15 | `cached_select_plan_rebinds_large_in_as_one_array_parameter` | The synthetic extension hook retains one complete List bind across a cache hit. |
| XT16 | `cached_select_plan_preserves_parameter_order_for_supported_query_shapes` | Cached/fresh equality across search, computed projection/WHERE/HAVING/order, partition ordering and subqueries. |

The original general/array test dialect uses PostgreSQL-kind `$n` rendering as a
synthetic compiler fixture. Native tests preserve the same responsibilities with
SQLite-kind `?n` and a test-only array callback; no PostgreSQL provider or SQLite
array backend is added. Fixture query/mutation intent strings now match the
source. The private cached-vector assertions are adapted through actual cache
counts, fresh-result equality and the same public canonical CachedSelectPlan
constructor/normalizer used by insertion; no production inspection API is added.

The current original frame measures 4,392 bytes; the native operation state
measures 2,872 bytes. The latter excludes its wrapper and separately allocated
buffers. Both satisfy the retained bound; this is not a total-memory or
performance comparison.

## Original observation and dependency scope

The independent `rust_sql_executor_source_5_0_6` observer preserves the exact
reference registry identities and all original SQL dependencies. Its disposable
view retains all twelve original source files and test bodies, with the existing
access-only diagnostic observer appended to the root. Historical protocol
drivers/helpers and corpora remain byte-identical. The new dynamic observer calls
the original public executor and its real default/overridden transport SPI.

| Source family | Outcomes |
| --- | ---: |
| New dynamic stream entry | 720 |
| Retained diagnostic guards/journal | 352 |
| Retained query/cache/partition protocol | 767 |
| Retained native/buffered stream protocol | 1,765 |
| Retained mutation / transaction / backend-kind | 2,029 / 2,029 / 93 |
| Retained repeated transport / executor errors | 344 / 144 |
| Total per native/imported frontend | **8,243** |

Comparisons use exact typed JSON without normalization; observation projections
exclude wall-clock equality. Historical rejected-intent inputs and valid controls
remain distinct, as documented in [the SQL protocol review](sql_protocol_5_0_6.md).
The original sixteen executor, six query diagnostic, twenty-one mutation
diagnostic and seven DiagnosticStream tests run unchanged: **50 tests**.
The separate source-qualified mutation continuation's new matrices retain their
own evidence; this review does not relabel them as freshly rerun here.

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_sql_executor_source_5_0_6.py \
    --record /tmp/sql_executor_source_506.json.gz \
    --profiles native modules_native sanitize
```

Native/imported profiles run all nine matrices and ten relevant unit contracts.
The sanitizer profile deliberately selects only the new 720-input dynamic stream
matrix and existing stream/move-only lifetime contracts; leak scanning is disabled.
Use fresh receipts; `--capture-initial` records differences without acceptance.
Provider, full runtime caller/publication, complete current integration and all
remaining P0-P7 exits retain independent requirements.

## Preserved initial evidence

The [first native fixture compile](parity/sql_executor_dynamic_fixture_initial_compile_5_0_6.json.gz)
used an optional setter for a ScopedBox selection; using the existing
`select_dynamic_fields` API repaired it. The
[first original fixture build](parity/sql_executor_source_initial_5_0_6.json.gz)
used a bare selection where the source requires Arc; that fixture-only error
was corrected without changing any original source.

The [initial broad capture](parity/sql_executor_source_initial_retry_5_0_6.json.gz)
passed all fifty original tests and matched the other eight source families.
Its 342 dynamic differences included a fixture-output defect: the JSON helper
inserts keys and cannot overwrite an initial `opened_result=false`. Writing the
final boolean once fixes that defect. The
[corrected pre-repair comparison](parity/sql_executor_dynamic_initial_differences_5_0_6.json.gz)
replays the same unchanged original observations and isolates the **24 actual
runtime differences**, including suppressed hook failure before cursor creation.
No original input or expected output was edited.

The [post-repair execution](parity/sql_executor_source_execution_5_0_6.json.gz)
passes **8,243 exact original/native/imported outcomes per frontend**, all
**50 unchanged original tests**, **native 19/imported 19** CTest contracts and
the targeted **ASan/UBSan 3** selection. That sanitizer scope contains **720**
dynamic-stream outcomes and existing stream/move-only cleanup checks; it does
not claim all 8,243 sanitizer outcomes. Selected final builds report no work;
leak scanning is disabled.

## Final qualification

The [real SQLite continuation](parity/sql_executor_source_consumers_5_0_6.json.gz)
passes native **2/2** (dynamic-field query/stream lifecycle and generated Q/E
graph example) and imported **1/1** (the same dynamic-field contracts), with
final no-work rebuilds. Pinned format/check passes **1,706 handwritten files**.

[Static verification](parity/sql_executor_source_static_5_0_6.json.gz) accepts
active module paths, **1,516 exports**, **1,007 canonical consumer bodies** and
all 42 generator anchors/17 capability records. Native discovery is **778**;
the module preset discovers **567 imported tests**. These inventories do not
establish full-suite acceptance.

The [preservation record](parity/sql_executor_source_preservation_5_0_6.json.gz)
contains the one-header production repair and the semantic-port diff. The
other **6,090 protected files**, including the retained **78 SDKs / 3,457 files**,
keep their original bytes. Historical source observers/corpora, generator
implementation and the already qualified mutation implementation are unchanged.

The [qualification receipt](parity/sql_executor_source_qualification_5_0_6.json.gz)
accepts only complete executor.rs, advancing source dispositions to **63
accepted / 59 pending / 2 excluded_external**. All seventeen capability statuses
and complete P0-P7 exits remain unchanged. Pending provider/runtime/governance/
privacy caller qualification, current full integration and standing behavior
decisions retain their requirements. Historical 735/736 and 718/721 epochs
remain failed. No new Java/generation, MIPS or physical-device acceptance is
claimed.
