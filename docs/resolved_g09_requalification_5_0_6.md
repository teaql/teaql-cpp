# G09 requalification after joined resolved-service repairs

Status: G09 requalified under the unchanged EX01-EX14 contract. No whole-source
or P0-P7 promotion.

The [reopening record](resolved_joined_regression_5_0_6.md) retains the initial
29/31 native union failure. Both repaired unions now pass 31/31. Requalification
keeps the original EX01-EX14 requirements and the frozen 37-method generator
review; it does not replace capability acceptance with the new focused probes.

## Input reconciliation

The four C++ application execution files recorded in
`parity/execution_responsibility_after_lookup_results_5_0_6.json` retain their
exact reviewed hashes: `entity_query.hpp`, `generated_query_execution.hpp`,
`generated_query_facets.hpp` and `query_results.hpp`. These are application facade
bodies, distinct from the changed data-service `entity_query.hpp`.

Relative to the last G09 schema requalification, three producer files and 22
runtime/package files differ. Nineteen of those 22, including the arena,
entity runtime, dynamic decoding and JSON implementations, match the subsequent
`generated_json_owned_nullable_installed_v5_5_0_6.json.gz` input epoch exactly.
That receipt includes its own native/module/sanitizer observations; it is reused
only within its documented cache/JSON/lifecycle scope, not as a new sanitizer run.
The other three files are the current data-service query header, relation
installation and its tests. The accepted relation-source review retains the
loaded-state correction; the current 31/31 unions recheck affected shared callers.
The query header repairs require the fresh execution regression and compiler gates.

## Current gates

- Joined service union: native31/module31 pass; complete original failure and
  repaired receipts remain separate.
- Original G09 regression selection: native37/module10 pass, both with no-work
  rebuilds, through `scripts/verify_resolved_g09_regression_5_0_6.py`.
  This checks the frozen parent
  acceptance hash, exact test lists, terminal JUnit, no-work builds, canonical
  consumers and unchanged runtime/producer inputs.
- Execution API compiler matrix: one positive and 24 negative contracts pass
  with the exact prior case names and current native compile configuration. The
  [compiler receipt](parity/resolved_g09_compiler_5_0_6.json.gz) records unchanged
  inputs and the complete successful output; temporary PCH/header files are removed.
- Generator suites: 79 tests pass (75 generation, two field-layout and two JSON
  metadata tests), with no failures, errors or skips.
- Strict generated reproduction: 25 workspaces / 1650 files match,
  using `scripts/generate_examples.py --check`. The
  [generator receipt](parity/resolved_g09_generator_5_0_6.json.gz) records both
  steps and unchanged runtime/producer inputs. No retained generated file is
  patched or overwritten; temporary generation directories are reclaimed.

The [evidence join](parity/resolved_g09_requalified_5_0_6.json.gz) verifies all
terminal results, the unchanged EX01-EX14 contract/source references and scoped
evidence reuse. The [verdict update](parity/resolved_g09_verdict_update_5_0_6.json.gz)
records the two ledger updates separately. Canonical verification covers 1,130
consumer bodies. The private typed-aggregate scope question remains a complete
resolved-source obligation, distinct from public generated execution.
Historical Java/compiler/reproduction results are not relabeled as current runs.
No MIPS, physical-router or external-provider verification is inferred.

The subsequent [private-entry readback and replay](resolved_private_aggregate_scope_5_0_6.md)
resolved the speculative scope difference as a documentation error (8/8 original,
native and module outcomes). It does not promote the complete resolved source.
