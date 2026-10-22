# Context JSON test-source qualification checklist: Rust 5.0.6

Historical checklist: initially pending. The current accepted conclusion is in
[the complete source review](context_json_tests_complete_5_0_6.md). The checkpoint
notes and counts below are retained as history.

Initial status: pending. The source `teaql-runtime/src/context/json/tests.rs` contains
fourteen tests and three macro-generated model fixtures. Passing the shared JSON
entry and graph implementation reviews does not automatically accept this test
source. Its unchanged original fourteen-test invocation is retained in
`parity/context_json_original_5_0_6.json.gz`.

## New generated-model observations

The new observer pair is
`tests/parity/rust_field_layout/src/bin/generated_json_isolation_5_0_6.rs` and
`tests/parity/generated_json_isolation_probe.cpp`. Both execute the unchanged
retained dynamic_json generated domain. Profile.note supplies a nullable scalar;
Order/Line supply forward/reverse graph views. No generated file is hand-edited.

Four focused cases add observations absent from ordinary JSON value comparisons:

- `batch`: three Profile rows, including equal IDs with NULL/different payloads
  and an omitted field. Observe the full snapshot identity matrix, JSON before
  mutation, loaded flags and dirty fields after changing only the first ledger.
- `owned`: an Order and two Lines with one explicitly loaded parent. Compare the
  address returned through the owned relation option with identity lookup, retain
  child loaded flags, check the sparse parent does not acquire reverse loading,
  and mutate one decoded root while another stays clean.
- `independent`: equal root and child IDs decoded as two array entries share
  immutable snapshot geometry while retaining different child quantities and
  independent clean ledgers.
- `views`: two same-identity forward parents have different scalar/reverse
  projections. Observe shared/nonshared snapshots, empty versus omitted reverse
  lists, clean children, full JSON readback and a detached model copy after the
  original wrapper and local state wrappers leave scope. The UserContext remains
  alive; this does not authorize use after Context closure.

The original outputs are asserted independently in the replay script before
being retained. C++ comparisons preserve every returned field, boolean, NULL,
array and JSON value without normalization. The fixtures deliberately use
existing generated models rather than pretending to reproduce JsonProbe,
GraphParent and GraphChild's exact field/nullability declarations.

Two initial observer compile errors were corrected before execution: the Rust
Line quantity is accessed through its public getter, and C++ ledger mutation
uses the generated model's actual EntityKey rather than an implicit integer
conversion. These were observer API errors, not changes to either runtime.

## Fourteen-test responsibility ledger

| Original test | Current evidence and remaining qualification |
| --- | --- |
| `graph_json_restores_typed_children_without_widening_repeated_identity_views` | New `owned` observes address identity, loading and root-ledger isolation; source fixture's nullable child field and complete named-model assertions still need consolidated mapping. |
| `graph_json_preserves_empty_omitted_null_and_fk_identity` | The [20-case nullable generated graph corpus](generated_json_nullable_graph_5_0_6.md) directly verifies empty/omitted lists, NULL/present/omitted foreign keys and relations, inferred identity, loaded flags and exact roundtrip JSON across native/modules/sanitizer. |
| `graph_input_rejects_conflicting_keys_malformed_edges_and_missing_decoders` | The 20-case nullable generated graph corpus covers conflicting keys and malformed edges; the qualified five-case input-boundary corpus supplies missing-decoder and malformed installed target-descriptor controls, including empty lists. |
| `independently_decoded_root_graphs_with_equal_ids_do_not_share_child_payloads` | New `independent` directly observes shared snapshots and independent child payloads/ledgers in generated models. |
| `incompatible_forward_target_views_do_not_silently_choose_one_payload` | The nullable generated graph corpus now directly repeats a parent identity through two children with contradictory values, checking exact conflict errors and suppression of both private scalar values; earlier three-view controls remain separate. |
| `one_graph_preserves_forward_projection_and_reverse_list_views_of_equal_ids` | New `views` directly observes snapshot identity, local projection selection, empty/omitted reverse views and detached wrapper readback. |
| `two_relations_to_equal_identity_keep_distinct_views_and_refuse_ambiguous_lookup` | The [generated dual-forward fixture](generated_json_ambiguous_views_5_0_6.md) now directly verifies both projections, ambiguous identity lookup and complete JSON readback in native/modules/sanitizer. |
| `inferred_foreign_keys_cannot_conflict_across_views_of_one_identity` | The [generated dual-forward fixture](generated_json_ambiguous_views_5_0_6.md) now verifies repeated link identities with contradictory inferred second-parent keys and exact error equality. |
| `graph_depth_is_bounded_before_hydration` | Existing 7072-case corpus contains 64/65/66 depth boundaries. |
| `overlarge_relation_rejects_before_allocating_or_decoding_child_views` | Eleven graph boundary/view cases include source-shaped oversized invalid children and adjacent valid-budget error precedence. |
| `native_json_keeps_null_omission_falsy_dates_decimal_and_no_mutation` | Scalar and generated presentation matrices cover individual behavior. The exact combined model, dynamic mutation flags and full roundtrip assertion remain to consolidate. |
| `batch_actual_shapes_share_snapshots_and_keep_values_and_ledgers_private` | New `batch` directly observes the snapshot matrix, NULL/omission, equal-ID private payloads and ledger isolation. |
| `aliases_and_unsupported_input_reject_without_exposing_values` | Existing matrices cover unknown fields, aliases, type errors, forbidden state and metadata. Exact nullable/nonnullable source fixture coverage remains to consolidate. |
| `native_batches_construct_geometry_per_shape_not_per_row` | New shape-allocation test covers 1/100/10000 public batch rows and constant owned-name cost; preserve the source's two distinct shapes and post-cache conversion checks in final qualification. |

This checklist identifies work, not an acceptance waiver. Neither the four new
cases nor all previously passing corpora alone promote the complete test source.
Source totals remain 109 accepted / 13 pending / 2 excluded_external; capabilities
remain 11 qualified / 6 partial, and P0-P7 remains incomplete.

## Verification gate

Passed: native, canonical imported-module and focused ASan/UBSan comparison of
all four original generated outputs; native and sanitizer retained-corpus CTest;
original/C++ generated file hashes; no-work builds; canonical bodies; and final
captured-input/reference guards. The complete receipt is
`parity/generated_json_isolation_installed_5_0_6.json.gz`. Leak scanning remains
disabled. The generated domains and runtime function bodies are unchanged in
this checkpoint.
Existing explicit Context-close/deep-copy tests retain their separate receipts.

## Generated input boundary continuation

Five additional generated contracts match the original in native, canonical
modules and focused ASan/UBSan: eleven rejection outcomes, two empty-relation
metadata/decoder checks, and alias batches at 1/100/10000 rows. Both present-field
spellings share snapshot identity while omitted-field rows retain a distinct
shape; loaded flags and JSON remain readable after the decode session returns.
The complete guarded receipt is
`parity/generated_json_boundaries_installed_v2_5_0_6.json.gz`.

The combined generated scalar/read-only roundtrip is separately verified in all
three profiles under the user-approved [JSON payload exception](json_payload_compatibility_exception_5_0_6.md).
Rust's generated LargeText/JSON metadata conflict remains a retained failure,
not an exact match. Complete expected C++ JSON, NULL/omission, falsy values,
Decimal/date/timestamp, readonly dynamic values and clean mutation flags pass.
Native and sanitizer each pass both retained boundary and exception CTests.

Comparator entry points now propagate mismatch exit codes. Deliberately wrong
probe output returns exit 1 for boundary/isolation/ambiguous comparisons; the
existing two generated native CTests still pass. Evidence is
`parity/generated_json_comparison_exit_codes_5_0_6.json.gz`.

The full test source remains pending: finish the exact original graph fixture's
nullable-field and NULL/empty/omitted relationship assertion mapping, and
consolidate source-level layout/cache assertions with these generated tests.
These additional scoped observations do not promote the source by themselves.

## Nullable graph continuation

Twenty generated nullable graph observations now qualify the exact output and
failure states of the original relation tests across native, canonical modules
and focused sanitizer. The guarded receipt is
`parity/generated_json_nullable_graph_installed_5_0_6.json.gz`; details and the
negative comparator control are in
[the graph checkpoint](generated_json_nullable_graph_5_0_6.md).

Remaining consolidation is specific: the first original test's nullable child
scalar must be observed alongside its repeated parent view and owned identity;
the direct source-level two-shape cache test must be mapped through cache
destruction and subsequent model conversion. Full source acceptance stays open.
