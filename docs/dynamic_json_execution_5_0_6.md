# Generated dynamic JSON one and stream execution

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This extends the existing generated JSON translation/list checkpoint to ordinary
single-result and typed-stream execution. It does not qualify the fixture's
known original generated schema/bootstrap failure.

## Original APIs and observation boundary

The existing original/native observers now accept an optional execution mode.
Inputs without it retain their previous list behavior and result shape. Original
Rust calls the complete frozen domain's `filter_with_json`, staged intent,
`execute_for_one` and `execute_for_stream` methods. Native C++ invokes their thin
Context-bound generated adapters. The database uses the same retained explicit
schema/dataset and actual SQLite executor/provider; no local query evaluator is
substituted.

The new declared matrix has **2,240** original/native outcomes across Person,
Order, Product and Line; normal/minimal projection; existing slicing; field and
nested forward/reverse filters; unknown paths; ordering controls; zero-sized pages;
sequential JSON controls and malformed/empty-array failures. There are 320 one
requests and 1,920 stream requests. Streams use chunk sizes 1/8 and consumption
limits 0/1/99, covering no item polling, early close and exhaustion.

Each successful translation retains the existing query/filter/order/slice
observation. Execution records typed ID/version rows and exact error text.
After a stream closes, both observers run an actual generated Person query through
the same Context/connection and report whether it can be reused. Rust drops the
stream and checks native pool availability; C++ explicitly cancels its borrowed
stream before the reuse request. All input Contexts are actually destroyed.

The only diagnostic representation adapter remains the existing exact frozen
Line `order` descriptor spelling: Rust stores quoted metadata and C++ stores a
raw identifier for dialect quoting. Original observations remain verbatim. No
SQL, business value or error normalization is introduced.

## Source staging and lifetime adaptation

Original Rust stream chunk configuration occurs before `purpose`, which returns
its executable type. C++ retains its already agreed before/after-purpose fluent
adaptation. Person's human predicate spelling is `whose_id_is` / `whoseIdIs`;
the reuse request uses those authentic generated methods.

Initial test-driver builds incorrectly applied `stream` after Rust purpose and
used the non-human ID predicate on Person. Both compilers rejected those calls.
The observers were corrected using the actual generated declarations; no generated
API or original source was altered to accommodate the fixture.

The logical stream observations preserve the previously documented source-lazy
versus C++ startup/error-delivery adaptation. Zero consumption does not claim
identical native statement start timing. This matrix compares resulting rows,
errors, early close and successful reuse; it does not turn those into a claim of
identical asynchronous scheduling or relation/aggregate stream enhancement.

The native observer disables the default PMR resource. Query/JSON/result state
belongs to the explicit UserContext; no hidden owner keeps it alive after close.
The existing 1,181-case corpus remains byte unchanged and reproduces against both
updated observers. The original schema failure remains separately verified.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_dynamic_json_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --locked --bin generated_dynamic_json_5_0_6
TEAQL_LOG_ENDPOINT=off python3 tests/parity/generate_dynamic_json_execution_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_dynamic_json_5_0_6 \
  --cpp build_native_current/teaql_generated_dynamic_json_probe --check
ctest --preset native --output-on-failure \
  -R '^(dynamic_json_execution_rust_parity|generated_dynamic_json_rust_parity|generated_dynamic_json_context_contracts)$'
```

Ordinary custom RequestPolicy and scalar/page trace framing now have a separate
[3,072-case execution checkpoint](dynamic_json_policy_5_0_6.md). Inherited/constant/
candidate/temporal models, broader policy/behavior combinations, protected logging,
complete readonly/dynamic storage/facet execution and full Assist/integration
acceptance remain separate obligations. G10/G13 and P0-P7
remain partial.

## Terminal verification

All **2,240/2,240** outcomes agree and reproduce against live original Rust.
There are 2,016 successful outcomes and 224 exact original parse panics. All
1,728 translated stream cases close and reuse the actual executor. The unchanged
1,181-case corpus and original schema-failure observation reproduce against both
updated observers. Native checks pass **3/3**, targeted ASan/UBSan **2/2** with
leak scanning disabled, and imported modules **1/1**, all after successful builds.
The Context contract is rebuilt and replayed independently.

Canonical consumer verification covers 158 bodies; 1,479 catalog exports and the
seven-module acyclic dependency graph verify. Pinned format/check covers 1,019
handwritten C++ inputs. See the
[terminal record](parity/dynamic_json_execution_5_0_6.json). No production function,
generator body or generated artifact was changed; Java/generation reproduction
and MIPS/physical-device gates were not rerun.
