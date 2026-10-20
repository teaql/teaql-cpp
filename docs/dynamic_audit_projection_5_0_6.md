# Dynamic audit projection qualification

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Status: the declared projection matrix passes; complete G14/G16/G17 and P7 remain open.

## Actual source and observation boundary

The Rust observer includes the unchanged original `event.rs` and `log_privacy.rs`
directly. It calls `RawAuditEvent::updated_with_old_values`, `build_safe_event`
and the original raw-event privacy projection. The native observer calls the
corresponding runtime APIs. Neither observer implements masking or scrubbing.
The [verification record](parity/dynamic_audit_projection_5_0_6.json) records source,
observer and corpus hashes, plus terminal test output.

The matrix contains **3968** inputs: 31 old/new value pairs, four field names,
two plaintext settings, two provenance settings, two explicit-mask settings and
four limits. Field names are `#extension`, `#password`, `ordinary` and `_private`.
Pairs distinguish missing values, actual NULL and typed NULL; text, combining
Unicode, long text, boolean, signed/unsigned integer boundaries, negative zero,
Decimal, date, timestamp, JSON, object and list payloads. Nested credential
payloads occur in both old and new values. Limits are absent, zero, three and eight.

Each result compares the complete safe-event and privacy-projected event Debug
strings, including ordering, field flags, length/reason metadata, trace and
bootstrap identity. Safe clone equality is also observed. Natural-language
fields deliberately contain old/new operands and an independently masked SQL
operand, so provenance scrubbing is exercised even when the operand is absent
from the updated field. The exact plaintext acknowledgement is set or removed
before each input in both observers. No output is normalized.

All **3968/3968** actual Rust/native results agree. No production runtime or
generator algorithm change was necessary. Earlier event-projection evidence
remains unchanged; its retained test is included in the native regression.

## C++ ownership and consumers

The native observer disables the default PMR resource. Every input creates and
actually destroys its UserContext and GlobalContext; event records, nested values,
provenance, projections and diagnostics use their selected Context allocator.
This checks scoped construction and cleanup, not cross-domain retention.

Relevant native tests pass **2/2**. The new comparison passes under ASan/UBSan
with leak scanning disabled, and through an import-only module consumer.
All **135** canonical consumer bodies verify; only their recorded prologues
differ. Pinned formatting and its check pass for **1015** handwritten C++ files.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build build_native_current --target teaql_dynamic_audit_projection_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
cargo build --offline --locked \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin dynamic_audit_projection_5_0_6
python3 tests/parity/generate_dynamic_audit_projection_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_audit_projection_5_0_6 \
  --cpp build_native_current/teaql_dynamic_audit_projection_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^(dynamic_audit_projection_rust_parity|audit_event_projection_5_0_6_rust_parity)$'
```

The sanitizer target has the same name in `build_sanitize_current`. Its CTest
run uses `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`.
The module target is `teaql_module_dynamic_audit_projection_probe`; its test is
`module_dynamic_audit_projection_rust_parity` in `build_modules_native`.

## Remaining full-chain work

These events are constructed at the projection boundary. The matrix does not
execute durable dynamic-field writes, mutation policy, commit/rollback, audit
delivery or file/log sinks. It does not qualify generated model storage identities,
all protected query/save configurations or asynchronous delivery. Those existing
requirements remain in the capability ledger and cannot be closed by projection
agreement. No MIPS or physical-device run was performed.
