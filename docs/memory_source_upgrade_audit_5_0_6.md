# Test-only memory service upgrade audit: Rust 5.0.6

Current disposition: **accepted** at the [final source checkpoint](memory_source_acceptance_5_0_6.md). The pending statements and staged closure checklist below retain their historical checkpoint scope.

Status: pending review; no source disposition or capability promotion.

The reference `teaql-runtime/src/memory.rs` is 1,097 lines at the locked runtime
commit `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Its module declaration in the
runtime root is gated by `cfg(test)` and explicitly describes a deterministic
test oracle. The public `inmemory_engine` module is a different responsibility.
The C++ `tests/parity/memory_data_service.hpp` likewise states that it implements
selected test contracts, not the complete helper or a production provider.

## Actual upgrade delta

Comparison with the locked 5.0.5 commit finds two scalar/filter changes:
`eval_filter` accepts LikePattern as a scalar operand, and `eval_value` evaluates
its expanded `pattern` string. A new original test compares six typed builders
with raw LIKE/NOT LIKE expressions on the `name = teaql` row. It checks both the
private memory evaluator and the published ExprEvaluator. The other helper
bodies are unchanged in this diff, which is not itself proof of current behavior.

## Focused candidate evidence

The [native candidate](parity/memory_like_native_candidate_5_0_6.json.gz) retains
the complete existing nine-contract C++ memory-service fixture and adds the six
LIKE pairs. Both the test-only service's fetch path and public ExprEvaluator
match typed/raw results, with explicit true/false expectations. The nine retained
contracts also pass. Compilation and linking finish successfully before execution;
the linked native archives are unchanged across the run. That receipt describes
the earlier candidate checkpoint. The six LIKE pairs are now installed as
`typed_like_matches_raw_patterns()` in `tests/parity/teaql_runtime/memory_service.cpp`;
the historical receipt alone does not qualify the current installed source.

The [original unchanged test](parity/memory_like_original_5_0_6.json.gz) passes
through the reference runtime view: one selected test passed, zero failed or
ignored, and 272 other library tests were filtered out. The receipt retains the
reference-view provenance. This is focused evidence only. No module-side
counterpart is established by that historical receipt.

## Direct private-filter differential diagnosis

The [original filter observation](parity/memory_filter_original_5_0_6.json.gz)
appends an observer to the locked runtime view, preserving every original body,
and directly calls private `eval_filter` for sixteen scalar/filter boundaries.
The selected Rust test passes and emits all sixteen unique observations.
Its disposable runtime view and Cargo target were removed after execution:
160,917,548 bytes. Dependency downloads remain shared and untouched.

The C++ counterpart `tests/parity/memory_filter_probe.cpp` calls the test helper's
actual `fetch_all` path. The [initial comparison](parity/memory_filter_initial_comparison_5_0_6.json.gz)
records **3 matching / 13 differing** results. This is a completed diagnosis,
not a passing qualification. The comparison command exits 1 for mismatches:

```bash
python3 tests/parity/diagnose_memory_filter_5_0_6.py \
  --cpp build_native_current/teaql_memory_filter_probe \
  --output /path/to/new_comparison.json.gz
```

The differences cover non-boolean scalar rejection; Soundex argument count,
argument type and result truthiness; GBK arity/identity; unsupported Count;
non-list IN/NOT IN errors; incomparable NULL BETWEEN; and mixed I64/U64 equality.
NULL, boolean true and NULL NOT LIKE match. At that initial checkpoint C++ leaked an out-of-range
exception for zero-argument Soundex instead of the reference's structured error.

These findings required a private test evaluator rather than delegation
to the public engine. They do not authorize changing the public engine to private
test-oracle semantics. The sixteen cases are an initial diagnostic surface;
scalar variants, recursive expressions, aggregates, ordering and mutation helpers
still require source-based coverage before accepting the whole source. The probe
was initially a diagnostic build target, not a passing acceptance gate.

## Filter repair checkpoint

The test helper now evaluates filter/scalar expressions using the private source's
rules: boolean-only truthiness, recursive short-circuiting, strict function arity
and types, explicit unsupported-function errors, signed/unsigned comparisons,
typed partial ordering, boundary-percent LIKE, list-only membership, NULL and
BETWEEN behavior. Date comparisons use `epoch_days()` because C++ Date exposes
equality but not ordering operators. All intermediate values and errors retain
the service's explicit Context allocation domain.

The [native repaired comparison](parity/memory_filter_native_repaired_5_0_6.json.gz)
matches all sixteen original observations, including complete error strings.
`private_memory_filter_rust_parity` now runs this comparison as a regression gate;
it and `original_runtime_memory_service_contracts` pass (2/2). The initial
3/16 receipt remains intact. Comparison output files are optional, and an existing
receipt cannot be overwritten.

The [canonical module comparison](parity/memory_filter_modules_repaired_5_0_6.json.gz)
also matches 16/16; `module_original_runtime_memory_service` passes (1/1).
Canonical consumer verification checks 1,110 consumer/fixture bodies. Both
affected native and module target builds finish with no work remaining. The
module probe is registered as a canonical consumer; its comparison is explicitly
run through the same script, rather than treating executable exit zero as parity.
Pinned format and format-check pass for 1,778 handwritten C++ files; module path
and source-provenance checks pass. Stable preset build outputs are retained;
two completed-task configure logs (285 bytes) were removed. No disposable Rust
build was needed for this repair because the retained original observation and
its source/observer hashes were verified before each comparison.

This historical checkpoint repaired only the fetch filter path. Ordinary projection/ordering
then delegated to the public query engine; aggregate projection and HAVING still
used public ExprEvaluator. The subsequent [row-pipeline repair](memory_pipeline_5_0_6.md)
replaces that delegation with eighteen exact source observations. All private branches
need broader original observations before complete helper acceptance. No public
runtime body, generated artifact or source disposition is changed by this repair.

## Complete responsibility consolidation

The [whole-source responsibility review](memory_complete_responsibility_review_5_0_6.md)
now maps construction, metadata, wrappers, query/filter/aggregate helpers,
mutations and traits to their C++ realizations. Query-time Decimal failure outside
the storage lock is now [qualified](memory_decimal_failure_5_0_6.md).
[Debug diagnostics](memory_debug_5_0_6.md) are implemented with focused exact-output
qualification; [contention and formatter-failure checks](memory_debug_failure_5_0_6.md)
now resolve the earlier evidence limit. Final source review remains open. Relation result lookup now restores the source's ordered
identity buckets instead of rescanning all result rows for every parent.
Final source acceptance still requires D03 closure; D01 and D02 are resolved.

## Remaining boundary

The current responsibility review identifies these concrete closure tasks. The
earlier checkpoint reports are evidence for their stated scopes, not a promise
that every item below is already implemented.

| Responsibility | Current evidence or implementation | Remaining closure work |
| --- | --- | --- |
| Row fetch, scalar/filter evaluation | Sixteen direct boundaries, eighteen pipeline queries, original callers and [272 recursive observations](memory_recursive_5_0_6.md) | Recursive short-circuit/subquery/scalar-error paths qualify in native/modules; consolidate Value variants against the supporting core comparison contracts. |
| Grouping and aggregate arithmetic | 208 aggregate and 1,459 grouping/identity observations | Review uncovered arithmetic/type branches. [Context-backed stable merge sorting](memory_sort_5_0_6.md) now qualifies 60 source comparisons and explicit complexity/stability controls for valid-order inputs. |
| Relationship aggregation | Identity matrix plus [160 shape/error observations](memory_relations_5_0_6.md) | Independent enhancement and partial-failure mutations are qualified in the [43 entry observations](memory_entries_5_0_6.md); include this evidence in consolidated review. |
| SmartList and typed wrappers | Four list/entity wrappers, original typed caller and [43 entry observations](memory_entries_5_0_6.md) | Thin adapters, loaded state and conversion errors qualify in native/module/sanitizer; consolidate with the full source review. |
| Metadata abstraction | [Custom MetadataStore and lifetime contract](memory_metadata_5_0_6.md) restores the original generic extension boundary through the existing C++ interface | Include default and custom store behavior in consolidated review; Context borrowing does not imply cross-thread UserContext access. |
| Construction, seeding and shared service state | Context-owned fixture, replacement seeding, mutex/poison observations | Explicit Context-borrowed sharing and close rejection qualify in the [entry checkpoint](memory_entries_5_0_6.md); preserve this adaptation rather than extending ownership to copy Rust Arc syntax. |
| Mutation and poisoned locks | 1,144 actual source observations, including fourteen panic/exception adaptations | Retain the explicit debug-profile adaptation and review source error precedence together with the wrapper/enhancement work. |
| Whole-source acceptance | Source still pending | Consolidate all source responsibilities, rerun unchanged original callers and focused current gates, then update the review hash/disposition only after the complete review passes. |

The standalone enhancement abstraction and thin wrappers are now present and
qualified by focused original/native/module observations. Whole-source acceptance
still requires consolidated Value/aggregate and complete responsibility review;
recursive evaluation and sorting
complexity have focused qualification under the documented valid-order contract.

### Ordering contract boundary

The [local standard-library contract review](parity/memory_sort_contract_review_5_0_6.json.gz)
records the installed Rust 1.97.1 rustdoc identity. `slice::sort_by` requires a
total-order comparator; a non-total comparator can panic and does not establish
a specified output order even when it returns normally. Its valid-input stable
sort has O(n log n) worst-case complexity. This is a documentation review, not a
retroactive assertion about the compiler used by older receipts.

Source inspection shows that the private helper maps incomparable values to
Equal. Arbitrary mixed seeded rows need not meet the total-order precondition:
I64(2) compares equal to NULL, and NULL to I64(1), but the two integers differ.
Do not turn one observed order for such inputs into a general TeaQL guarantee.
Retain those observations with their scope; qualify coherent ordering inputs and
preserve stable ties. The former quadratic insertion sort has been replaced by
[Context-owned stable merge sorting](memory_sort_5_0_6.md), with 60 original
valid-order comparisons and explicit comparison-bound/stability checks.

The helper's former `resolve_functions` rewrite and public evaluator delegation
have been replaced by private-source evaluation. Selected matching observations
must not be presented as a complete API port. Preserve the source ledger's
pending status until the private oracle responsibility, its callers, helper
coverage and current-source evidence have been reviewed together. Public engine
acceptance does not automatically accept this separate test-only source. The
active goal and all production runtime obligations remain unchanged.

## Current original callers

The [fresh original caller receipt](parity/memory_service_callers_original_5_0_6.json.gz)
executes all nine current runtime-root tests named `memory_data_service_*`:
typed selection, relation aggregates, ordinary aggregates, grouping/extended
filters, extended aggregates/HAVING, Soundex, string-match builders,
property-to-property comparison, and mutation/optimistic-lock behavior. All nine
pass, zero fail or are ignored, and 264 unrelated library tests are filtered out.
The controller extracts the nine names from the locked source and checks every
name in terminal output; it does not interpret a successful empty selection as
evidence. The reference-view provenance is retained with the result.

These tests correspond to the nine preserved C++ fixture functions exercised by
the native candidate. They establish current caller observations, not full
coverage of every private helper branch or whole-runtime-root acceptance.
