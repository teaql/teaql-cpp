# Resolved query preparation: callback and projection contracts

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: query behavior lookup and select preparation in
`teaql-runtime/src/data_service/resolved.rs`, corresponding to RQ05 and the
preparation portion of RQ06. Actual-source/native/imported comparisons pass
all 1,036 inputs; selected native checks pass 4/4 and module checks pass 3/3.
No production implementation or generator output changes in this continuation.

The source fixture is appended to the complete pinned runtime through
`ReferenceRuntimeView`. It calls both actual private methods
`prepare_select_query` and `prepare_select_query_owned`, requiring equal results
and equal callback observations. The borrowed input must remain unchanged.
The C++ fixture calls the shared service preparation adapter; its executor throws
if preparation performs a query or mutation.

The independent 1,036-input matrix observes:

- Query-entity behavior selection when the repository entity remains Parent.
- Absent, observing, rewriting and rejecting behavior and RequestPolicy hooks.
- Intent validation before hooks, and bound service intent overriding missing
  query intent without adding trace entries.
- Service/query trace ordering as observed by callbacks and in the result.
- Rewritten entity and projection visibility at the next callback.
- Relation-local projection keys selected from the final entity metadata,
  duplicate-key suppression, missing relations and absent metadata.
- Behavior suppression during real generated schema bootstrap, with RequestPolicy
  still active. Both fixtures enter through `ensure_schema` and a registered
  bootstrap callback rather than setting an artificial test flag.

The main cross product has 1,024 inputs. Twelve additional controls vary metadata
availability and trace presence with observing or rewriting hooks. Complete query
snapshots, errors and callback inputs are compared without output normalization.
Default PMR is disabled in the C++ program; all retained fixture state belongs
to its UserContext.

The first frontend comparison used inconsistent fixture construction: Rust
assigned `comment` directly while C++ called the builder, which adds a trace
frame. All 524 differences per frontend are retained in the
[fixture mismatch receipt](parity/resolved_select_preparation_fixture_mismatch_5_0_6.json.gz).
The C++ fixture now assigns the raw field, matching the source input. This is a
fixture correction, not a runtime bug fix. The same receipt retains an initial
module build selection failure for a nonexistent entity-service test target;
native and module test selections now reflect the actual registered targets.

The [installed receipt](parity/resolved_select_preparation_installed_repaired_5_0_6.json.gz)
records both matching matrices, exact test selections, canonical module consumer
checks, successful builds and a final no-work build for each frontend. The
[earlier installed attempt](parity/resolved_select_preparation_installed_5_0_6.json.gz)
retains a Rust linker Bus error. The filesystem had only 161 MB available;
removing the regenerable Rust incremental cache restored approximately 3 GB.
The successful attempt used `CARGO_INCREMENTAL=0`; runtime source, expected
results and debug assertion mode were unchanged. No C++ production code changed,
so this continuation runs no additional sanitizer or MIPS profile.

Reproduce the source and both installed gates with a fresh receipt destination:

```bash
source scripts/toolchains/activate.sh
CARGO_INCREMENTAL=0 python3 scripts/verify_resolved_select_preparation_5_0_6.py \
    --record /tmp/resolved_select_preparation_fresh.json.gz \
    --cpp build_native_current/teaql_resolved_select_preparation_probe \
    --module build_modules_native/modules/teaql_module_resolved_select_preparation_probe \
    --check-gates
```

This evidence does not exercise typed identity/version projection, provider
execution, mutation preparation, dynamic loading or every composed caller of
the resolved service. Whole-source and capability acceptance retain those
separate obligations. No source or capability verdict advances here.
