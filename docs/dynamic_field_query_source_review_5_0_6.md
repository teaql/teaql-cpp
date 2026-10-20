# Dynamic-field query integration source review: Rust 5.0.6

Reference commit: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Complete source: `teaql-runtime/tests/dynamic_field_query.rs` (1,759 lines).
SHA256: `f69e18140d672bab10bbd14949f6843308ce5d5f766cf6990646b6ba24b90357`.

Status: this complete integration-test source is accepted after the terminal
original23/native3/module3 gate and complete review of the helper and test bodies.
One plain-carrier counterpart is explicitly handwritten; the others use actual
generated models. The exact request-projection assertion is now observed at the
executor boundary. This verdict does not accept the containing production
implementations, unresolved model naming, other allocation sources or phase exits.

## Complete source inventory

The file contains 23 tests and the helpers below; it contains no standalone
production implementation whose behavior can be accepted by counting tests.
The allocation-counter module is a separately inventoried source dependency.

| Helper | Responsibility and counterpart |
| --- | --- |
| Indexed `School`, dynamic property map and layout constants | Actual generated `cpp_examples::Product` supplies id/version/name, readonly values, indexed snapshots and persistent dynamic fields. Model names, revisions and slot order follow the generated model, not the test's hard-coded Rust layout. |
| Local `Plain` | The retained handwritten `example::Product` deliberately has no readonly carrier. This one counterpart is not described as generated. |
| Non-Clone `JsonGraphRow`, fixed layout and repeated reverse metadata | Dedicated generated `cpp_graph_serialization::JsonGraphRow` supplies the same graph roles. An actual allocator-copy positive control verifies the no-copy measurement during borrowed and consuming rendering. |
| `SerializationParent` and boxed `SerializationRelations` | Dedicated generated parent, forward node and reverse member types preserve all four object/list presentation paths. The model, storage and selected-edge adaptations are explicitly documented in the graph continuation. |
| `FixedRows` executor and stream | C++ uses the independent entity service, SQLite executor and real statements. Three/four-row fixtures, exact chunk sizes, long tail and statement counts are asserted. SQLite fixture setup finishes before statement counting starts. |
| Fault enum and `ObservedProvider` | `ChildProvider` receives a trusted Context marker, owner, IDs and validated intent. Five injected faults reproduce omitted owner, unexpected owner, unselected value, wrong type and wrong owner. Private error canaries remain protected. |
| `definitions`, Context builders, query helper and recording aliases | Context-scoped definitions retain the two note/extra fields and revision. Fixtures independently register metadata, entity registry and optional provider. Queries preserve explicit intent, projections, limits and dynamic selections. |

## Test-by-test reconciliation

Counterpart functions below are in
`teaql_runtime/tests/dynamic_relation_tests.cpp` unless another file is stated.
“Readonly file” means `tests/generator/generated_readonly_property_tests.cpp`;
“graph file” means `tests/generator/generated_graph_serialization_tests.cpp`.

| Original test | Counterpart and reviewed assertions |
| --- | --- |
| `entity_without_derived_carrier_keeps_nullable_defaults_without_claiming_presence` | `plain_carrier_readonly_defaults`: decoded native values, unsupported carrier flag, no value/presence for `_missing`, native name and persistent name. |
| `readonly_property_schema_survives_typed_decode_without_presence_or_fixed_slots` | Readonly file, `readonly_read_allocation_contracts`: shared snapshot/schema, typed NULL versus absent value, declared type, no fixed slot/bit or dirty field, wrong-type error names the property and hides the secret. Reads at 1/100/10,000 perform no counted allocation. |
| `derived_property_reads_borrow_values_preserve_null_absence_and_allocate_nothing` | Same function: actual pointer borrowing, native/persistent/empty-key exclusions, zero/false/empty values, ordinary and JSON NULL presence, absent values, unchanged snapshot, no fixed bits or mutation intent. The three measured read counts remain zero. |
| `graph_decoders_retain_dynamic_read_metadata_without_promoting_it_to_columns` | `generated_graph_read_metadata`: sidecar never becomes a native column; shared decoder plus batch/list/option/single registry entries; frozen entity/edge lookup; Value/NULL/NotLoaded and shared availability; generated JSON retains the private value; clearing row relations clears the sidecar. |
| `borrowed_flat_graph_json_is_cycle_bounded_and_never_clones_entities` | Graph file, `forward_cycle`: live root name, both frozen peers, identity/version-only cycle boundary, unchanged snapshot and equal borrowed/consuming JSON, with observed zero entity-state copies. |
| `borrowed_reverse_graph_json_distinguishes_empty_and_unselected_lists` | Graph file, `reverse_projection`: both names and order, selected empty array, selected NULL, all three unselected identities omit both relations, child originals remain private. |
| `flat_json_does_not_leak_sibling_detail_into_an_unselected_or_filtered_edge` | Graph file, `sibling_isolation`: unselected edges omit full/restricted siblings; selected full edge resolves detail; NULL key emits NULL. An additional selected restricted edge assertion prevents canonical sibling detail leakage. |
| `serialization_preserves_fixed_derived_and_persistent_names_without_promoting_not_loaded` | Readonly file, `generated_entity_serialization(false)`: native/derived/persistent names, zero/false/empty values, two NULL namespaces, absent unselected field and absent runtime/storage metadata. |
| `sparse_serialization_omits_absent_fixed_fields_and_internal_mutation_metadata` | Readonly file, `generated_entity_serialization(true)`: native field remains unloaded/omitted; mutation values retain audit/new intent; presentation omits all five internal keys and the private reason. |
| `nested_entity_json_keeps_extensions_and_does_not_serialize_mutation_internals` | Graph file, `nested_projection`: both direct objects and both list members retain native/derived/persistent values, explicit NULL and absent fields; all five mutation keys stay private, while literal business `_payload._comment` remains. Governed nested values retain the child reason. |
| `relation_json_distinguishes_selected_null_empty_and_not_loaded` | Graph file, `selected_relations`: all four omitted edges, both selected NULL objects and both selected empty lists. |
| `query_and_stream_carry_readonly_schema_without_installing_absent_values` | `generated_readonly_stream_case(0)`: three typed/list rows and three streamed rows preserve declared absent type without presence/fixed bits; compatible rows share a snapshot. No dynamic provider is installed, matching the original Context. |
| `readonly_property_schema_survives_cache_miss_and_hit_without_extra_provider_calls` | `generated_readonly_stream_case(1)`: two reads of three rows retain equal schema/shared snapshot and perform exactly one physical statement, proving an actual cache hit. |
| `root_stream_retains_one_snapshot_across_full_chunks_tail_and_cursor_drop` | `generated_readonly_stream_case(2)`: all 1,003 IDs/names, 14 chunks of 73 plus 54, ordered indexes, zero versus NULL, absent declared property, no fixed bits/dynamic payload/dirty fields; retained first/second/final rows preserve schema after cursor close; one SQL statement. |
| `child_enhancement_dynamic_selection_reaches_typed_views_and_survives_root_loading` | `generated_child_dynamic_selection(false/true, 0)`: exactly two physical queries, one/two provider calls, all three states for note/extra, no intent, shared availability and generated JSON with optional root extra. |
| `malformed_dynamic_child_enhancement_fails_without_returning_partial_entities` | `generated_child_dynamic_selection(false, 1..5)`: all five faults return an error rather than a partial typed result, one provider call, no private canary. |
| `typed_query_batches_once_and_preserves_private_null_missing_and_ledgers` | `generated_typed_dynamic_batch(false)`: exact provider intent, three IDs, one statement/batch, states and snapshot sharing, independent ledgers, explicit augmentation and sibling preservation, selected values do not become native write intent. The recording executor observes exactly one request with projection `id`, `version`, `name` in that order before delegating to the original SQL executor. |
| `basic_typed_query_duplicates_and_result_lifetime_keep_the_same_contract` | `generated_typed_dynamic_batch(true)`: four rows, one batch including duplicate IDs, shared snapshot with distinct carriers; explicit Global deep copy survives actual result/Context destruction and rebind, while an ordinary source reference is rejected. |
| `malformed_provider_batches_fail_closed_without_returning_partial_rows_or_values_in_errors` | `generated_root_dynamic_boundary(1..5)`: every exact source error code and private-canary exclusion, no partial result and one provider batch. |
| `no_selection_needs_no_provider_and_streaming_cannot_silently_drop_a_selection` | `generated_root_dynamic_boundary(0)`: unselected rows have no dynamic fields/provider calls; selected query reports missing provider; selected stream cannot add a statement/provider call. |
| `dynamic_stream_rejects_before_cursor_even_when_a_provider_is_configured` | `generated_root_dynamic_boundary(6)`: installed provider does not permit selected dynamic streaming; zero statements and provider calls. |
| `composing_a_ledger_never_replaces_child_dynamic_payload_with_parent_values` | `generated_dynamic_ledger_case(true)`: child NULL/missing payloads and snapshots survive state attachment; child original IDs remain 2/3 and parent private text remains unchanged. |
| `dynamic_mutation_intent_is_explicit_row_local_and_cannot_be_silently_saved` | `generated_dynamic_ledger_case(false)`: incompatible layout rejects Set/Delete unchanged; NULL dirty key and dynamic-only ledger, sibling isolation, wrong type leaves full ledger unchanged; Delete changes availability; graph and both save entries reject before SQL without losing intent; explicit adoption, nested frame pop and clear preserve independent ownership. |

## Explicit adaptations and evidence boundaries

The original clone-after-Context-drop contract uses Rust shared ownership. The
approved C++ counterpart performs an explicit deep copy into Global storage and
checks rejection of the original Context reference after actual destruction.
Ordinary C++ values do not acquire implicit ownership.

SQLite executes real projections and stream cursors in place of `FixedRows`.
The readonly `_count` result is explicitly projected from the fixture table.
The generic service/executor/provider layers are exercised, rather than replaced
by behavior inside the example. Request projection, storage SQL and decoded
availability are distinct observations; one must not be silently substituted
for another during complete-source acceptance.

The [graph continuation](generated_graph_serialization_5_0_6.md) documents the
locked model evaluator's cycle restriction, `children` helper collision,
relation-NULL/FK precedence, explicit loaded SmartLists and boxed-storage roles.
These adaptations preserve the exercised graph semantics without claiming that
the unresolved naming collision or every raw-column decoder is accepted.

The original no-Clone node is replaced by generated models with explicitly
measured allocator-aware copying. Positive controls must observe a real copy;
JSON allocation itself is not claimed to be zero. Readonly allocation controls
observe both Context arena requests and ordinary/aligned C++ heap allocations.
Their scope is the measured reads, not all application/system allocations.

## Evidence and terminal result

The following focused documents link their original-test, C++ and provenance
receipts: [child selection](dynamic_child_selection_5_0_6.md),
[readonly streams](generated_readonly_stream_5_0_6.md),
[ledgers](generated_dynamic_ledger_5_0_6.md),
[root boundaries](generated_root_dynamic_boundaries_5_0_6.md),
[typed batches](generated_typed_dynamic_batch_5_0_6.md),
[allocation](generated_readonly_allocation_5_0_6.md),
[entity JSON](generated_entity_serialization_5_0_6.md),
[graph read metadata](generated_graph_read_metadata_5_0_6.md), and
[graph serialization](generated_graph_serialization_5_0_6.md).

The complete 23-test original binary and the three C++ executables pass in the
[focused installed receipt](parity/generated_graph_serialization_installed_5_0_6.json.gz),
including native/module/scoped-sanitizer checks. The
[wider SDK migration](generated_embedded_graph_sdk_migration_5_0_6.md) separately
tracks the new adapter across all 79 active SDKs and its integration gate.
The [closure gate](parity/dynamic_query_source_closure_installed_5_0_6.json.gz)
passes all 23 unchanged original tests and all three native/imported counterpart
programs after the projection correction. It requires the completed 79-SDK
integration receipt and verifies every prior captured input except the reviewed
test fixture is unchanged. Canonical consumers, binary hashes, no-work builds and
its own unchanged-input/reference guards pass. No fresh sanitizer or MIPS run is
claimed for this test-observer-only correction.

`ProjectionRecordingExecutor` is a test-only subclass of the real SQL executor.
It stores the incoming projection in UserContext memory and delegates the complete
request and observer to the existing implementation. Both ordinary and enhanced
typed queries must yield one exact ordered native projection, while the retained
SQLite/provider assertions continue to pass. It does not replace query execution
with a local implementation or change generated/production behavior.

[Source acceptance](parity/dynamic_query_source_acceptance_5_0_6.json.gz) records
only this complete 1,759-line integration-test source. Active source totals become
**93 accepted, 29 pending, two external exclusions**; capabilities remain
**11 qualified, six partial**. Successful dynamic persistence, containing
production sources, broader privacy/governance and P0–P7 exits remain separate.
