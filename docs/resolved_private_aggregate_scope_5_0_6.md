# Private typed-aggregate diagnostic scope (Rust 5.0.6)

## Scope

The reference is the clean locked Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, with `resolved.rs` SHA256
`f15e11d112c32c3a076d35d7b548a2afe33f4a49236e61b454ea0fe57433f6d8`.
This continuation resolves the erroneous readback in the earlier
[resolved review](runtime_resolved_source_review_5_0_6.md). The private typed
entry calls `query_scoped_service_with_aggregates`, not the non-aggregate helper.
The source and C++ both include separate aggregate arguments in a diagnostic
clone when metadata capture is enabled. No runtime repair is justified by that
previous hypothesis.

## Direct observer

An appended Rust observer calls the actual private
`fetch_entities_with_relation_aggregates_internal::<Parent>` body. Original
runtime bodies remain byte-identical. The native fixture calls the corresponding
`EntityDataServiceAccess` entry with its existing typed carrier. Eight cases
cross dynamic selection, metadata capture and bound request intent. Each records
classification order, the original service's intent/privacy snapshot and the
terminal error. A separately supplied aggregate targets `AggregateSecret`.

The metadata fixture intentionally lacks the parent descriptor. All cases stop
at that boundary, after any initial classification; this is not successful
provider execution, dynamic-field hydration or whole-source acceptance. The
executor forbids mutations. No production or generated source is modified.

The first original observer failed to compile because it named
`DynamicFieldSelection` at the crate root instead of `teaql_core::dynamic_fields`.
Its incomplete receipt is retained separately and is not behavioral evidence.
The corrected observer and its successful receipt must have matching hashes.

## Verification

- [Original observer](parity/resolved_private_aggregate_scope_original_v2_5_0_6.json.gz): eight outcomes; original bodies preserved.
- [Native comparison](parity/resolved_private_aggregate_scope_native_5_0_6.json.gz): 8/8 exact matches.
- [Module comparison](parity/resolved_private_aggregate_scope_modules_5_0_6.json.gz): 8/8 exact matches.
- Native CTest: new scope parity plus existing typed-aggregate Context and source
  parity contracts, 3/3 passed. The [checkpoint](parity/resolved_private_aggregate_scope_checkpoint_5_0_6.json.gz)
  retains JUnit, the test log, receipt hashes and cleanup observations.
- Canonical module verification: 1,131 consumer/fixture bodies. Locked-source
  inventory, module paths, generator ledger and whitespace checks pass.
- Pinned clang-format formats/checks 1,796 handwritten C++ files.

Classification is `[Parent, AggregateSecret]` only when metadata capture is
on and the service has no bound intent, independently of dynamic selection.
Other cases classify nothing before the missing-descriptor failure. Parent
intent/privacy snapshots remain unchanged in all eight cases. This private-entry
behavior is distinct from the public wrapper's mandatory complete scoping.

Both owned Rust build directories were removed: 134,717,907 bytes from the
failed compile and 161,523,128 bytes from the successful observation. Stable
native/module preset outputs remain available for subsequent work. No fresh
sanitizer, MIPS, physical-router or generator reproduction run is claimed.
The source disposition remains `pending_review`; G09's prior qualification is
unchanged and P0-P7 are not complete.
