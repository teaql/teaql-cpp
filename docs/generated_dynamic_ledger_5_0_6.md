# Generated dynamic-field ledger composition and mutation

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/dynamic_field_query.rs`.

This continuation checks two original tests:

- `composing_a_ledger_never_replaces_child_dynamic_payload_with_parent_values`
- `dynamic_mutation_intent_is_explicit_row_local_and_cannot_be_silently_saved`

The complete 23-test source remains pending. These tests qualify ledger
composition and rejection of unbound dynamic writes; they do not establish
successful provider-backed dynamic persistence or complete graph-save parity.

## Counterpart

`generated_dynamic_ledger_case` in
`teaql_runtime/tests/dynamic_relation_tests.cpp` uses the retained generated
`cpp_examples::Product` model and a real SQLite executor. The Rust fixture uses
macro-derived `School` and a fixed-row executor. Product supplies the same
id/version/name values for these assertions; its native field indexes and
relation metadata are preserved when constructing an incompatible revision.
The query explicitly orders three rows by id. No generated artifact is patched.

| Original contract | Generated C++ observations |
| --- | --- |
| Replace child runtime state with the parent state | `attachRuntimeStateRecursive` preserves child NULL and NotLoaded payloads, snapshot identities and original IDs 2 and 3. The parent's text value remains unchanged. |
| Reject incompatible layout revision | Both `updateDynamicField` and `deleteDynamicField` report `DYNAMIC_FIELD_LAYOUT_MISMATCH`; neither payload nor mutation intent changes. |
| Record explicit row-local NULL | Only `#note` is dirty. The ledger contains dynamic Set(NULL), no scalar changes, and no sibling mutation. Availability shares the compatible sibling snapshot. |
| Reject wrong value type | A Boolean supplied to a Text field fails without changing the complete prior change set. |
| Distinguish Delete from NULL | Delete produces NotLoaded and shares the matching unloaded snapshot while retaining explicit Delete intent. |
| Reject unbound graph/save operations | Graph construction and both audited save entries report `DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED`. A SQLite statement counter verifies no additional SQL; Delete remains retryable. |
| Compose and isolate ledger frames | Explicit adoption retains the complete dynamic change map. Popping an inner Set(NULL) leaves outer Delete intact. Clearing the original committed state does not clear the composed ledger. |

All values and state borrows remain within the live UserContext. Composition
does not imply ownership beyond Context destruction. The generated adapter
delegates to production runtime state and save mechanics; the test supplies
fixtures and assertions rather than reimplementing those mechanics.

## Verification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_dynamic_ledger_source_5_0_6.py \
  --record docs/parity/generated_dynamic_ledger_installed_5_0_6.json.gz
```

The controller requires a fresh receipt. It reproduces the example SDK and runs
the two unchanged original tests with exact filters in the byte-checked runtime
view and locked dependency identities. Disabled external-exporter development
dependencies are omitted. Native and imported-module checks run the complete
dynamic relation executable, requiring the two new ledger cases, three readonly
schema cases and seven child-selection cases. Canonical module consumers, exact
CTest selection, binary hashes, no-work builds and unchanged inputs/references
are checked separately.

No production implementation changes are introduced by this continuation.
Whole-source acceptance and P0–P7 completion require the remaining contracts.

## Terminal result

The [installed receipt](parity/generated_dynamic_ledger_installed_5_0_6.json.gz)
passes **original2, generated cases2, native1/module1**, retaining the three
readonly-schema and seven child-selection cases. SDK reproduction, canonical
module consumers, no-work rebuilds and unchanged input/reference guards pass.
Source totals remain **92 accepted / 30 pending / 2 excluded_external**;
capabilities remain **11 qualified / 6 partial**. No fresh sanitizer, MIPS or
physical-device result is claimed.
