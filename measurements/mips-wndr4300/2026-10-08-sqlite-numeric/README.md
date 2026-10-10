# MIPS SQLite numeric conversion and emulator control

This experiment uses the current uncommitted worktree, pinned TEAQL Rust v5.0.5
at 62940ae2b88e7acc32763853b8b4685a01c7e90a, and the OpenWrt 24.10.4 ath79/nand
GCC 13.3.0 musl toolchain. Results are QEMU observations, not physical-router
execution. Target code uses -march=74kc -msoft-float; emulation uses QEMU 74Kf.

The private toolchain previously assigned QEMU 24Kc unconditionally, overriding
the configured cache during regeneration. The SQLite binding test raises SIGILL
at sqlite3_config's lwx instruction. The same executable passes its native
contract on 74Kf. The target remains soft float; an emulator model change does
not introduce a hardware-FPU dependency. The tracked
[toolchain](../../../scripts/toolchains/openwrt-wndr4300.cmake) preserves explicit
emulator settings and defaults to 74Kf when qemu-mips is available.

Correct emulation exposes independent numeric failures with the existing SQLite
3.40.1 archive: 7 of 1,084 binding observations and 27 of 6,029 decoding
observations differ from original Rust, whose bundled engine is SQLite 3.53.2.
All three native binding/decoding/error contracts pass. The complete parity checks
remain failing; no input, expected value or comparator tolerance is changed.
Every discrepancy is retained in
[the observation record](../../../tests/parity/sqlite_mips_engine_differences.json).

The isolated [reproduction](sqlite-affinity-repro.cpp) links SQLite directly and
reports the value's IEEE754 bits. No TEAQL code or JSON formatter participates.
With SQLite 3.40.1 on MIPS, CAST('0.0000000000000000000000000001' AS NUMERIC)
returns 1.0000000000000001e-28, bits 3a1fb0f6be50601a, with sizeof(long double)=8.
The original Rust observation is 1e-28. The old amalgamation's sqlite3AtoF uses
LONGDOUBLE_TYPE scaling; the new amalgamation calls sqlite3Fp10Convert2. A
same-engine control is required to distinguish engine conversion from provider
binding semantics; it does not authorize changing those semantics.

The isolated SQLite 3.53.2 control now returns 9.9999999999999997e-29, bits
3a1fb0f6be506019: the binary64 value represented by the original Rust JSON 1e-28.
The two MIPS controls differ by one representable value. The
[isolated corpus controls](isolated-corpus-controls.json) now exercise every
recorded mismatch through native SQLite alone: 34/34 old values are reproduced,
and 34/34 new values match the unchanged original Rust expectations by exact
IEEE754 bits. [compare-engines.py](compare-engines.py) records each setup/query
and returns failure if either side differs. The complete TEAQL rebuild and
five-check parity selection on 3.53.2 subsequently finish and pass 5/5 in
82.13 seconds. All 1,084 bindings and 6,029 decoding observations match. This
checkpoint precedes the later structured driver-error layout change; it proves
the numeric engine control, not final current-code or all-stage regression.
Terminal native/sanitizer checks, both MIPS failure profiles, raw isolated
outputs, binary hashes and source hashes are retained alongside this report.

The baseline Cargo.lock selects libsqlite3-sys 0.38.1. Its SQLite 3.53.2
amalgamation SHA256 is
0a409f1633283fa31a9126b11fbfd64a1991c5d30defad07e5745d4667f5e23d.
The installed 0.38.2 amalgamation has the identical hash. The control compiles
this existing source for MIPS, keeping the original 3.40.1 archive intact.
Its compact flags enable thread safety and column metadata; this experiment
does not claim parity for every optional SQLite extension.

## Reproduction configuration

```sh
cmake -S . -B build-mips-toolchain-review -G Ninja \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DCMAKE_TOOLCHAIN_FILE=scripts/toolchains/openwrt-wndr4300.cmake \
  -DTEAQL_OPENWRT_TOOLCHAIN_DIR=/path/to/toolchain-mips_24kc_gcc-13.3.0_musl \
  -DSQLite3_INCLUDE_DIR=/path/to/sqlite-amalgamation \
  -DSQLite3_LIBRARY=/path/to/libsqlite3.a

python3 measurements/mips-wndr4300/2026-10-08-sqlite-numeric/compare-engines.py \
  --old /path/to/sqlite-3.40.1-affinity-repro \
  --new /path/to/sqlite-3.53.2-affinity-repro
```

Do not reuse a build directory's compiler/toolchain identity when changing the
toolchain. Static linkage here is an experimental configuration, not a required
production deployment mode. No target binaries or toolchain archives are added
to the repository. Full provider composition, final all-stage regression and
physical-device validation remain open.
