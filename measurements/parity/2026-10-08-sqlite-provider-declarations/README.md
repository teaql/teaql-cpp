# SQLite native payload and provider declaration checkpoint

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
This directory records selected current-source evidence. It does not accept the
whole provider, later runtime/save/privacy integrations or stages 3–6.

| Configuration | Registered checks | Elapsed test time |
| --- | --- | --- |
| Native | 8/8 | 7.78 s |
| ASan/UBSan, leak detection enabled | 8/8 | 17.91 s |
| OpenWrt MIPS under QEMU 74Kf | 7/7 | 78.67 s |

Each selected final build is followed by a no-work build and nonempty compiler
dependency-record check. results.json records all test names, binary/source
SHA-256 fingerprints, configuration and limitations. The complete CTest output
for each configuration is retained alongside it.

The driver corpus preserves all previous 467 observations and adds 96 actual
original calls with independent native category/extended-code fields. The native
and MIPS probes both match all 563 observations against live original Rust.
The transport corpus preserves all 796 original cases and adds one observation
of all eight capability flags and four public default constants. Native and MIPS
both match all 797 against live original Rust. The MIPS conversion probe also
matches all 36 portable FromSqlError observations against live Rust.

The separate 36-case raw converted Debug profile compares complete native
64-bit diagnostics without sentinel normalization. Its check runs only on
non-cross-compiling 64-bit builds, explaining the one-check difference for MIPS.
The portable profile verifies unknown-index identity against local SIZE_MAX,
complete model diagnostics, Display and actual causes.

Earlier extended attempts use an old transport probe or a stale compiled
negative-capability assertion. They fail and are not counted in this checkpoint.
The three affected provider/transport objects are invalidated only in the
isolated review directories before rebuilding stable sources. No original
observation is removed or normalized.

## Reproduction

Build the original oracles into a repository-local Cargo target:

```sh
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" CARGO_BUILD_JOBS=2 \
  cargo build --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin sqlite_error_model_oracle --bin sqlite_transport_oracle \
  --bin sqlite_from_sql_error_oracle --locked --offline
python3 tests/parity/generate_sqlite_error_model.py \
  --rust build/rust-original-roundtrip/debug/sqlite_error_model_oracle
python3 tests/parity/observe_sqlite_provider_declarations.py \
  --rust build/rust-original-roundtrip/debug/sqlite_transport_oracle
python3 tests/parity/generate_sqlite_from_sql_error.py \
  --rust build/rust-original-roundtrip/debug/sqlite_from_sql_error_oracle
```

Build the six C++ targets named in results.json, then run their eight named
CTest checks. For sanitizer validation set ASAN_OPTIONS=detect_leaks=1 and
UBSAN_OPTIONS=halt_on_error=1. The MIPS configuration uses SQLite 3.53.2 and the
OpenWrt toolchain documented in scripts/toolchains/openwrt-wndr4300.cmake.
No physical-router execution is claimed.
