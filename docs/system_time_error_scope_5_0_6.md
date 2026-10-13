# System-time error replay under the 5.0.6 reference

Status: scoped auxiliary native/source comparison passes. This does not qualify
the complete Value/time group, request-runtime migration or G13/P7.

The full native integration run rejected the historical system-time error corpus
because its baseline is 5.0.5. The old corpus, generator defaults and original
Rust oracle remain unchanged. A separately named Rust binary in the already
locked 5.0.6 oracle workspace invokes the same original standard `SystemTime`,
`Duration`, `Result` and `Error` methods; no arithmetic or formatter implementation
is copied into the oracle.

The new corpus retains all 84 boundary instants and 7,056 ordered pairs. It
records reversed-span errors, successful durations, clone results, complete
Debug modes, Display widths/precision and default error-source behavior. All
**7,056/7,056** observations match both live Rust replay and the native C++ probe.
The comparison enforces the active commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; it does not change the label on the old
observations. Standard-library behavior is auxiliary evidence, not an original
generated-model or governed-SQL privacy comparison.

The active CTest family now uses
`compare_system_time_error_scope_5_0_6.py`. The historical comparator remains
available for its original baseline. Reproduce the new observations with:

```sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
    CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
    cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml \
    --bin system_time_error_scope_5_0_6
python3 tests/parity/generate_system_time_error_scope.py \
    --rust /tmp/teaql_cpp_field_layout_rust_506/debug/system_time_error_scope_5_0_6 \
    --output tests/parity/system_time_error_scope_5_0_6_cases.json.gz
python3 tests/parity/compare_system_time_error_scope_5_0_6.py \
    --cpp build_native_current/teaql_system_time_error_scope_probe \
    --rust /tmp/teaql_cpp_field_layout_rust_506/debug/system_time_error_scope_5_0_6
```

No runtime implementation changed for this replay. Routine verification used
native only; no sanitizer or MIPS claim is inferred. Full native integration
still has other historical baseline and native/example migration failures.
