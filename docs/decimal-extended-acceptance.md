# Extended Decimal arithmetic acceptance

Baseline: TEAQL Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
with its locked `rust_decimal` 1.42.1 dependency. Public normal construction still
admits scales 0 through 28. Explicit rescale can retain greater scales; subsequent
operations must preserve the original implementation's behavior rather than
silently treating these values as ordinary scale-28 decimals.

## Repairs established by original calls

- Aligned addition/subtraction and short unaligned paths retain their result
  scale, including canonical zero. The original wide-buffer path separately
  rounds down to the supported precision. C++ previously always reduced to 28.
- Intermediate alignment beyond the source buffer now reports a domain exception,
  corresponding to the original panic. Previously C++ returned an empty optional
  and the saturating wrappers substituted a bound, which changed the contract.
- Exact division retains the original checked-constructor scale failure. A
  nonzero remainder beyond the original power-table range also reports a domain
  exception. C++ previously reduced these values or normalized them into zero.
- Remainder retains independent single-word, low-64-bit and buffered paths. Its
  staged 64/96-bit windows, retained words and unsigned wrap behavior are adapted
  from the locked implementation. A mathematically simplified modulus does not
  reproduce every source result. Checked construction in the buffered path still
  rejects excessive result scale; raw paths can preserve it, even for zero.

The internal arithmetic constructor does not relax public `from_parts`,
`set_scale`, unpacked conversion or wire decoding. It is accessible only to the
arithmetic implementation. Results are scalar values; diagnostics and observation
strings still require an explicit destination Context. No runtime dependency is
added. The original implementation's MIT attribution remains in
`LICENSES/rust_decimal-MIT.txt`.

## Evidence

`decimal_extended_cases.json.gz` retains complete inputs and outputs for 8,056
unchanged original public-call observations. The deterministic matrix combines
six operands, both signs and requested scales 0, 28, 29, 38, 56, 255 and maximum
u32. Another 1,000 seeded pairs use arbitrary 96-bit coefficients and independent
scales. Each input checks both rescaled operands, all five checked arithmetic
operations, three saturating operations, ordering, raw coefficient/sign/scale,
text and formatter failures. Panics are caught per operation by the Rust fixture;
no source arithmetic algorithm is copied into the oracle.

The first comparison exposed 708 differing inputs. After repairs, all 8,056
observations match native C++. The seven existing native Decimal checks also
pass, retaining arithmetic, scale/encoding, parsing, numeric conversions,
foundations and serde/formatting coverage. Native contracts with default PMR
unavailable separately check retained scale, zero remainder, checked division
failure and original intermediate overflow.

The new differential check is registered as `decimal_extended_rust_parity`.
All Decimal CTest comparisons now forward the configured cross emulator.
Both selected cross-platform builds and their tests completed on 2026-10-07:
ASan/UBSan with leak detection passed all eight Decimal checks (8/8), and
OpenWrt MIPS/QEMU passed the same eight checks (8/8). All 8,056 MIPS observations
also matched live original Rust. These checks cover the new matrix and every
existing Decimal corpus. The complete native consumer rebuild and full CTest finished successfully:
205/205 checks passed, and the native probe also matched all 8,056 live original
observations. This checkpoint precedes the subsequent Entity public-header
repairs; a separate full-current consumer build is now running for those. Original panic observations
come from the locked Rust debug oracle; C++ reports these boundaries explicitly
with exceptions and does not reproduce unsafe native shifts or memory access.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin decimal_oracle --locked --offline
python3 tests/parity/generate_decimal_extended.py \
  --rust /home/philip/shared-cargo-target/debug/decimal_oracle
cmake --build build --target teaql_decimal_probe teaql_decimal_tests
ctest --test-dir build --output-on-failure -R '^decimal_.*'
python3 tests/parity/compare_decimal_extended.py --cpp build/teaql_decimal_probe \
  --rust /home/philip/shared-cargo-target/debug/decimal_oracle
```

For sanitizer CTest use the corresponding build directory and
`ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1`. For MIPS use
`build-wndr4300-memory` and supply
`--cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'` to the live comparator.

This closes the identified extended-binary-arithmetic gaps for the recorded
matrix. It does not prove every Decimal API/serialization responsibility,
arbitrary combination or source-test requirement, nor complete stage 2 or the
six-stage migration. Physical-router execution remains unverified.
