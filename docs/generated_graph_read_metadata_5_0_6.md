# Graph decoder read metadata and plain-carrier defaults

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/dynamic_field_query.rs`.

This continuation maps two original tests:

- `entity_without_derived_carrier_keeps_nullable_defaults_without_claiming_presence`
- `graph_decoders_retain_dynamic_read_metadata_without_promoting_it_to_columns`

The counterparts are in `teaql_runtime/tests/dynamic_relation_tests.cpp`.
They deliberately exercise different carrier contracts.

## Plain-carrier defaults

`plain_carrier_readonly_defaults` decodes id/name through the retained
handwritten `example::Product`, which inherits the production trait defaults
and has no readonly property carrier. It checks the decoded scalar values,
`supports_dynamic_property_load() == false`, and absent values/presence for
`_missing`, native `name` and persistent `#name`.

The original derives a plain id/name struct without the runtime carrier macro.
The handwritten example is an explicit adapter for that absence contract; it
is not claimed as generated API evidence. The original example implementation
is unchanged.

## Generated graph read metadata

`generated_graph_read_metadata` uses actual generated `cpp_examples::Product`
as the counterpart to macro-derived School. Three shared-layout compact rows
carry Text, NULL and NotLoaded dynamic `note` values. The trusted read metadata
is separate from native id/version/name columns.

The test checks the complete original sequence:

1. Installing dynamic read metadata leaves each row at three native columns;
   converting to a native map does not create a `#note` column.
2. `decode_compact_rows_with_read_metadata` preserves all three states.
   Text and NULL share availability, missing uses a different snapshot, and
   no row has pending dynamic mutations.
3. The registered generated decoder executes batch, list, option and single
   compact-row entry points against one graph builder and runtime state.
4. After freezing the graph, the resolved three-member list retains its
   private value, NULL and NotLoaded state with shared/separate snapshot
   identities as appropriate.
5. Both root entity lookup and option-relation lookup retain the private value.
   Generated JSON serialization emits `#note` from each.
6. Clearing loaded relation metadata from a compact-row copy also removes its
   dynamic read metadata.

The synthetic `JsonGraphRow/77` owner and `children`/`single_child` edge keys
are retained from the original. Registering Product supplies actual generated
decode and serialization adapters; no generated header is patched. The test
calls production hydration and graph operations rather than rebuilding them.
All graph borrows stay within the live UserContext.

## Verification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_graph_read_source_5_0_6.py \
  --record docs/parity/generated_graph_read_installed_5_0_6.json.gz
```

The controller requires a fresh receipt. It reproduces the example SDK and runs
the two unchanged original tests with exact filters in the byte-checked runtime
view and locked dependency identities. Disabled external-exporter development
dependencies are omitted. Native and imported-module checks run the two
counterparts and retain all child-selection, readonly-stream, dynamic-ledger,
root-boundary and typed-batch markers.

Exact CTest selection, canonical module consumers, binary hashes, no-work
rebuilds and unchanged input/reference guards are checked separately.
This does not accept borrowed cyclic graph serialization, nested relation
projection, sibling-edge isolation or the entire source.

## Terminal result

The [installed receipt](parity/generated_graph_read_installed_5_0_6.json.gz)
passes **original2, counterparts2, native1/module1**. SDK reproduction,
canonical module consumers, no-work rebuilds and unchanged input/reference
guards pass. All retained executable markers pass. Formatting and format
checks cover 1,726 handwritten C++ files.

Eighteen original test identities now have focused continuation receipts;
the unsupported-carrier case is explicitly handwritten, while this graph case
uses generated Product. Five original tests remain: borrowed cyclic graph JSON,
borrowed reverse graph JSON, sibling-edge detail isolation, nested extension
and mutation-metadata filtering, and selected-null/empty/unloaded relations.
Source totals remain **92 accepted / 30 pending / 2 excluded_external** and
capabilities **11 qualified / 6 partial**. No fresh sanitizer, MIPS or
physical-device execution is claimed.
