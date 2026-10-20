# Test-only memory service upgrade audit: Rust 5.0.6

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
the linked native archives are unchanged across the run. The candidate remains
outside the worktree while the SDK integration holds its source inputs fixed.

The [original unchanged test](parity/memory_like_original_5_0_6.json.gz) passes
through the reference runtime view: one selected test passed, zero failed or
ignored, and 272 other library tests were filtered out. The receipt retains the
reference-view provenance. This is focused evidence only. No module-side
counterpart for this new candidate has yet been installed or executed.

## Remaining boundary

The test helper's `resolve_functions` and evaluator delegation are not a second
full implementation of Rust's private evaluator. Selected matching observations
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
