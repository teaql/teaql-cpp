# Query diagnostic tree traversal (Rust 5.0.6)

Status: all 28 direct original-source observations match native and module
consumers. Complete `resolved.rs` acceptance remains pending.

## Source responsibilities

This continuation executes the actual private `query_scoped_service` in the
complete locked runtime source, including its call to
`expand_query_facet_diagnostics`. The observer does not replace or reimplement
that traversal. A recording executor returns a synthetic private value for each
classified entity and rejects SQL/mutation execution.

Seven query shapes cross select metadata capture enabled/disabled and valid/
missing request intent: empty, relation, child enhancement, relationship
aggregate, object group, facet and their combination. Each nonempty branch has
a nested child; the relation shape also has a relation without an explicit query.
The parent starts with an existing privacy snapshot. Each observation captures
classification call order, the derived snapshot and intent, the unchanged parent,
exact error and structural equality of the original query before/after scoping.

The original observations establish:

- Invalid intent rejects before any executor classification, even with capture
  disabled. With valid intent and capture disabled, inherited privacy remains
  available but no query nodes are classified.
- The relation without a query is not classified. Each explicit relation, child,
  relationship aggregate and object group visits its nested child.
- Facets follow the existing diagnostic expansion semantics. In this fixture the
  facet's entity is classified but its child enhancement is not retained by that
  expansion; the observer preserves this result rather than inventing traversal.
- The mixed query visits root, group/nested, aggregate/nested, facet,
  child/nested and relation/nested in that order. The input query and parent
  privacy remain unchanged in all 28 cases.

This is an RQ26 caller witness. The separately accepted generated-support source
retains responsibility for the expansion transformation itself. Separate aggregate
arguments are not present in this fixture: their enhanced-entry diagnostic path
is covered by [generated private-field execution](dynamic_json_privacy_5_0_6.md)
and implementation readback. This witness does not claim database execution,
protected sink output, mutation policy, thread safety or full source acceptance.

## Evidence and reproduction

The [original receipt](parity/resolved_scope_tree_original_5_0_6.json.gz) records
one passing original test, 28 observations, original-body preservation and
successful cleanup of its 160,956,103-byte disposable source/build directory.
The canonical C++ consumer uses the same handwritten probe body. Its queries,
snapshots and redaction values use the UserContext; only external test call
recording uses ordinary test-process storage.

```sh
source scripts/toolchains/activate.sh
python3 scripts/observe_resolved_scope_tree_5_0_6.py --output <fresh-original.json.gz>
cmake --build --preset native --target teaql_resolved_scope_tree_probe -j 2
ctest --test-dir build_native_current -R '^resolved_scope_tree_rust_parity$' --output-on-failure
cmake --build --preset modules_native --target teaql_module_resolved_scope_tree_probe -j 2
python3 tests/parity/compare_resolved_scope_tree_5_0_6.py --cpp build_modules_native/modules/teaql_module_resolved_scope_tree_probe
```

The comparator checks complete case identity/order, the source/observer hashes
and executable identity. It does not treat compiler success as semantic parity.
No runtime or generator implementation change is made by this continuation.

The [native receipt](parity/resolved_scope_tree_native_5_0_6.json.gz) and
[module receipt](parity/resolved_scope_tree_modules_5_0_6.json.gz) each record
28/28 matching observations. The registered tree and prior snapshot CTest gates
pass 2/2. Canonical module verification passes for 1,129 consumer/fixture bodies.
No fresh sanitizer or MIPS gate is needed for these test-only changes. Stable
preset builds are reused; the only disposable original-source build was removed.
Source counts remain 112 accepted / 10 pending / 2 excluded_external.
