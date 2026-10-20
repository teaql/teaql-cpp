# Public facet execution for Rust 5.0.6

This checkpoint aligns the runtime's public `execute_facets` entry with frozen
Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. G08, G17 and complete P0-P7
qualification remain open. Generated-domain helper behavior is a separate
locked-source contract, described below.

## Source findings and runtime correction

The first old observer aborts on an unrecognized RequestIntent error. The
original public runtime returns that error; the fixture's error formatter
panics. A companion observer adds the actual error variants and optional root
purpose input, while calling the unchanged original runtime algorithms.
Historical drivers and their inputs remain preserved.

The expanded original/native comparison initially finds 1,096 differences in
1,178 observations. Native public facet execution still uses the earlier
behavior. The correction preserves the existing public function and service
interfaces and implements the 5.0.6 responsibilities:

- Validate root comment and purpose before metadata/service dispatch, including
  empty facet options.
- Resolve the outer entity and relation even when include-all is selected.
- Merge outer filters into applicable aggregates and apply the existing
  relation restriction when required.
- Materialize the complete QuerySelection, inherit root intent, and place the
  modeled relation edge in the query's own trace. Pass an empty repository
  prefix so ancestry is not duplicated.
- Retain one invocation-local diagnostic source covering future facet and
  aggregate bindings. It is classified through the existing service machinery,
  never executed or stored as Context state.
- Fetch each facet sequentially, recursively execute its nested options, and
  replace the fetched list's prior facet map with those recursive results.
  Duplicate names retain the source's last-result-wins behavior.

The operation copies inputs and nested state into UserContext. Its cancellation
path stops the active child or recursive operation before query/row storage is
reclaimed. No new dependency, shared owner or ownership domain is introduced.

## Public runtime and generated helper distinction

The frozen generated domain contains its own helper in
[request_support.rs](../tests/generator/parity/dynamic_json_rust/lib/src/request_support.rs).
Lines 551-588 retain `Calculate facet`, a separate repository trace prefix and
nonrecursive compact facet results. The public runtime helper at lines 374-440
of the original `teaql-runtime/src/generated_support.rs` uses validated root
intent, query-owned ancestry and recursive nested execution.

These are two actual source contracts. The generated C++ operation retains its
qualified generated-helper semantics. Its unchanged 1,184-case regression still
passes after this public runtime correction. This checkpoint does not change
the generator, patch original generated files or infer identical behavior from
the matching helper names.

## Observation and ownership scope

All **1,178/1,178** public observations match without normalization. Inputs retain
all 370 historical stimuli and add valid intent, validation ordering and nested
execution matrices. They include 48 actual SQLite inputs, 1,062 recorded runtime
callbacks, metadata/Policy failures, suspension/resumption and cancellation.
Full public query Debug, request/prefix traces, rows, aggregate/facet metadata
and exact errors are observed.

A separate **36/36** actual SQLite matrix covers direct, nested and aggregate
future bindings, include-all/restriction, three canaries and logging switches.
Its 18 logged controls retain 48 facet statements and 18 follow-up statements.
Stable diagnostic fields observe parameterized SQL, masked Debug SQL, intent,
parameters, trace and outcome. Timing is outside this observation schema; no
source result is normalized. Future protected literals are absent from facet
diagnostics. A subsequent query on the same Context retains its unrelated
intent, demonstrating that the private classification source does not persist
on Context. Disabled logging produces empty histories.

Native controls disable the default PMR resource and check root and nested
pending work during actual User deletion, foreign-future rejection without
taking cancellation authority, input snapshots, and complete Global copies of
actual nested results. Existing generic-purpose and checked-runtime-expiry
controls remain active.

The purpose-wrapper diagnostic and DataStore families also have fresh original
replays: **46/46** and **251/251**. Separate active corpora retain the unchanged
historical files; see [their record](parity/runtime_support_baselines_5_0_6.json).

## Reproduction and remaining gates

```sh
set -e
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_runtime_baselines_506 \
  CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_generated_support_5_0_6/Cargo.toml
cmake --build --preset native --target \
  teaql_generated_support_probe teaql_generated_support_tests
python3 tests/parity/generate_public_facets_5_0_6.py \
  --rust /tmp/teaql_cpp_runtime_baselines_506/debug/teaql_generated_support_506 \
  --cpp build_native_current/teaql_generated_support_probe --check
python3 tests/parity/generate_public_facets_5_0_6.py \
  --rust /tmp/teaql_cpp_runtime_baselines_506/debug/teaql_generated_support_506 \
  --cpp build_native_current/teaql_generated_support_probe --logs --check
python3 scripts/verify_runtime_support_baselines_5_0_6.py
```

The related native selection passes 11/11, targeted ASan/UBSan passes 4/4 with
leak scanning disabled, and canonical module checks pass 3/3. The module verifier
checks 172 consumer/fixture bodies. Their terminal results belong in the
[verification record](parity/public_facets_5_0_6.json). The full native suite
has not been rerun or accepted. Broader models, policies, sensitive sinks,
plaintext controls and capability/phase exit criteria remain separate work.
No MIPS, physical-device or GPU verification is claimed.
