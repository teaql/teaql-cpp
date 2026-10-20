# Inherited sample execution and trace: Rust 5.0.6

Status: all 92 original/native comparisons and the declared native, module and
sanitizer gates pass. G12/P6/P7 remain open.

The runtime and producer are the exact upstream and generator ledger locks.
This test extends the earlier sample checkpoint's source-only inherited row
counts to complete paired SQLite row/report/diagnostic observations.

## Actual application execution

Both observers call the existing public generated module factories and sample
entry. They install the actual SQLite provider, scoped IDs and distinct generated
application executor, then run ensure_schema. The original inherited entity,
checker, behavior, runtime and sample files remain unchanged. Test code selects
inputs and observes outcomes; it does not implement sample recipes or persistence.
The independent Rust manifest uses reference registry versions/checksums.

Four metadata/checker/behavior/combined factories run the three-level
School/Academy/ResearchAcademy hierarchy. The 92 configurations exercise Tiny
and Small, zero/seven/boundary seed, normal/missing/modified prepared roots,
one/two consecutive calls and real INSERT-trigger failures in each business table.
Failure cases run a second invocation against the same database and Context.

Each round compares all physical columns and SQLite value kinds in platform,
school, academy, research and ID-space tables, before/after state, the complete
sample report or error and every ordered SQL diagnostic field except clocks:
log context/provenance, operation, comments, purpose, audit reason, full typed
trace, parameter Debug, raw/debug/pretty SQL, counts/type/affected rows and summary.
Only started_at, ended_at and elapsed are omitted because clocks differ.
No IDs, row order, errors, trace nodes, SQL text or binding values are normalized.

The observation includes the actual inherited audit-mask behavior: safe logs
retain their existing projections while raw SQLite rows remain authoritative.
Root and constant loading errors, statement/save failure and earlier committed
rows use existing runtime semantics. This is not a new transaction around an
entire sample invocation, nor a successful recovery after every possible failure.

## Exact original/native outcomes

All **92/92** configurations and **144** invocations agree, including **26868**
ordered SQL records and **11808** business rows across post-invocation snapshots.
These row counts include retained earlier rows and repeated observations, not
11808 unique newly created entities. **12** invocations reject actual writes.
The eight-case smoke archive also agrees on **1472** ordered SQL records.

The full original/native observations remain in
[the comparison archive](parity/inherited_sample_source_5_0_6.json.gz), and the
source corpus retains the unchanged model/producer provenance. SQL context and
safe binding projection are compared as actual diagnostic data; there is no
synthetic trace renderer or callback implementation in either observer.

## Ownership and preparation failures

Native execution disables default PMR. Every owning input/result leaves scope
before actual UserContext deletion; live User arena bytes must return to zero.
The observer uses a 512 MiB test budget for stored diagnostic/operation data;
production limits are unchanged. SQLite is the only runtime dependency.

Preparation records preserve the test manifest's missing direct provider trait
import, the ambiguous native executor name and the exact existing DebugWriter
constructor/sequence API corrections. These were observer fixes, not changes to
original or runtime behavior. Source/type inventory alone is not acceptance.

## Completed gates

Native comparison **1/1**, retained sample/async-save regressions **2/2**, module
consumer **1/1** and targeted ASan/UBSan **1/1** pass. Leak scanning is disabled.
The full 92-case matrix executes on all three selected consumers. Canonical
module verification checks **621** consumer/fixture bodies; pinned formatting
checks **1411** handwritten C++ files. The existing **1483** exports and reference
locks verify. No production runtime, generator or original body changed; Java,
SDK regeneration, full-suite and MIPS/physical-device gates are not rerun here.

[Terminal evidence](parity/inherited_sample_execution_5_0_6.json) and
[validation logs](parity/inherited_sample_validation_5_0_6.json.gz) retain actual
outcomes. The preparation failures remain separate immutable records.

## Reproduction

```sh
set -e
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
cargo build --offline --locked \
  --manifest-path tests/parity/rust_inherited_sample_5_0_6/Cargo.toml
cmake --build --preset native --target teaql_inherited_sample_execution_probe
python3 tests/parity/compare_inherited_sample_execution_5_0_6.py \
  --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/teaql_inherited_sample_506 \
  --cpp build_native_current/teaql_inherited_sample_execution_probe
```

A fresh archive is required for capture; retained smoke and original observations
are immutable. This matrix does not close progress/info log delivery, arbitrary
scalar/JSON/Business-ID sample configurations, customized providers/policies,
async interleavings, complete Assist or G12/P6/P7 acceptance.
