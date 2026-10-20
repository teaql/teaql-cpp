# Generated borrowed graph and nested-relation serialization

Reference runtime: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Reference generator: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Original tests: `teaql-runtime/tests/dynamic_field_query.rs`.

This continuation exercises the five remaining original graph tests:

- `borrowed_flat_graph_json_is_cycle_bounded_and_never_clones_entities`
- `borrowed_reverse_graph_json_distinguishes_empty_and_unselected_lists`
- `flat_json_does_not_leak_sibling_detail_into_an_unselected_or_filtered_edge`
- `nested_entity_json_keeps_extensions_and_does_not_serialize_mutation_internals`
- `relation_json_distinguishes_selected_null_empty_and_not_loaded`

## Observed defect and runtime-first repair

The generated serializer supplied only a relation name and loaded flag to
`GeneratedEntityJson::reverse_many` and `reverse_one`. Those paths read frozen
graph edges. They did not receive the entity's existing local relation storage,
so an owned, populated reverse list disappeared from nested JSON.

The [failure receipt](parity/generated_graph_embedded_relation_failure_5_0_6.json.gz)
retains the fixture, runtime header and actual failed output. The cycle,
reverse-graph and sibling-isolation cases passed first; nested JSON omitted
`child_rows` and `boxed_children`.

The shared runtime now accepts optional embedded relation views through new
overloads. A loaded embedded list is serialized directly, including an empty
list. An embedded object is borrowed directly. Otherwise the existing frozen
graph lookup remains available. The new object overload retains selected NULL
when neither embedded nor frozen data exists. Unloaded relations stay omitted.
The existing two-argument interfaces remain unchanged for older adapters and
container-declared graph-only reverse relations.

One private runtime helper serializes list members with the existing traversal.
No ownership, SQL execution, graph lookup or JSON filtering algorithm is emitted
into generated models. The producer now supplies its existing private member
pointer to the runtime overload. Java assertions cover both one and many adapters.

The [runtime-first receipt](parity/generated_graph_runtime_first_5_0_6.json.gz)
records native embedded-object/list, NULL, empty and unloaded checks, followed by
the unchanged cycle/reverse/sibling scenarios, before changing the producer.
Generation uses the independent `cpp_support` workspace. Generated headers are
reproduced, never patched.

## Dedicated SDK and fixture adaptations

The model and complete Rust/C++ SDKs live in
`tests/generator/parity/graph_serialization_contracts`. The reproduction script
builds the real producers, records model and implementation hashes and formats
C++ output with the pinned formatter. Earlier SDKs remain under
`children_name_collision` and `before_embedded_projection`.

The model uses a self-referencing `JsonGraphRow` for cyclic and reverse graph
tests, plus `SerializationParent` and `SerializationMember` for owned nested
relations. Scalar id/version/name and dynamic extension values match the original.

The following adaptations are explicit:

- The original macro fixture allows a parent and child model to reference one
  another. The locked KSML evaluator rejects that multi-entity model cycle.
  The [initial model failure](parity/graph_serialization_model_failure_5_0_6.json.gz)
  is preserved. Forward children and reverse-list members therefore use distinct
  generated types with the same scalar and extension values. All four nested
  serialization paths remain asserted.
- A reverse relation named `children` collides with the generic
  `selectChildren()` helper. The C++ SDK failed compilation; the locked Rust
  SDK also emits duplicate `select_children` declarations. The
  [collision receipt](parity/graph_serialization_name_collision_5_0_6.json.gz)
  and complete failed SDKs are retained. The runnable fixture maps original
  `children` to declared `child_rows`. This does not resolve or qualify the
  colliding query API. A separate naming-policy decision remains open.
- Generated object/FK lookup gives an explicit NULL relation alias precedence
  over the scalar FK. The macro fixture instead has independently declared
  fields and uses NULL as a selection marker. The counterpart uses
  `CompactRow::mark_relation_loaded` to request an edge without replacing its
  non-NULL FK. The earlier
  [diagnostic fixture failure](parity/generated_graph_serialization_initial_failure_5_0_6.json.gz)
  is preserved; no lookup precedence is changed.
- Generated reverse relations are installed runtime views rather than fields
  reconstructed from arbitrary raw columns. Owned list fixtures therefore
  explicitly construct loaded SmartLists, equivalent to the original test's
  assignment from vectors. They do not change raw reverse-column decoding.
- The original nested fixture groups two relations inside a boxed reverse
  struct. C++ uses its established scoped object/list storage for the four
  declared relation roles; both forward objects and both lists are tested.
- Generated fields are private. `updateName` sets the live scalar used by the
  cycle test; its existing loaded bit remains unchanged. The original writes
  that public scalar directly.

## Assertions

| Case | Required behavior |
| --- | --- |
| Forward cycle | Live root name is used; two frozen peers are expanded; revisiting a node emits only id/version and no further relation or name. Snapshot identity is unchanged and consuming JSON equals borrowed JSON. |
| Reverse graph | Two selected children retain their values. A selected empty list and NULL one-object edge remain distinct. The same graph edges stay absent from unselected views of IDs 1, 2 and 3. Private original values are absent. |
| Sibling isolation | Unselected peers reveal no detail. A selected full view can use its canonical sibling; an explicitly restricted id-only edge does not borrow that sibling's name. A selected NULL FK emits NULL. The restricted selected-edge assertion supplements the original test. |
| Nested objects and lists | Both objects and both list members retain native, derived and persistent values, explicit NULL and omitted dynamic fields. All five internal mutation keys are absent, while literal business `_payload._comment` remains intact. MutationValues still contains child audit intent. |
| Relation presence | All four unselected relations are omitted. Selected objects emit NULL; selected empty lists emit arrays. |

## No-clone measurement

The original cyclic node intentionally lacks Rust `Clone`. Generated C++ models
support explicit allocator-aware copies, so this test measures their actual
use. Test-only linker wrappers count the complete/base allocator-copy
constructors of `EntityRuntimeState`, which every generated entity copy calls.
A real generated-node copy is required to increment the counter before testing.
Borrowed rendering and consuming JSON must then report zero copies.

This instrumentation changes only test linkage. It does not add production
allocation/copy counters or claim that constructing the output JSON allocates
nothing. Fixture cloning outside the measured presentation call is allowed.

## Verification and remaining work

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_graph_serialization_source_5_0_6.py \
  --record docs/parity/generated_graph_serialization_installed_5_0_6.json.gz
```

The controller requires a fresh receipt. It reproduces the dedicated Rust/C++
SDKs and retained main example SDK, preserves successful Java test reporting,
and executes all 23 unchanged original tests with one test thread in the
byte-checked runtime view. Disabled external-exporter development dependencies
are omitted while locked dependency identities are retained.

Native, imported-module and scoped ASan/UBSan profiles run the new graph target
and both previous dynamic-relation/readonly-property targets. Existing mutation,
stream, JSON injection, actual Context deletion and zero-allocation controls
remain required. Leak scanning is disabled by repository policy. Exact CTest
selection, canonical module consumers, binary hashes, no-work rebuilds and
unchanged input/reference guards are checked separately.

The focused graph repair is not the full generator migration gate. Other
retained SDK families still require bounded regeneration with the changed
producer and their compiler/Java/semantic gates. The relation/helper naming
collision remains unresolved. Complete-source review, capability/phase exits
and P0–P7 completion are not inferred from a successful focused receipt.

Terminal focused result: [installed receipt](parity/generated_graph_serialization_installed_5_0_6.json.gz) is complete. All 23 original tests, 74 Java tests, native3/module3/scoped-sanitize3, main and dedicated SDK reproduction, canonical consumers and unchanged-input/no-work checks pass. This result does not promote the complete source or close the wider SDK migration.
