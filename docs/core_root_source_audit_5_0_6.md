# Core root upgrade audit: Rust 5.0.6

Status: accepted for this source under the final closure below. The historical
[5.0.5 root review](core_root_source_acceptance.md) is not current acceptance.
The active reference `teaql-core/src/lib.rs` has 1,030 lines and twenty-six root
tests at commit `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`c1f1f8b32f3a70f241e7f689ce3387e41d42979b14dd02477eae30a0979d781a`.

## Reviewed baseline delta

A direct diff from the locked 5.0.5 commit shows these changes:

- Public dynamic_fields and dynamic_properties modules, plus private load_state
  and request_intent module declarations.
- Root reexports for EntityJsonTraversal, FieldLayout, LoadedSnapshot,
  RelationShapeCache, CompactRowLayout and the four request-intent models.
- A hidden serde_json crate reexport used by Rust-generated code. C++ does not
  acquire this Rust dependency; generated/runtime JSON contracts use the existing
  native JSON/value infrastructure. This language-specific binding needs explicit
  treatment in the full root review.
- Expansion of one existing test, `java_style_string_match_builders_expand_like_patterns`:
  both `tea` and `%tea_\%`, each through all six string-match builders, must retain
  distinct original and expanded pattern strings. A raw `like(name, "%tea%")`
  separately retains a Value operand rather than LikePattern provenance.

The other twenty-five root tests retain their source bodies in this diff. Their
current macro/helper dependencies still require execution and review; unchanged
test text alone is not acceptance of new dependency behavior.

## Current C++ evidence and gap

`verify_public_exports.py` passes eight core public module namespaces and
seventy-eight root reexport names. Its lexical inventory does not establish
member behavior or cover the bare `pub use serde_json;` statement. Root
EntityGraph types continue to use their distinct core namespace rather than the
runtime identity graph. The final eval module remains included in the scan.

The installed functional fixture already uses LikePattern for the six `tea`
cases, but omits the escaped/wildcard input and the raw-LIKE Value distinction.
The [isolated corrected candidate](parity/core_root_like_candidate_5_0_6.json.gz)
retains the entire existing functional fixture, replaces only that test body,
and adds all twelve original-pattern checks plus the direct raw-LIKE comparison.
The complete functional executable passes against unchanged current native
archives. No production Expr behavior was changed for this result.

The candidate remains outside the worktree during the frozen SDK integration.
After installation, verify the full original-root functional, entity-mapping,
composition and presentation responsibilities on native and canonical imported
consumers. Reconcile the current root exports and dependency behavior with the
complete source before promoting its disposition. Existing complete Expr source
acceptance is a dependency, not proof of this whole root test suite.

## Current original execution

The [fresh original-core receipt](parity/core_root_original_5_0_6.json.gz) records
69/69 unchanged library unit tests passing, with zero ignored or filtered cases.
All twenty-six root test names are extracted from the current source and checked
individually in the successful output. Source, crate manifest, reference lock and
baseline lock hashes are unchanged across execution. The command uses the exact
reference checkout with offline/locked resolution and an external target directory.
This is original-side evidence, not a claim that all sixty-nine tests are root
contracts or that the C++ root is already accepted.

The native counterparts for the twenty-six root tests are distributed across
`tests/parity/teaql_core/entity_mapping.cpp` (ten model/mapping contracts),
`tests/parity/teaql_core/functional_contracts.cpp` (six root expression contracts
among its seventeen total checks), `tests/parity/teaql_core/composition_contracts.cpp`
(five root query/list/SafeExpression contracts plus other checks), and
`teaql_core/tests/presentation_api_tests.cpp` (five root presentation contracts
among its broader API assertions). The current queued installer refreshes the
functional body only. Whole-root qualification must also execute the other
canonical bodies against the current native/imported implementation and reconcile
helper/macro behavior; the queued functional gate alone is insufficient.

## Explicit root-test correspondence

This source-location inventory identifies every current root test individually.
It does not establish assertion equivalence or replace execution. Mapping,
functional and composition refer to the three files listed above; presentation
refers to `teaql_core/tests/presentation_api_tests.cpp`. Multiple Rust tests share
one C++ presentation function, so executable counts must not be reported as
individual root-test counts.

| Locked Rust root test | C++ counterpart location |
| --- | --- |
| `derive_entity_descriptor` | mapping: `derive_entity_descriptor` |
| `derive_maps_checked_integer_and_decimal_fields` | mapping: `derive_maps_checked_integer_and_decimal_fields` |
| `derive_allows_partial_projected_records` | mapping: `derive_allows_partial_projected_records` |
| `derive_relation_descriptor_and_register` | mapping: `derive_relation_descriptor_and_register` |
| `derive_decodes_boxed_forward_relation_without_changing_relation_semantics` | mapping: `derive_decodes_boxed_forward_relation_without_changing_relation_semantics` |
| `register_entities_macro_registers_multiple_descriptors` | mapping: `register_entities_macro_registers_multiple_descriptors` |
| `default_table_name_matches_java_sql_repository_rule` | mapping: `default_table_name_matches_java_sql_repository_rule` |
| `derive_maps_json_date_and_timestamp_types` | mapping: `derive_maps_json_date_and_timestamp_types` |
| `query_builders_cover_filters_sort_aggregates_and_relations` | composition: `query_builders_cover_filters_sort_aggregates_and_relations` |
| `compare_columns_builds_property_to_property_filter` | functional: `compare_columns_builds_property_to_property_filter` |
| `sound_like_builds_soundex_equality` | functional: `sound_like_builds_soundex_equality` |
| `java_style_string_match_builders_expand_like_patterns` | functional: `java_style_string_match_builders_expand_like_patterns` |
| `large_in_builders_use_large_binary_ops` | functional: `large_in_builders_use_large_binary_ops` |
| `ordinary_in_builders_promote_more_than_twenty_values` | functional: `ordinary_in_builders_promote_more_than_twenty_values` |
| `subquery_builder_projects_requested_field` | functional: `subquery_builder_projects_requested_field` |
| `smart_list_supports_entity_ids_versions_and_records` | composition: `smart_list_supports_entity_ids_versions_and_records` |
| `smart_list_supports_java_style_collection_helpers` | composition: `smart_list_supports_java_style_collection_helpers` |
| `safe_expression_supports_null_safe_chaining_and_defaults` | composition: `safe_expression_supports_null_safe_chaining_and_defaults` |
| `safe_expression_exposes_java_style_empty_and_callbacks` | composition: `safe_expression_exposes_java_style_empty_and_callbacks` |
| `web_style_and_action_bind_frontend_metadata` | presentation: `original_web_contracts` |
| `web_response_wraps_entity_and_list_payloads` | presentation: `original_web_contracts` |
| `web_response_includes_facets` | presentation: `original_web_contracts` |
| `xls_block_context_matches_java_navigation_model` | presentation: `original_navigation_contract` |
| `xls_workbook_groups_pages_and_blocks_as_json_payload` | presentation: `original_workbook_contract` |
| `dynamic_properties_roundtrip_into_json` | mapping: `dynamic_properties_roundtrip_into_json` |
| `base_entity_data_roundtrips_record_and_dynamic_properties` | mapping: `base_entity_data_roundtrips_record_and_dynamic_properties` |

The required native CTest selection is
`original_core_entity_mapping_contracts`, `original_core_functional_contracts`,
`original_core_composition_contracts`, and `presentation_api_contracts`.
The imported-module selection must be resolved from the current canonical
contract manifest and CTest discovery, rather than assuming that similarly named
presentation probes execute the same body. Run these after the active integration
and queued installation have finished, and retain binary/source hashes with the
results. The LIKE expansion remains pending installation at this checkpoint.

## Missing canonical module registrations

Inspection of the current canonical contract manifest confirms that only the
functional fixture above is registered. The mapping, composition and complete
presentation API fixtures are absent. Existing `presentation_probe` and
`presentation_api_probe` entries point at different source bodies and do not
close this gap.

The [registration candidate](parity/core_root_module_registration_candidate_5_0_6.json.gz)
retains three proposed entries, each importing `teaql_runtime`, and the exact
prepared consumers plus their two shared fixtures. Preparation uses the existing
canonical adaptation tool with only its manifest location redirected outside the
worktree. All five bodies retain their canonical text; all three entry prologues
import the intended module, with no project declaration headers retained.
This proves preparation only, not module compilation or execution.

After the current integration and queued installer terminate, add these entries
and their `teaql_module_` targets to the module build preset. Configure and compile
the current native and module variants, run all four root fixture bodies in each
profile, then apply the canonical-body verifier and source/binary hash guards.
Merge these additions into the then-current manifest rather than replacing it
with this candidate's old base. No source disposition has changed.

## Hidden JSON reexport binding review

The current Rust `teaql-macros/src/derive_impl.rs` emits
`Option<::teaql_core::serde_json::Value>` as the return type of the generated
`borrowed_json` implementation. This is a concrete consumer of the root's hidden
crate reexport; the root-export scanner does not cover it. That generated method
uses `EntityJsonTraversal`, loaded scalar/relation fields, runtime graph lookups,
reserved-key removal, and dynamic fields before returning JSON.

The C++ language binding is `std::optional<Json> borrowed_json(Memory,
EntityJsonTraversal&) const`, emitted in the retained generated models. Its
`GeneratedEntityJson` adapter receives the destination resource, traversal,
entity type, source address, runtime state and entity identity. The core
`EntityDefaults::into_json` path invokes that hook with the explicit destination
resource. This preserves a language-native JSON return-type boundary without
adding the Rust serde_json crate or claiming to reproduce that crate's entire
public API in C++.

This review accounts for the hidden reexport's binding responsibility only.
Loaded graph traversal, cycle behavior, reserved-key privacy and dynamic fields
still need their own current semantic evidence. The active generated hydration
integration includes graph-view and JSON-hydration comparisons, but its running
state is not yet successful evidence. Do not use the return-type correspondence
to promote those source contracts.

## Imported numeric helper visibility repair

The complete native7 selection passes, but the first imported compilation of
`entity_mapping_fixture.hpp` fails because `detail::append_float` is declared
through headers and not visible to module consumers. The failure is retained in
`parity/source_completion_candidate_verification_5_0_6.json.gz`.

The core module now includes the existing number-text declaration explicitly in
its global fragment and reexports its existing overload set in the established
`teaql::detail` export block. No function body, assertion, number formatting rule
or ownership behavior changes. The canonical mapping fixture is unchanged.
`parity/source_completion_module_exports_verification_5_0_6.json.gz` checks that
this module interface is the only captured input changed since the failed run,
then repeats formatting and the selected native/module validation with complete
JUnit output retention. This continuation is still running; the core source
remains pending until it finishes and the source review is reconciled.

## Final source acceptance

The complete source responsibility reviewed here is accepted under the
[current closure evidence](hydration_trace_core_source_closure_5_0_6.md).
Original and installed native/imported gates are terminal and successful; the
relevant focused concurrency sanitizer evidence is retained with its own scope.
Earlier pending/isolated-candidate statements describe historical checkpoints.
This accepts only the named source, not the enclosing runtime, provider, generator
capability or P0-P7 goal.
