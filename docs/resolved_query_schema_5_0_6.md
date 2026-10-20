# Query-result schemas and resolved typed decoding

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: prepared compact/query results, aggregation-cache returns, query-scope
classification and ordinary resolved typed lists in `data_service/resolved.rs`.
G09 has supplemental acceptance under its existing contract; the complete
resolved-service source and cross-capability integration remain pending.

## Reproduced counterexamples

The complete locked Rust runtime supplies 2,304 observations: 768 retained
controls and 1,536 readonly-schema, result-type, logging, cache and backend
controls. Before repair, both native and imported C++ frontends differ on 288
inputs each: 256 result-schema differences and 48 classification differences,
with 16 inputs exhibiting both. Cache-event observations agree.

An additional 192 original-source observations exercise ordinary typed lists.
The native pre-repair probe differs on all 192 provider-request observations,
including empty and failed results. There are also 32 differing result-schema
observations. This supplemental witness reuses the existing handwritten entity
contract; it does not imitate a generated model or alter original runtime bodies.

The original source and both pre-repair comparisons are retained:

- `parity/resolved_query_schema_source_5_0_6.json.gz` and
  `parity/resolved_query_schema_before_5_0_6.json.gz`.
- `parity/resolved_query_typed_schema_source_5_0_6.json.gz` and
  `parity/resolved_query_typed_schema_before_5_0_6.json.gz`.

The earlier fixture compilation failure and supplemental input-count assertion
failure remain separate historical receipts; neither is acceptance evidence.
`parity/resolved_query_schema_gate_registration_failure_5_0_6.json.gz` records
an intermediate gate failure: all 45 native and 17 registered module tests passed,
but the controller correctly rejected the missing module capture test. Explicit
registration was added before the complete 45/18 replay; the required selection
was preserved.

## Shared runtime correction

`PreparedEntityQueryOperation` now attaches the supplied readonly property
definitions to returned rows, including compact/owned execution and cache hits.
It stores the original provider rows in the cache before attaching the current
query's schema. It does not store request privacy in the shared cache.

`EntityRowsOperation` now creates the request scope independently of logging
capture, using the presence of bound request intent. Explicit aggregate entries
create that scope before SELECT preparation; ordinary enhanced entries create
it after preparation and aggregate normalization. A scope is not recreated merely
because a bound request lacks query privacy. Diagnostic classification still
respects capture settings; cache-hit classification can use the existing scope
when capture is disabled.

Ordinary typed lists now use the resolved prepared-compact operation followed by
the existing shared metadata decoder with a fresh Context-owned runtime state.
They no longer delegate this responsibility to the base/context typed query
path. The base service, Context service, provider and generated APIs retain their
independent contracts. The later [prepared-selection continuation](resolved_dynamic_typed_5_0_6.md)
generalizes that existing operation to ordinary typed lists, qualifies callback
selection and cache bypass, and removes the duplicate private decoder. Complete
supported indexed-carrier and enhanced caller reconciliation remains separate.

Readonly schema attachment and type rejection are separate responsibilities.
The handwritten fixture retains the schema and reports validation explicitly;
a model may perform validation during its own conversion. This change does not
introduce universal validation into the base `Entity` trait.

## Ownership and verification

The joined installed gate passes native 45/45 and imported-module 18/18,
including both source corpora (2,304 and 192 observations per frontend) and the
lifetime program. Both final builds report no work; canonical module consumers
also pass. The authoritative joined receipt is
`parity/resolved_query_schema_installed_5_0_6.json.gz`. The latter uses the null default PMR
resource and exercises all four row/list entry modes, enabled cache miss/hit,
temporary query destruction and typed runtime-state attachment. Explicit copies
to Global retain their schema after actual UserContext deletion and rebinding
through a new UserContext. No wrapper extends the destroyed Context's lifetime.

The focused ASan/UBSan gate passes 4/4 with leak scanning disabled, including
the existing deferred-page Context-destruction program. Its receipt is
`parity/resolved_query_schema_sanitize_5_0_6.json.gz`. The joined installed
receipt is pending at this working checkpoint.
The installed controller includes the retained G09 execution regression selection,
both new corpora, ownership checks, query/cursor and aggregate/dynamic-row checks,
canonical module consumers and final no-work builds.

```bash
source scripts/toolchains/activate.sh
CARGO_INCREMENTAL=0 python3 scripts/verify_resolved_query_schema_5_0_6.py \
    --record /tmp/resolved_query_schema_fresh.json.gz \
    --cpp build_native_current/teaql_entity_query_probe \
    --module build_modules_native/modules/teaql_module_entity_query_probe \
    --check-gates
```

## Remaining boundary

The [stream continuation](resolved_stream_schema_5_0_6.md) repairs the remaining
readonly chunk propagation: source/native/imported 769, native10/module8 and
scoped ASan/UBSan4 pass. The joined verdict
`parity/resolved_query_schema_verdict_requalified_5_0_6.json.gz` restores G09
within its existing EX01-EX14 contract. Original failure receipts and earlier
45/18 nonstream/compiler/Java checkpoints retain their respective scopes.
Dynamic-aware/enhanced compositions and complete resolved-service/phase
acceptance remain independent. Source totals stay 84 accepted, 38 pending and
2 excluded; capability totals return to 11 qualified and 6 partial.

Verification disk maintenance removed 272,451,542 bytes of regenerable GCC
installer archives from `.tools/toolchain-upgrade`; installed toolchain checks
still pass. No installed compiler, active build output or source was removed.
