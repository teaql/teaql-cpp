# SQLite relation membership source review: Rust 5.0.6

Status: accepted for this complete named source. Original13, installed native1,
module1 and focused sanitizer1 pass all 79 scenarios; three adjacent native
regressions also pass. Canonical bodies, no-work builds and final captured-input
and reference guards pass.

## Locked source and fixture

The reviewed source is `teaql-provider-sqlite/tests/relation_membership.rs`
(874 lines) at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`e03cfb802d981f5c2a86e9eec888d1a06ac860d986b3b315255de78fa2f6f43b`.
The original receipt is `parity/relation_membership_original_5_0_6.json.gz`.

The handwritten C++ candidate mirrors the source's handwritten graph adapters.
It uses real SQLite schema creation and a runtime transaction to seed parent 100
and children 101/102, with version 1 and the original private name. Metadata
retains the aliased forward relation, sibling relation, reverse children and
parent name masking. Typed decoders and runtime state are Context-owned.
No generated artifacts or manually fabricated trace frames are used.

## Coverage and observed gap

The first four tests retain the complete 32-case forward matrix: filtered and
visible detail, window/probe thresholds, aliased foreign keys, sibling relations
and logging switches. Assertions distinguish scalar foreign keys from hydrated
objects, identity-only filtered detail, physical log count and relation routes.
Independent requests check that original foreign keys remain intact.

Four aggregate tests cover raw and typed results, filtered and visible parents,
aliased keys and logging. The alias-collision test preserves membership keys
when an earlier aggregate overwrites the presentation alias. Typed edge tests
require a non-null identity-only owned view while sibling detail remains in the
shared identity table. Sixteen explicit-projection cases check inverse wiring,
filtered/visible views, selected counts and root NotLoaded state.

The final privacy test covers raw, typed and owned entry points with logging
on/off. Future aggregate bindings must mask root intent before execution; an
independent request must retain its unrelated literal intent.

The nested-selection test covers cyclic and non-cyclic graphs with logging
on/off. It requires projected parent counts, distinct loaded states, exactly
one aggregate SQL execution, and the full two-edge aggregate trace path.
The complete candidate fails its first non-cyclic case: the parent exists and
its name remains NotLoaded, but `selected_count` is absent. Diagnostic receipt:
`parity/relation_membership_nested_diagnostic_candidate_5_0_6.json.gz`.

The runtime's `EntityRowsOperation` only transferred embedded relation aggregates
when another enhancement flag was present. Hydration removes descendant relation
loads because the hydration planner owns them; a nested selection containing
only an aggregate therefore bypassed this transfer. The locked Rust
`fetch_prepared_all_with_aggregates` transfers every nonempty embedded aggregate
list and deduplicates explicit declarations. The C++ repair removes the extra
condition while retaining that existing deduplication and execution path.
The repair is qualified by the installed and adjacent receipts listed below.

## Evidence boundaries

`parity/relation_membership_typed_edges_candidate_v2_5_0_6.json.gz` passes all
first eleven tests. The first typed-edge candidate retains its compile failure
caused by redundant PMR allocator arguments. The complete candidate and nested
diagnostic receipts retain the runtime failure; they must not be reported as
passing acceptance. Candidate receipts embed their exact source for replay.
Capability totals remain 11 qualified / 6 partial; P0-P7 remains incomplete.

The repaired complete candidate receipt is
`parity/relation_membership_nested_fix_candidate_5_0_6.json.gz`. All thirteen
source tests pass, including all four nested-selection scenarios. The installed
counterpart is `tests/parity/teaql_provider_sqlite/relation_membership.cpp`;
its final success marker is emitted only after every scenario completes.

## Source test mapping

| Original test | Installed counterpart |
| --- | --- |
| `filtered_forward_window_keeps_membership` | `verify(true, 0, ...)`, eight cases |
| `visible_forward_window_is_loaded_once` | `verify(false, 0, ...)`, eight cases |
| `filtered_forward_probe_keeps_membership` | `verify(true, 32, ...)`, eight cases |
| `visible_forward_probe_is_loaded_once` | `verify(false, 32, ...)`, eight cases |
| `aggregate_with_distinct_forward_field` | `aggregate(false, filtered, false)`, four cases |
| `aggregate_with_visible_overlapping_forward_field` | `aggregate(true, false, false)`, two cases |
| `aggregate_with_filtered_overlapping_forward_field` | `aggregate(true, true, false)`, two cases |
| `aggregate_with_typed_flat_hydration` | `aggregate(aliased, filtered, true)`, eight cases |
| `aggregate_alias_cannot_change_later_aggregate_membership` | `aggregate_alias`, one case |
| `typed_filtered_reference_has_an_edge_owned_identity_only_view` | `typed_edges`, four cases |
| `explicit_forward_projection_survives_reverse_inverse_wiring_with_aggregates` | `explicit_projection`, sixteen cases |
| `future_aggregate_bindings_are_masked_before_parent_sql_for_all_result_shapes` | `future_privacy`, six cases |
| `nested_typed_selection_executes_its_related_counts_once` | `nested_counts`, four cases |

All 79 scenarios execute through existing runtime/service/executor/provider
boundaries. Rust shared graph ownership is represented by checked Context-bound
C++ state; it does not extend the Context lifetime. The fixture disables the
PMR default resource, so its dynamic runtime data must use explicit resources.
The candidate diagnostic printing is absent from the installed test.

## Installed qualification

`parity/relation_membership_installed_5_0_6.json.gz` records successful native,
canonical module and focused ASan/UBSan builds/tests. Leak scanning remains off.
Each executable reports all thirteen source tests and the 32-case forward
matrix only after completing the full 79-scenario body. The module consumer
verifier checks the imported body against the installed canonical source.

`parity/relation_membership_adjacent_5_0_6.json.gz` records successful native
relation hydration, relation enhancement and typed aggregate context contracts.
Both receipts include successful no-work rebuilds and final source/reference
identity guards. Original and failed candidate receipts remain immutable.
Acceptance covers only the complete named SQLite test source and this runtime
repair; broader runtime helper reviews and generator completion remain open.
