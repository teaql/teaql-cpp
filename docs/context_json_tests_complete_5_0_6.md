# Complete Context JSON test-source review (Rust 5.0.6)

Status: accepted for this complete named test source. Broader generator parity and P0-P7 remain incomplete.

Source: `teaql-runtime/src/context/json/tests.rs`, 575 lines, SHA-256
`d7321358e2cda1fa817d4a77548dae9407079d872b2eac298e5ecffacd32d3b4`,
at locked commit `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

This review covers all fourteen tests and their three typed fixture definitions.
The fixtures provide indexed scalar layouts, aliases, nullable fields, forward
options and reverse lists; they are test scaffolding rather than additional
runtime APIs. C++ uses retained generated models and the shared runtime fixture
to exercise these responsibilities. No generated file is hand-edited.

## Complete responsibility mapping

| Original test | C++ counterpart and evidence |
| --- | --- |
| `graph_json_restores_typed_children_without_widening_repeated_identity_views` | `generated_json_owned_nullable_probe`: Platform/Record reproduces two typed children with string/NULL values, loaded scalar flags, explicit/omitted parent loading, same-address owned parent identity, a sparse parent without reverse loading, exact graph JSON and private root mutation. Replay compares every output against the unchanged generated Rust SDK. |
| `graph_json_preserves_empty_omitted_null_and_fk_identity` | The 20-case nullable graph corpus checks empty/omitted reverse lists and explicit NULL, foreign-key-only, relation-only and omitted forward states, including inferred identities and exact output. |
| `graph_input_rejects_conflicting_keys_malformed_edges_and_missing_decoders` | Nullable graph corpus: conflicting key, NULL versus non-null identity, wrong edge cardinality, missing owner/nested IDs, NULL list/member and wrong scalar. Input-boundary corpus: missing typed registry and mismatched installed target descriptor, even for an empty list. |
| `independently_decoded_root_graphs_with_equal_ids_do_not_share_child_payloads` | Isolation `independent`: equal root/child IDs share immutable shape while retaining different typed child values and clean separate ledgers. |
| `incompatible_forward_target_views_do_not_silently_choose_one_payload` | Nullable graph corpus: two equal-ID nested parents with contradictory private values reject with the original error, exposing neither private value. Three-view graph controls cover conflicts against every preceding view. |
| `one_graph_preserves_forward_projection_and_reverse_list_views_of_equal_ids` | Isolation `views`: root/first snapshot sharing, distinct second projection, loaded/omitted fields, empty/omitted reverse lists, clean children and exact nested JSON. Detached copied view stays readable with Context alive. |
| `two_relations_to_equal_identity_keep_distinct_views_and_refuse_ambiguous_lookup` | Generated dual-forward `ambiguous`: preserve both explicit projections and reject ambiguity in identity-only lookup. |
| `inferred_foreign_keys_cannot_conflict_across_views_of_one_identity` | Generated dual-forward `inferred_conflict`: contradictory inferred keys for a repeated link identity reject with exact original error. |
| `graph_depth_is_bounded_before_hydration` | The unchanged 7072-case actual-runtime corpus includes depth 64/65/66 controls, with registered/unregistered and indexed-layout modes. |
| `overlarge_relation_rejects_before_allocating_or_decoding_child_views` | Eleven actual-source graph limit/view cases cover oversized malformed arrays, the adjacent accepted node budget, nested remaining budget and error precedence. |
| `native_json_keeps_null_omission_falsy_dates_decimal_and_no_mutation` | Combined scalar/read-only test checks complete JSON and roundtrip, loaded NULL versus omission, 0/false/empty string, Decimal scale, date/timestamp, nested JSON and readonly dynamic values; dirty/new/deleted/dynamic mutation flags remain clean. The generated JSON metadata exception is explicit below. |
| `batch_actual_shapes_share_snapshots_and_keep_values_and_ledgers_private` | Isolation `batch`: full snapshot identity matrix, equal-ID private scalar values, NULL/omission and mutation of only one row. |
| `aliases_and_unsupported_input_reject_without_exposing_values` | Input-boundary corpus and nullable graphs cover unknown fields, wrong types, required NULL, negative IDs, invalid date, forbidden runtime/persistent state, duplicate aliases, missing owner identity and missing/mismatched installed entity metadata. Original outputs and private-value suppression are asserted. |
| `native_batches_construct_geometry_per_shape_not_per_row` | Direct `JsonReadSession` test retains compact rows for 1/100/10000 inputs with alternating metadata/column aliases. All present-field rows share one layout; an omitted-field row has a distinct layout. After the session and input wrappers are destroyed, conversion to generated Record preserves loaded NULL, omission and non-null alias payload. The separate allocation regression verifies that owned field-name cost remains constant per shape. |

The direct cache test uses the runtime's existing session interface; it neither
exposes private fields nor creates a new ownership domain. Layout identity and
the allocation regression establish reuse without relying on an implementation-
specific container size. The original test drops its shape cache before model
conversion; the C++ test likewise ends the decode session before conversion.
Context remains alive, as required for borrowed C++ data.

## Fixture adaptations and the approved exception

The nullable owned graph maps GraphParent.name to Platform.name,
GraphChild.name to Record.type_field (C++ application getter `type()`),
child_list to record_list, and parent/parent_id to platform/platform_id. The
observed children remain ordinary generated typed entities; no input metadata is
patched to make the fixture pass. Separate Holder/Record tests exercise nullable
foreign keys. Assertions are mapped by responsibility; fixture identifiers are
not presented as identical source declarations.

The original handwritten JsonProbe test has a JSON descriptor and succeeds. The
retained generated SampleRecord instead receives a conflicting Rust LargeText
annotation. The user-approved [JSON payload exception](json_payload_compatibility_exception_5_0_6.md)
keeps C++ JSON support. The original generated failure is retained and the C++
combined success is checked against explicit expected JSON. It is **not** counted
as an exact generated Rust comparison. This does not waive the original runtime
source's scalar, readonly-dynamic or no-mutation assertions.

## Evidence chain

- `parity/context_json_original_5_0_6.json.gz`: all fourteen unchanged original tests pass, with source provenance.
- `parity/context_json_shape_installed_v2_5_0_6.json.gz`: complete scalar/shape source, six allocation samples and 7072 exact observations per profile.
- `parity/context_json_graph_installed_5_0_6.json.gz`: graph source, 7072 existing and 11 added observations per profile, unique-view allocation and lifecycle checks.
- `parity/generated_json_isolation_installed_5_0_6.json.gz`: four generated snapshot, owned-view and ledger contracts across native/modules/sanitizer.
- `parity/generated_json_ambiguous_installed_5_0_6.json.gz`: two generated dual-forward contracts across the same profiles.
- `parity/generated_json_boundaries_installed_v2_5_0_6.json.gz`: five exact input-boundary contracts plus the separately approved combined payload contract across the same profiles.
- `parity/generated_json_nullable_graph_installed_5_0_6.json.gz`: twenty nullable graph inputs across the same profiles.
- `parity/generated_json_owned_nullable_installed_v5_5_0_6.json.gz`: final nullable owned graph, three direct cache lifetime samples per profile and a consolidated native integration gate (12/12 passed).

These checkpoints preserve their own captured source epochs; earlier receipts
are not relabeled as new runs. Negative comparator controls are retained in
`parity/generated_json_comparison_exit_codes_5_0_6.json.gz` and
`parity/generated_json_nullable_graph_negative_5_0_6.json.gz`.

Acceptance is limited to this named test source's responsibilities. It does not
close other runtime/provider source reviews, all generator capabilities, or
P0-P7. C++ Context lifetimes, SQLite scope and the approved JSON exception remain
part of the contract.

## Module visibility discovered by the final test

The first final checkpoint passed native owned/cache tests but failed the same
canonical module consumer: `detail::JsonReadSession` was declared in the headers
but absent from the module export catalog. The failed receipt is retained as
`parity/generated_json_owned_nullable_installed_5_0_6.json.gz`. The module now
exports the existing declaration, with isolated lookup coverage. No session
function body, ownership rule or ABI definition changed.

Export verification also detected an existing catalog drift: `append_float`
was already exported from teaql_core, but absent from the reviewed catalog and
lookup consumers. Its catalog locator and lookup coverage now match the
existing declaration; its export and implementation are unchanged.

The initial observer compilation used the Rust-style field-derived getter
`typeField()` instead of the actual C++ application getter `type()`. Correcting
the observer does not change the generated SDK; the compile failure is retained
in `parity/generated_json_owned_nullable_compile_diagnostic_5_0_6.json.gz`.

The second checkpoint compiled the corrected module consumer, but its subsequent
live replay found the Rust executable absent from the temporary Cargo target
directory. The process failed before observing module output; that receipt
(`parity/generated_json_owned_nullable_installed_v2_5_0_6.json.gz`) is incomplete.
The final checkpoint rebuilds the unchanged oracle under the repository-ignored
`.tools/oracles/field_layout_5_0_6` directory and repeats verification. No missing
artifact is treated as a successful comparison or as an implementation failure.

The third checkpoint passed native/module owned and cache tests before its
execution handle reported exit 143 during sanitizer compilation. No controller
or compiler process remained; the cause is not established. The fourth receipt
resumes the same captured input/reference epoch after revalidating those guards,
retains the earlier steps and profiles, and rebuilds the unfinished sanitizer
target before running it. The interrupted build is never considered successful.

After a later continuation, the fourth execution handle and its processes were
absent, and all three preset build directories plus the Rust target artifacts
had been removed. Captured source inputs and reference revisions were unchanged.
The fifth checkpoint preserves successful earlier observations and rebuilds the
remaining gates. It uses a fresh task-owned Rust target with exit cleanup,
disabled incremental compilation and reduced debug information, preserving dev
debug assertions and the locked dependency graph. CMake uses only the committed
preset paths. Shared caches are not evicted.

## Final acceptance

The fifth checkpoint completed successfully. The nullable owned graph matches
its unchanged generated Rust observer; direct cache lifetime samples at
1/100/10000 rows pass in native, canonical modules and focused sanitizer.
Native consolidated integration passes 12/12; focused sanitizer CTest passes
2/2; the rebuilt module import contract passes 1/1. All 1518 export declarations,
isolated lookup coverage, canonical consumers, no-work rebuilds, generated
file hashes and captured-input/reference guards pass. The earlier successful
native/module observations retained in the resumed receipt are explicitly
identified by their original steps; the compact evidence survives artifact cleanup.
Pinned format/check passes 1777 handwritten C++ files. Leak scanning remains
disabled; no MIPS or physical-device execution is claimed.

The complete fourteen-test source is accepted with the fixture mappings and
approved generated JSON exception described above. Active source totals become
110 accepted / 12 pending_review / 2 excluded_external. Capability totals remain
11 qualified / 6 partial. No complete generator phase or P0-P7 acceptance follows
from this source review.

## Artifact lifecycle

The task-owned Rust target was removed after the controller reaped its children.
The cleanup record includes its exact path and byte count. Only compact reports,
source revisions, hashes and expected results remain as evidence. The three
committed CMake preset directories remain reusable project caches; their measured
sizes and retention rationale are recorded in
`parity/context_json_tests_artifacts_5_0_6.json.gz`. No shared Cargo download cache,
source fixture, database or unrelated session output was removed.
