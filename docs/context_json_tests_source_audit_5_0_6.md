# Context JSON test-source qualification checklist: Rust 5.0.6

Status: pending. The source `teaql-runtime/src/context/json/tests.rs` contains
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
| `graph_json_preserves_empty_omitted_null_and_fk_identity` | Existing scalar/graph and generated hydration matrices cover these states; complete original fixture mapping remains open. |
| `graph_input_rejects_conflicting_keys_malformed_edges_and_missing_decoders` | Existing 7072 outcomes cover conflicts/cardinality/registration. Preserve the separate malformed installed target-descriptor control in final test-source qualification. |
| `independently_decoded_root_graphs_with_equal_ids_do_not_share_child_payloads` | New `independent` directly observes shared snapshots and independent child payloads/ledgers in generated models. |
| `incompatible_forward_target_views_do_not_silently_choose_one_payload` | Existing conflict corpora and three-view boundary extension exercise rejection and private-value suppression; exact source-fixture consolidation remains open. |
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
