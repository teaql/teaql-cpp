# Shared-state source test responsibilities: Rust 5.0.6

Current status: all three complete test files are accepted. The later
[typed-batch and source review](generated_load_state_bulk_5_0_6.md) closes all
nineteen shared-load-state responsibilities, bringing source dispositions to
19 accepted / 105 pending. G15 stays partial for the separate quoted-FK cases.

The initial checkpoint below accepted the dynamic-field and readonly-property
test sources and kept the shared-load-state file pending. This audit reviews
all three original files, rather than accepting them because G15/G16 already
had capability receipts. All 33 actual original tests pass: 11 dynamic-field,
3 readonly-property and 19 shared-load-state tests. Passing the original tests
establishes the reference behavior, not the C++ result.

References are the exact runtime and generator commits in `upstream.lock.json`
and the generator capability ledger. The reference sources and original user
checkouts remain unchanged.

## Reproduced sharing defects

The [two initial native witnesses](parity/shared_state_source_initial_5_0_6.json.gz)
preserve complete pre-repair production bodies and both failures:

- A new storage binding copies the definition's field dictionary, although the
  source explicitly shares that immutable dictionary.
- A same-Context SelectQuery clone copies its dynamic selection, although the
  source shares an immutable selection and replaces only the receiving query's
  selection on change.

`DynamicFieldDefinitions` now freezes a successful Context-backed dictionary into
an independent managed node. Binding construction and same-scope copies borrow
that node; permitted Global-to-User borrowing follows the existing parent-domain
rule. Explicit destination copies retain a destination-owned dictionary when the
source cannot lend. A definition can be explicitly destroyed without destroying
the dictionary still used by another definition in that Context. Registered node
cleanup and all blocks remain bounded by the owning Context.

Pure metadata construction on an explicitly supplied non-Arena resource retains
its existing allocator-aware value behavior. It creates no hidden owner. Invalid
definitions fail before dictionary publication. The dictionary is a managed
object and consumes one object identity; individual map/string backing allocations
do not each acquire identities.

The public SelectQuery selection field keeps its existing ScopedBox type and
methods. Context-backed selection construction registers the immutable payload
with the arena. A null deleter resource marks this already registered payload;
the ordinary wrapper runs no second destructor and owns no lifetime. Same-Context
clones reuse the payload; replacement creates a different payload. Explicit
cross-domain query copies copy it into the destination. Non-Arena value
construction retains its existing owning box. All other owning ScopedBoxes keep
their original allocator/deletion path.

This adds no reference count, private query region, singleton, TLS owner or
generator dependency. The native selection handle remains 16 bytes with an
8-byte pointer. Rust's Option<Arc<_>> is one word; this is a documented C++
handle-representation adaptation, not an ABI-size equality claim. The shared
immutable payload and independent replacement responsibility are preserved.

For registered selection payloads, `ScopedBox::get()` and `release()` expose a
borrow; they do not transfer the arena's destructor registration or ownership.

## Complete dynamic-field test mapping

The source is `teaql-core/tests/dynamic_field_state.rs`.

| Original test | C++ responsibility and evidence |
| --- | --- |
| `storage_binding_is_opaque_and_does_not_clone_the_definition_dictionary` | The new `binding_dictionary` contract checks matching-binding identity, the exact same dictionary address, same-schema versus full equality, independent profile/executor differences and opaque diagnostics. It also destroys the original definition and actually deletes UserContext before checking the explicit Global copy. |
| `compact_enhancements_merge_private_values_and_share_actual_union_geometry` | `bulk_sidecars` runs all original 1/100/10,000 sizes through real batch construction, relation-shape marking and checked CompactRow enhancement. It checks common result layout/union selection, row-specific text, incoming zero, selected NULL, absent Bool and map exclusion. Existing carrier comparisons retain small-shape merge errors and values. |
| `incompatible_dynamic_enhancements_reject_atomically_without_provenance_leaks` | Existing definition/value/carrier observers and native contracts distinguish no binding, another profile/executor, owner, revision and type mismatch. They retain the receiver and hide private binding/value data. Full equality remains separate from same-schema. |
| `enhancement_empty_selection_keeps_existing_values_and_superset_keeps_its_shared_arc` | Dynamic value merge keeps loaded receiver data for absent input and reuses subset/superset selection identities. Existing actual-source geometry matrices and `dynamic_fields_contracts` check these branches. |
| `wrapper_preserves_zero_false_empty_null_and_not_loaded_without_io` | Core field wrappers preserve all three states, declared types, known-unloaded versus unknown names and zero/false/empty text. The actual-source definition/value corpus retains variant/error outputs; these operations perform no provider I/O. |
| `one_selection_snapshot_per_actual_batch_shape_and_private_row_values` | Actual batch keys determine selection, including shared Value/NULL shapes and distinct absent shapes. Existing batch/clone tests and the large enhancement contract check private row values with shared immutable geometry. Copies retain their own payload after other row wrappers are destroyed. |
| `unknown_definitions_wrong_types_and_derived_namespace_are_rejected_without_values_in_errors` | Original-code/name/type/typed-NULL checks remain separate from readonly `_` validation. The unchanged source corpus compares precise codes/fields and private-canary exclusion; failed writes leave the receiver unchanged. |
| `selecting_another_shape_does_not_widen_old_rows_or_revisions` | Core snapshots and value carriers publish replacement geometry without widening earlier rows, preserve old values and distinguish owner/revision identity. |
| `dynamic_hydration_preserves_fixed_bits_relations_and_sibling_state` | The 176-case carrier matrix and current native contracts preserve fixed/overflow/relation state, selected `#` names, no-op identity, absence rejoining and owner rejection. Derived `_` names never become fixed/durable availability. |
| `query_selection_is_one_shared_pointer_and_cloning_cannot_change_siblings` | The new `query_selection` contract requires actual pointer identity after a same-Context clone and different identity after replacement. It checks membership independence, explicit Global copying, original-query destruction and actual UserContext deletion. The C++ wrapper width differs as explained above. |
| `value_only_assignment_retains_selection_and_delete_detaches_only_that_payload` | Existing assignments/deletes preserve no-op identity, selected NULL and failed-validation atomicity. Delete changes only the receiver's geometry; known-missing/unknown remain distinct and another payload does not widen its sibling. |

The original helper `definitions()` retains all four names and their declared
types. The C++ contract uses the same owner/revision and vocabulary for the
large-batch obligations; existing broader Value/validation observers supplement
those fixtures without replacing their assertions.

## Complete readonly-property test mapping

The source is `teaql-core/tests/dynamic_property_metadata.rs`. Its metadata
factory describes three fixed fields and no relation, independently of readonly
result types.

| Original test | C++ responsibility and evidence |
| --- | --- |
| `compatible_result_shapes_share_schema_not_values_or_presence` | `readonly_metadata` checks shared snapshot/schema identity, declared-but-absent `_missing`, no fixed `_count` slot, unchanged fixed field count, separate zero/NULL row values and successful validation. |
| `shape_changes_preserve_metadata_and_conflicting_enhancements_are_atomic` | Insert/remove retains schema. Conflicting type declarations remain distinct shapes and fail checked enhancement before scalar overwrite, without including the private value in errors. The new contract and existing actual-source carrier/cache matrices cover these boundaries. |
| `explicit_types_validate_null_variants_without_creating_presence` | Invalid/duplicate readonly names fail. Untyped NULL, matching typed NULL, native zero and supported JSON zero validate; mismatched typed NULL and private Text fail. Original schema observations retain exact field-bearing errors, separately from persistent validators. |

## Historical shared-load-state pending review

The source is `teaql-core/tests/shared_load_state.rs`. Its 19 original tests all
execute, including both concurrent tests. Earlier G15 and G01 acceptance provides
scoped evidence for the mechanisms below; it cannot automatically accept this
whole file.

| Original test responsibility | Current disposition |
| --- | --- |
| Flat relation shape metadata | Existing row/snapshot and allocation contracts check shared geometry, exclusion from row/map values, independent unselected shapes and preservation after scalar insertion. |
| Fixed/overflow boundary slots | Existing snapshot source/contract matrices retain bits 0/31/32/63 and lazy 64/65/129 slots, private clearing and unchanged fixed bits. |
| No-op and private divergent availability | Snapshot pointer/identity and auxiliary-sharing contracts retain no-op identity and isolated changed overflow/named state. |
| Unchanged durable selections and relation markers | Carrier source cases retain no-op/value-only reuse, replacement/removal, base-state rejoining and foreign-owner rejection. |
| Loaded FK versus target/derived availability | Existing relation and generated helper matrices keep scalar identity, known NULL, loaded object detail and missing view distinct. |
| Invalid generated tables | Complete FieldLayout validation tests and original construction corpus preserve duplicate/out-of-range/index/mapping/member failures rather than renumbering. |
| Revision and internal wire boundary | Existing incompatible-layout and Indexed serialization/deserialization controls retain identity/revision separation and reject client masks. |
| Independent typed rows and absent defaults | Generated model/core hydration and G01 observations distinguish unloaded defaults from staged mutation values, including explicit zero/false. |
| NULL/missing/empty/zero/false typed payloads | Existing ordinary/optional/generated model and Checker matrices retain these distinct states. |
| COW rejoining and cross-revision refusal | Snapshot/source contracts retain canonical empty rejoining, removal of auxiliary presence and foreign-layout refusal. |
| Manual actual-shape grouping | CompactRow source and initialized-cache matrices preserve independent values and do not union selected columns. |
| Physical scalar alias payload | Existing native/physical alias decode and generated relation observations retain actual values rather than materialized defaults. |
| **Three distinct canonical/member/physical aliases** | **Historical reproduced gap.** Repaired by the later runtime/generated adapter and typed-batch review linked above. No whole-file acceptance follows while a loaded member alias decodes as missing. |
| Same columns across types/revisions | Initialized and model matrices distinguish layout identity/mask numbering while retaining repeated same-type reuse. |
| FK scalar/NULL versus nested object | Existing original generated forward-object and loaded-graph observations preserve local identity and explicit detail availability separately. |
| Cold concurrent metadata installation | Original test passes its 16-thread barrier and asserts one layout/state. C++ uses the approved owner-thread catalog/pool adaptation: one Global layout, separate User pools and foreign-thread refusal. It does not claim concurrent mutation of a Context. |
| Concurrent decoder snapshot reuse | Original 16-thread test passes. Native per-Context reuse and isolation follow the same owner-thread adaptation, without a process-static type singleton. |
| Small/large typed decoding without shared values | Original 1/100/10,000 cases and the existing model/shape/batch responsibilities remain part of the pending whole-file review; raw row sharing alone is not typed-model acceptance. |
| Shape detachment and Checker narrowing | Existing generated hydration and sparse Checker controls preserve value-only layout reuse, changed-shape detachment and narrowing without widening a previous view. |

The [actual three-name witness](parity/shared_state_three_name_alias_gap_final_5_0_6.json.gz)
uses the original `ThreeNameRow` vocabulary and the real generated lookup helper:

| Input column | Canonical field marked loaded | Actual value decoded |
| --- | --- | --- |
| `business_name` | true | true |
| `display_name` | true | **false** |
| `legacy_url` | true | true |

The unmodified Rust test requires the actual value for every known alias.
FieldLayout already recognizes the member spelling, but the generated scalar
lookup receives only canonical and physical names. At this initial checkpoint the generator emitted
that two-name call. Its knowledge must be extended; normalization or weakening
the loaded check would conceal the missing payload.

The initial required follow-up was runtime first: allow lookup to use the existing validated
FieldLayout aliases while retaining canonical/physical precedence and forward-FK
versus object behavior; qualify original known/unknown and conflicting-alias
results; then emit the thin model adapter and reproduce generated artifacts.
Preserve current function calls through a compatible default/overload. G15 LS10
and whole G15 qualification are reopened until this path has evidence. The
previous qualified entry and its scoped gates remain preserved in the audit.

## Lifetime verification

The [initial sanitizer failure](parity/shared_state_source_sanitize_initial_5_0_6.json.gz)
is retained. The new fixture incorrectly kept an owning factory Result containing
a User-allocated PMR map alive after deleting that UserContext. Its destructor
read the freed map. The repair scopes this temporary before deletion and keeps
every original/sibling/Global retention assertion. Construction values must be
destroyed before their allocator; registered Context payloads and ordinary
non-owning handles have different responsibilities.

Generator/SDK files are unchanged by the dictionary and query-selection repairs.
The three-name follow-up must not be claimed implemented merely because these
sharing fixes or the current kernel checks pass. Full G13/P7 qualification and
the unrelated compatibility decisions remain open.

## Large-batch lookup and terminal qualification

The [second sanitizer result](parity/shared_state_source_sanitize_timeout_5_0_6.json.gz)
retains the original 180-second timeout after the fixture-lifetime repair. The
complete 10,000-row case repeatedly resolved freshly registered objects through
the arena's address-to-slot scan. That made this workload quadratic.

`Arena::slot_for_address` now checks the latest registered record first, with
the same range and live-state validation, before its unchanged forward scan.
Existing thread/closed guards still run first. It adds no table, cache, owner or
allocation. Interior addresses, explicit destruction, foreign owners/threads and
actual Context deletion remain covered by `context_reference_contracts`.
The complete matrix still runs 1/100/10,000 rows with the same 180-second timeout.
The final sanitizer run passes the large contract in 10.77 seconds; this is a
single recorded run, not a performance guarantee.

The [terminal gate record](parity/shared_state_source_gates_5_0_6.json.gz) retains
the complete logs and current successful no-work builds:

| Gate | Result and scope |
| --- | --- |
| Original integration tests | 33/33 unchanged Rust tests: dynamic fields 11, readonly metadata 3, shared load state 19. Original concurrent tests retain their 16-thread barriers. |
| Native kernel selection | 16/16, including the complete new shared-state contract, 57 allocation cases, CompactRow/source comparisons and generated API regression. |
| Imported kernel selection | 15/15 canonical imported contracts/comparisons; the native-only generated API target is not invented for modules. |
| Targeted ASan/UBSan | 6/6, including actual deletion, private/sibling sharing and 10,000-row enhancement; leak scanning disabled. |
| Complete G16 integration selection | Native 46/46 and imported 45/45, recorded separately after the lookup repair. |
| Actual G16 source corpora | Seven unchanged families, 918 observations per frontend. Original dependency identities/checksums and complete outputs remain retained. |
| Adjacent indexed-state source | 727 observations per frontend: layouts 44, snapshots 427, carriers 176 and initialized-cache 80, with the separately checked existing ownership adaptation. |
| Structural and formatting checks | 846 canonical imported bodies, current source provenance/paths, frozen references and pinned format/check of 1,658 handwritten C++ files. These checks do not establish semantic parity. |

The [source replay](parity/shared_state_source_replay_5_0_6.json.gz) and
[implementation review](parity/shared_state_source_implementation_review_5_0_6.json.gz)
retain exact original tests, current production deltas and the scoped C++
ownership/handle/owner-thread adaptations. Its draft-document hash is preserved
with the complete earlier document in the terminal gate record; the
[final acceptance receipt](parity/shared_state_source_acceptance_5_0_6.json.gz)
binds this finalized document and the updated authoritative dispositions.

Only `dynamic_field_state.rs` and `dynamic_property_metadata.rs` are accepted
by this checkpoint. Source status at this initial checkpoint advanced to **18 accepted / 106 pending**.
`shared_load_state.rs` remains pending and G15 remains partial for LS10's
three-name typed decoding. The native registry is now 724; selected results do
not relabel the historical 718/721 full-suite result or qualify current P7.
No generator/SDK files changed in this checkpoint. Java/compiler and complete
artifact gates are not rerun or claimed by it. No MIPS or physical-router run
is added.

## Reproduction

After activating the locked tools, build the native and canonical imported
`shared_state_source_tests` targets before executing their outputs:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_shared_state_source_tests
cmake --build --preset modules_native --target teaql_module_shared_state_source_tests
python3 scripts/verify_shared_state_sources_5_0_6.py \
    --receipt /tmp/shared_state_source_replay_new.json.gz
python3 scripts/verify_load_state_source_5_0_6.py \
    --implementation-review docs/parity/shared_state_source_implementation_review_5_0_6.json.gz \
    --receipt /tmp/shared_state_adjacent_source_new.json.gz
```

For the last verifier, pass the current implementation review above explicitly
with `--implementation-review`, select the native or imported executable
directory, and use a fresh output receipt. Its default review represents an
earlier implementation checkpoint and must not silently accept changed bodies.
The recorded source replay commands and terminal commands are retained verbatim
in the linked receipts.
