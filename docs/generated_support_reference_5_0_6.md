# Generated support: exact-reference dependency replay

The fresh gate is terminal and passes: nineteen original SQLite tests,
2,196 exact observations on each original/native/imported frontend, and seven
selected CTests per C++ frontend. Both final selected builds report no work.
That replay left the source disposition pending. The subsequent
[complete source review](generated_support_source_review_5_0_6.md) joins its
evidence with direct private-tree observations and qualifies GS01-GS19.
Capability and phase verdicts do not change.

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The production source is
`teaql-runtime/src/generated_support.rs`, SHA-256
`f21089ff92ae378fa74174f1eb42c028728034e3cb92841fd0795afd14119a2c`.
This checkpoint concerns source dependency provenance and existing behavioral
observations. It does not qualify the complete source, G08/G13/G17 or P0-P7.

## Why a separate reference project is necessary

The retained `rust_generated_support_5_0_6` and
`rust_runtime_baselines_5_0_6` locks each contain 45 registry package identities
outside the reference lock; `rust_field_layout` contains 46. Their successful
executions are historical evidence, not proof of exact reference dependencies.
The historical facet-test harness additionally requests rusqlite 0.40.2 rather
than the reference's 0.40.1. Those projects, scripts, outputs and locks remain
unchanged.

The independent
[reference project](../tests/parity/rust_generated_support_reference_5_0_6/Cargo.toml)
uses the reference versions and checksums, including futures-util 0.3.32,
tokio 1.52.3, async-trait 0.1.89 and rusqlite 0.40.1. Its Cargo lock starts from
the reference lock; subsequent verification builds use `--offline --locked`.
No production dependency changes.

Four binaries compile the existing observers directly through manifest paths.
No observer body or runtime algorithm is copied or rewritten. A fifth binary
uses the existing blocking test executor with only its source include redirected
to the unchanged original `facet_trace.rs` suite. Both original test files remain
in the clean pinned reference checkout.

The first independent build omitted the macro dependency used by the unchanged
handwritten Order model. Its terminal compiler failure is retained in the
[initial failure record](parity/generated_support_reference_initial_build_failure_5_0_6.json.gz).
Adding the explicit reference macro dependency fixes the development harness;
it does not change a model or the runtime.

The first full driver run then reproduced all observations and passed native
7/7, but its success-summary assertion expected CTest's older zero-failure
clause. The current tool prints `100% tests passed out of 7`. The
[terminal failed run](parity/generated_support_reference_initial_driver_failure_5_0_6.json.gz)
and [driver/output snapshot](parity/generated_support_reference_initial_driver_failure_details_5_0_6.json.gz)
remain unchanged. The corrected assertion accepts either successful spelling
while still requiring terminal zero and exactly the seven discovered test names.
The fresh execution record is separate from that failed driver run.

## Behavioral scope

The reproduction gate executes both original SQLite suites: nine request/trace
tests and ten facet tests. These include removed COUNT source privacy, failure
publication, intent with logging disabled, batch-local intent, future facet
classification, repeated COUNT/facet composition, loaded relationship facets,
shared targets, empty keys, slow typed hydration and read-only sidecars.
These are original execution results; running an original test does not by
itself establish matching C++ behavior for every assertion in that test.

The unchanged comparison corpora have the following responsibilities:

| Corpus | Inputs | Observation boundary |
| --- | ---: | --- |
| Public facets | 1,178 | Purpose wrappers, selective restriction, metadata errors, sequential/nested execution, duplicates, failures and cancellation |
| Public facet logs | 36 | Actual SQLite facet results, intent/privacy/trace records and unrelated subsequent queries |
| Purpose wrapper Debug | 46 | Formatting, purpose validation, copying, readonly access and consuming extraction |
| Derived query diagnostics | 576 | Actual scalar/typed/stream reads with removed or future bindings, failures and logging switches |
| Derived query composition | 360 | Repeated direct/nested/aggregate sources, COUNT/clone/empty-option composition and five execution routes |
| **Total per frontend** | **2,196** | Exact outputs; no normalization |

The gate compares fresh original execution to every retained expected value,
then compares separately rebuilt native and imported-module consumers to those
same values. Expected values and corpus metadata remain byte-identical. Original
observer dependency metadata remains historical; the new execution record binds
the exact current project and lock without rewriting earlier evidence.

Existing C++ contracts also check Context-bound snapshots across suspension,
pending cancellation, expired runtime rejection, foreign-future authority,
move-only generic wrappers, both service concepts, the separate hidden audited
save shape and explicit destination retention after actual source User deletion.
The gate requires exactly seven named CTests per selected frontend and waits for
successful builds before executing them. Final selected rebuilds check that the
same targets are stable.

## Subsequent source review and remaining capability exits

The subsequent complete review maps all six service delegations, public concept
constraints, recursive diagnostic traversal and complete query-state preservation
to this dependency-correct evidence and the new private source observations.
Diagnostic source presence alone does not prove private-tree classification;
the complete review retains these separate responsibilities.

In particular, that source review retains these separate branches:

- `PurposedQuery<T>` accepts arbitrary purpose text and a move-only inner value;
  it does not acquire Rust Clone/Debug/Default or serialization requirements.
- `for_exact_count` snapshots the source once, clears only projection,
  expression projection, ordering, slice and relations, then appends COUNT.
  Existing aggregates and the remaining complete query state survive.
- `query_has_facets` searches direct facets, queried relations, child
  enhancements and object groups. It does not also search expression subqueries
  or core relation-aggregate queries.
- Facet diagnostic conversion applies selection runtime metadata, adds builder
  relation-aggregate sources and recursively expands facet sources. The
  executable query and later independent invocations remain separate.
- Empty facet options still validate both root intent fields. Facet execution
  resolves the outer entity and named relation even for `include_all_facets`;
  that flag skips restriction only. Direct restriction's absent entity is a
  Graph error, while execution's absent entity is MissingEntity.
- Nested execution retains root diagnostic provenance, passes an empty separate
  repository trace, replaces returned nested facet maps and executes duplicate
  names sequentially before retaining the last result. Failures stop the loop.

These are requirements from the complete current source, not additional API
behavior introduced by the new observer project.

Generated quoted-FK behavior, complete assembly/sample/Assist qualification,
protected query/save publication and full current integration retain their
independent obligations. The historical failed full native epochs are unchanged.
No sanitizer, MIPS, physical-router, external-provider or GPU result is inferred
from this development observer replay. Runtime, generator and generated SDK
implementation bytes are preserved.

The [byte-preservation record](parity/generated_support_reference_preservation_5_0_6.json.gz)
checks all 3,457 files in 78 SDKs, 2,201 protected source/artifact files and
fifteen existing runtime/observer files against the preceding checkpoints.
This is byte preservation, not a Java test or fresh generation claim. The
[before archive](parity/generated_support_reference_before_5_0_6.json.gz)
retains the original ledgers, source bodies and dependency differences.

## Reproduction

From the repository root, use a fresh evidence path:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_support_reference_5_0_6.py \
    --module-build-dir build_modules_native \
    --record /tmp/teaql_generated_support_reference_fresh_5_0_6.json.gz
```

The [execution record](parity/generated_support_reference_execution_5_0_6.json.gz)
contains commands, exit codes, raw output, inputs, binary identities and source
snapshots. Its `complete` field applies only to the specified reproduction gate;
whole-source and full-goal acceptance are separate false fields.
