# Generated entity JSON namespaces and sparse metadata

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/dynamic_field_query.rs`.

This continuation maps two original tests:

- `serialization_preserves_fixed_derived_and_persistent_names_without_promoting_not_loaded`
- `sparse_serialization_omits_absent_fixed_fields_and_internal_mutation_metadata`

`generated_entity_serialization` in
`tests/generator/generated_readonly_property_tests.cpp` uses actual generated
Product with the same id/version/name scalar surface as the original
macro-derived School. Production serializers and generated artifacts are
unchanged. Context allocation and snapshot ownership remain explicit.

## Source assertions

The complete case decodes a native name, derived `_name` and derived `_nil`.
It installs selected persistent fields through the generated dynamic carrier
and the runtime snapshot operation. Serialization must preserve all three
namespaces independently:

| JSON field | Expected value |
| --- | --- |
| `name` | `"native"` |
| `_name` | `"derived"` |
| `#name` | `"persistent"` |
| `#zero` | `0` |
| `#false` | `false` |
| `#empty` | `""` |
| `_nil`, `#nil` | `null` |

The declared but unselected `#missing` property is absent. Neither `__load_state`
nor `storage_binding` appears in the result.

The sparse case omits the native name while retaining derived `_name`.
After adding a private mutation comment and marking the entity as new,
`into_values` must retain `_comment` and `_is_new`: this remains the governed
persistence carrier. `into_json` must omit the unloaded native name and all
five internal fields `_comment`, `_is_new`, `_is_deleted`, `_dirty_fields` and
`_original_values`. The derived value remains present, and the private comment
text does not appear anywhere in this presentation result.

This establishes the single-entity projection boundary. It does not by itself
establish nested-child filtering, cycle handling, sibling-edge isolation or
selected-null versus empty-list relation behavior.

## Verification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_entity_serialization_source_5_0_6.py \
  --record docs/parity/generated_entity_serialization_installed_5_0_6.json.gz
```

The controller requires a fresh receipt. It reproduces the example SDK and runs
the two unchanged original tests with exact filters in the byte-checked runtime
view and locked dependency identities. Disabled external-exporter development
dependencies are omitted. Native and imported-module checks run the actual
generated counterpart, retaining JSON injection rejection, explicit Global-copy
lifecycle and all six readonly zero-allocation measurements with positive
controls.

Exact CTest selection, canonical module consumers, binary hashes, no-work
rebuilds and unchanged input/reference guards are checked separately.
The complete dynamic-query source and P0–P7 remain pending.

## Terminal result

The [installed receipt](parity/generated_entity_serialization_installed_5_0_6.json.gz)
passes **original2, generated cases2, native1/module1**. SDK reproduction,
canonical module consumers, no-work builds and unchanged input/reference
guards pass. Existing JSON injection, explicit Global-copy and six readonly
allocation measurement cases pass in both frontends. Formatting and format
checks cover 1,726 handwritten C++ files.

Sixteen original test identities have focused generated receipts. Seven remain:
plain-carrier defaults, graph read-metadata decoding, borrowed forward/reverse
graph serialization, sibling-edge isolation, nested metadata filtering, and
selected-null/empty/unloaded relation serialization. Source totals stay
**92 accepted / 30 pending / 2 excluded_external**; capabilities stay
**11 qualified / 6 partial**. No fresh sanitizer, MIPS or physical-device
result is claimed.
