# Generated execution option constructors: Rust 5.0.6

This review closes EX12's configuration responsibility. The independent G09
execution acceptance and P4/P7 exits still require their complete review and
integration gates. References remain runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and producer
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Complete source responsibility

The frozen request template has five thin delegates: default/configured continuous
pagination, default/configured ID-set pagination and the Top-N parent threshold.
They have no entity-specific branch or service lookup. The core constructors and
all five delegates are retained in the
[source review](parity/execution_options_source_review_5_0_6.json), together with
the unchanged inputs from the earlier complete 37-method review.

| Contract | Source behavior and native mapping |
| --- | --- |
| Default continuous options | Namespace `default`, TTL 600 seconds. Native default delegates to the same configured constructor. |
| Default ID-set options | Namespace `default`, TTL 600 seconds, maximum 3,000,000 IDs. The maximum bounds IDs, not bytes or all Context allocations. |
| Validation order | Reject a trimmed-empty namespace before TTL zero; for ID sets, reject maximum zero last. Rust panic text and native exception text agree exactly. |
| Namespace storage | Use whitespace only for validation. Preserve the original nonblank namespace, including surrounding whitespace, embedded NUL and long values. Zero-width space and BOM alone are not Rust whitespace. |
| Option replacement | Replace the selected option without clearing the other optimization or Top-N setting. A rejected native replacement does not publish partial state. |
| Top-N threshold | Store an optional native-width value, including zero. This setter neither runs SQL nor guarantees a particular physical plan. |

The default/configured equivalences, guard precedence, coexistence and replacement
are observed through actual generated School, Academy, ResearchAcademy and
Enrollment requests. Both observers record the complete core query Debug state
before and after every operation; they do not reimplement query construction.

## Actual original observations and ownership

The immutable [source corpus](../tests/parity/execution_options_5_0_6_cases.json.gz)
contains **152 scenarios / 636 operations**, including **108** exact constructor
rejections. Inputs cover all constructor branches, default equivalence, zero/one/
maximum integer values, eleven blank namespace forms and five nonblank forms.
The [paired comparison](parity/execution_options_initial_5_0_6.json.gz) retains
every raw source/native result without diagnostic or query normalization.

Rust consumes each tested request clone. If that operation panics, the observer's
previous clone remains; this is not a claim that a moved failed Rust request can
be reused. C++ lvalue mutation validates before replacing its option. Every
native input disables default PMR allocation and actually deletes UserContext
after destroying its temporary views, then checks that User arena bytes are zero.
The threshold comparison uses native64 `usize`/`size_t`; it is not a new MIPS run.

## Composition with execution evidence

Configuration is distinct from physical optimization. The
[key/store/build-lock review](query_optimization_5_0_6.md) qualifies canonical
keys, real stores and shared build coordination. Deferred
[ID-set](deferred_id_set_page_5_0_6.md) and
[continuous-page](deferred_continuous_page_5_0_6.md) comparisons retain misses,
hits, errors, expiry, bounds, fallback, waits, cancellation and interleavings.

The [hot-cache review](execution_hot_cache_and_lookup_5_0_6.md) uses real stores
through repeated generated requests, changed Policy and replacement. Its eight
physical witnesses retain actual ID-set build-to-hit transitions and cursor seek
SQL; sticky plan observations are not counted as store callbacks. The
[SQL protocol review](sql_protocol_5_0_6.md) retains valid-request threshold-zero/
positive and metadata-capture transport cases. Native `sql_query_executor_contracts`
also checks probe versus window selection and explicit zero. The
[relation planner review](relation_planning_extensions_5_0_6.md) separately
qualifies grouped per-parent slicing and stable ordering, with real SQLite
integration. These records are not a promise that every configured query uses
an optimized physical path.

## Reproduction

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_execution_options_probe
ctest --preset native --output-on-failure -R '^execution_options_rust_parity$'
cmake --build build_modules_native --target teaql_module_execution_options_probe
ctest --preset modules_native --output-on-failure -R '^module_execution_options_rust_parity$'
```

The original observer builds with `--offline --locked` from
`tests/parity/rust_dynamic_json_privacy_5_0_6/Cargo.toml`, binary
`execution_options`. Its registry versions/checksums and complete frozen generated
library provenance are verified by the comparator. Pass `--rust PATH` to replay
the retained corpus against that binary as well as the native probe.

All **152/152** native and imported-module observations agree. The constructor
observer changes no production, generator or generated artifact. Existing
sanitizer evidence remains at its stated checkpoint; no new sanitizer or target
run is required for this observation-only addition. The
[terminal record](parity/execution_options_terminal_5_0_6.json.gz) retains the
completed commands and source identities.
