# Scalar baseline requalification for Rust 5.0.6

This checkpoint reexecutes historical inputs against the frozen original
`teaql-core` at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. It does not
rename old observations or establish complete generator/runtime parity.

## Original execution and retained evidence

The three existing drivers in `tests/parity/rust_values` are byte unchanged:
`decimal_oracle.rs`, `decimal_inline_oracle.rs` and `src/main.rs`. A separate
development manifest compiles them directly, without substituting a local
evaluator or modifying their bodies. Exact direct dependencies retain Decimal
1.42.1, Chrono 0.4.45, serde_json 1.0.150 and num-traits 0.2.19. The dedicated
Cargo lock fixes the resolved dependencies. No production dependency is added.

All ten matrices execute the same historical input sequences through fresh
original binaries and the existing native C++ probes. Comparison retains every
result/error field and numerical kind, including floating negative zero.
No normalization is applied.

| Matrix | Exact original/native observations |
| --- | ---: |
| Decimal arithmetic | 963 |
| Extended Decimal arithmetic | 8,056 |
| Inline Decimal strings | 484 |
| Decimal scale and codec | 1,148 |
| Decimal parsing | 8,576 |
| Decimal primitive conversions | 6,058 |
| Decimal foundations | 4,280 |
| Decimal JSON and formatting | 3,008 |
| Core Value conversions | 722 |
| ID and JSON text formatting | 848 |
| **Total** | **34,143** |

All original/native observations agree and reproduce. None of these scalar
outputs changes from its historical counterpart, but that conclusion comes from
fresh execution. Historical files remain byte unchanged. Separate
`*_5_0_6_cases.json.gz` files record the current source results, input/driver
hashes and manifest/lock hashes. Active comparators select these new files.
The [checkpoint record](parity/scalar_baselines_5_0_6.json) retains the matrix.

The auxiliary diagnostic driver is separately recompiled against 5.0.6. Its
94 inputs also match native exactly. Every historical output changes because
the source query diagnostics now include the 5.0.6 shape. Its historical file
is preserved; see [the independent record](parity/core_aux_debug_5_0_6.json).

## Full integration findings and Context fixture correction

The complete native build succeeded. The subsequent 549-test run finished with
472 passing and 77 failures: 76 historical corpus baseline rejections and one
Context factory exhaustion fixture failure. The generated API compiler gate
passed in that run. This is a terminal failing integration result, not full
5.0.6 acceptance; see [the retained run](parity/native_integration_5_0_6_pending.json).

An independent compiled allocation probe found one issued ID before the factory
and three after successful construction, with the previously published integer
still accessible. The fixture's quota of three therefore admitted the operation.
The root wrapper is an arena payload; its graph and the fresh operation's graph
still allocate checked identities. The fixture now sets the quota to two, so
the second graph allocation fails after the first is published. It checks the
object-limit error, the unchanged integer, two issued/live objects, and complete
User byte reclamation after actual deletion. Production factory behavior is
unchanged.

The focused native selection passes 13/13: the ten scalar corpora, auxiliary
diagnostics, corrected Context contract and diagnostic composition. The Context
contract separately passes ASan/UBSan 1/1 with leak scanning disabled. The
canonical imported Context contract and 360-case diagnostic composition pass
2/2. Module preparation reuses the actual copied fixture path and preserves
include position relative to macros. Its verifier now checks file existence
and canonical include/macro placement; missing-path and reordered-driver
negative controls both reject. All 166 canonical consumer bodies verify.
The initial scanner/compile failures and controls remain in the
[composition record](parity/derived_query_composition_5_0_6.json).

The
complete suite has not been rerun or declared passing; remaining historical
failures need their own actual-source qualification. Pointer-width and other
scalar/runtime configurations remain independent gates.

## Reproduction

```sh
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target \
  teaql_decimal_probe teaql_decimal_inline_probe teaql_value_probe \
  teaql_core_aux_debug_probe teaql_original_context_tests \
  teaql_derived_query_composition_probe
python3 scripts/verify_scalar_baselines_5_0_6.py
python3 scripts/verify_core_aux_debug_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(original_runtime_context_contracts|core_aux_debug_rust_parity|decimal_rust_parity|decimal_extended_rust_parity|decimal_inline_rust_parity|decimal_scale_rust_parity|decimal_parsing_rust_parity|decimal_numeric_rust_parity|decimal_foundation_rust_parity|decimal_wire_format_rust_parity|value_rust_parity|format_rust_parity|derived_query_composition_rust_parity)$'
cmake --preset modules_native
cmake --build build_modules_native --target \
  teaql_module_derived_query_composition_probe teaql_module_original_context_tests
python3 scripts/verify_module_consumers.py --build-dir build_modules_native
ctest --preset modules_native --output-on-failure \
  -R '^(module_derived_query_composition_rust_parity|module_original_runtime_context_contracts)$'
```

The replay script builds unchanged source drivers with `--offline --locked`,
waits for successful completion, then compares the active corpus with live
original and native results. `--record` writes separate active files only after
every original/native matrix passes. This checkpoint leaves G01-G17 and P0-P7
partial. No MIPS or physical-device test was run.
