# Direct comparator and source-fixture join for Rust 5.0.6

This checkpoint joins the complete existing direct-corpus cohort to the
current native registry. It continues [SQ02](semantic_qualification_requirements_5_0_6.md#acceptance-obligations)
after the [executed import review](executed_import_boundary_review_5_0_6.md).
G13, G14 and G17 remain partial at **14 qualified / 3 partial**.

## Checked relationships

The [ordinary index](parity/direct_corpus_binding_join_5_0_6.json.gz) and
[optimized index](parity/direct_corpus_binding_join_optimized_5_0_6.json.gz)
contain **305 direct comparator registrations / 323,568 observations**. Each
registration has exactly one matching source contract among its required
CTest fixtures. The cohort uses **27 source fixture owners** and **44 retained
execution selections**. Extra prerequisite fixtures remain intact.

The [permanent verifier](../scripts/verify_direct_corpus_bindings_5_0_6.py)
resolves the actual shared-adapter call in each script and records:

- The exact script, source hash, CTest command and native target.
- The nonempty corpus, byte hash, locked runtime baseline and complete count.
- The required fixture, setup test, contract file/hash and named contract.
- The exact retained execution selection/hash used by that source fixture.

Test names, native targets, source/corpus counts and available declared corpus
hashes must agree. The runtime and generator revisions remain
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The shared `compare_values.py` identity is pinned to the already qualified
implementation. It preserves JSON types, floating negative zero, exact object
membership, LF record framing and complete output cardinality. The actual
registered command must select the declared native executable with an empty
native emulator. Both existing empty-emulator argument spellings are admitted.
No result normalization or source observer is added by this join.

A separate [complete-source exit review](parity/direct_corpus_failure_exit_review_5_0_6.json.gz)
retains all 305 wrapper bodies and hashes. Every actual `__main__` entry
propagates the shared comparison result through `SystemExit` or `sys.exit`;
none silently discards its failure return. This is a static caller review,
joined to the actual shared-adapter controls below, not 305 additional runtime
executions.

## Invalid relationship controls

Ten controls alter copies of the actual collected registry/contract state:
missing direct entry, duplicate CTest name, missing required fixture, missing
fixture setup, wrong corpus baseline, wrong source baseline, wrong count,
wrong binary, missing execution selection and wrong corpus hash. Each is
rejected under ordinary Python and `python -O`; valid indexes have identical
rows and control outcomes in both modes.

These controls exercise the join's structural decisions. They do not replace
the source validators' compiler/dependency/raw-output controls or establish
new original runtime execution.

## Actual source and native verification

The [terminal execution](parity/direct_corpus_binding_join_native_5_0_6.json.gz)
selects all 27 actual source fixture owners, the new join test and the three
existing corpus admission, inventory and comparator-result control tests.
CTest additionally runs the required executed-helper fixture, giving
**32/32 ordinary and 32/32 optimized tests**, with exact registry/JUnit names
and no failures, errors or skips.

The run preserves **11,957 input hashes**, both locked reference worktrees and
the affected native Value probe. This qualifies the current direct source/
fixture relationship together with actual source validation. It reuses the
retained original executions under their existing private-function, adapter,
dependency and profile scopes; it does not relabel them as fresh Rust builds.

The registry is now **1,098 tests / 616 Python entry scripts**, with zero
direct assertions in registered Python entries. The 32-test selection is not
a complete current native epoch. The last broad 1,075/1,075 epoch retains its
historical source state.

The four compact reports total **581,377 bytes**. The coordinator removes
**22,757 bytes** of owned JUnit output after all children exit. Stable build
directories and shared caches remain; no Rust/C++ compilation or copied
native executable is required.

## Remaining SQ02 work

Join the **160 named custom comparison registrations**, their 35 source
cohorts, and comparisons outside the immediate `tests/parity` directory to
their exact current source selections, normalization and complete result
boundaries. Account for source/control registrations added since the old
custom locator inventory and finish dynamic-loading/executable-string
dispositions. The absence of direct assertions and a successful direct-cohort
join do not establish complete SQ02 or G13/G14/G17/P0-P7 acceptance.

Reproduce the structural join without compiling Rust:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_direct_corpus_bindings_5_0_6.py
python3 -O scripts/verify_direct_corpus_bindings_5_0_6.py
ctest --preset native -R '^parity_direct_corpus_binding_join$' --output-on-failure
```
