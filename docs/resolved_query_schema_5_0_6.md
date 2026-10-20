# Query-result schema counterexamples

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

This is an investigation checkpoint, not a completed repair. The complete locked
Rust runtime supplies 2,304 observations: 768 retained controls and 1,536 new
readonly-schema, result-type, logging, cache and backend controls. Both native
and imported-module C++ probes reproduce 288 differing inputs each. There are
256 inputs with result-schema differences and 48 with classification differences;
16 inputs exhibit both. Cache-event observations agree.

The C++ prepared query path does not attach supplied readonly property definitions
to returned rows. The logging-disabled aggregation cache-hit path also omits
classification observed in the reference. The ordinary typed-list decoding path
needs separate investigation; no typed-path acceptance follows from this corpus.

G09 is reopened as partial under RQS01. Previous execution and cursor acceptance
receipts remain historical evidence. Current capability totals are 10 qualified
and 7 partial; source totals remain 84 accepted, 38 pending and 2 excluded.
No production repair for these counterexamples is included in this checkpoint.

Evidence:

- `parity/resolved_query_schema_source_5_0_6.json.gz`: complete source execution.
- `parity/resolved_query_schema_before_5_0_6.json.gz`: reproduced differences on
  both installed frontends.
- `parity/resolved_query_schema_fixture_compile_failure_5_0_6.json.gz`: retained
  initial fixture compilation failure, corrected before the comparison.

The `resolved_query_schema_rust_parity` and
`module_resolved_query_schema_rust_parity` tests intentionally remain failing
regressions until the runtime is repaired. Other passing focused cohorts do not
establish full-suite acceptance.

To reproduce the native counterexamples with an already built probe:

```bash
source scripts/toolchains/activate.sh
python3 tests/parity/compare_resolved_query_schema_5_0_6.py \
    --cpp build_native_current/teaql_entity_query_probe
```

A nonzero result is expected at this checkpoint. Repair must preserve cache
storage ordering, Context ownership and the separate resolved/base-service
responsibilities, followed by relevant regression and lifetime checks.
