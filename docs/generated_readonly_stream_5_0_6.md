# Generated readonly schema across lists, cache and streams

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/dynamic_field_query.rs`.

This is focused caller evidence for three unchanged source tests:

- `query_and_stream_carry_readonly_schema_without_installing_absent_values`
- `readonly_property_schema_survives_cache_miss_and_hit_without_extra_provider_calls`
- `root_stream_retains_one_snapshot_across_full_chunks_tail_and_cursor_drop`

The whole 23-test source remains pending. Existing ordinary/readonly stream
matrices do not alone demonstrate this generated entity, cache and full-chunk
composition.

## Counterpart and observations

`generated_readonly_stream_case` in
`teaql_runtime/tests/dynamic_relation_tests.cpp` uses actual generated
`cpp_examples::Product`, SQLite and the public entity data-service entry points.
The source uses macro-derived `School` and a fixed-row executor. Both models
supply the tested id/version/name scalars and indexed readonly carrier; model
names and physical layout identifiers are explicit adaptations.

| Case | Assertions |
| --- | --- |
| List followed by stream | Three list rows and three streamed rows retain declared `_missing: I64` with no value, presence or fixed loaded bit. Compatible rows share one snapshot. There is no persistent dynamic-field payload or dirty field, and no dynamic provider is installed. The stream uses chunks of two and one rows. |
| Cache miss followed by hit | Two enhanced typed reads each return three rows. All share compatible snapshot/schema identity, preserve the absent property's type and perform exactly one SQLite statement across both reads. `InMemoryAggregationCache` is installed explicitly in UserContext. |
| Full chunks, short tail and cursor close | All 1,003 rows retain ordered IDs, exact names, `_count` presence and alternating zero/NULL values, plus absent `_missing: I64`. Every row has the same indexed snapshot; readonly properties never acquire fixed loaded bits or mutation state. Chunk indexes are contiguous: thirteen chunks of 73 and one tail of 54, fourteen total. Exactly one SQLite statement runs. Rows 1, 2 and 1,003 remain readable with their schema and snapshot after cursor cancellation/closure. |

The fixed-row source executor returns `_count` directly. SQLite requires an
explicit result expression because `_count` is not a native modeled field. The
C++ fixture uses a trusted `project_raw("_count", "_count")` against its local
fixture column and explicitly orders by id. It does not add `_count` to the
model, relax unknown-field validation or change the source's value/null and
chunk assertions. The initial fixture's ordinary `project("_count")` failed
with `SQL compile error: unknown field: _count`; its source and failure evidence
are preserved in
`parity/generated_readonly_stream_fixture_failure_5_0_6.json.gz`. This was an
executor-fixture mismatch, not evidence of a runtime parity defect.

SQLite setup creates 1,003 rows for all modes; the first two query modes select
three via an explicit limit. A statement trace installed after setup distinguishes
cache hits from repeated database execution. Query/stream mechanics, schema
attachment and snapshot interning are production implementations; the test does
not reproduce those algorithms. No generated header is patched.

Retention is within the still-live UserContext after closing the cursor. It is
not implicit retention past Context destruction or a claim of zero allocations.
The existing seven dynamic-child cases, raw/flat relation controls, revision
conflict and attachment-key regressions continue running in the same executable.

## Verification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_readonly_stream_source_5_0_6.py \
  --record docs/parity/generated_readonly_stream_installed_5_0_6.json.gz
```

The controller requires a fresh receipt. It reproduces the complete example SDK,
runs each of the three original tests with an exact filter in the byte-checked
runtime view and locked dependency identities, then builds/runs the generated
counterpart under native and imported-module profiles. Disabled external-exporter
development dependencies are omitted. It checks the three-case/1,003-row/14-chunk
marker and retained seven-child-case marker, exact CTest selection, canonical
module consumers, binary hashes, no-work builds and unchanged inputs/references.
Each profile has one CTest registration; rows and chunks are internal assertions.

No production or generator implementation changed. Whole dynamic-query and
resolved-service source acceptance, remaining allocation/serialization/mutation
contracts and P0–P7 exits stay open. No fresh sanitizer, MIPS or physical-device
result is claimed.

## Terminal result

[Installed receipt](parity/generated_readonly_stream_installed_5_0_6.json.gz)
passes **original3, generated cases3, native1/module1**, including the full
1,003-row/14-chunk assertions and seven retained child-query cases. Complete
example SDK reproduction, canonical module consumers, no-work builds and
unchanged inputs/references pass. Source and capability verdicts stay unchanged:
**92 accepted / 30 pending / 2 excluded_external**, **11 qualified / 6 partial**.
