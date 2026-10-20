# Complete entity-runtime source review

Status: the complete mapped source is accepted for ER01-ER32 with the explicit
Context, owner-thread and exception adaptations below. Full generator
capabilities and P0-P7 have separate exits.

Reference: Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, complete
`teaql-runtime/src/entity_runtime.rs` (2,493 lines). The historical
[5.0.5 API review](entity_runtime_api_contract.md) retains its own evidence.

## Responsibility and branch reconciliation

| ID | Original source | C++ mapping and reviewed contract |
| --- | --- | --- |
| ER01 | LoadedRelation, RelationHandle | Three states remain distinct. Loaded-empty lists retain a value; loaded-null options do not. Borrowed reads never issue SQL. Native handles validate their Context. |
| ER02 | EntityKey constructors and cached identity | Owned/static source names map to explicit allocator-owned names. Clone retains the construction-time identity even if public `id` changes. Public entity-name changes still affect ordering. |
| ER03 | EntityIdentityKey and Eq/Ord | Null and TypedNull share identity; nonnegative I64 normalizes to U64; negative I64 remains separate. Float bits, Decimal, text, date, timestamp, serialized JSON, and distinct object/list tags preserve source ordering and equality. |
| ER04 | EntityGraphBuilder tables | Per-type identity tables preserve replacement, type namespace and counts. Native erased payloads destroy replaced RAII values and reject unsupported destination copies explicitly. |
| ER05 | JSON option index | Indexing requires an installed JSON index, a present entity, a loaded ID and numeric conversion. Duplicate identities mark the index ambiguous; replacing the relation still occurs. |
| ER06 | Relation list/option storage | Canonical owner names, owner IDs, relation names and view coordinates remain separate. Ordinary APIs use view zero; JSON APIs preserve the supplied coordinate. |
| ER07 | RelationFacetResults | Nonempty ordinary list facets and explicit facet installation are query metadata. Empty replacement does not clear a previously retained facet map. JSON-list installation does not infer facets. Null to-one views can retain facets. |
| ER08 | freeze and FrozenEntityGraph | Publishing is one-shot. Weak or already frozen views return an intact builder. Native Context ownership replaces source Arc reclamation without changing publication authority. |
| ER09 | resolve_entity | Direct type/ID tables take precedence. JSON option lookup then checks coordinate/type/ID, unambiguous edge, stored option type and presence. |
| ER10 | Relation access and handles | Raw resolvers return absence for a type mismatch; checked handles raise the source-shaped mismatch failure. Absent, empty-list, null-option and present values remain distinct. |
| ER11 | EntityChangeSet native values | Native changes remain MutationValues keyed by EntityKey. Set replaces only that field; get is distinct from extension intent. |
| ER12 | DynamicMutationMetadata | Optional dynamic changes, immutable definitions and first-original maps retain separate responsibilities inside the compact native allocation. First definitions/originals win; pending mutation replaces. None and stored NULL remain distinct. |
| ER13 | clear_entity and field_names | Clearing removes native changes, dynamic changes, definitions and originals for the key. Changed names union native fields and `#code` extension names. |
| ER14 | ChangeSetStack | Current frame is lazy; reads search top to bottom. Push, pop and current-frame clear preserve other frames. Entity clear covers every frame; changed names form a union. |
| ER15 | OriginalVersions | The first slot and overflow map stay distinct. Same-key insert overwrites its version; merge visits the source first slot before overflow. |
| ER16 | EntityMutationLedger | Root/entity reasons, new/delete sets, materialized/compact originals, keyed originals, assigned/unassigned trace scopes, versions, deferred errors and the private legacy is_new flag remain distinct. Public mark_as_new updates the key set, as in Rust. |
| ER17 | LazyLedgerSlot | Readonly access does not allocate a binding or ledger. First Clone shares one empty binding; mutation materializes one ledger and seeds keyed originals from the loaded row. Source poison recovery maps to the existing owner-thread Context contract. |
| ER18 | LoadedSnapshotCarrier | Inline storage promotes once without copying the value buffer; clones share the immutable row. Returned originals are private copies. Replacing one view preserves earlier aliases. Nested relation results are stripped. |
| ER19 | EntityGraphReference | Preserve, promote and weak are separate operations. Actual Clone promotes; fresh graph sharing preserves the source role. Native graph/detached values expire at Context close, including checked handles. |
| ER20 | Default and Clone | Explicit Context construction replaces Default allocation. Same-domain Clone shares ledger/graph, copies private dynamic values and shares immutable originals. Explicit destination retention deep-copies graph cycles and ledger aliases with a temporary destination-owned copy session. |
| ER21 | EntityRuntimeState PartialEq | Dynamic values compare before slot/ledger identity. Independent cold states differ; graph/view/snapshot content does not replace ledger identity. NaN payloads can make self-comparison false. |
| ER22 | with_json_view / with_loaded_view_from | Consuming calls preserve receiver role/binding. Loaded-view transfer replaces only coordinate, snapshot and private dynamic payload; it does not import source intent. Lvalue convenience retains explicit copy behavior. |
| ER23 | fresh_with_shared_graph / with_shared_graph | Fresh state gets a new cold ledger and no loaded payload while retaining source graph/view. Existing-state graph rebinding retains receiver ledger/loaded payload and preserves source graph role. |
| ER24 | Checked adoption preflight | Empty source and same ledger are no-ops. Source snapshot/version order and complete definition compatibility are checked before field intent is imported. Target allocation itself may occur during a failed check, as in Rust. |
| ER25 | Checked adoption merge | Source frames collapse into target current intent. Deletes/new keys, versions, complete trace tokens, unassigned ownership, local reasons and originals retain their separate precedence. Root request reason is never imported; source intent stays retryable. |
| ER26 | Unchecked adoption and errors | Void attachment records composition errors for later save preflight. Checked default methods preserve missing-target before missing-source precedence. Error Display, Clone/equality and absent cause remain explicit. |
| ER27 | New/delete and frame APIs | Delete clears every pending frame for that key without silently removing its new marker. Readonly getters avoid ledger allocation. Mutation/read copies use the bound allocator. |
| ER28 | Original snapshots and versions | Per-view originals precede root legacy originals; keyed originals prefer ledger capture. Version lookup requires a present numeric match. Composition/keyed fallback deliberately retains optional numeric equality, including None == None. Both materialized/compact legacy variants remain represented. |
| ER29 | Dynamic update/delete | Missing definitions and owner mismatch precede assignment. Original capture occurs before typed validation/update. Set returns selection growth, Delete returns prior selection. Native field-name borrowing is stabilized against payload replacement. |
| ER30 | Trace replacement and allocation | Replacement preserves immutable parent sharing. Allocation accepts only positive numeric IDs from explicit unassigned ownership, rebinds ancestors iteratively, caches shared parents and preserves unchanged tokens/node indices. It does not infer owners from entity names. |
| ER31 | clear_committed and annotations | Successful cleanup clears pending frames, markers, versions, reasons, trace maps and originals while retaining deferred composition errors and the per-view loaded snapshot/dynamic values. Failed saves must keep their pending intent. |
| ER32 | Traits, derives and public defaults | Complete structured diagnostics retain private fields and variants. Relation handles remain copyable borrows; EntityKey/sets/stacks/errors preserve their source equality responsibilities. LedgerEntity receivers and sources must be Entity-capable; CRTP construction itself stays unconstrained. Native exception/RAII and owner-thread adaptations remain explicit below. |

## Two reproduced repairs

The [equality witness](parity/entity_runtime_equality_initial_gap_5_0_6.json.gz)
records **176 differences among 338 inputs**. The old native state equality
treated identical dynamic-payload pointers as equal before comparing values.
Rust compares the payload first, so legal F64 NaNs make even self-comparison
false. The native guard now distinguishes absent payloads and compares every
present payload before testing ledger identity. No input is rejected or
normalized to manufacture equality. The matrix includes absent fields, finite
values, signed zero, subnormal/max-finite values, infinities, distinct quiet and
signaling NaNs, cloned/independent/rebound states and cold/materialized ledgers.

The [LedgerEntity compiler witness](parity/ledger_entity_guard_initial_gap_5_0_6.json.gz)
records two target forms accepted by C++ and rejected by Rust. A receiver that
only supplied a runtime-state getter could invoke instance or delegated
composition despite lacking Entity. The source argument was already constrained.
Method-level `requires LedgerEntity<Derived>` now enforces the target contract
after the CRTP-derived type is complete. The merge body is unchanged.
The [compiler repair](parity/ledger_entity_guard_repair_5_0_6.json.gz) checks one
positive and three negative programs in each language. The positive native
program also runs through the registered native/imported contract.

The earlier [consuming loaded-view repair](loaded_view_move_5_0_6.md) retains its
separate before/after and sanitizer evidence. This continuation preserves those
512 outcomes and actual Context deletion assertions.

## Actual-source evidence

The independent observer project uses exact reference registry versions and
checksums. A disposable runtime view preserves all 67 original source/test/vector
files and appends access-only adapters. All 273 original runtime test names are
checked; the complete **21 embedded entity-runtime tests** run unchanged.

The old identity observer included the complete module in an isolated namespace.
Its new root adapter retains every observer-body byte after `use pinned::*` and
delegates private inspection to the actual crate-root runtime. The fixture path
changes only to its new directory. Other retained drivers compile directly
against the crate-root runtime. Legacy dynamic-ledger corpus metadata lacked a
driver hash; the controller verifies its original source hashes and records the
actual reused driver's current hash. It does not invent historical provenance.

| Family | Exact original/native/imported outcomes |
| --- | ---: |
| Identity graph, frames, ledger and composition | 241 |
| Complete model and formatting variants | 252 |
| Dynamic mutation/state behavior | 147 |
| JSON graph coordinates and option index | 2,304 |
| Consuming loaded view | 512 |
| Dynamic state equality | 338 |
| Full dynamic state/change-set diagnostic layouts | 432 |
| **Total** | **4,226** |

The new 432-case diagnostic matrix crosses absent/NULL/value originals, optional
storage binding, loaded rows, materialized ledgers, Clone and nine mutation/frame/
cleanup actions. It compares compact/pretty state and change-set diagnostics and
the cloned view after mutation. A single declared field avoids unspecified Rust
HashMap diagnostic ordering; no output normalization occurs. Raw diagnostic
models retain source payloads and are not safe publication sinks by themselves.

The [initial combined collector](parity/entity_runtime_source_execution_5_0_6.json.gz)
retains a metadata-reader failure when it encountered that legacy corpus without
a driver field. The [subsequent 16-check run](parity/entity_runtime_source_retry_execution_5_0_6.json.gz)
passes all seven families and native/imported contracts before the receiver-guard
repair. The [guard-aware run](parity/entity_runtime_source_after_guards_5_0_6.json.gz)
then passes native17 and all original observations, but its imported configuration
fails after an existing generated ledger test is accidentally registered twice.
The duplicate is removed; the original registration and fixture are preserved.
The [imported continuation](parity/entity_runtime_source_module_continuation_5_0_6.json.gz)
reuses byte-checked source/native evidence and passes imported17. A separate
[native stability audit](parity/entity_runtime_source_native_stability_5_0_6.json.gz)
confirms the same native input graph reports no pending work. The reproduction
helper now requires that check before resuming a partial run.

The [final qualification](parity/entity_runtime_source_qualification_5_0_6.json.gz)
checks all evidence before accepting this source. The paired compiler controls
pass 1 positive / 3 negative programs in each language. Existing generated
CamelCase composition, explicit retention and actual SQLite Save pass
[native2](parity/entity_runtime_generated_callers_5_0_6.json.gz) and
[imported1](parity/entity_runtime_generated_imported_callers_5_0_6.json.gz).
These are current application regressions, not a fresh generator replay.

Final selected builds report no work. Pinned formatting/check covers 1,703 C++
files; 996 canonical module bodies and 1,516 exports across seven acyclic packages
verify. The [preservation audit](parity/entity_runtime_source_preservation_5_0_6.json.gz)
checks all 78 existing SDKs / 3,457 files and 5,741 protected artifacts against
the start of this continuation. The two default-method bodies are byte-identical;
production changes are the value-first equality guard and delayed receiver bounds.

Only entity_runtime.rs advances, bringing source dispositions to **57 accepted /
65 pending / 2 excluded_external**. All seventeen capability verdicts remain
unchanged. Historical failed full-suite epochs and the narrower earlier source
checkpoints retain their outcomes.

## Ownership and source-test adaptations

Thirteen original scalar/composition/frame tests have direct ports in
`tests/parity/teaql_runtime/entity_runtime_contracts.hpp`, including the same
512 trials of 32 deterministic mutations. Native entity-runtime tests additionally
check shared original-row/value buffers, independent returned maps, keyed view
precedence, immutable trace ancestors, commit cleanup and eager RAII payload
destruction. The graph-reference contracts separately verify weak publication,
actual Clone promotion, explicit retention, actual User deletion and exactly-once
payload cleanup.

Rust's two concurrent-first-clone tests execute in the original suite. C++ uses
the agreed owner-thread Context contract: sequential first/later clones must
share one binding and immutable row, while cross-domain retention is explicit.
The implementation does not promise simultaneous mutation of one UserContext.
Rust last-Arc-drop graph reclamation maps to complete Context reclamation;
ordinary native handles never extend its lifetime. UnwindSafe/RefUnwindSafe have
no C++ marker trait: existing exception unwinding, registered destruction and
allocator failure contracts provide the native responsibility.

These adaptations retain graph/reference, snapshot/carrier, stack/ledger, dynamic
metadata and service boundaries. They do not accept graph Save, SQL, authorization
or privacy callers merely because entity-runtime model behavior agrees.

## Reproduction

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_entity_runtime_source_5_0_6.py \
    --record /tmp/teaql_entity_runtime_source_fresh.json.gz --modules
```

Use a fresh receipt path. Routine checks are native and imported. The changes in
this continuation affect equality and compile-time capability bounds; no new
allocator/lifetime mechanism or external dependency is introduced. Existing
scoped ASan/UBSan evidence remains separate, with leak scanning disabled. No new
MIPS/QEMU, physical-device, Java/generation or complete-suite result is claimed.
