# Sample progress Info logging: Rust 5.0.6

The shared runtime now dispatches the six original sample-generation Info log
sites through an optional application-owned `SampleDataLogSink`. Generated
recipes continue to delegate to the shared sample runtime; no per-model logging
algorithm or production dependency is added.

## Original responsibility and Context adaptation

The locked producer is `45e1572c4956ca963410d8ea436ead3c8857087a`; the runtime
reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The complete unchanged
Rust inheritance library executes its actual sample entry with a test-only Rust
`log::Log` collector. Original level, module target and message remain in the
retained source corpus. The existing test observer is copied with only Medium
scale dispatch added; original generated and earlier observer files are unchanged.

| Original log site | Shared runtime dispatch position |
| --- | --- |
| Start with scale and seed | First sequence poll, before any root/constant/model step. |
| Model start with expected count | After required dependency checks, before its generation loop. |
| Model progress every 20 records | After a successful save and count update, before reverse-reference publication. The first successful record reports index zero. |
| Model completion | After its complete successful loop. |
| Missing required dependency | After recording the skip and before returning success for that model step. |
| Whole-operation completion | After every step succeeds, using generated/skipped table counts. |

Rust's process log facade becomes an explicitly registered Context capability:

```cpp
user_context.insert_resource(owner, static_cast<SampleDataLogSink&>(sink));
```

Allocate the sink in that declared Global/User owner. Its `memory_resource()`
identifies the owner; existing resource registration validates domain and
liveness. `enabled(user_context)` gates formatting, and the synchronous
`info(user_context, message)` receives borrowed text. A retained message needs
an explicit copy into the sink owner's allocator. The sample-specific Info
method replaces the original module-path routing; C++ does not manufacture a
Rust module target. Original INFO/target metadata is retained and validated
before comparing exact message bytes and order.

Missing and disabled sinks construct no message. A separate header-only unit
uses a throwing formatter to prove the formatting branch is unreachable. Public
module consumers exercise the exported sample entry instead of importing the
private dispatcher. No singleton, default PMR
owner, thread-local cache or shared lifetime extension is introduced. SQL
logging and mutation audits remain separate runtime responsibilities.

## Execution and lifetime checks

The retained 40-case matrix executes 62 sample invocations and records 478
original Info messages. It covers Tiny/Small scales, both enabled states,
normal/missing roots, two seeds, repeated invocation, Medium scale and actual
SQLite save failures at school/academy/research_academy. Comparison includes
complete timeless SQL diagnostics, reports/errors and persisted rows from the
underlying sample observer; timestamps are omitted as in its established
contract, and other output is not normalized.

This log matrix uses the combined runtime factory. The earlier
[inherited sample matrix](inherited_sample_execution_5_0_6.md) retains its separate
92-case, four-factory full-row/SQL comparison; its output is not relabeled as
progress-log evidence.

The module preparation failure caused by a unit calling the private dispatcher
is also retained. It was corrected by separating public-entry tests from the
internal formatter unit; no private helper was added to the export catalog.

Native observers disable default PMR, destroy owning values before actual
UserContext deletion, and require live User bytes to return to zero for every
case. Medium observation uses a 1 GiB test arena budget; production limits are
unchanged. The initial observer include-path build failure is retained separately
from behavioral comparison evidence.

Independent runtime tests exercise lazy start, pending re-polls, resource
replacement, explicit cancellation, actual User deletion, missing/disabled
formatters, throwing callbacks, Global-owned retained copies, expired resources
and unrelated owner rejection. Existing synchronous callback scopes protect
both the invoking Context and the sink owner. A child-process control verifies
that closing the active Context from the callback terminates before reclamation.
Exceptions propagate through the asynchronous operation's failure path; neither
failure nor cancellation produces a successful whole-operation completion.

Generated Assist documents registration, ownership, borrowed messages and these
failure boundaries. Regeneration remains the only way to change generated files.

## Completed gates

The complete original/native comparison passes **40/40**. Native runtime and
retained sample regressions pass **6/6**; module import/lifetime checks pass
**3/3**, plus **14/14** retained sample comparisons. Targeted ASan/UBSan checks
pass **3/3**, plus the same **14/14** comparisons. The selected module and
instrumented matrices each compare **99** original messages; neither is claimed
as a full 40-case replay. Leak scanning is disabled.

Related Java regressions pass **53/53**. All **23 workspaces / 276 files** and
**44** complete contract SDKs strictly reproduce. Only **30** SDK Assist files
change; their generated code remains byte-identical. Canonical module checks
verify **624** consumer/fixture bodies and **1485** exports across seven packages.
Pinned formatting checks **1416** handwritten C++ files. Original sources and
reference locks verify. Full-suite, MIPS and physical-device gates are not rerun.

[Raw validation output](parity/sample_progress_validation_5_0_6.json.gz) preserves
terminal successes and preparation failures, including the corrected manual
module executable path. These failures do not become successful behavior evidence.

## Reproduction and remaining scope

```sh
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_sample_progress_probe teaql_sample_data_logging_tests
python3 tests/parity/compare_sample_progress_5_0_6.py \
  --cpp build_native_current/teaql_sample_progress_probe
ctest --preset native -R '^sample_data_logging_'
```

`--rust` replays the original collector alongside the retained corpus. `--cases`
selects explicit retained indices for focused imported/sanitizer consumers;
initial capture always requires all 40 cases. Original observations are immutable.

This progress-log responsibility does not close G12/P6/P7. Arbitrary scalar,
JSON and Business-ID sample configurations, customized providers/policies,
complete asynchronous interleavings and full Assist workflows retain their
existing obligations. The whole generator goal remains active. Terminal gates
and their actual verification scope are recorded in
[the evidence record](parity/sample_progress_logging_5_0_6.json).
