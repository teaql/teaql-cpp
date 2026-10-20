# G15 shared load-state requirements: Rust 5.0.6

Status: G15 partial after the current complete-source audit reopens LS10. The
[native three-name witness](parity/shared_state_three_name_alias_gap_final_5_0_6.json.gz)
marks a member alias loaded but fails to decode its actual value; canonical and
physical aliases succeed. The later
[runtime-first repair](typed_member_alias_runtime_5_0_6.md) qualifies an opt-in
metadata-aware lookup through 130 actual-source comparisons per frontend and
native5/module4/sanitizer2. The
[generated continuation](generated_member_aliases_5_0_6.md) now passes that
binding and reproduces all 78 SDKs. Its native44/imported32/sanitizer3, source260
per profile, Java80 and complete193 compiler gates pass. The later [typed-batch/source review](generated_load_state_bulk_5_0_6.md)
accepts all nineteen original test responsibilities with 12 complete batches,
40,404 row values per profile, cold metadata/owner-thread and actual-deletion
controls. Source status is 19 accepted/105 pending. Five preserved direct
quoted-FK differences remain outside those unquoted tests and keep G15 partial. The earlier fourteen-row
[acceptance review](shared_load_state_acceptance_5_0_6.md) and its terminal gates
remain historical evidence for their exercised paths.
This review preserves the complete indexed-state responsibility; it does not
replace it with the original narrow School fixture or a method inventory.

References are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.
The [source review record](parity/g15_current_responsibility_review_5_0_6.json.gz)
binds the locked source bytes, implementation hashes, existing evidence, previous
ledger questions and current test registrations. That record is a review artifact,
not a new semantic comparison or proof that all phase gates have passed.

## Source responsibilities and acceptance obligations

| ID | Complete responsibility | Implementation and existing evidence |
| --- | --- | --- |
| LS01 | Number normalized fixed fields in parent-first order. Retain inherited identity/version once; reject cyclic/unresolved inheritance, blank/duplicate/incompatible fields; exclude `#` and `_`; hash the authentic type/order/type/SQL metadata revision input. | Frozen `ObjectDescriptor` and `PredefinedFieldIndex`; C++-local `CppFieldLayoutMetadata`; [78 direct actual metadata comparisons](shared_load_state_metadata_5_0_6.md), [generated indexed metadata](generated_indexed_models_5_0_6.md), [inheritance acceptance](inheritance_acceptance_5_0_6.md). |
| LS02 | Validate runtime geometry, mandatory id/version, complete canonical/member/column mappings, alias ambiguity and typed-member agreement. Keep relation names separate and preserve layout identity in state equality. | `FieldLayout::from_generated`, [44 original construction cases](field_layout_5_0_6.md), distinct equal-metadata layouts in the initialized-cache matrix. |
| LS03 | Retain exact fixed bits, lazy overflow and named relation/durable availability. FullyLoaded covers fixed slots; unknown/derived projections do not invent slots or loaded data. | `LoadedSnapshot::fully_loaded/projection/is_loaded`; [snapshot contracts](loaded_snapshot_5_0_6.md), fixed boundaries through 141 fields and initialized 2/65/130-field layouts. |
| LS04 | Preserve no-op identity and immutable previous views. Changing a fixed bit, overflow slot or named selection detaches only the necessary geometry. Unknown mutable fields fail with the source message. | `with_loaded`, component nodes and `SnapshotPool`; 1,920 auxiliary-sharing observations and native allocation/pointer assertions in the snapshot contracts. |
| LS05 | Intern geometry rather than values. Check the matching bucket before miss cleanup, prune expired hints, cap admission at 512 per layout and return valid states after saturation. | Explicit Context-owned `SnapshotPool`; [80 exact initialized-cache cases](initialized_snapshot_cache_5_0_6.md), including 511/512/513/520 states, released entries and repeated projections. |
| LS06 | Preserve all six legacy/indexed LoadState variants, conversion, identity mismatch, loaded/unloaded transitions and runtime-only Indexed serialization. | `eval::LoadState`; snapshot source corpus and native contracts. Legacy conversion must not treat a client mask as trusted runtime state. |
| LS07 | Readonly schema describes types without declaring actual presence. Schema attach/replace/clear preserves availability; equal schema reuses state. Retain logical equality and the locked cached u64 schema hash where it appears in diagnostics/cache keys. | `DynamicPropertyDefinitions`, `LoadedSnapshot::with_property_schema`; [schema hash](readonly_schema_hash_5_0_6.md), [readonly generated access](generated_readonly_properties_5_0_6.md), initialized schema replacement and row diagnostics. |
| LS08 | Replacing durable selections validates the canonical owner, removes only old `#` markers and preserves fixed/relation/readonly state. Value-only edits reuse availability; NULL is selected, absent differs from NULL, Delete removes selection. | `with_dynamic_fields` and `GeneratedDynamicFields`; [core definitions/carriers](dynamic_fields_5_0_6.md), [generated carrier comparison](generated_dynamic_fields_5_0_6.md), [governed durable Save](dynamic_typed_save_audit_5_0_6.md). |
| LS09 | Share ordered result geometry without sharing mutable row values. Cache primary and secondary typed snapshots by actual layout identity. Shape/schema changes publish a new layout; equal-schema reuse and insertion/removal retain relation metadata. | `ColumnLayout`/`CompactRow`; snapshot row comparisons, initialized-cache matrix, [row diagnostics](row_diagnostics_5_0_6.md), public compact-row source/API tests. |
| LS10 | Generated construction begins Indexed/NotLoaded. Setters mark only their actual field, including zero/false/empty/NULL. Typed hydration consumes row availability; manual/unindexed trait defaults remain separate. | Frozen entity template and `teaql-macros::derive_impl`; generated Model delegates to shared runtime. [G01 acceptance](entity_helper_acceptance_5_0_6.md), constructor, scalar/optional, inherited and source/result model matrices. |
| LS11 | Checker reconstruction restores loaded fields separately from dirty intent. Copy original mutation values only when dirty reconstruction requests it; merge back actual changes and retain sparse required/type-failure behavior. | `TypedEntityChecker`, generated loaded/dirty hooks; [keyed originals and sparse reconstruction](keyed_originals_checker_5_0_6.md), [G02/G03 acceptance](checker_behavior_acceptance_5_0_6.md). A materialized default must not widen the database update. |
| LS12 | Keep original rows immutable and distinct from current values, availability and shared mutation ledger. Promote an inline row on first same-domain clone; keyed lookup, receiving precedence, loaded-view rebinding and committed clearing retain their source semantics. | `EntityRuntimeState::copy_loaded_view_from`, ledger originals and `EntityRuntimeAccess::original_values_for`; [original-row contracts](original_snapshot_field_helpers_5_0_6.md), keyed originals and [current runtime-model replay](state_baselines_5_0_6.md). |
| LS13 | Preserve availability and readonly/durable sidecars through the actual typed query/graph/cache/facet/aggregate boundaries. Readonly declarations and cached graph membership must not manufacture loaded scalar/relation values or persistence authority. | [Loaded graph reconstruction](loaded_row_graph_5_0_6.md), [dynamic JSON composition acceptance](dynamic_json_acceptance_5_0_6.md), [source/result decoding](execution_source_result_models_5_0_6.md), typed aggregate and inherited graph/source matrices. Full storage, trace and aggregate responsibilities retain their G16/G17/G08 owners. |
| LS14 | Enforce the approved C++ ownership adaptation at each boundary: scoped metadata/snapshot/value ownership, explicit destination copies, independent retained values, checked expired identities and cleanup after actual Context deletion. Exercise native and imported APIs without copying production algorithms into observers. | Existing default-PMR-disabled kernel/generated/graph controls; [Context lifetime](context_lifetime.md), initialized-cache ownership observation, typed aggregate and source/result lifecycle tests. Refresh the combined gate rather than adding historical totals. |

The runtime source boundary includes `load_state.rs`, `eval.rs`, the Entity
defaults in `entity.rs`, CompactRowLayout/CompactRow in `query.rs`, readonly
definitions in `dynamic_properties.rs`, the indexed branches in
`teaql-macros/src/derive_impl.rs`, original-row/ledger methods in
`entity_runtime.rs`, and typed reconstruction in `checker.rs`. Generator
numbering alone does not describe the macro-generated runtime behavior.

## Reconciled historical questions

The ledger's previous G15 questions mixed pending work with historical
implementation defects. The review record preserves them verbatim before this
reconciliation:

- The old 169 CompactRow formatting differences were repaired by the 1,081-case
  diagnostic replay. The later 69-case schema-hash matrix qualifies nonempty
  uninitialized diagnostics; the 80-case initialized-cache matrix records actual
  initialized representations and semantic cache behavior separately.
- Generated changed/eval helpers, sparse typed Checker reconstruction and
  inherited model adapters now have their G01/G02/G03/G04 acceptance evidence.
  Their original narrow checkpoint documents remain historical.
- Durable/readonly carrier installation and real SQLite query/Save paths now
  exist. Their scoped matrices must be mapped to LS08/LS13; their existence
  alone does not qualify every G16 storage or privacy responsibility.
- Original-row dictionaries and governed runtime behavior have later source
  replays. Private helper and public generated evidence remain distinct.

Rust's Arc/Weak ownership, address-bearing cache diagnostics and concurrent
Mutex/OnceLock access are not represented as literal C++ equivalence. C++ owns
published snapshots in its Context until explicit destruction/close; operations
use the Context owner thread. Secondary cache keys use checked scoped identities.
These are the existing ownership/owner-thread adaptations, not a new lifetime
domain or an implicit shared owner. Raw diagnostics remain available; randomized
or address-bearing text is not rewritten to claim byte equality.

## Newly identified module integration gap

The existing module gate executes the initialized-cache comparator and generated
carrier lifecycle checks. It did not register the complete canonical bodies of:

- `teaql_core/tests/field_layout_tests.cpp`
- `teaql_core/tests/loaded_snapshot_tests.cpp`
- `teaql_core/tests/dynamic_carrier_tests.cpp`
- `teaql_core/tests/compact_row_api_tests.cpp`

The module contract registry and `modules_native` build preset now include all
four. `prepare_contracts.py` removes header includes and imports the existing
production modules while preserving the test bodies. No second test algorithm,
runtime implementation or generated domain artifact is introduced.
The terminal build succeeds, native passes 4/4, imported consumers pass 4/4 and
the canonical-body verifier checks 798 bodies. The
[integration record](parity/g15_kernel_module_integration_5_0_6.json.gz) retains
the complete logs and four canonical source hashes. This closes the identified
module-registration gap; it does not qualify the whole LS01-LS14 matrix.

Five additional canonical imported observers now replay six complete existing
original-source corpora. Original-row cases use the existing multi-mode snapshot
observer. Both frontends pass all six registrations:

| Corpus | Inputs per frontend |
| --- | ---: |
| FieldLayout | 44 |
| LoadedSnapshot, schema and named relation availability | 427 |
| Dynamic carriers | 176 |
| Immutable original snapshots | 56 |
| Readonly schema hash/diagnostics | 69 |
| Public CompactRow API | 401 |

The [input identity record](parity/g15_kernel_comparison_inputs_5_0_6.json.gz)
binds all 1,173 inputs, drivers and the exact comparison engine. The
[terminal comparison record](parity/g15_kernel_module_comparisons_5_0_6.json.gz)
retains configure/build completion, native 6/6, imported 6/6 and 803 canonical
bodies. These are current replays of retained source observations, not new live
Rust execution. Neither corpora nor observer function bodies were changed.

For reproduction, build the four `teaql_module_*_tests` and five
`teaql_module_*_probe` targets named in the registry with `modules_native`, then
run the registered tests:

```sh
source scripts/toolchains/activate.sh
cmake --preset modules_native
cmake --build --preset modules_native --target \
  teaql_module_field_layout_tests teaql_module_loaded_snapshot_tests \
  teaql_module_dynamic_carrier_tests teaql_module_compact_row_api_tests \
  teaql_module_field_layout_probe teaql_module_loaded_snapshot_probe \
  teaql_module_dynamic_carrier_probe teaql_module_readonly_schema_hash_probe \
  teaql_module_compact_row_api_probe
ctest --preset modules_native --output-on-failure \
  -R '^module_(field_layout|loaded_snapshot|dynamic_carrier|compact_row_api)_contracts$|^module_(field_layout|loaded_snapshot|dynamic_carrier|original_snapshot|readonly_schema_hash_5_0_6|compact_row_api)_rust_parity$'
python3 scripts/verify_module_consumers.py --build-dir build_modules_native
```

## Acceptance boundary

The final review maps every LS01-LS14 requirement to its exact source, actual
observations, supported model/configuration disposition and lifecycle controls.
Native41/module29, the full 54-positive/139-negative compiler gate, metadata78,
strict77-SDK/3,468-file preservation and full1,649-file formatter/check pass.
Historical sanitizer records retain their original scope; no new lifetime
implementation is introduced by this acceptance work.

G15 qualification does not close P1/P7, G08, G12-G14, G16 or G17.
The pending G12/G14 wire-profile compatibility decision is independent of these
loaded-state obligations.
