# Flat graph and typed operand integration repair

The full native build completed all 1,067 steps. Its subsequent 658-test run
finished with 654 passes and four failures. The
[raw initial build/test evidence](parity/native_integration_before_runtime_configuration_5_0_6.json.gz)
and [pre-repair sources](parity/flat_graph_before_legacy_boundary_repair_5_0_6.json.gz)
are retained. This is an integration follow-up, not full G06/G08/G13/G15 or P7
acceptance. References remain locked Rust 5.0.6 and generator 45e.

## Actual failures and source responsibilities

The loaded-row graph comparison exposed a real runtime regression: the existing
three-argument `detail::attach_flat_relation_graph` had acquired the scoped view
behavior needed by generated query composition. Rich forward objects were now
published as owner views, and missing/invalid parent IDs failed before the
source's identity-table operation. The complete original
`ResolvedEntityDataService::flatten_relation_graph` body in
[resolved.rs](../.reference/teaql-rs/teaql-runtime/src/data_service/resolved.rs)
requires different handling of ordinary forward details, identity-only forward
references and reverse relations.

The shared private flattener now selects two explicit protocols:

- `IdentityGraph` preserves the original three-argument contract. It traverses
  children before key rewriting, retains source facet/NULL ordering, requires
  owner IDs for reverse and identity-only views, deduplicates rich forward
  entities in the identity table and preserves the original load markers/errors.
- `ScopedViews` preserves the five-argument C++ composition entry. It retains
  the caller's graph/root and independent projection views, scoped facets and
  nested relations required by the qualified generated analysis adapters.

Both share descriptor lookup, relation extraction and child conversion. The
existing public signatures, ownership and service/provider separation remain.
Do not use a scoped composition repair to change the identity-only protocol.

The aggregation-cache comparison had identical values and cache events but two
different binding variants. The original observer uses `with_id_is(10_u64)`;
the C++ fixture used `withIdIs(10)`. Restored source-shaped generic Value inputs
now correctly preserve that C++ signed literal. The fixture explicitly supplies
`std::uint64_t(10)`, matching its original input rather than changing the runtime's
generic conversion behavior.

The full compiler gate also retained an obsolete requirement that
`withNameIs(42)` must fail. The original scalar predicate admits `Into<Value>`;
that call is now a positive contract. The separate unconvertible-object negative
contract remains and passes. No source API is removed to satisfy the old refusal.

## Terminal focused evidence

After successful compiler/linker termination, native and selected ASan/UBSan
selections each pass **7/7**, with leak scanning disabled. Module ownership and
graph boundaries pass **4/4**, and imported consumers reproduce all **2,032**
loaded-row graph, **2,808** matching/statistic and **1,944** relation-analysis
observations. The immutable corpora remain unchanged. Native cache comparison
additionally restores all **204** outcomes. The updated operand compiler pair
passes one positive and one genuinely unconvertible negative.

The bootstrap extension replay now passes alone in **113.20 seconds** under its
unchanged **180-second** limit after the other heavy checks completed. The
[isolated result](parity/bootstrap_timeout_isolated_replay_5_0_6.json.gz) is
separate from the retained full-suite timeout. The complete updated compiler
gate is running alone and still needs its terminal result. Focused green checks
do not relabel the initial full suite or accept every capability and integration
requirement.
