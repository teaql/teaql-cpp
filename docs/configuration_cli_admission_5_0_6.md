# Configuration CLI admission repair, Rust 5.0.6

Configuration comparisons must admit the complete input matrix before reporting
that retained outcomes match. Five comparison scripts now check exact record
shape, input order and JSON value types. Topology extensions additionally verify
the model variant named by the corpus.

This is admission and native regression evidence for G13/SQ02 and G17/GP13.
It does not add original-runtime source acceptance or close P0-P7. Runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a` remain the reference commits.

## Complete configuration selection

| Family | Model variants / invocations | Retained inputs |
| --- | --- | --- |
| Scalar candidates | 8 / 8 | 376 |
| Constant metadata | 8 / 8 | 768 |
| Constant topology | 5 / 5 | 480 |
| Topology extensions | 3 reused / 3 | 2,880 |
| Type configuration | 6 / 6 | 576 |
| Total | 27 distinct / 30 | 5,080 |

Scalar matrices derive candidate cardinality from the authentic generated
observation metadata. They retain positive/negative named candidates, known/
unknown selections, repeated filters, minimal projection and logging choices.
The bootstrap matrices retain all six database states, four module assemblies,
one/two rounds and both logging choices. Extension inputs retain all target,
Checker and Behavior combinations in their original order.
The separate `bool_actual` fixture has an original-source compilation failure
and is not one of these eight executable scalar candidate models.

The shared input helper reproduces the pure recipes in the unchanged historical
collectors. A separate gate evaluates those retained recipe ASTs and checks every
corpus input. It also binds the thirty actual CTest commands, including the
model `--case`, comparator path, native binary and required recipe fixture.
Executing the input-producing AST is not executing the Rust observer.

## Reproduced admission failures

The [preceding actual CLI controls](parity/configuration_cli_admission_before_5_0_6.json.gz)
complete **780 controls**, reproducing **249 false acceptances** and no rejection
of a valid complete matrix:

- Twenty-seven invocations each accept nine corruptions: empty list/object,
  partial, appended, duplicate-at-equal-count and reordered cases, an extra input
  field, a Boolean input replaced by an integer, and an extra case-record field.
- Three topology-extension invocations each accept an extra case-record field
  and an incorrect model-variant declaration.

Other controls cover missing expected values, wrong baselines, malformed case
containers, incomplete/extra/blank/invalid JSON output, observer failure,
added/missing result fields, changed text and integer-to-Boolean/float changes.
The strict result comparator remains unchanged.

Replay observers use actual native outputs for the valid matrix. Two intentional
input aliases allow a counterfeit observer to return matching output for a
corrupted input, isolating recipe admission from observer validation. These
replayed outputs are comparison-boundary controls, not Rust-source evidence or
claims about application execution on invalid input. Disposable corpora are
ordinary task-owned files. Generated fixtures are only read through links, with
their bytes checked again after the controls.

## Implementation and permanent checks

The four previously unguarded scripts use the existing shared case-shape guard
and exact input recipes. Topology extensions retain their existing `cases()`
recipe and add the case declaration and exact row-shape checks. Existing output
comparison, generated fixture checks and optional live-original branches retain
their responsibilities. No generated domain code or expected outcomes change.

`parity_configuration_cli_recipes` is a fixture prerequisite for all thirty
comparisons and `parity_configuration_cli_admission`. The latter runs the same
780 controls against installed comparators, including a positive complete-matrix
control for every invocation. It has no default report-file side effect.

The [installed native reconciliation](parity/configuration_cli_installed_native_reconciled_5_0_6.json.gz)
verifies **32/32 passing CTest checks**, **780/780 controls** with no unexpected
acceptances/rejections, and **5,080 retained native outcomes**. Registration is
**964**, not a full 964-test execution. The affected native build reports no work.
Pinned formatting of 1,809 handwritten files, its check and the reference/path/
generator guards pass. No fresh sanitizer, MIPS or physical-device run is claimed.

The [initial result record](parity/configuration_cli_installed_native_5_0_6.json.gz)
retains the actual successful CTest exit and all 32 passing JUnit records. Its
packaging assertion stops because JUnit truncates admission stdout at 1,024 bytes.
The reconciliation retains the complete CTest log, checks every model completion
and the 780-control summary, and verifies unchanged inputs and native executable
hashes. It reuses that one successful run explicitly; it does not invent a rerun.

The [preservation proof](parity/configuration_cli_preservation_5_0_6.json.gz)
restores every preceding comparator byte by removing only the admission edits.
The CMake change is an appended registration block. All 1,814 C++ inputs, 831
producer inputs, 80 SDKs / 3,560 generated files and 2,467 protected artifacts
retain their previous identities. An initial preservation assertion omitted
module/include suffixes; its preparation failure remains separate from acceptance.

Measured task-owned cleanup returns **33,326,563 bytes** from the diagnostic
workspace and JUnit directory. The installed admission workspace is also removed;
its final size was not exported by the default report-free gate. Stable native
outputs and shared caches remain in place. Compact receipts retain the commands,
source identities, actual failures and terminal results.

## Remaining source qualification

The subsequent [configuration source checkpoint](configuration_cli_source_qualification_5_0_6.md)
now supplies the original execution, dependency closure and driver binding for
these 27 models / 30 selections. The admission receipt itself retains its earlier
scope. The following requirements describe the source follow-up identified at
that admission checkpoint.

This repair does not qualify the complete dependency/driver execution of these
five families. Their next source gate must preserve the complete locked runtime
archive, all 27 unchanged generated model configurations, observer helpers and
complete raw results for the 30 actual selections. Each variant needs its own
observer package/lock when generated package names collide; do not rename or
patch generated crates to combine variants. Reference-seeded resolution must
reject registry identities outside the reference lock.

The [static dependency review](parity/configuration_cli_dependency_review_5_0_6.json.gz)
finds that four historical collectors explicitly request `rusqlite =0.40.2`,
while the reference lock contains `0.40.1`. The 27 generated manifests declare
`0.40` and remain unchanged. This inspection does not measure an actual compiled
dependency closure. A successful replay of the historical collectors alone
would not establish reference-lock acceptance.

Reused topology models must remain explicitly associated with their distinct
extension drivers. Native comparisons use identity semantics; no normalization
or approved compatibility exception is introduced by this repair. Temporal
constant collectors, other custom/protected programs, full G14/AP02 and all
remaining integration/P0-P7 requirements remain separate.
