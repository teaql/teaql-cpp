# Embedded hydration sibling key snapshots

This checkpoint compares complete original `enhance_query_relations_internal`
through an append-only direct access bridge. Actual prepared child fetches reach
a recording executor, rather than a substituted planner or private fetch method.
The executor returns deterministic rows and captures query entity/filter state;
this is runtime execution evidence, not physical SQLite qualification.

## Actual source controls

Eight complete original/native observations vary independently:

- Ordinary versus nested descendant relation batches.
- A first relation alias that does or does not replace the shared local key.
- An explicitly requested inverse target with an empty filtered result.

Complete parent value maps and request filter diagnostics are compared and
reproduce **8/8**. The original methods, 610 archived file prefixes and registry
versions/checksums are retained. See the
[source provenance](parity/hydration_sibling_keys_5_0_6_source_provenance.json) and
[driver review](parity/hydration_sibling_keys_5_0_6_driver_review.json).

## Shared runtime correction

The first comparison found **4/8** differences, all with local-key aliasing.
C++ fetched later siblings using already attached object lists instead of the
original scalar keys, and consequently attached the wrong sibling results.
The [initial complete outputs](parity/hydration_sibling_keys_5_0_6_initial_gaps.json)
remain recorded. Ordinary controls, including explicitly requested inverse
projection, already agreed and are unchanged.

The embedded forest now captures only the local key fields needed by its plans
before fetching siblings. Each embedded node similarly captures descendant keys
before recursively attaching nested relations. Query dispatch and final parent
attachment use those stable key rows, while inverse convenience objects still
reflect the current parent payload as in the original runtime.

The existing attachment API remains available. A key-aware overload validates
row counts and Context domains and uses snapshots for matching. Flat and compact
flat mode behavior is unchanged. Snapshot containers and nested values use the
same UserContext resource; no lifetime extension or new allocation domain is
introduced.

## Modules and fixture corrections

The new native fixture initially included only the lower base/Context header;
the public runtime data-service umbrella provides the actual entity/hydration
definitions. That [compile failure](parity/hydration_sibling_keys_5_0_6_initial_cpp_error.json)
is retained.

The canonical import-only consumer is checked against the same complete corpus,
not an empty-input executable smoke. Its manifest intentionally has no standalone
command cases. Module CMake now skips an empty case array instead of iterating
indices 0 and -1; the [initial configure error](parity/hydration_sibling_keys_5_0_6_initial_module_configure_error.json)
is retained. Existing nonempty contract arrays are unchanged.

## Reproduction and acceptance limits

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_hydration_sibling_keys_probe
python3 tests/parity/replay_hydration_sibling_keys_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/hydration_sibling_keys_5_0_6 \
  --cpp build_native_current/teaql_hydration_sibling_keys_probe --check
```

Relevant native tests pass **9/9**, including existing generated facet and relation
analysis tests. Targeted ASan/UBSan passes **2/2** (leak scan disabled), import-only
module checks **3/3** and **111** canonical consumer/fixture bodies verify. Both
native and imported probes run all eight source observations. Pinned format/check
covers **995** handwritten files and module paths verify after terminal builds.
See the [terminal record](parity/hydration_sibling_keys_5_0_6.json).

The [binding inventory](parity/active_corpus_baseline_after_hydration_keys_5_0_6.json)
counts 137 active, 64 historical and 21 custom selections; static bindings do
not prove semantic acceptance. The historical 356-input hydration family,
flat graph, dynamic fields, facets, privacy, additional model configurations
and P7 remain open. All 17 full capabilities remain partial.
