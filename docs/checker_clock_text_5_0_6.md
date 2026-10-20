# Generated string-clock fixes: Rust 5.0.6

Status: the demonstrated required-string compilation gap is repaired and its
48 source observations match. G02/G03 and the
complete generator goal remain open. The initial failure is preserved in
[the original gap archive](parity/checker_unique_string_initial_gap_5_0_6.json.gz).
The later [configured scalar clock checkpoint](checker_clock_configurations_5_0_6.md)
extends the shared adapter to numeric, Date, Boolean and JSON fields. The
string-specific implementation and counts below describe this earlier checkpoint;
clock_text now delegates to clock_value without changing its Timestamp behavior.

## Source contract and runtime repair

The frozen producer defines `uniStr()` as a string with createFunction metadata.
Its Checker template passes Context fix_time to the generated setter. That
setter accepts Into<Value>, preserves the old stored text when the operand
cannot convert to text, stages the original Timestamp operand, and marks the
field loaded. This source behavior is retained even though it does not create a
new unique string.

The original C++ Checker called the public string setter with Timestamp and
failed compilation. The repair keeps the public setter typed as string_view.
The shared runtime now implements update_text_value for required and optional
text. Existing typed text updates delegate to the same implementation:

| Operand | Required storage | Optional storage | Mutation operand |
| --- | --- | --- | --- |
| Text | Trimmed text | Present trimmed text | Original untrimmed Text |
| NULL | Previous text | None | NULL |
| Timestamp or another non-text operand | Previous text | Previous optional value | Original operand |

Optional text is allocated before replacing its previous value. This preserves
the prior construction order on allocation failure. All nested text uses the
owning Context resource; the new overloads do not add a default allocator or an
implicit owner.

GeneratedFieldChecks::clock_text delegates clock selection and fix evidence to
the existing clock implementation. It stages the value through the shared text
helper and marks the storage field loaded through the owner's SnapshotPool.
Evidence paths and storage keys remain separate parameters. The generator emits
a thin call to this helper for string clock fields; timestamp-field adapters
retain their existing implementation and rule order.

## Semantic and ownership checks

The new source observer calls the complete unchanged generated NoteChecker in
the [minimal original model](../tests/generator/checker_unique_string.xml).
The 48-input matrix crosses Create/Update/Unknown, full/sparse load state, four
text payloads and two fixed process wall clocks. Empty, whitespace, long Unicode
and embedded-NUL text are data fixtures. Results include the actual stored
getter, load availability, changed-value presence, exact Timestamp operand and
complete findings in three naming profiles. No result normalization is applied.

The native probe additionally checks optional NULL/fallback behavior, untrimmed
mutation operands and the existing graph-clock evidence contract. These are
native runtime controls, separate from the 48 direct source observations. Its
negative compile assertion retains the public setter's typed input. Observations
are explicitly copied into Global storage; UserContext is actually deleted before
output is consumed. Default PMR is disabled. No SQL execution or committed-save
claim follows from this direct Checker matrix.

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_checker_clock_text_probe teaql_sample_fixed_clock
python3 scripts/verify_checker_clock_text_5_0_6.py
```

The verifier builds with --offline --locked, checks registry versions and
checksums against the runtime reference, verifies frozen producer artifacts,
and compares the native results with live original execution. Generation remains
a development step. The original handwritten examples and source worktrees are
preserved.

## Remaining acceptance

Terminal selected gates pass native 7/7, modules 1/1 and ASan/UBSan 5/5 with
leak scanning disabled. Java generator/metadata regressions pass 46/46. All
21 retained workspaces/252 files and 44 contract SDKs strictly reproduce;
566 canonical consumer/fixture bodies and 1386 pinned-format C++ files verify.
The [execution record](parity/checker_clock_text_5_0_6.json) retains successful
results and fingerprints. The [source record](parity/checker_clock_text_source_5_0_6.json)
retains the live comparison and exact dependency checks. Initial observer and
test-fixture errors are preserved separately in
[their archive](parity/checker_clock_text_initial_validation_failures_5_0_6.json.gz).
No MIPS or physical-device run was performed.

Optional and configured non-timestamp clock-field model branches still require
their own original-source dispositions and generated execution matrices. This
repair closes the demonstrated required uniStr compilation difference; it does
not qualify all G03 clock configurations, graph persistence, initialization,
privacy, loaded-cache or request-intent behavior. Their existing requirements
remain in the capability ledger.
