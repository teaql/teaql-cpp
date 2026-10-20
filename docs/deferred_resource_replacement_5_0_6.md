# Suspended query resource replacement

Status: native, imported, sanitizer and relevant regression checks pass.
Complete G08/G09/G13/G17 and P0-P7 remain open.

References remain Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Original borrowing boundary

The actual unchanged generated Order API creates a future borrowing UserContext.
Three independent original compiler controls attempt typed-resource, entity-
registry and ID-set-store replacement before dropping that future. Each produces
exactly one E0502 error. A positive control compiles and runs the same setters
after dropping the future. Registry registration takes the original registry
value; the first fixture's incorrect Arc wrapper and its E0277 are retained as
preparation evidence, not as the required borrow refusal.

The [original compiler record](parity/context_mutation_borrow_5_0_6.json.gz)
retains complete commands, diagnostics and source. The helper verifies original
registry versions/checksums before compilation. No original generated/runtime
algorithm is changed to create these errors.

C++ permits explicit mutation/destruction of registered resources while its
Context-owned operation is pending. Those actions are verified as C++ ownership
adaptations. They are not invented executable Rust actions, nor scheduling
comparisons against Rust code that cannot compile.

## Actual execution and observed boundaries

The new fixture includes the existing gated real executor and query builder from
the [deferred facet/page checkpoint](deferred_facet_page_5_0_6.md). There is no
second pagination, SQL, facet or decoding implementation. Real SQLite tables,
generated Order/Person carriers and actual in-memory optimization stores remain
unchanged. Instrumented ID-set wrappers delegate storage to the real store.

The 143 scenarios cross list/one/page, ordinary/ID-set/continuous configuration,
root/facet suspension and the ordinary/continuous page count boundary where
reachable. Seven profiles establish distinct contracts:

| Profile | Scenarios | Observed contract |
| --- | ---: | --- |
| Executor registration | 60 | Started work retains its selected executor. A later facet resolves the new registration; a facet already pending keeps its old work. Resume, cancel and actual Context close are separate actions. |
| Registry replacement/destruction | 20 | Destroying a replaced registry does not invalidate a completed lookup. Later facet lookup consults the replacement. Registry membership is distinct from application authorization. |
| Captured provider destruction | 20 | The pending executor's actual checked provider dependency becomes invalid. Access raises ReferenceError, the operation fails and clears its wake target; a new execution uses the replacement. |
| Unreplaced registry destruction | 11 | A later required lookup rejects the expired registry. Installing a valid replacement permits a new query. |
| Policy replacement | 11 | A Policy installed during count/root suspension denies the later Person facet before its executor starts. Clearing the Policy permits a real retry. |
| ID-set store replacement | 18 | An already selected build publishes through its original store. A later execution uses the replacement. Cancellation/close release the pending work and build lock. |
| Captured store destruction | 3 | Publication through an expired checked store rejects. The replacement can acquire the released lock and publish a fresh result. |

Outcomes contain 46 resumed successes, 26 explicit cancellations, 26 actual
Context closes, 34 expired-dependency rejections and 11 Policy denials. All 163
successful initial/retry business-result snapshots agree with their unchanged-
data original cases from the earlier corpus, including complete loaded-state
and typed JSON, facets and totals. That comparison does not assert that Rust
executed the C++ resource mutation actions.

GlobalContext explicitly owns controls and observations; query, provider, store,
registry and work objects belong to UserContext. Default PMR allocation is
disabled. A 32 KiB work buffer is checked during cancellation before arena
reclamation. Every input actually deletes its UserContext and requires no
remaining live User bytes or callback target. This does not claim worker-thread
or GPU shutdown coverage.

## Fixture reuse and preservation

The previous observer is included with its entry-point name remapped. Adding an
explicit final `return 0` makes that renamed ordinary function well-defined;
the original CLI main already had the same implicit return behavior. The exact
previous file is preserved in
[the fixture snapshot](parity/deferred_resource_fixture_before_5_0_6.cpp.gz).
Every other byte and every query/executor method remain unchanged. Module
consumers use the same included fixture under the same macro state and preserve
its assertions through canonical-body verification.

Runtime/service/executor/provider layers and both allocation scopes remain
independent. Generator/SDK artifacts are unchanged, and these lifetime controls
add no runtime dependency or private ownership region.

## Completed verification

| Gate | Result |
| --- | --- |
| Native replacement/destruction scenarios | 143/143 |
| Original unchanged-data business results | 163/163 complete snapshots |
| Source borrow controls | One compiled/executed positive; three independent E0502 refusals |
| Current native integration | 6/6, including previous facet/page, binding and store-lifetime controls |
| Imported-module integration | 3/3, including the unchanged 1,944-case composed query matrix |
| Targeted ASan/UBSan | 1/1, all 143 scenarios; leak scanning disabled |
| Formatting / canonical consumers | 1,636 files / 787 bodies |

The [terminal record](parity/deferred_resource_replacement_terminal_5_0_6.json.gz)
retains both C++ observation sets, the source borrow diagnostics, final selected
build/test output and the exact fixture-body proof. Final builds no longer emit
the renamed-main return warning. No runtime/generator/generated implementation
change, new Java/reproduction run, full suite, MIPS or physical-device execution
is claimed.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_deferred_resource_replacement_probe
ctest --preset native --output-on-failure -R '^deferred_resource_replacement_contracts$'
python3 scripts/verify_original_context_mutation_borrow_5_0_6.py
```

Broader configured/protected-model execution, complete stream and optimization
dispositions and final G09/P4/P7 integration remain required. These ownership
controls do not replace any of those requirements.
