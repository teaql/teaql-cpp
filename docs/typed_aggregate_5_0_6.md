# Complete private typed-aggregate replay against Rust 5.0.6

This checkpoint targets runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. It qualifies the retained private
`fetch_entities_with_relation_aggregates_internal` entry point, including its
current intent validation and dynamic-field carrier boundary. It does not close
G08, G13, G15, G16 or G17 as a whole.

## Original execution rather than a reduced helper

The historical Rust driver extracted one private method into a controlled
`SourceRepo` shell. That shell bypassed executor orchestration. It no longer
compiles against the current source because the original method now needs scoped
request intent, dynamic-field loading and the shared read-metadata decoder.
The failure is retained in
[the state-baseline compiler record](parity/state_baselines_5_0_6_initial_compiler_failures.json).
It is a fixture compatibility failure, not a failure to compile the original
runtime.

The new [builder](../tests/parity/build_typed_aggregate_oracle_5_0_6.py) archives
the exact upstream commit into a separate ignored build directory. It verifies
the original bytes of all 610 files. Only `data_service/resolved.rs` receives an
append-only access method, which delegates directly to the existing private
method. No production algorithm is extracted, replaced or stubbed. The original
registry decoder, request validation, service preparation and executor path all
run. Registry package versions and checksums match the original lock.

The [observer](../tests/parity/rust_typed_aggregate_5_0_6/src/main.rs) implements
only controlled executor rows, failure injection, a plain test entity and JSON
reporting. The original reference checkout and historical driver remain intact.
Both languages disable SQL diagnostic output for this focused matrix.

## Paired inputs and initial gaps

The [current corpus](../tests/parity/typed_aggregate_scope_5_0_6_cases.json.gz)
contains 146 exact observations with no output normalization:

| Input group | Count | Responsibility exercised |
| --- | ---: | --- |
| Unchanged historical inputs | 48 | Current rejection of missing query intent; all source outcomes differ from the historical bypassing driver. |
| Explicit-intent versions of those inputs | 48 | Empty/reduced projection, identity/version injection, zero/nonzero rows, provider failure and model decode failure. |
| Missing, empty and whitespace intent controls | 14 | Actual current request constructors, with and without dynamic selection. |
| Explicit dynamic-selection controls | 36 | Empty rows, unsupported plain carriers, provider failure and carrier rejection before model decoding. |

The extra inputs are separately declared in
[the control fixture](../tests/parity/typed_aggregate_controls_5_0_6.json).
They do not replace historical inputs or silently fill their intent. Old corpora
remain historical evidence rather than being relabeled as current observations.

The initial comparison found 12 mismatches, retained in
[the initial gap record](parity/typed_aggregate_5_0_6_initial_gaps.json).
C++ returned decoded plain models or model errors for nonempty dynamic selections;
the original helper rejected carriers lacking indexed dynamic-field support.

## Runtime correction and native integration

`TypedAggregateOperation` now uses the existing `DynamicRowsOperation` and shared
`decode_compact_rows_with_read_metadata` path. It validates the dynamic request
scope, accounts for aggregate arguments when preparing diagnostic bindings,
fetches native rows, checks carrier support, loads extensions and then decodes
through the shared metadata-aware decoder. Empty results skip dynamic provider
loading. It keeps its fresh decoding root and does not enter enhanced-entity
ID-set paging or replace independent pagination observations.

This reuses the runtime/service/provider contracts; no generator algorithm or
new production dependency is added. Query state, retained rows, augmented load
state and pending operations belong to the executing UserContext. Cancellation
joins pending row and dynamic operations.

[Dynamic query contracts](../teaql_runtime/tests/dynamic_query_tests.cpp) exercise
the corrected private entry point with the existing indexed School carrier,
actual SQLite native rows and the in-memory extension provider. They check loaded
extensions, explicit NULL, no mutation ledger entries and distinct availability
for a row without extension values. Additional pending-provider cases exercise
both explicit cancellation and actual UserContext deletion, checking exactly one
join and destruction before arena reclamation.

The first sanitizer run caught a mistake in the new deletion fixture: a stack
`SelectQuery` using User arena storage survived the User's deletion. The corrected
fixture allocates the query and service through Context ownership. The complete
[failed sanitizer diagnostic](parity/typed_aggregate_5_0_6_fixture_lifetime_failure.json.gz)
is preserved. This does not change the contract requiring stack values with arena
storage to be destroyed before that arena closes.

The retained [typed-aggregate lifecycle contracts](../teaql_runtime/tests/typed_aggregate_tests.cpp)
also check ID-page independence, reduced identity projection, checked-root expiry,
explicit Global retention and nested allocator propagation.

## Reproduction and acceptance boundaries

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_typed_aggregate_scope_probe teaql_typed_aggregate_tests teaql_dynamic_query_tests
python3 scripts/verify_typed_aggregate_5_0_6.py
TEAQL_LOG_ENDPOINT=off ctest --preset native --output-on-failure \
  -R '^(typed_aggregate_scope_rust_parity|typed_aggregate_context_contracts|dynamic_query_contracts)$'
```

The verifier rebuilds the complete original runtime and checks committed inputs,
outputs, source provenance and dependency identity. `--record` updates the current
corpus only after all paired observations match. Module replay uses
`--build-dir build_modules_native/modules --cpp-prefix teaql_module_` after building
the corresponding module targets.

Results are recorded in [the paired summary](parity/typed_aggregate_5_0_6.json)
and [terminal verification](parity/typed_aggregate_5_0_6_terminal.json).
The 146 source comparisons use a plain carrier and no separate relation aggregate
arguments. Indexed-carrier loading and pending cleanup are native integration
checks, not additional cross-language observations. Full grouped/relation aggregate
configuration, privacy and sink matrices remain separate requirements. No full
native suite, MIPS/QEMU or physical-device run is claimed here. Sanitizer leak
scanning remains disabled.

The broader module-root registration audit is still pending: its
`docs/module_runtime_root_contracts.json` and historical source-test dispositions
refer to Rust 5.0.5. `verify_module_runtime_roots.py` correctly rejects this ledger
under the active 5.0.6 lock. The old records are preserved; registration locators
need source-based migration rather than a baseline-label change. The canonical
consumer and export checks are separate from that unresolved acceptance gate.

The subsequent [runtime-root migration checkpoint](runtime_roots_5_0_6.md) closes the stale registration-ledger gap through committed 5.0.6 source/fixture review, all 75 original root tests and executed native/module counterparts. The historical ledger is preserved; this follow-up does not imply complete implementing-leaf or generator acceptance.
