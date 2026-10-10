# Timestamp public API and transparent JSON acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-core/src/time.rs`. Timestamp is the signed millisecond scalar, independent
of the native UTC-to-Value conversion and Date's calendar representation.

The complete production time.rs contract is now accepted within the grouped
Value/time/enabled-Decimal review. The final native suite passes 303/303 in
74.81 seconds after a no-work final refresh, with fresh 21-check combined and
13-check scalar/memory selections passing on ASan/UBSan with leak scanning
disabled and on MIPS/QEMU. This includes public-field mutation, exact source
tests, transparent signed JSON, fallback datetime, ordering/hash and the
source clock-floor correction. See value_api_acceptance.md for the final
grouped acceptance; later historical sections retain their original gates.

| Source responsibility | C++ mapping and evidence |
| --- | --- |
| Public i64 tuple field and as_millis | Public signed `millis` storage, explicit signed constructor and as_millis retain read/write access to the complete scalar. Mutable borrowing updates the same value used by conversion, comparison, hashing, Debug and transparent JSON. |
| From<i64>, From<u64> | Constructor and from_u64; unsigned conversion wraps into the signed domain as in the original cast, including u64 maximum to -1. |
| now | System-clock milliseconds. Both exact original source tests and the differential oracle bound now between independent before/after clock observations. |
| to_datetime | DateTime calendar components use floor for negative milliseconds and the original chrono calendar bounds. Out-of-range input falls back to the epoch rather than saturating or throwing. |
| Debug | Standalone Timestamp tuple diagnostics inherit all nine tested formatter modes, including signed/hex values and padding. |
| Clone, Copy, Default, PartialEq, Eq | Ordinary scalar C++ copy, default zero and value comparison; copying does not allocate or extend any Context. |
| PartialOrd, Ord, Hash | Total signed comparison and standard hash over the complete signed millisecond value. The earlier 67-input core audit supplies 16,698 comparisons and native ordered/hashed-container checks. Hash bytes/bucket algorithms are not a persisted wire format. |
| Transparent Serialize/Deserialize | JSON integer output with no object wrapper; accepted signed integer range and raw JSON type/syntax rejection are compared with the actual original derive. C++ from_json_value returns optional rejection; generic serializer implementations and serde's external diagnostic wording are not claimed. |

The new oracle records 99 actual original calls. Sixty-eight scalar constructions
cover signed limits, unsigned wrapping, epoch/negative fractions, day boundaries,
leap dates, times beyond a 32-bit seconds range, and both chrono calendar edges.
Thirty-one raw JSON inputs distinguish integer from float/exponent/string,
unsigned overflow, malformed syntax, surrounding whitespace and other root
values. The oracle observes actual original methods and derives, rather than
reimplementing conversion or formatting algorithms.

All 99 observations match standalone native and OpenWrt MIPS/QEMU probes. The
two unchanged source unit tests pass independently. Their exact inputs and
assertions are retained in timestamp_api_tests.cpp, alongside default-PMR-disabled
Global retention of a Timestamp, its transparent JSON and raw diagnostic after
actual User deletion. Native, ASan/UBSan with leak detection and standalone MIPS/QEMU ownership
contracts all completed successfully, including both exact source tests.

The new timestamp_api_contracts/timestamp_api_rust_parity checks are registered
in CMake after the owning Value full build passed 219/219. Fresh named acceptance completed: native full regression 224/224 and ten
selected sanitizer/MIPS checks 10/10 each, as recorded below. This document does not establish generic serde
error parity, full Decimal/API acceptance, all six stages or physical-router
execution. No production dependency or timestamp representation changed.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin timestamp_api_oracle --locked --offline
python3 tests/parity/generate_timestamp_api.py \
  --rust build/rust-original-roundtrip/debug/timestamp_api_oracle
python3 tests/parity/compare_timestamp_api.py --cpp <probe> \
  --rust build/rust-original-roundtrip/debug/timestamp_api_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'.

## Named integration checkpoint

The fresh ASan/UBSan build with leak detection and OpenWrt MIPS/QEMU build each
completed successfully and passed all ten selected checks: the five new
metadata/scalar checks plus core model, core API (native contract and Rust
comparison), Value model and Context reference contracts. Fresh MIPS probes
match all 160 metadata, 99 Timestamp and 115 Value inputs against live original
Rust. The metadata contract includes the latest qualified public-facade
assertion; the Timestamp contract includes both exact original source tests.
The full native build and regression completed with 224/224 checks passed.
Fresh named native probes also match all 160 metadata, 99 Timestamp and 115 Value
inputs against live original Rust. These are completed integration results;
remaining stage-wide API/privacy/source-test requirements are separate.

## Public field repair, 2026-10-08

Later full-body review traces now() through chrono's timestamp_millis, which
combines floor-based Unix seconds with nonnegative subsecond milliseconds.
The native system-clock read now uses chrono::floor<milliseconds> instead of
duration_cast, preserving negative fractional milliseconds if the OS clock
is before the Unix epoch. The original clock-bracket fixture observes the same
floor boundary. No system clock is changed by the tests; the existing UTC
conversion corpus independently covers negative fractional instants. Fresh
Timestamp/UTC consumers and the full native final refresh remain required.

The complete time.rs source review finds an omission in the earlier mapping:
the Rust tuple field is public and writable; construction/readback alone do not
preserve mutable borrowing. A native compile fixture fails against the earlier
private storage. C++ now names the public i64 field `millis`, without adding
storage, allocation, ownership or an alternate cached value. All methods use
that same field. Default, scalar copy, structural comparison and hashing retain
their existing responsibilities.

Nine original public-field mutations extend the previous 99 inputs unchanged
to 108. The Rust fixture borrows `&mut value.0` and writes through it; the C++
fixture borrows `timestamp.millis`. Complete subsequent datetime, Debug, JSON
and copy observations include signed extremes, negative fractional milliseconds
and calendar boundaries. The native contract also checks unchanged independent
copies, ordering/hash after mutation, and one-i64 storage. These are behavioral
checks of the omitted capability, not a substitute for complete Value/Decimal
or stage acceptance. The compile fixture now passes, standalone current-source
native contracts pass with default PMR disabled, and all 108 observations match
live pinned Rust. The named native/sanitizer/MIPS rebuilds and full consumer
integration are pending.

The earlier integrated probe is retained as a negative control: it matches
99/108 observations and fails exactly the nine new field-mutation inputs.
The current-source probe matches 108/108. Both unchanged original time tests
freshly pass 2/2 using the repository-local locked offline target.

The named entity/Timestamp platform checkpoint passes 12/12 under ASan/UBSan
with leak scanning disabled (7.81 seconds) and 12/12 on MIPS/QEMU (6.68 seconds).
It includes both new Timestamp checks. Later inherited Decimal default methods
require a final consumer refresh before complete group/stage acceptance.
