# Complete SQL mutation orchestration source review

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This review covers all **523 lines** of
`teaql-sql/src/mutation_execution.rs`, including its private state, helpers,
transport implementation, ordinary tree execution and guarded entry.
The containing executor, runtime graph/save, governance and safe publication
retain independent qualification requirements.

## Reproduced differences and runtime repairs

The new public-source matrix has 2,016 orchestration inputs and 24 stateful
descriptor-provider inputs. Its initial native comparison reproduced **292
differences** while the retained mutation, transaction and backend-kind matrices
continued to agree.

**Guarded sidecars, 276 differences.** The locked guarded entry deliberately
constructs its observed transport with `inherited_redactions: None`. The C++
shared dispatcher instead performed ordinary request precollection for guarded
work, propagating an attached diagnostic sidecar into returned and physical
metadata. It now constructs empty inherited provenance for this entry. This
preserves the source distinction between ordinary inherited request provenance,
raw compiled bindings, target-ID provenance and derived-readback provenance.
Raw bindings and parameter policies are unchanged. This review qualifies raw
provenance against the source; complete runtime privacy/publication remains a
separate gate.

**Descriptor cache, 16 differences.** The original ordinary entry receives the
executor's cached descriptor lookup. C++ precollection called SchemaProvider
directly, then used cached descriptors for compilation. A provider that changes
its name-mask definition after a cache warm-up exposed both extra lookups and
inconsistent inherited classification. Precollection now uses the same existing
`SqlExecutorState::entity_descriptor` as leaf compilation. The cache fixture
records actual provider lookups, SQL, bindings, policies and secrets; neither
observer implements caching or privacy algorithms.

The repair changes only the private helper's explicit cache argument, its lookup
call, and the dispatcher's guarded/ordinary selection. The application-facing
Q/E/model APIs and generated code are unchanged. Existing users of the private
detail helper must rebuild against its additional cache parameter.

## Complete responsibility reconciliation

The native implementation is split between
`teaql_sql/src/mutation_execution.cpp` and
`teaql_sql/include/teaql/sql/detail/mutation_execution.hpp`. Ordinary, guarded
and transaction facades continue to delegate into these shared mechanics.

| ID | Locked source responsibility | Native implementation and preserved behavior |
| --- | --- | --- |
| ME01 | BatchIntentScope and cloned MutationExecutionScope, lines 8–19 | Validated root intent, ordered immutable batch ancestry, inherited provenance and invocation recorder remain separate. Context-owned nodes and borrows replace Arc ownership. |
| ME02 | `statement_trace`, lines 21–72 | Materialize root-to-leaf batch reasons with the item's entity type; repeated first or last reason occupies one slot while retaining its original entity identity. Keep explicit graph frames and remaining trace order. |
| ME03 | `capture_values` inside precollection, lines 80–104 | Preserve field classification through the existing binding SPI, generated-SQL context, current/old values, credential names and Unknown fallback. Values remain raw execution data. |
| ME04 | `batch_intent_redactions`, lines 74–140 | Retain own diagnostic provenance for ordinary requests; only batch roots traverse descendants. Use LIFO traversal, inherited child sidecars, current and old update values, insert literal `id`, and update/delete/recover target IDs. Precollection never compiles or executes a future sibling. |
| ME05 | Lookup closure used by precollection | Use the same executor descriptor cache as compilation, including warmed metadata and shared transaction state. Missing metadata remains Unknown; missing entities still fail in execution order. |
| ME06 | ObservedTransport fields, lines 143–154 | One leaf adapter and fallback recorder per invocation. Retain backend, operation, item trace, root comment, per-write binding provenance, inherited provenance, target identity and successful readbacks independently. |
| ME07 | `metadata`, lines 156–227 | Copy compiled SQL/policies/parameters without eager debug SQL; capture start/end, absent counts and backend request ID, typed operation, comment and trace. Readback uses the last write's binding provenance, current target identity and ordinary inherited provenance. |
| ME08 | Derived-readback trace inside `metadata` | Prepend Comment and the fixed verification Purpose using the first trace entity or `unknown`; inherit all non-Comment/non-Purpose frames in order. Readback is derived work, not a new caller request. |
| ME09 | `emit`, lines 228–232 | Optional invocation recorder receives physical metadata. Actual user sinks are reached through the failure journal, preserving its exception isolation. |
| ME10 | `retain_successful_readbacks`, lines 234–249 | With actual readbacks, retain logical summary and create physical children in write-then-read order; clear retained readbacks after attaching. Empty readback storage leaves summary structure untouched. Native batch item grouping remains intact. |
| ME11 | `fetch_all_compact_sql`, lines 253–273 | Always construct readback diagnostics, including without an observer. On success set actual row count, retain the readback and optionally record it; on failure preserve the provider error and one terminal diagnostic. |
| ME12 | `execute_sql`, lines 274–296 | Refresh binding provenance before every write. With no observer, forward without a physical write diagnostic. With a recorder, preserve write outcome/affected count and original transport failure. |
| ME13 | Ordinary `execute`, lines 298–324 | Create one failure journal, capture root intent and complete batch provenance before execution, recursively share the invocation scope, disarm only on total success, and drain ordered facts on failure/cancellation/unwind. |
| ME14 | Batch branch of `execute_tree`, lines 340–403 | Collapse adjacent equal native batch reasons, execute children sequentially, stop at first failure, sum writes only, concatenate present SQL with `; `, concatenate parameters and retain each native item group. Return empty GeneratedValues and no snapshot. |
| ME15 | Ordinary leaf compilation, lines 405–452 | Resolve entity, compile the matching Insert/Update/Delete/Recover command, preserve operation, independently determine target and persisted IDs, and construct observed transport only after successful ordinary compilation. Hard deletes have target identity but no persisted ID. |
| ME16 | Ordinary write/result, lines 453–481 | Return actual affected rows, Success metadata and distinct empty GeneratedValues. Read back only when requested, exactly one row was affected, and a persisted ID exists. Require exactly one returned row; zero or multiple rows are PersistedRecord failure despite successful SELECT metadata. Snapshot comes from the actual compact row. |
| ME17 | Guarded `guarded`, lines 483–523 | Validated Update/Delete/Recover only; capture observed backend before compilation, retain the request's execution trace/comment and target identity, deliberately omit inherited sidecars, and attach successful physical readbacks before disarming the journal. |
| ME18 | Required guarded executor helper, executor.rs lines 1676–1776 | Compile guard into the write; for a single affected persisted object, read with `id AND guard`. Preserve its distinct returned-summary provenance and actual write/read metadata. No match or hard delete invents no readback. This helper review does not accept whole executor.rs. |
| ME19 | Error, pending and cleanup composition | Compile, Transport and PersistedRecord remain distinct. Context cancellation maps future Drop; release child work before observers, preserve successful earlier facts, keep unknown inflight counts, and isolate sink failures. Noncopyable native errors use the existing completion-transfer path. |
| ME20 | Private ownership/default/copy boundaries | Source helpers are private; no new public Default/Debug/serde protocol is invented. Scope copies borrow their invocation's Context-owned ancestry/provenance/recorder and copy root intent into that same Context. They are not cross-domain retention APIs. All ordinary results remain Context borrows. |

The original batch count uses checked arithmetic in the Debug qualification
profile; native code explicitly rejects u64 overflow rather than permitting C++
undefined behavior. The retained corpus includes maximum-u64 transport counts.
This checkpoint does not infer a new release-profile or MIPS arithmetic result.

## Original source and observer provenance

The independent `tests/parity/rust_mutation_execution_source_5_0_6` project uses
only exact reference registry identities. A disposable SQL view preserves all
twelve original source files and all original dependency declarations. The
existing appended diagnostic observer is inert in these public executor calls.
The supplemental observer-only serde_json dependency is reference-pinned.

The execution observer and its value/SQL fixture helpers are reused byte-for-byte.
The additional cache observer includes that unchanged observer and adds only a
stateful SchemaProvider plus public query/mutation calls. Its provider switches
mask definitions after optional prewarming and records lookups; the real executor
decides what remains cached. The native counterpart uses the actual shared
SqlExecutorState, typed transport and public executor/transaction entries.

| Matrix | Inputs | Covered behavior |
| --- | ---: | --- |
| New mutation orchestration | 2,016 | Five mutation forms; ordinary/guarded, executor/transaction, observer absent/present, three sidecar configurations, write/read success/error/pending/unwind and cardinalities, eight batch shapes including empty/repeated/nested/future-unknown cases. |
| New descriptor cache | 24 | Both mask transitions, cold/warm cache, zero/one/two children and executor/transaction sharing. Actual provider lookup order and privacy remain observable. |
| Retained mutation protocol | 2,029 | Existing command, snapshot, trace, intent, compiler/fault and extreme-count boundaries. |
| Retained transaction protocol | 2,029 | Existing transaction readback/fault/intent behavior. |
| Retained backend-kind protocol | 93 | Existing invocation capture, lazy compilation and backend-hook failure ordering. |
| Total per original/native/imported frontend | **6,191** | Exact outputs with no normalization; the existing observation projection omits wall-clock timestamps. |

The retained protocol matrices include historical inputs rejected for missing
intent as well as explicit-intent controls that execute. These totals count
observed outcomes, not SQL statements; their exact split remains in
[the protocol review](sql_protocol_5_0_6.md). New orchestration requests have
explicit owned reasons; unsupported guarded forms are intentional compile
rejection controls.

The original six query and twenty-one mutation tests also run unchanged against
the exact source view. Their complete fixture/helper/assertion review is in
[the execution-test acceptance](sql_execution_tests_source_review_5_0_6.md).
The selected native/imported checks include both semantic ports, transaction
execution and move-only error/lifetime contracts.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_mutation_execution_source_5_0_6.py \
    --record /tmp/mutation_execution_source_506.json.gz \
    --profiles native modules_native sanitize
```

`--bootstrap-corpora` is only for a missing new corpus. Existing corpora must
match fresh original outputs. `--capture-initial` records differences without
claiming semantic acceptance. Use fresh receipt paths. Source bodies, helper
copies, observer dependencies, actual binaries, registered tests and terminal
build/test results are recorded; selected final builds must report no work.
Sanitizer leak scanning is disabled. Complete provider/runtime/publication,
source executor, current full integration and P0-P7 exits remain independent.

## Retained initial evidence

The [initial comparison](parity/mutation_execution_source_initial_retry_5_0_6.json.gz)
retains all 276 orchestration and 16 descriptor-cache differences, along with
the three unchanged matching protocol families. It is explicitly a difference
capture, not semantic acceptance.

The new cache fixture initially tried to serialize Vector<String> directly as
Json; its [compile failure](parity/mutation_execution_fixture_initial_compile_5_0_6.json.gz)
is retained. Explicit conversion to Json::Array repaired the fixture only.
The first original-observer build also lacked the crate-root fixture_value
module required by the unchanged SQL helper. Its
[failed receipt](parity/mutation_execution_source_initial_5_0_6.json.gz) is retained;
the corrected observer imports that existing helper without changing its body.

The first post-repair replay matched all 6,191 source/native outcomes, but two
CTest comparators rejected descriptive text in the new corpora's baseline field.
That [failed test receipt](parity/mutation_execution_source_execution_5_0_6.json.gz)
is retained. The [metadata-only repair](parity/mutation_execution_corpus_metadata_repair_5_0_6.json.gz)
preserves both initial corpus byte streams and verifies identical case payload
hashes before replacing the baseline field with the locked commit. No input or
original expected outcome was edited to pass a comparison.

The [corrected execution](parity/mutation_execution_source_execution_retry_5_0_6.json.gz)
passes all **6,191 original/native/imported observations per frontend**, all
**27 unchanged original tests**, and **native 9/imported 9/ASan-UBSan 9** checks.
Every selected build finishes successfully and each final rebuild reports no
work. Leak scanning is disabled. These are focused checks, not a new full-suite
result or physical-device claim.

## Final qualification

The [consumer follow-up](parity/mutation_execution_source_consumers_retry_5_0_6.json.gz)
passes the existing generated Q/E example and ledger composition on actual
SQLite: native **2/2**, imported **1/1**, with final no-work rebuilds. Both
formatter commands pass for **1,705 handwritten C++ files**. The initial consumer
collector's relative JUnit path was resolved under the preset's build directory;
the already successful native test output and original receipt are retained,
then reused explicitly by the corrected collector. No test was silently rerun
or its result relabeled.

[Static verification](parity/mutation_execution_source_static_5_0_6.json.gz)
passes source/module paths, **1,516 exports**, **1,003 canonical bodies**, all
42 generator source anchors and 17 capability records. Native discovery is
**777**; the module preset discovers **563 imported tests**. These counts are
inventories rather than full integration results.

The [preservation record](parity/mutation_execution_source_preservation_5_0_6.json.gz)
contains the complete two-file production diff and verifies the other **6,089
protected files** unchanged, including the retained **78 SDKs / 3,457 files**.
Original helper/corpus bytes remain intact; both new corpora preserve their
original outcome payloads across the documented baseline-metadata correction.

The [qualification receipt](parity/mutation_execution_source_qualification_5_0_6.json.gz)
accepts only the complete mutation_execution.rs source, moving dispositions to
**62 accepted / 60 pending / 2 excluded_external**. Capability verdicts and
full P0-P7 exits remain unchanged. The historical failed 735/736 and 718/721
epochs stay failed. Full current integration, pending executor/provider/runtime
sources and standing behavior decisions remain required. No new Java/generation,
MIPS or physical-device result is claimed.
