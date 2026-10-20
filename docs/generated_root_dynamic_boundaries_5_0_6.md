# Generated root dynamic-query failure boundaries

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/dynamic_field_query.rs`.

This continuation maps three original tests to seven generated Product cases:

- `malformed_provider_batches_fail_closed_without_returning_partial_rows_or_values_in_errors`
- `no_selection_needs_no_provider_and_streaming_cannot_silently_drop_a_selection`
- `dynamic_stream_rejects_before_cursor_even_when_a_provider_is_configured`

These root-entry assertions are independent of the earlier child-enhancement
failure cases. The whole 23-test integration source remains pending.

## Fixture and assertions

`generated_root_dynamic_boundary` in
`teaql_runtime/tests/dynamic_relation_tests.cpp` uses the actual generated
`cpp_examples::Product`, independent entity data service and SQLite executor.
The original uses macro-derived `School` and a fixed-row executor. The C++
fixture explicitly selects id/version/name and orders three rows by id; model
names and provider implementation are fixture adaptations. Generated headers
and production runtime bodies are unchanged.

Five root queries select dynamic Text field `note`. Each invokes the dynamic
provider once and returns a failed result, with no partial typed list:

| Provider fault | Required error code |
| --- | --- |
| Missing requested owner | `DYNAMIC_FIELD_BATCH_OMITTED_OWNER` |
| Additional unrequested owner | `DYNAMIC_FIELD_UNREQUESTED_OWNER` |
| Unselected `extra` payload | `DYNAMIC_FIELD_UNSELECTED_VALUE` |
| Boolean supplied for Text | `DYNAMIC_FIELD_TYPE_MISMATCH` |
| Definitions for a different entity | `DYNAMIC_FIELD_OWNER_MISMATCH` |

All five diagnostics must omit `PRIVATE-CANARY`, including the case containing
that private payload. The provider also checks the trusted Context marker,
owner, three requested IDs and query intent.

The sixth case installs no provider. An unselected query returns three generated
rows without dynamic payloads. Selecting `note` fails with
`DYNAMIC_FIELD_PROVIDER_MISSING`. A subsequent selected stream fails without
another SQLite statement or a provider call.

The seventh case installs a valid provider and immediately opens a selected
stream. It fails before any SQL statement or provider call. Statement counting
starts after table setup. This is rejection of unsupported dynamic streaming,
not evidence that such streaming is implemented.

## Verification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_root_dynamic_source_5_0_6.py \
  --record docs/parity/generated_root_dynamic_installed_5_0_6.json.gz
```

The controller requires a fresh receipt. It reproduces the example SDK, runs
the three unchanged original tests with exact filters in the byte-checked
runtime view, and runs the C++ executable through native and imported-module
profiles. Locked dependency identities are preserved; disabled external-exporter
development dependencies are omitted. It requires all seven new boundary cases
and the retained child-selection, readonly-stream and dynamic-ledger markers.
Exact CTest selection, canonical module consumers, binary hashes, no-work
rebuilds and unchanged input/reference guards are checked separately.

These cases do not establish complete dynamic-query source acceptance,
duplicate-owner retention semantics, allocation cost, successful dynamic
persistence or P0–P7 completion.

## Remaining source reconciliation

The four focused generated continuations account for ten original test
identities: child selection (two), readonly list/cache/stream (three), dynamic
ledger (two), and the root boundaries here (three). This is a scope inventory,
not an additional test run or whole-file acceptance. Thirteen original tests
still need explicit complete-source reconciliation:

| Source area | Original tests still requiring reconciliation |
| --- | --- |
| Carrier defaults and readonly reads | `entity_without_derived_carrier_keeps_nullable_defaults_without_claiming_presence`; `readonly_property_schema_survives_typed_decode_without_presence_or_fixed_slots`; `derived_property_reads_borrow_values_preserve_null_absence_and_allocate_nothing` |
| Graph decoding and borrowed serialization | `graph_decoders_retain_dynamic_read_metadata_without_promoting_it_to_columns`; `borrowed_flat_graph_json_is_cycle_bounded_and_never_clones_entities`; `borrowed_reverse_graph_json_distinguishes_empty_and_unselected_lists`; `flat_json_does_not_leak_sibling_detail_into_an_unselected_or_filtered_edge` |
| Entity and relation serialization | `serialization_preserves_fixed_derived_and_persistent_names_without_promoting_not_loaded`; `sparse_serialization_omits_absent_fixed_fields_and_internal_mutation_metadata`; `nested_entity_json_keeps_extensions_and_does_not_serialize_mutation_internals`; `relation_json_distinguishes_selected_null_empty_and_not_loaded` |
| Typed hydration and duplicate retention | `typed_query_batches_once_and_preserves_private_null_missing_and_ledgers`; `basic_typed_query_duplicates_and_result_lifetime_keep_the_same_contract` |

Existing lower-level tests can contribute evidence after their assertions and
actual callers are mapped. Their presence alone does not close these rows.
Rust's implicit retained clone after Context destruction requires an explicit
destination-owned copy in C++; the approved two-lifetime design must not be
changed to reproduce shared ownership implicitly. Allocation and clone-count
assertions need measured evidence rather than value-only checks.

## Terminal result

The [installed receipt](parity/generated_root_dynamic_installed_5_0_6.json.gz)
passes **original3, generated cases7, native1/module1**. The retained child,
readonly-stream and ledger cases pass in both profiles. SDK reproduction,
canonical module consumers, no-work builds and unchanged input/reference
guards pass. Formatting and format checks cover 1,725 handwritten C++ files.
No fresh sanitizer, MIPS or physical-device result is claimed.

The source stays pending, with **92 accepted / 30 pending / 2 excluded_external**
overall. Capabilities remain **11 qualified / 6 partial**, and P0–P7 is incomplete.
