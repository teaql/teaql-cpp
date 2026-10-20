# Application-owned extensions in generated sample execution

Status: complete original/native/imported comparison and selected instrumented
execution pass. This checkpoint supplements G12/P6 and preserves the
remaining G01-G17 and P0-P7 requirements.

References remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. The independent original observer
uses the complete unchanged candidate domain, original runtime and SQLite
provider. Model/SDK/source hashes and reference registry identities are checked
before recording or replaying an observation.

## Complete sample calls

The 140 original configurations execute 176 actual sample invocations. They
retain 1,608 immutable policy plans, 14,172 application callbacks, 1,204 actual
sample Info messages and 25,136 ordered SQL records, plus complete reports,
errors and before/after business-table rows. All four module factories are
covered. SQL wall-clock timing is omitted under the established sample contract;
other compared values are not normalized.

Application configuration covers:

- No custom checker, Person name/phase repair, Person rejection at ID 3, and
  Record rejection at ID 3 through generated typed CheckerLogic wrappers.
- No custom Behavior, query recording, descending constant-reference queries,
  rejected constant queries and registered insert-denial callbacks.
- Application allow, exact approval and Record denial, missing roots/constants,
  real Person/Record SQLite trigger failures and repeated calls.
- Enabled/disabled progress collection through the actual original log facade
  and the explicitly owned C++ sample sink.

Custom extensions are installed through an application module after the initial
generated runtime/schema setup. Both observers apply the same module mode with
the new typed checkers/behaviors before sample generation. No generated file or
production algorithm is modified to provide the observation.

The original yields 116 success/skip invocations, 44 custom-check failures,
eight policy-denied errors and eight real transport errors. Checker observations
retain entity, identity, name, status, prior finding count and model location.
The stored repaired fields, policy plans and SQL verify that changes pass through
the real persistence path. Callback order and repeats are compared directly.

## Query Behavior and graph persistence remain distinct

The actual callbacks are 6,404 Checker calls and 7,768 query Behavior calls.
There are **zero** `before_insert` calls on this sample graph-save path, even
when the application registers an implementation that would reject ID 3.
Those modes remain successful where other rules permit; the observer does not
fabricate an insert denial or change graph persistence to invoke that hook.

This agrees with the locked runtime bodies: `data_service/graph.rs`,
`execute_graph_plan_scoped`, constructs batch commands and calls prepared batch
execution. The distinct entity-service `resolved.rs::prepare_insert_command`
invokes the single-command `before_insert` hook. These responsibilities are not
merged. Applications must not treat this hook as graph authorization; the
whole-plan Mutation Policy is exercised separately.

The rejected constant query instead follows sample source `unwrap_or_default`:
its reference pool becomes empty and both dependent models are skipped. Reverse
constant ordering changes the sampled reference IDs through an actual query,
not through fixture-generated rows.

## Logs and lifetime adaptation

Original INFO level, authentic module target and message bytes are retained in
the immutable corpus's `source_raw` observations. The comparator validates the
level/target and exact correspondence to each message before comparing the
dedicated C++ sink's messages. It does not manufacture a Rust module target in
C++. This is the existing [sample logging adaptation](sample_progress_logging_5_0_6.md).
Failure/skip/success progress sequences remain distinct.

C++ rules, Behavior objects, policy callbacks, messages and observation values
belong to UserContext. Default PMR is disabled. Each case destroys observation
values before actual UserContext deletion and requires live User bytes to return
to zero. Production limits are unchanged; no owner extends a Context lifetime.

## Verification

Original corpus: `tests/parity/custom_sample_5_0_6_cases.json.gz`.
Focused instrumentation has a separate actual original process-order corpus;
its selection is 23 cases, 32 invocations, 402 plans, 3,449 callbacks, 239 Info
messages and 5,495 SQL records. Process-wide mutation execution IDs are compared
unchanged, not selected from a different full-process history.

Native and imported execution each pass the full 140-case matrix. ASan/UBSan
passes the independent 23-case process-order matrix with leak scanning disabled.
Formatting/check covers 1,486 C++ files; 693 canonical imported bodies verify.
The initial C++ observer's
dependent-template compilation failure is retained; explicitly naming the
existing RuntimeModule fixes the fixture without changing generated/runtime code.
This checkpoint does not alter generator code, so Java and artifact reproduction
are not repeated. MIPS and physical-device execution are outside this run.

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_custom_sample_probe
python3 scripts/verify_custom_sample_5_0_6.py \
  --cpp build_native_current/teaql_custom_sample_probe
```

The module build preset also omitted 93 registered imported contract targets.
They are now included, and the consumer verifier rejects future omissions. All
279 preset target names resolve in the configured Ninja graph. This is build
registration evidence, not a full build/test-suite result.

See the [terminal record](parity/custom_sample_execution_terminal_5_0_6.json.gz),
[preset gap audit](parity/module_preset_contract_gap_5_0_6.json) and
[sample matrix](sample_qualification_matrix_5_0_6.md),
[assembly matrix](runtime_assembly_qualification_matrix_5_0_6.md) and
[full generator plan](cpp_generator_parity_plan.md).
