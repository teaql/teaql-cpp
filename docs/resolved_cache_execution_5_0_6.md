# Resolved aggregation-cache execution (Rust 5.0.6)

## Contract and observer

The complete locked `resolved.rs` is observed through an appended test module;
original source bodies and registry versions remain unchanged. The 288 cases
cross borrowed/owned/compact entry, no/concrete/custom/both cache registration,
enabled/disabled cache selection, hit/miss/provider failure, SQL metadata capture
and scoped/unscoped query privacy. Every query supplies valid comment and purpose;
the separate prepared-cache intent matrix retains invalid-intent coverage.

When both caches are registered, the concrete cache is deliberately preloaded,
including custom-cache miss and provider-failure cases. A custom hit returns an
empty-but-present row set; concrete hits and provider reads have distinct marker
rows. Thus a fallback to the concrete cache cannot accidentally look correct.
Cache age is zero (unbounded) to avoid timing-dependent expiration outcomes.
Actual TTL/store behavior has its separate cache-source qualification.

The observer records exact result values/errors, privacy-classifier invocation
counts and ordered telemetry/cache/provider effects. Cache and provider families,
cache success outcomes, error categories and the active execution-family path at
get/put/query are compared without normalization. Language-specific provider type
names, operation attributes, clock timing and telemetry drop/activation counts
are not emitted and are not claimed by this matrix. Telemetry SPI lifecycle and
fail-open adapter behavior retain their separately accepted source contracts.

The native probe invokes the existing `PreparedEntityQueryOperation`. Its canonical
module consumer imports the same implementation and test body. Instrumentation
is test-only; no runtime or generated implementation changes are intended.

## Source observations

The source records 72 cache scopes: 24 hits, 24 misses and 24 provider failures.
There are 48 nested provider scopes, with provider failure followed by cache
failure in all 24 failing cache loads. All 48 custom gets execute in the cache
scope; all 16 successful custom puts execute after provider success in the cache
scope. The other 216 calls bypass cache scopes, including all compact entries.

A scoped cache hit reclassifies privacy even with metadata capture disabled.
An unscoped hit does not invent a query-privacy container. With both backends
registered, custom misses query the provider instead of falling back to the
preloaded concrete cache. Provider errors propagate and do not produce a put.
These are controlled executor calls, not live SQLite statement observations.

## Qualification

- [Original observer](parity/resolved_cache_execution_original_5_0_6.json.gz): 288 complete-source observations, with all original bodies preserved.
- [Native](parity/resolved_cache_execution_native_5_0_6.json.gz) and
  [module](parity/resolved_cache_execution_modules_5_0_6.json.gz): 288/288 matches each.
- Native CTest: new cache execution parity, prepared-cache intent parity,
  readonly query-schema parity, generated aggregation-cache Context contracts
  and telemetry contracts, 5/5 passed.
- Canonical module verification: 1,133 consumer/fixture bodies. Pinned
  formatting/check: 1,797 handwritten C++ files. Locked-source inventory,
  module paths, generator ledger and whitespace checks pass.

The [checkpoint](parity/resolved_cache_execution_checkpoint_5_0_6.json.gz) retains
JUnit, the complete native test log, receipt hashes and cleanup evidence. The
owned Rust source/build view was removed (161,048,462 bytes). Stable native and
module preset outputs remain available; no new sanitizer/MIPS or physical-device
run is claimed. Runtime and generator implementations are unchanged.

This is scoped RQ14/RQ15 evidence, not complete resolved-source, generated
configuration or P0-P7 acceptance. Backend panics,
suspended execution, schema attachment and public Behavior/Policy preparation
remain separate evidence obligations; a cache backend exception must not be
inferred to be fail-open merely because telemetry adapters are fail-open.
