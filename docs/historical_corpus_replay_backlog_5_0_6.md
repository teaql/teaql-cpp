# Active historical corpus replay backlog

Status: this initial backlog checkpoint is preserved below. Its running-build
and staged-fix statements describe that earlier checkpoint, not the current
state. The subsequent full CTest is terminal; raw-LIKE, audit-event and row
corrections are applied. See [owner facet checkpoint](hydration_owner_facets_5_0_6.md) and
the [current binding inventory](parity/active_corpus_baseline_after_owner_facets_5_0_6.json):
138 direct active-baseline selections, 64 historical selections and 21 custom
dispatchers remain to be reviewed. Active bindings do not prove complete parity.
All complete capabilities and P7 remain open.

## Initial active selections

The initial native CTest registry had **499** tests. A static inventory of its
**214** parity-script entries finds:

| Direct corpus binding | Entries |
| --- | ---: |
| Active 5.0.6 baseline | 62 |
| Historical baseline requiring actual source replay | 131 |
| Custom dispatcher requiring independent review | 21 |

The [machine-readable inventory](parity/active_corpus_baseline_inventory_5_0_6.json)
records the exact selected comparator, corpus, baseline, observation count and
file hash for directly resolved adapters. Active baseline means only that the
corpus commit matches the lock; it does not prove live source provenance or
behavioral agreement. Historical evidence is preserved. Custom dispatchers may
already have valid versioned evidence; the tool deliberately does not guess
their selection/projection rules. They remain unresolved by this inventory.

`inventory_parity_baselines.py --check` fails for historical/missing corpora and
unresolved dispatchers. Ordinary inventory mode succeeds without claiming gate
acceptance. This provides a concrete migration queue alongside the complete
CTest terminal report, rather than treating baseline rejection as parity.

```sh
source scripts/toolchains/activate.sh
ctest --preset native --show-only=json-v1 > /tmp/teaql_native_current_registry.json
python3 scripts/inventory_parity_baselines.py \
  --registry /tmp/teaql_native_current_registry.json \
  --report docs/parity/active_corpus_baseline_inventory_5_0_6.json
```

## Additional original-call replays

Three historical input families now have separately versioned actual Rust 5.0.6
corpora:

| Family | Original inputs |
| --- | ---: |
| Standalone DataType and typed-NULL structural formatting | 10 |
| Entity default hooks and legacy dynamic mutation sequences | 102 |
| SafeExpression/TeaqlEmpty evaluation and callback traces | 448 |

All **560/560** original observations reproduce in a second live run. Original
input order is unchanged, no expected output is normalized, and old corpora and
drivers remain intact. The DataType driver is byte-identical. The other two only
change the relative import of the existing shared fixture module to compile in
the active observer workspace; their original calls and logic are unchanged.
The general replay tool saves historical corpus and adapted driver hashes with
the exact active lock, and can compare C++ before writing or check existing data.

These are original runtime calls, not the complete generated Q/E/model matrix.
After the complete native build succeeds, all three families match actual Rust
plus native C++ (560/560). Six more families bring the source replay to 4,007
observations: eight whole families match 2,918/2,918; the query family exposes
42 raw-LIKE AST differences, with an isolated correction passing 1,089/1,089.
See [query_raw_like_source_gap_5_0_6.md](query_raw_like_source_gap_5_0_6.md).
Active CMake tests still select historical comparators during the running full
CTest; their terminal outcomes must be retained before selection migration.

```sh
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family datatype_debug --family entity_defaults_api --family safe_expression_api \
  --rust-dir /tmp/teaql_cpp_field_layout_rust_506/debug --check
# After compilation exits successfully:
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family datatype_debug --family entity_defaults_api --family safe_expression_api \
  --rust-dir /tmp/teaql_cpp_field_layout_rust_506/debug \
  --cpp-dir build_native_current --check
```

The inventories/replays do not alter runtime dependencies, generators, production
C++ or the current native build inputs. No sanitizer, module, MIPS or complete
source capability acceptance is inferred from them.
