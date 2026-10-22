# Prepared aggregation-cache intent order (Rust 5.0.6)

Status: the original 18 cases reproduced eight native differences. Repaired
native and module consumers match all 18; public-entry regressions pass.

## Source boundary

The locked `data_service/resolved.rs` methods `fetch_prepared_query`,
`fetch_prepared_query_owned` and `fetch_prepared_compact_owned` are private
prepared-query paths. Borrowed/owned paths first select the aggregation cache.
A hit returns cached rows without constructing a provider request; a miss obtains
request intent before starting provider execution. Compact queries bypass this
cache. Public query entries validate intent during preparation before reaching
these methods. This distinction does not authorize an application to omit intent.

An append-only full-source observer exercises all 18 combinations of borrowed,
owned and compact entry; cache hit/miss; and missing comment, missing purpose or
valid intent. An application-registered cache records get/put and can return an
empty-but-present hit. A recording executor counts actual query calls. Original
runtime methods and inputs remain unchanged.

The [original receipt](parity/resolved_cache_intent_original_5_0_6.json.gz)
records exact errors, row counts and cache/provider side effects. Its isolated
source/build tree was removed: 160990211 bytes, with cleanup
confirmed. No external cache provider was added.

## Reproduced difference and repair

The [native pre-repair receipt](parity/resolved_cache_intent_native_before_5_0_6.json.gz)
matches 10/18. In four invalid-intent miss cases C++ rejected before the cache get,
whereas Rust performed the get before rejecting. Four invalid-intent hit cases
returned rows in Rust but were rejected in C++. The compact cases already match.
The initial C++ fixture needed the mandatory executor `capabilities()` override;
its compilation failure and absent-binary comparison attempt establish no behavior.

`PreparedEntityQueryOperation` now validates/binds intent after a failed cache
lookup, before provider-scope creation and request execution. Cache hits preserve
the preexisting bound scope and current-query privacy classification. Public
entry preparation, provider intent requirements, cache ownership and cancellation
algorithms are unchanged. The [repaired native receipt](parity/resolved_cache_intent_native_repaired_5_0_6.json.gz)
matches all 18 observations.

## Qualification

```sh
source scripts/toolchains/activate.sh
python3 scripts/observe_resolved_cache_intent_5_0_6.py --output <fresh-receipt.json.gz>
cmake --build --preset native --target teaql_resolved_cache_intent_probe -j 2
python3 tests/parity/compare_resolved_cache_intent_5_0_6.py --cpp build_native_current/teaql_resolved_cache_intent_probe
```

The comparator binds original source/observer hashes and executable identity.
The full-source intent behavior is a private-method contract, not an exception to
mandatory application intent. The registered public query/capture, schema/cache
and complete enhancement regressions pass: four native CTest gates, plus module
comparisons of 768 query-capture, 2,304 schema/cache and 754 enhancement cases.
The [module receipt](parity/resolved_cache_intent_modules_repaired_5_0_6.json.gz)
records 18/18 matching observations; canonical consumer verification passes
for 1,127 bodies. The focused execution-order repair is qualified.
No fresh sanitizer/MIPS result is claimed for this execution-order change.
Whole `resolved.rs` acceptance remains pending; source counts stay 112 accepted /
10 pending / 2 excluded_external.
