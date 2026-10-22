# Generated nullable JSON graph contracts (Rust 5.0.6)

This checkpoint uses the unchanged `typed_member_aliases` generated Rust and
C++ domains: `Holder.record_id` is nullable, `Holder.record` is a forward
relationship, and `Record.holder_list` is its reverse list. Model metadata,
registrations and typed hydration come from the generated SDKs.

## Twenty original observations

The retained corpus is
`tests/parity/generated_json_nullable_graph_5_0_6_cases.json.gz`. Its driver
executes the original runtime with the original generated SDK, not a translated
implementation of the JSON parser. The C++ probe executes the corresponding
public UserContext decoder and generated types.

| Inputs | Observations |
| --- | --- |
| NULL foreign key and NULL relation, foreign key alone, relation alone, both omitted, NULL relation alone, NULL foreign key alone | Exact JSON, inferred foreign key, separate key/relation loaded flags, option state and clean ledger. A populated typed option and identity lookup borrow the same entity address. |
| Conflicting explicit foreign key, non-null foreign key with NULL relation, array-valued forward relation, missing nested identity, NULL foreign key with non-null relation | Exact original errors; invalid states are not silently changed into missing data. |
| Empty reverse list and omitted reverse list | Loaded flag, zero versus absent list count, and exact roundtrip JSON. |
| NULL reverse list, NULL member, missing root identity, conflicting member key, wrong member scalar type | Exact original rejection and validation boundaries. |
| Two nested views of one parent with contradictory scalar values | Conflict rejection without either private input value in the error. |
| NULL scalar in a repeated parent view reached through a reverse member | Exact nested JSON and inferred keys without widening or dropping the explicit projection. |

This covers nine successful and eleven rejected inputs. All errors are checked
for the private-value sentinel. Output comparison preserves JSON scalar kinds;
no field-name or error normalization is applied.

## Source-test mapping and limits

This adds direct generated coverage for
`graph_json_preserves_empty_omitted_null_and_fk_identity`, the malformed-edge
portion of `graph_input_rejects_conflicting_keys_malformed_edges_and_missing_decoders`,
and `incompatible_forward_target_views_do_not_silently_choose_one_payload`.
The missing-decoder and incompatible-target controls remain in the separately
qualified generated input-boundary corpus.

It does not by itself close the complete fourteen-test source. The original
GraphChild fixture's nullable child scalar and full assertion mapping must still
be consolidated with the earlier generated ownership/projection tests and
source-level shape-cache evidence. The approved JSON payload compatibility
exception is independent of these graph contracts.

## Verification

The [complete receipt](parity/generated_json_nullable_graph_installed_5_0_6.json.gz)
records 20 exact original observations in each of native, canonical modules and
focused ASan/UBSan. Native/sanitizer retained-corpus CTests pass. Original and C++
generated file hashes, canonical consumers, no-work rebuilds and captured input
and reference guards pass. Format/check covers 1776 handwritten C++ files.

The [negative control](parity/generated_json_nullable_graph_negative_5_0_6.json.gz)
feeds deliberately incorrect observations and verifies that the comparator
returns exit 1. Leak scanning is disabled. No runtime, generator or generated
SDK implementation changed; no MIPS or physical-device run was needed.
