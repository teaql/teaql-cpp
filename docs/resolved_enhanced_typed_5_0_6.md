# Enhanced typed query scope timing (Rust 5.0.6)

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
`teaql-runtime/src/data_service/resolved.rs`.

## Reproduced difference

The public ordinary enhanced entry scopes an explicit diagnostic source, if
present, before preparation. Otherwise an unbound service prepares the query,
then establishes its query scope. The explicit relation-aggregate and owned
entries establish their scope before preparation even with an empty aggregate
argument. A bound request intent and a diagnostic privacy accumulator are separate
states; capture-disabled execution still requires the correct intent scope.

C++ previously delegated the ordinary entry to the explicit aggregate entry.
Policy rewrites therefore could not affect the ordinary entry's scoped intent,
and classification observed the incoming dynamic selection instead of the
prepared selection.

The [original-source replay](parity/resolved_enhanced_typed_source_5_0_6.json.gz)
contains 3,456 inputs across all three entries, incoming selection, Behavior and
Policy selection changes, policy intent rewrites, capture/cache switches, empty
rows, SQL failure and dynamic-provider failure. It reuses the handwritten indexed
carrier and ready provider from the ordinary typed continuation.

The [corrected native fixture before repair](parity/resolved_enhanced_typed_before_5_0_6.json.gz)
has 704 differences, all in the ordinary enhanced entry: 576 request differences,
416 classification differences and 72 dynamic-provider intent differences
(overlapping categories). Both explicit entries match.

An earlier [fixture comparison](parity/resolved_enhanced_typed_fixture_trace_difference_5_0_6.json.gz)
used C++ `comment()` to rewrite a field while Rust assigned the field directly.
That C++ API also appends a trace node. Both fixtures now perform direct field
assignment; the earlier 1,568 differences are not a runtime defect count.

## Runtime correction

The ordinary public entry uses the existing optional-diagnostic-source adapter.
Its operation prepares the query and identity projection before establishing a
scope when no request intent is bound. Explicit aggregate and owned entries retain
their unconditional public scope adapter. The shared operation still handles all
three entries, and separate aggregate arguments remain diagnostic children when
building a scope. Data-service, executor and provider boundaries are unchanged.

For an initial intent `C0` rewritten by Policy to `C1`, the ordinary entry sends
`C1` to SQL and dynamic loading; the explicit entries retain their bound `C0`.
The original source establishes this distinction. Tests assert it directly rather
than relying only on whole-output equality.

## Verification scope

The [installed receipt](parity/resolved_enhanced_typed_installed_5_0_6.json.gz)
records a fresh original-source replay and 3,456/3,456 matching observations in
both native and imported-module frontends. Native12/module12 pass, including
retained ordinary typed/dynamic/schema cases, query-entry/cursor protocols,
nonempty enhanced aggregates, membership snapshots and Context cleanup.
Canonical module consumers and no-work rebuilds pass; inputs stayed unchanged
throughout the verifier run. Formatting, baseline and module-path checks pass.
There is no fresh sanitizer, MIPS or physical-device qualification in this epoch. The source matrix observes 6,048 SQL
requests, 864 dynamic-provider calls, 13,824 ordered callbacks and 4,320 query
classifications. Its explicit assertions cover entry-specific intent binding,
prepared selection, cache query counts, loaded values and error ordering.

These are direct enhanced reads with empty aggregate arguments. Nonempty graph
and aggregate execution, pagination and Context cleanup retain separate regression
programs; this matrix does not independently qualify every such composition,
whole `resolved.rs`, generated APIs or P0–P7. G13 remains partial.
