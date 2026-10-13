# Generated aggregation cache execution: Rust 5.0.6

Status: fixed-model generated cache execution and ownership checkpoint verified. Full G08/P5 remains
partial; this checkpoint does not resolve the separate quoted-Line attachment
question or qualify all cache/concurrency/trace configurations.

## Fixed source and implementation

Runtime v5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and producer
`45e1572c4956ca963410d8ea436ead3c8857087a` remain locked. The unchanged complete
generated dynamic_json_rust crate supplies actual Q methods and execution.
Recording backend wrappers delegate get/put/invalidate calls to the original and
C++ InMemoryAggregationCache implementations; they do not replace caching logic.
All database operations use the existing fixed FK-enabled SQLite fixture.

The source fetch_prepared_all_with_aggregates takes query-owned relation aggregate
declarations out of the base execution query, then merges explicit declarations
without adding identical duplicates. Source query-owned duplicates remain; only
duplicate explicit transport is excluded. C++ EntityRowsOperation now performs
this normalization at its owned snapshot boundary. It preserves the reusable
caller query and existing independent service/executor/provider roles.

This fixes a real semantic difference: C++ previously retained child aggregation
metadata in the base-row cache key. Changing a child filter then missed the base
cache and issued extra SQL, unlike the fixed runtime. The fix does not rewrite
cache observations or omit key fields in the canonicalization function.

## Evidence scope

The source matrix observes literal cache keys, namespace, requested max age,
hit/miss, put and invalidation events, actual typed relation metric values, and
cumulative SQL/parameters/comment/purpose. It covers count and sum, absent cache
backends, root/child enablement, TTL changes, default and explicit propagation,
explicit child overrides, empty roots, comments, purpose, filters and clear.
Direct SQLite updates demonstrate stale cached values until expiry or clear;
governed generated saves exercise namespace invalidation independently.

Zero TTL means no expiry. Four real-clock scenarios use a wide 30 ms separation
against a 1 ms maximum age; they do not claim exact scheduling at the expiry
boundary. Cache hits return copied rows and never synthesize SQL-success logs.
The source's purpose-sensitive keys and other actual key rules are retained.

Context tests check independent get results, namespace invalidation, explicit
Global cache ownership after source UserContext destruction, destination-scope
nested payloads, retained results after invalidation, and repeated actual SQLite
execution with identical core/explicit aggregate transport. A null default PMR
resource ensures allocator propagation. Global caches are application-owned
resources, never hidden singletons or implicit ownership extensions.

Artifacts:

- tests/parity/rust_field_layout/src/bin/generated_aggregation_cache_5_0_6.rs
- tests/parity/generated_aggregation_cache_probe.cpp
- tests/parity/generate_generated_aggregation_cache.py
- tests/parity/compare_generated_aggregation_cache.py
- tests/parity/generated_aggregation_cache_5_0_6_cases.json.gz
- teaql_runtime/tests/generated_aggregation_cache_tests.cpp

The source corpus contains 204 exact scenarios and 588 actual generated SQLite
executions, including 20 governed-save scenarios, literal event/key comparisons,
missing-backend behavior and four expiry/no-expiry cases. The mutation uses a typed
Decimal value in both APIs, preserving SQL parameter provenance without rewriting
logs. All observations match without normalization. The observed events include 756 get calls: 394 hits and 362 misses, with 362 puts
and 20 entity-namespace invalidations. Native CTest passes 5/5, targeted ASan/UBSan
3/3 with leak scanning disabled, and actual module facet/relation execution 1/1.
Formatting checks pass for 928 handwritten C++ files; locked reference/export/path
integrity and diff checks pass. All 17 capability records remain unqualified.
No generator or application API changed, so Java/compiler/reproduction gates from
the prior checkpoint were not rerun. The pinned SQLite archive is not sanitizer
instrumented. No MIPS,
physical-router or GPU adapter execution is claimed. Full runtime-scope telemetry,
privacy/trace provenance, replacement while pending, additional model/nested
configurations and complete cache governance remain separate qualification work.
