# Graph relation facet storage and readback

Rust 5.0.6 stores query-only facet metadata separately from relation lists and
optional entities. A loaded null to-one can therefore expose facets without
changing its cardinality. This metadata belongs to a frozen read graph, never
to original mutation snapshots or mutable ledger ownership.

## Runtime implementation

`EntityGraphBuilder::install_relation_facets` installs facet maps by canonical
owner entity, owner ID and relation name. Empty maps do not overwrite previous
metadata. Nonempty replacement maps replace the whole previous facet map.
`install_relation_list` also registers its nonempty facets, matching the source;
installing a later empty-facet list preserves previous separate metadata.

`EntityRuntimeState::relation_facet` reads one compact SmartList without I/O or
ledger materialization. Missing graph, owner, relation or facet returns no value.
Facet lookup does not create a relation view. Relation lists and loaded null
options keep their existing load states. Fresh runtime views share the frozen
graph while keeping independent mutable ledgers.

The C++ graph has one optional Context-allocated facet index. It materializes
only for nonempty metadata. Nested maps, lists, compact rows and strings inherit
the graph's allocator. Explicit destination copies deep-copy the index along
with existing graph content. These APIs preserve the existing two ownership
domains; there is no new allocator domain or implicit shared ownership.

## Original source observations

The observer directly uses the unchanged locked source's public graph builder,
freeze, relation facet and fresh-state APIs. It does not reconstruct these Rust
algorithms or append test access to the source. The [source provenance](parity/graph_relation_facets_5_0_6_source_provenance.json)
records the clean reference commit, original file hashes, external driver,
locked dependencies and completed source binary.

The 72 stimuli vary owner spelling, explicit/list installation, empty/populated
results, empty updates, replacements and owner IDs. Each checks 36 owner/key/
relation/facet combinations before freeze, after freeze and in a fresh graph
view, together with independent comments and loaded null option behavior.

The first source build tried to call `has_mutation_context`, which is private
and compiled only inside original source tests. Its [failure](parity/graph_relation_facets_5_0_6_initial_source_error.json)
is retained. The external observer uses public graph/ledger behavior instead.
The first C++ fixture also contained redundant allocator arguments in a JSON
vector insertion; its [compile failure](parity/graph_relation_facets_5_0_6_initial_cpp_error.json)
is retained. Initial output differences are retained separately; extra fixture
fields must be removed at construction, never hidden by comparison normalization.

The new comparison entry initially discarded the failure boolean returned by
`compare_values.main`. Initial CTest parity passes therefore do not count as
acceptance. The [comparator failure review](parity/graph_relation_facets_5_0_6_initial_comparator_error.json)
records the correction to `SystemExit`, an actual known-mismatch exit code of
1 and an AST audit finding no other such comparator. Final gates rerun the
complete corpus after correcting the extra fixture field.

## Terminal verification

The corrected fixture reproduces **72/72** complete original/native outputs.
Native checks pass **4/4**, targeted ASan/UBSan **3/3** (leak scan disabled), and
import-only module checks **2/2**. All **114** canonical consumer/fixture bodies,
**1,477** catalog exports and module paths verify; pinned format/check covers
**1,000** handwritten files. Terminal compiler/linker exit codes precede every
execution. See the [terminal record](parity/graph_relation_facets_5_0_6.json).
The [static corpus inventory](parity/active_corpus_baseline_after_graph_relation_facets_5_0_6.json)
counts 140 active, 64 historical and 21 custom selections; it proves bindings,
not semantic acceptance.

## Lifetime integration and remaining work

A native contract installs a long-string facet alongside a loaded null relation,
checks the pristine mutation ledger and explicitly copies the runtime graph to
GlobalContext. The copied nested payload is independent and remains readable
after actual UserContext close and destruction. The default PMR resource is
disabled. Another assertion ensures query-only loaded facets are excluded from
original snapshots.

This checkpoint implements storage and readback. It does not yet connect flat
or compact-flat hydration dispatch to that storage. Source paths in
`data_service/relation.rs` and loaded-row graph reconstruction in `resolved.rs`
require separate query execution/ordering/error/cancellation qualification.
JSON graph view coordinates and generated model facet APIs also remain open.
All full capabilities and P7 remain partial.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  RUSTFLAGS='-C linker=/usr/bin/cc' cargo build --offline --locked \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin graph_relation_facets_5_0_6
cmake --build build_native_current --target teaql_graph_relation_facets_probe
python3 tests/parity/replay_graph_relation_facets_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/graph_relation_facets_5_0_6 \
  --cpp build_native_current/teaql_graph_relation_facets_probe --check
```
