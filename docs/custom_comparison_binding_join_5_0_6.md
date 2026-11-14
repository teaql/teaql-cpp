# Custom comparison and source-fixture join for Rust 5.0.6

This checkpoint continues the [direct-corpus join](direct_corpus_binding_join_5_0_6.md)
with the complete **160 named custom comparison registrations** from the
retained 1,058-test locator inventory. Source, selection and actual execution
are joined below. Complete SQ02 and G13/G14/G17 remain open at
**14 qualified / 3 partial**.

## Current source associations

The [binding manifest](parity/custom_comparison_binding_manifest_5_0_6.json.gz)
records **35 source cohorts / 180 explicit source-member selections**. Each
current command retains its earlier qualified selection and native role.
The two changed script paths use the existing hash-checked comparator
transitions for generated expression diagnostics and SQLite raw Debug output.
The original scripts and source records remain unchanged.

The [permanent verifier](../scripts/verify_custom_comparison_bindings_5_0_6.py)
checks the exact native command, active script hash, required fixture, unique
source setup, source-validator command and source-contract identity. Selected
metadata members and source/helper/evidence hashes remain explicit. The
comparison family includes `compare_*`, twelve `diagnose_memory_*` entries
and the clock process wrapper. A filename prefix alone did not identify the
whole cohort.

Selection retains the existing responsibility of each comparator:

- Profile checks select the declared debug expectations; formatter checks
  retain the audit, SQL and configuration groups.
- Inherited Checker entries select one modeled subtype and 1,440 outcomes
  each, covering all five models across their five registrations.
- Transaction comparison selects the two historical sync/async routes,
  **366 outcomes**. The separate 144-case lease matrix retains its own direct
  registration and source binding.
- The clock boundary retains **36 actual value/process-failure cases**.
- Transaction-intent framing, additional generated consumers and both memory
  engine matrices remain in their actual caller selections.

The index retains declared projections and their exact metadata/code
identities. Existing qualified comparators and protocols perform projection,
width/profile selection and result checks. Their observations are not
rewritten by the join, and overlapping source datasets are not summed as
new independent observations.

## Corruption controls and actual execution

Thirteen controls alter copies of the current associations: missing or
duplicated entries, changed comparison arguments, reverted historical script,
missing source requirement/setup, changed source command, wrong baseline or
contract hash, missing/changed/duplicated member selection and an unqualified
extra comparison. All reject in ordinary and optimized Python. The
[ordinary](parity/custom_comparison_binding_join_5_0_6.json.gz) and
[optimized](parity/custom_comparison_binding_join_optimized_5_0_6.json.gz)
index receipts retain identical valid rows and control outcomes.

The new CTest index requires all 35 source fixtures and the independent
configuration recipe fixture. The
[terminal execution](parity/custom_comparison_binding_join_native_5_0_6.json.gz)
runs all 160 actual comparisons, all source prerequisites, both binding
indexes and the recipe prerequisite: **198/198 ordinary and 198/198 optimized
tests**, with exact registry/JUnit membership and no failures, errors or skips.

All **11,958 source inputs, 150 evidence files, 707 native ELF files** and both
locked reference worktrees remain unchanged. Native binaries are executed
after their existing completed builds. No new Rust/C++ compilation, sanitizer,
MIPS or physical-device execution is involved.

The coordinator removes **106,111 bytes** of owned JUnit output after its
children exit. Four compact binding/execution reports total **608,964 bytes**.
Core dumps are disabled only for the owned coordinator and its descendants;
shared caches, stable builds and other sessions remain untouched.

The registry now contains **1,099 tests / 617 Python entry scripts**, with
zero direct assertions in those entries. The latest broad 1,075/1,075 epoch
retains its historical source state; this is a complete named-custom cohort
execution rather than a current full native epoch.

## Remaining SQ02 boundaries

Comparisons outside the immediate parity directory and source/control
registrations retain their own provenance and result-boundary obligations.
The [child-verifier witness](parity/generator_verifier_child_preedit_5_0_6.json.gz)
already identifies a concrete continuation: source prerequisites invoke the
historical generator verifier, whose 21 assertions disappear in optimized
Python. Six actual processes show that a hand-edited Rust provenance marker
and a wrong model hash are rejected normally but accepted under optimization.
No repository metadata is edited by that isolated witness. The later
[child/parent repair](generator_child_optimized_checks_5_0_6.md) qualifies the
21-check replacement and both active source callers, with 18/18 native tests
per mode and P0 1/1. Remaining paired source/native, dynamic and control
boundaries still require complete SQ02 signoff.

Reproduce the index and its required source fixtures:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_custom_comparison_bindings_5_0_6.py
python3 -O scripts/verify_custom_comparison_bindings_5_0_6.py
ctest --preset native -R '^parity_custom_comparison_binding_join$' --output-on-failure
```
