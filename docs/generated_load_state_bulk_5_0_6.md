# Generated typed batches and complete shared-state test review

Status: all nineteen responsibilities in the original
`teaql-core/tests/shared_load_state.rs` have scoped C++ evidence and are accepted
with the explicit Context ownership and owner-thread adaptations below. Source
dispositions advance to **19 accepted / 105 pending**. **G15 remains partial**:
the five separately reproduced quoted-FK differences are outside this test
file's unquoted fixtures and remain unaccepted. Full G13 and P0-P7 are open.

The runtime reference remains Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the frozen generator is
`45e1572c4956ca963410d8ea436ead3c8857087a`. The original source, generator,
generated Rust fixtures and all existing C++ SDK bytes are unchanged by this
checkpoint. No production runtime function is changed.

## Actual typed decoding, including every large-batch value

The [C++ observer](../tests/parity/generated_load_state_bulk_probe.cpp) calls the
actual generated `Record::from_compact_row` and inherited
`ChildRecord::from_compact_row`. The
[Rust observer](../tests/parity/rust_field_layout/src/bin/generated_load_state_bulk_5_0_6.rs)
calls the unchanged original generated counterparts. Neither implements a local
row decoder or replacement snapshot cache.

The twelve cases cross two models, **1 / 100 / 10,000** rows and either one shared
ColumnLayout or independently built equal layouts. Each result contains every
row's unique ID and actual NULL/text value: **40,404 row values** in total.
The comparator checks complete output objects and arrays against both the saved
corpus and freshly executed original output, without result normalization.

Every row must share its model's immutable indexed snapshot and installed layout,
have no overflow or named set, mark all three authentic scalar aliases loaded,
leave omitted fields unloaded and retain clean/non-new state. The first row is
NULL; other rows contain distinct strings longer than inline-string storage.
Changing the last value must preserve its snapshot and every sibling's payload.
Checker narrowing must detach only that row's projection and preserve siblings.

The original nineteen-test file also runs unchanged, including its original
1/100/10,000 inputs and both sixteen-thread tests. The additional generated model
uses authentic `type` / `type_field` / `legacy_type` names; it is an independent
model acceptance fixture, not a renaming of the original TypedIndexedRow input.
The earlier runtime observer retains the original ThreeNameRow vocabulary.

## Cold metadata, Context ownership and actual destruction

For each ordinary/inherited generated model, a fresh GlobalContext and UserContext
create sixteen independent equal row shapes. Layout identity and scoped snapshot
reuse must hold while IDs, NULL and omitted fields remain private. A real worker
thread calls the generated layout factory and must receive the existing
`Context accessed from a non-owner thread` error. This tests the approved C++
owner-thread boundary; it does not claim Rust's shared-Context concurrent access.

An actual generated object containing a 2,048-byte string is explicitly copied
into Global storage. The original UserContext is deleted, its live byte count
returns to zero and its surviving checked entity borrow rejects access. A new
UserContext can read the independent Global copy, which uses the Global allocator,
and borrows the same Global FieldLayout. Its own decoded snapshot has a distinct
identity from the deleted User pool. Default PMR allocation is disabled throughout.
Owning row/container/string temporaries leave scope before their storage owner.

The large C++ fixture explicitly permits 512 MiB of User allocation to observe the
complete matrix and serialize all results. This is a test budget, not a production
default, measured peak, constant-memory stream claim or performance guarantee.

## Whole original test-file responsibility mapping

All rows below refer to complete original tests, including their assertions,
not just the presence of a function name. Earlier evidence is reused only where
its implementation bodies and relevant generated inputs remain unchanged.

| Original test | Mapped assertions and evidence |
| --- | --- |
| `flat_relation_availability_is_shared_shape_metadata_not_a_row_value` | CompactRow/snapshot and shared-state contracts retain common relation geometry, exclude marker row/map values, separate unselected shapes and preserve markers after scalar insertion. [Sharing review](shared_state_source_tests_5_0_6.md), [row allocation audit](load_state_allocations_5_0_6.md). |
| `bitmap_and_lazy_overflow_keep_all_boundary_slots` | Boundary bits 0/31/32/63 and lazy slots 64/65/129, private clear and fixed-bit retention are covered by complete snapshot contracts and the actual source corpus. [State acceptance](shared_load_state_acceptance_5_0_6.md). |
| `value_only_changes_retain_identity_and_availability_diverges_privately` | No-op identity, overflow detachment and unchanged named selection are retained in snapshot and auxiliary-sharing comparisons. [State acceptance](shared_load_state_acceptance_5_0_6.md). |
| `unchanged_dynamic_selection_preserves_identity_and_relation_markers` | Value/NULL shape reuse, selection replacement, overflow/relation retention, removal/base rejoining and owner refusal are covered by the 176 actual carrier observations and contracts. [Sharing review](shared_state_source_tests_5_0_6.md). |
| `loaded_fk_does_not_claim_target_details_or_dynamic_property_availability` | Local FK presence stays distinct from target detail and derived-property presence; unknown mutation fails and explicit relation selection detaches. [State acceptance](shared_load_state_acceptance_5_0_6.md), [loaded graphs](loaded_row_graph_5_0_6.md). |
| `invalid_generated_tables_fail_instead_of_renumbering` | Complete FieldLayout construction/validation contracts and 44 original outcomes preserve duplicate, index, incomplete mapping/member and invalid-name errors. [State acceptance](shared_load_state_acceptance_5_0_6.md). |
| `layout_revision_and_internal_wire_boundaries_remain_distinct` | Layout revision separation, indexed version mutation and rejection of indexed wire serialization/deserialization retain their exact source branches. [State acceptance](shared_load_state_acceptance_5_0_6.md). |
| `independently_built_same_shape_rows_share_snapshot_and_leave_absent_defaults_out_of_mutations` | Generated ordinary/optional helper corpora retain clean defaults, explicit zero/false and sparse mutation values. New independently shaped real typed batches require shared state without loading omitted fields. [State/model mapping](shared_load_state_acceptance_5_0_6.md). |
| `predefined_payloads_keep_null_missing_empty_zero_and_false_distinct` | G01 ordinary/optional model comparisons distinguish missing/NULL/empty text, explicit zero/false, availability and emitted values; new actual typed NULL/string results supplement those distinct branches. [State/model mapping](shared_load_state_acceptance_5_0_6.md). |
| `cow_rejoins_compatible_shape_without_retaining_overflow_or_cross_revision_masks` | Add/remove overflow and named selections rejoin canonical empty state and discard auxiliary sets; another revision refuses conversion. [State acceptance](shared_load_state_acceptance_5_0_6.md). |
| `changed_and_manual_row_shapes_are_grouped_before_decode_without_union` | Actual CompactRow and initialized-cache observations group equal manual shapes while preserving omitted version and private IDs. [Allocation audit](load_state_allocations_5_0_6.md), [state acceptance](shared_load_state_acceptance_5_0_6.md). |
| `physical_scalar_alias_decodes_actual_payload_not_a_loaded_default` | Exact runtime alias cases and actual generated ordinary/inherited decoders retain actual physical-column payload and loaded member state. [Runtime aliases](typed_member_alias_runtime_5_0_6.md), [generated aliases](generated_member_aliases_5_0_6.md). |
| `canonical_member_and_physical_aliases_decode_the_same_value_and_unknown_names_load_nothing` | Original-vocabulary runtime 130 and actual generated 260 outcomes retain all ordered alias subsets, precedence, unknown spelling, NULL/text, dirty/new state and member payload. The prior failures are preserved. [Alias acceptance](generated_member_aliases_5_0_6.md). |
| `identical_column_shapes_do_not_reuse_masks_across_entity_types_or_revisions` | Initialized-cache and FieldLayout/snapshot observations retain reordered slot bits, entity/revision separation, actual values and repeated same-type identity. Actual ordinary/inherited generated model batches supplement the type boundary. [State acceptance](shared_load_state_acceptance_5_0_6.md). |
| `physical_fk_alias_and_nested_object_are_distinguished_without_losing_identity` | Existing unquoted forward-object helpers and loaded graph comparisons preserve scalar/NULL/object identity, scalar-loaded versus detail-loaded state and nested target IDs. This test uses unquoted `related`; it does not exercise the five separate quoted-FK discrepancies. [Loaded graphs](loaded_row_graph_5_0_6.md), [alias runtime](typed_member_alias_runtime_5_0_6.md). |
| `cold_concurrent_type_layout_installation_preserves_one_layout_and_snapshot` | The original sixteen-thread barrier executes unchanged. New C++ cold factories, sixteen independent shapes, actual foreign-thread refusal and Global metadata reuse qualify the approved owner-thread adaptation. |
| `concurrent_decoders_reuse_one_immutable_snapshot` | The original sixteen-thread decoder test executes unchanged. C++ shared and independently built shapes reuse one User pool; another User pool is isolated while metadata stays Global-owned. No concurrent mutation of a C++ Context is advertised. |
| `typed_decode_shares_one_snapshot_for_small_and_large_results_without_sharing_values` | New actual generated 12-case/40,404-row matrix checks every payload, shape, clean/default flag, value-only identity and sibling independence, including all original batch sizes. |
| `row_shape_changes_detach_while_value_changes_keep_the_projection_cache` | Existing CompactRow/cache source contracts retain value-only shape identity and column-add detachment. New actual typed batches add explicit Checker narrowing, snapshot detachment and sibling projection controls. [Allocation audit](load_state_allocations_5_0_6.md). |

This accepts this finite source-test file. It does not accept every macro,
generator model/configuration, query, provider or unrelated test-file obligation.
The original test's unquoted FK behavior and the additional quoted-FK witness
have different inputs; accepting one does not normalize or waive the other.

## Terminal gates and preservation

| Gate | Result | Evidence |
| --- | --- | --- |
| Original complete source test file | 19/19, no ignored tests; both sixteen-thread tests included | Full terminal output in the final receipt |
| Native selection | 6/6 plus twelve freshly executed original/C++ batches | [Native](parity/generated_load_state_bulk_native_5_0_6.json.gz) |
| Canonical imported selection | 6/6 plus the same complete live-source comparison | [Modules](parity/generated_load_state_bulk_module_5_0_6.json.gz) |
| Targeted ASan/UBSan | 3/3 plus the same live-source comparison; leak scan disabled | [Sanitizers](parity/generated_load_state_bulk_sanitize_5_0_6.json.gz) |
| Pinned formatting | Format and check pass for 1,668 handwritten C++ files | Formatter log in the final receipt |
| Current registration and source provenance | 728 native tests and 857 canonical imported bodies | Structural outputs in the final receipt; counts are not semantic evidence |

Each profile has terminal successful compiler/linker exits and a final successful
no-work build. Current tested source/binary hashes and exact comparison receipts
are rechecked. The fifteen previously audited core/runtime/memory implementation
files remain byte-identical; the qualified alias runtime/producer and all 78
SDKs and 2,201 protected original/historical files also retain their prior bytes.
The prior Java80/compiler193/full reproduction gates are reused with those
bindings, not rerun or represented as fresh executions for test-only additions.

The [previous disposition archive](parity/generated_load_state_bulk_previous_disposition_5_0_6.json.gz)
preserves complete preceding source/capability entries and documents. The
[final acceptance receipt](parity/generated_load_state_bulk_acceptance_5_0_6.json.gz)
binds the complete mapped tests, new/reused evidence, structural checks and
current authoritative entries. Historical receipts and their earlier document
hashes retain their checkpoint scope.

Whole G15 remains partial for the five original/current quoted-FK differences,
which were identical across previous/current C++ SDKs. Their complete values and
errors stay in the [quoted witness](parity/generated_member_alias_quoted_witness_5_0_6.json.gz).
G08/G12 behavior decisions, other complete source responsibilities and current
full native SQ05/P7 remain open. Registry 728 does not relabel the historical
718/721 full-suite result. No MIPS or physical-device result is added.

## Reproduction

Activate the locked tools and use a fresh receipt path for each profile:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_load_state_bulk_5_0_6.py \
    --profile native --record /tmp/teaql_bulk_native_fresh.json.gz
```

The same helper accepts `modules_native` or `sanitize` when relevant. It waits
for all builds to finish, runs the registered selection, executes the real Rust
observer and compares every result before writing a complete receipt.
