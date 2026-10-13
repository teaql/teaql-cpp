# Generated execution extensions: Rust 5.0.6 checkpoint

Status: G09 and the relevant G12/G17 application-executor responsibilities are
partially implemented and qualified for the recorded fixture. P0–P7 remain active.

## Fixed source and public API

Runtime v5.0.6 is pinned at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The producer is `45e1572c4956ca963410d8ea436ead3c8857087a`.
The complete unmodified dynamic_json generated crate supplies the reference.
The new application methods are thin adapters over the shared runtime:

```cpp
auto Q = user_context.queries<cpp_dynamic_json::query_factory>();
auto request = Q.orders().comment("inspect generated execution")
                         .purpose("qualify generated execution");
auto& count = request.executeForCount(user_context);
auto& exists = request.executeForExists(user_context);
auto& first = request.executeForFirst(user_context);
auto& page = request.executeForPage(user_context, 1, 10);
```

Purpose stages execution; comment and purpose must remain nonblank. Exactly one
UserContext supplies runtime dependencies. Page offset/limit are values, not
additional providers or Contexts. Operations freeze destination-owned snapshots
and preserve the reusable description. The source/result model distinction stays
intact; count and exists do not decode the adapted result model.

Count clears normal/expression projections, ordering, slicing and relations,
then appends the source `count` aggregate and reads its first compact-row value.
Missing/NULL/negative/text/bool/floating results fail with the source Graph
message; positive integer and source-supported Decimal conversion use the core
Value protocol. Other query state is preserved, including existing aggregates,
readonly/dynamic metadata and optimization hints. The ordinary runtime list
ceiling still applies as it does in the source.

Exists retains projections, ordering and offset, replaces the limit with one,
clears relations and tests compact-row presence. First aliases the already
accepted first-or-empty one behavior. Ordinary page execution counts before
fetching the typed list. ID-set pages fetch the list first and reuse any supplied
total; only absent totals trigger a count. A known total of zero is present.
The existing ID-set/continuous-page protocols are exposed with camelCase
configuration methods. No external cache component is introduced. Invalid
configuration preserves source diagnostics, with C++ exceptions adapting Rust
panic behavior.

## Restored application executor responsibility

The frozen producer emits a distinct ServiceRuntimeExecutor over its SQL
executor. Its query/mutation protocols retain default observation behavior and
conservative unclassified query-intent provenance. Streams, transaction and
dynamic-field SPIs delegate to the existing inner executor.

C++ previously aliased the SQL executor directly. That merged responsibilities,
classified application intent differently and recorded failure SQL logs that the
original generated wrapper did not expose. Each generated domain now defines a
distinct ServiceRuntimeExecutor over shared GeneratedServiceRuntimeExecutor
mechanics. DataServiceExecutor aliases that application type. Query/mutation
calls delegate SQL work while preserving the frozen default observation and
intent behavior; underlying SQL executor observers remain available independently.
Stream/transaction/provider/governance algorithms are not duplicated. Inherited
C++ guarded-mutation support remains an existing protocol adaptation, not a new
Rust wrapper implementation claim. Cache ownership checks and explicit Context
lifetimes are preserved.

## Actual source observations

The corpus contains **1,440** unchanged original/C++ observations: **1,232**
successes, **112** execution failures and **96** configuration panics/exceptions.
Each compares results, total counts, complete errors and ordered SQLite SQL,
parameter Debug values, comment and purpose. No SQL/result/error normalization
is applied. It covers four model types, normal/minimal selections, result-model
adaptation, nested criteria, empty/absent rows, offset/limit replacement,
zero-sized pages, ID-set bounds/fallback, continuous-page hints and missing tables.
The old dynamic JSON corpus remains separately qualified.

Both programs use the same existing foreign-key-enabled SQLite schema and data.
This does not qualify successful generated ensure_schema; its unchanged source
failure remains recorded in the dynamic JSON checkpoint. Missing-table cases
rename a real table after setup and exercise actual executor failure, rather than
fabricating result errors. The separate scalar Context unit tests fabricate only
source row availability/value cases to test the asynchronous conversion and
cleanup boundary; they are not paired SQLite observations.

## Lifecycle and verification

Scalar composition forwards child wakes, consumes completed rows and cancels
pending child work before Context host-memory reclamation. Unit tests use null
default PMR, a live 32 KiB cleanup buffer and actual UserContext deletion. Runtime
SQLite tests verify independent snapshots, unsliced counting, retained exists
offsets, typed pages/total counts and first/one equivalence.

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
python3 scripts/verify_generator_parity.py
python3 scripts/generate_examples.py --check
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin generated_execution_5_0_6
python3 tests/parity/compare_generated_execution.py --cpp build_native_current/teaql_generated_execution_probe --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_execution_5_0_6
```

Final gates completed after compiler/linker termination:

| Gate | Result |
| --- | --- |
| Native execution/API/candidate/checker/constant/relation regressions | 10/10 |
| Targeted ASan/UBSan execution, Context and retained JSON tests | 5/5; leak scanning disabled |
| Module import contracts | 1/1 |
| Java generator/field-layout/naming/pluralization | 35/35 |
| Generated compiler contracts | 3 positive, 3 negative |
| C++ artifact reproduction | 15 sets, 165 files |
| Module export catalog | 1,458 entries across seven modules |

The constant topology probe now registers each fixture's actual domain executor
rather than assuming all generated executor aliases identify one SQL type.
Its original 36-case corpus remains unchanged and passes. Frozen references
remain clean; no production dependency was added.

## Remaining scope

- Further inherited/constant/candidate/temporal/dynamic model configurations,
  grouped/aggregate/facet pages and complete policy/trace/privacy matrices.
- Deferred typed-page stages, both cancellation branches, custom store behaviors,
  replacement providers and repeated cached-page interleavings.
- Remaining general Q/predicate/projection APIs and pagination conveniences.
- Complete generated assembly, sample-data, immutable field/diagnostic and
  source-disposition qualification; neither wrapper restoration nor these counts
  complete G12/G17 or the full generator goal.
