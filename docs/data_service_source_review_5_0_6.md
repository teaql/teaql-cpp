# Data-service public source and request envelope review

Status: DS01-DS18 and RV01-RV07 are accepted for the three complete named
source files, with explicit C++ ownership/async adaptations. Complete runtime,
provider, capability and P0-P7 acceptance remain open.
Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

The scope is the complete 555-line `teaql-data-service/src/lib.rs`, its three
root unit tests, and the complete request-intent/privacy integration tests.
The existing [request-vector review](request_vectors_source_review_5_0_6.md)
retains the earlier temporary implementation and its successful evidence. This
continuation installs its complete C++ body and authentic fixture permanently.
The underlying SQL-log and dynamic-storage modules retain their separate source
acceptances.

## Complete responsibilities

| ID | Source contract | Mapping and required evidence |
| --- | --- | --- |
| DS01 | Capabilities and operation variants | Preserve eight independent false-default capability flags and seven distinct operation values. Observe complete Debug/default/Clone state. Native capability equality is an additional C++ convenience, not an invented Rust derive. |
| DS02 | Complete QueryRequest state | Keep query, independent trace snapshot, owned QueryIntent and the two separate capture flags. New requests default debug false and metadata true. Explicit owned intent is independent of query-builder text. |
| DS03 | Request construction and validation | `from_query` validates the query's optional comment/purpose; diagnostic frames and disabled logging cannot supply missing intent. Exact error code/field and untrimmed valid text remain. Core intent lexical rules retain the separate RI source acceptance. |
| DS04 | Query execution trace projection | Choose the first nonblank Operation/Request/Comment/Purpose/AuditReason name, preserving its original bytes; otherwise use the current query entity. Replace all stored Comment/Purpose frames with the owned pair, retaining all other nodes in order. Projection leaves the captured trace unchanged. Later query-builder edits cannot replace owned intent or its captured trace. |
| DS05 | Distinct command algebra | Preserve Insert/Update/Delete/Recover and Batch of complete MutationRequests. Intent-free command construction is separate from an executable request. Nested batches retain their own envelopes. |
| DS06 | MutationRequest state and construction | Preserve its command, private owned MutationIntent and optional private diagnostic provenance. New requests start without inherited private state. Comment access reads owned intent; child reasons and arbitrary command trace tails do not supply the root. |
| DS07 | Statement-local mutation lineage | Stored trace is the actual command trace; Batch has an empty root trace. Execution replaces only the first existing AuditReason's comment, preserving every ancestor/child node and identity. Without one, prepend the owned reason with the command entity and a positive convertible native ID. Insert IDs come from the actual `id` value; batch children project independently. |
| DS08 | Immutable private provenance | Binding preserves a request-local snapshot. Extending the mutable source does not affect it. Cloning a command into an independent request does not copy envelope provenance. Debug exposes no private values; allowed debug filtering retains the source forced-redaction distinction. Source Arc retention maps to explicit Context-owned copies. |
| DS09 | Query and mutation results | Rows and metadata stay independent. GeneratedValues and optional authoritative EntitySnapshot remain separate types; affected rows do not replace either. Complete nested copies and diagnostic values are observed. |
| DS10 | Complete ExecutionMetadata | Preserve all fourteen fields, ordered independent statement metadata, SqlLogContext, backend/operation, both SystemTimes, optional affected/result counts, trace/comment/request ID, parameterized text, structured parameters and debug text. `unrecorded_query` uses Unix epoch, Query, Some(count) and otherwise empty/absent state. Metadata capture is not permission for plaintext. |
| DS11 | Query executor defaults | Preserve the independent capability/error protocol, absent executor-owned dynamic store, pure unclassified intent fallback and mandatory query method. Default observed query delegates to the ordinary method without inventing diagnostics or invoking the optional observer. |
| DS12 | Stream protocol and defaults | Preserve rows/chunk index/last flag, borrowed stream lifetime and independent observer type. Ordinary observed streaming delegates directly. Unsupported dynamic-field enhancement returns the exact stream error before opening a cursor. The existing dynamic-storage contracts cover plan construction and this default boundary. |
| DS13 | Mutation and guard executor defaults | Ordinary and guarded observed defaults delegate once and do not invent callbacks. GuardedMutationRequest keeps its trusted guard outside ordinary mutation ownership. Atomic enforcement remains a provider obligation; a separate read-before-write does not satisfy the protocol. |
| DS14 | Transaction protocol | Associated transactions support query/mutation plus consuming commit/rollback. Explicit C++ handles enforce Context identity, consumed state and pending-work cleanup without shared ownership. Concrete errors remain typed. Actual SQL/SQLite transaction bodies retain independent acceptance. |
| DS15 | Schema, ID and descriptor protocols | Preserve SchemaRequest/Result, schema execution, internal ID allocation and the independent metadata SchemaProvider. Its source Arc descriptor lookup maps to an explicitly scoped native borrow; it does not become runtime schema initialization or a new ownership domain. |
| DS16 | Full model derives and lifetime | Preserve source Debug/Clone models and operation Copy/PartialEq/Eq, including recursive metadata and nested commands. The default-PMR-disabled contracts check all nested destination allocators, explicit Global retention after actual User deletion, independent mutable copies, moved requests and terminal transaction guards. |
| DS17 | All original tests and the full shared fixture | Execute all three root tests, four intent/trace tests and the privacy test from the actual frozen crate. The permanent native/imported semantic port keeps all twenty fixture cases, three trace contracts and the complete private snapshot assertions. |
| DS18 | Current source/protocol/frontend qualification | Reproduce both complete 40-input model matrices, the 296 inherited-default/intent controls and additional actual request projection/identity observations. Preserve source/corpus identities, exact dependency versions/checksums, canonical bodies and current installed tests; do not infer whole runtime/provider or generator qualification. |

## Ownership and interface adaptations

All dynamic native request/result/provenance data uses the selected Global or
User allocation scope. Retention requires an explicit destination copy; ordinary
wrappers do not keep the source Context alive. Callback and resource access use
the established Context owner-thread rules. Source associated error types and
future/stream outputs map to typed native Result/operation/stream protocols.

The pure executor protocol accepts its low-level allocation resource; runtime
and application facades retain their distinct UserContext responsibility. No
data-service, SQL executor, transport, provider or runtime layer is merged.
Schema metadata lookup remains distinct from runtime `ensure_schema`.

Request Debug is a raw model protocol. Private provenance's source-defined Debug
representation does not expose its values. This review does not authorize raw
parameter output or accept every governed caller, SQL sink or asynchronous
publication path.

## Durable vectors and source observations

The new `request_vector_tests.cpp` is restored from the previously verified temporary
port. Only its fixture-helper include is made repository-relative. Its original
assertions and the twenty-case external fixture remain unchanged; the copied
fixture uses an underscore filename while retaining its authentic contract ID.
Both frontends execute the actual request/intent/provenance APIs.

The additional observer invokes real QueryRequest/MutationRequest projection
methods. Query cases vary all nine node kinds, blank/nonblank root names, ordered
eligible roots and both capture flags, then mutate the query builder. Mutation
cases vary all command kinds, missing/native/mixed-type IDs, existing/absent/
duplicate audit roots, parent/child lineage and nested batches. The native
observer uses the existing DebugWriter sequence formatting over actual nodes;
it does not implement trace selection or ID conversion locally.

The [initial observer compile record](parity/data_service_request_observer_initial_compile_5_0_6.json.gz)
retains incorrect TraceNode argument order and direct vector/span Debug calls.
The correction uses the existing constructor order and sequence-formatting
interface; no production constructor, trace algorithm or original source is
changed to accommodate the observer.

## Acceptance boundary

The [terminal acceptance record](parity/data_service_source_acceptance_5_0_6.json.gz)
joins the complete responsibility review, unchanged original source tests,
[source comparisons](parity/data_service_source_final_5_0_6.json.gz) and
[installed selections](parity/data_service_source_installed_5_0_6.json.gz).
All compiler/linker/test processes are terminal with zero exit status.

| Evidence | Completed scope |
| --- | --- |
| Original tests | Three root tests, four intent/trace tests and one privacy test; both existing SQL-log unit tests also pass in the original crate run. |
| Complete model observations | Two retained 40-case matrices, including nested destination copies and model diagnostics. |
| Inherited executor defaults | 296 actual-source query/mutation/guard/stream/default-intent outcomes. |
| Request projection and identity | 932 actual-source query and mutation envelopes, including nested batches. |
| Source/native/imported comparison | All 1,308 outcomes agree on each frontend; retained corpora remain byte-identical. |
| Installed contracts | Native 12/12 and imported 12/12, with exact JUnit case sets, no skips and final no-work builds. |
| Formatting and structure | Apply/check of 1,681 handwritten C++ files; 930 canonical imported consumer/fixture bodies and active package/source provenance verify. |
| Preservation | Existing production/test bodies, six native archives, all 78 SDKs/3,457 files and 2,201 protected artifacts retain their hashes. |

The first reference-lock guard rejected 44 divergent registry identities in the
historical SQL-protocol observer workspace. Its output is not source evidence
for this acceptance. The independent `rust_data_service_source_5_0_6` observer
uses the original versions and checksums, including futures-core 0.3.32. An
initial incorrect 0.3.34 pin was rejected before comparison; both lock failures
remain in separate receipts. The historical workspaces and their locks remain
unchanged. The new observer calls unchanged original APIs rather than copying
runtime algorithms. No production C++ implementation, generator or generated SDK
body changed in this continuation.

Exactly `teaql-data-service/src/lib.rs`, `tests/request_intent_vectors.rs` and
`tests/request_privacy.rs` advance from pending review to accepted. Source status
is **43 accepted / 79 pending / 2 excluded_external**. All seventeen capability
statuses remain unchanged: eleven qualified and six partial. Native discovery is
744 after the two new checks; this is not a whole-suite pass. The historical
735/736 and 718/721 failed full epochs retain their original outcomes. Whole
governed runtime execution, G13/G14/G17 and P0-P7 retain their independent
obligations and unanswered source-behavior decisions. No new sanitizer, MIPS,
GPU, external-provider or physical-router result is claimed.

## Reproduce the source comparison

After configuring both frontends with the locked toolchain, build the four
observer executables in each before invoking the reproducer:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_data_service_model_probe teaql_data_service_model_scope_probe \
    teaql_executor_defaults_scope_probe teaql_data_service_request_source_probe
cmake --preset modules_native
cmake --build --preset modules_native --parallel 2 --target \
    teaql_module_data_service_model_probe teaql_module_data_service_model_scope_probe \
    teaql_module_executor_defaults_scope_probe teaql_module_data_service_request_source_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_data_service_source_5_0_6.py \
    --module-build-dir build_modules_native --record "$replay_directory/source.json.gz"
```

The reproducer checks reference identity, runs the actual original tests, checks
registry versions/checksums, waits for a successful locked observer build and
compares every source/native/imported result and exact corpus bytes. The full
installed twelve-test selection commands, output, canonical-body validation and
JUnit results are retained in the installed receipt linked above.
