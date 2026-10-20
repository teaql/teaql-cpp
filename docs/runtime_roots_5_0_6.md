# Runtime root contract migration to Rust 5.0.6

This checkpoint targets the exact runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. It migrates the root source/fixture
registration audit to the current baseline and executes the original root tests
and their existing C++ counterparts. It does not establish complete runtime,
implementing-leaf or generator parity.

## Source review and preserved history

The current `teaql-runtime/src/lib.rs` still contains 75 root tests. No test names
were added or removed. Thirty function bodies remain byte-identical; 45 changed.
The [source review](parity/runtime_roots_5_0_6_source_review.json) records every
changed body as an exact old/new diff, together with its source and C++ hashes.

Most changes supply the explicit request intent now required by the runtime:
`fixture_select` creates meaningful test comments and purposes, mutation fixtures
bind `MutationIntent`, and ledger fixtures set a root comment. The relation,
aggregate, cache and pagination inputs retain their reviewed business scenarios.
One entity-binding assertion now uses `fixture_select(repo.entity())` instead of
`repo.select()`; it still expects `Order`. The C++ fixture continues to check both
the service entity and its selection binding.

A memory-aggregate query struct literal also initializes the new facet, relation
aggregate, purpose, readonly-definition and dynamic-selection fields to empty or
None. This original `cfg(test)` helper remains reference-only; it is separate from
the public in-memory execution engine and its request boundary.

Production root changes include commit-audit, document-round-trip and dynamic-field
modules, the shared read-metadata decoder and `DynamicFieldChanges` exports.
The current runtime umbrella and module facade expose these declarations.
Their implementing algorithms and generated configurations retain their separate
qualification requirements.

The original [5.0.5 registration ledger](module_runtime_root_contracts.json),
[source-test dispositions](parity/source_test_dispositions.json) and
[acceptance review](runtime_root_source_acceptance.md) remain unchanged. The new
[current root ledger](parity/v5_0_6/runtime_root_contracts.json) derives locations
from the committed 5.0.6 inventory, records actual called C++ functions and retains
historical assertion reviews. Neither matching names nor source hashes alone are
acceptance evidence.

## Executed scope

| Profile or check | Result | Scope |
| --- | ---: | --- |
| Complete original Rust runtime test executable | 75/75 | Exact `tests::...` root filters; unmodified source, no extraction or access bridge. |
| Native C++ root selections | 10/10 | All 75 root responsibilities through the nine fixture executables and isolated-process output gate. |
| Canonical module root selections | 10/10 | The same called functions and assertions through the import frontend. |
| Native and module namespace contracts | 1/1 each | Existing public namespace lookup/dispatch contracts. |
| Current registration audit | 75/75 | Committed source locations, fixture body hashes, entry calls and actual CTest module registrations. |
| Rejection controls | 3/3 | Historical baseline, changed C++ body hash and missing module registration. |

The root selections cover registration, service boundaries, validation, events and
relations, typed queries, aggregate/cache behavior, pagination, the private memory
reference helper, Context identity/locks and the optimistic-conflict output
subprocess contract. The complete detailed historical assertion reviews remain
attached to each current root mapping.

The original source test executable contains other runtime tests; only the 75 root
tests are selected here. Its original Cargo lock and existing test dependencies
are used unchanged. No live PostgreSQL, MySQL, telemetry backend or remote service
is invoked. C++ retains the documented adaptation of the synthetic source SQL
dialect to SQLite placeholders, explicit Context ownership and native thread IDs.

The [source execution report](parity/runtime_roots_5_0_6_source_execution.json)
records all executed root names and source/lock fingerprints.
[Terminal evidence](parity/runtime_roots_5_0_6_terminal.json) retains profile results
and source output. The original and C++ suites passing independently support the
reviewed root contracts; they are not an observation-by-observation comparison
of every runtime method or generated model configuration.

## Verification and reproduction

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/runtime_root_contracts_5_0_6.py
python3 scripts/verify_runtime_root_source_5_0_6.py
ctest --preset modules_native --show-only=json-v1 > /tmp/teaql_modules_registry.json
python3 scripts/verify_module_runtime_roots.py \
  --ctest-json /tmp/teaql_modules_registry.json
```

The source verifier waits for compilation before executing the test binary. It
requires every current root test to report success and compares its reproducible
source/lock/test-name summary to the committed report. Volatile process and timing
output is kept separately in the ignored source build directory.

`verify_module_runtime_roots.py` now uses the current ledger, compares it against
fresh committed source and called C++ bodies, and checks the registered module
tests. A `--ledger` override is subject to the same exact checks. The
[negative controls](parity/runtime_roots_5_0_6_negative_controls.json) demonstrate
actual rejection of stale or incomplete records.

No production or generated C++ was changed in this checkpoint. The full native
integration record still has 15 outstanding recorded failures after its selected
follow-ups. G01-G17 and P0-P7 remain incomplete; root registration migration does
not close those capabilities. No sanitizer, MIPS/QEMU, physical-device or full
runtime-suite run is claimed here.

## Complete original library execution checkpoint

The [complete original library receipt](parity/runtime_library_original_5_0_6.json.gz)
runs the unchanged runtime library unit-test executable through the retained
reference-runtime view with offline, locked dependency resolution and one test
thread. Terminal outcome: **272 passed, 0 failed, 1 ignored, 0 filtered out**.
The receipt contains the complete output and reference-view provenance.

The ignored test is `log_privacy::tests::file_log_child`, marked by the original
source as a subprocess fixture for `file_log_gate_runs_in_isolated_processes`.
It is not counted as an independently executed passing test. The complete run
includes tests from private child modules as well as runtime-root tests; 272
must not be described as the number of root contracts or matched C++ tests.

This supersedes neither the focused source reviews nor their C++ evidence.
It provides the current original-side library execution result for subsequent
reviews. Native/imported consumer correspondence, helper completeness and source
acceptance remain separate. Integration-test executables and external provider
tests are outside this `--lib` invocation; no source or capability disposition is
promoted by this checkpoint.
