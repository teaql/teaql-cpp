# Native UTC Value conversion

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The original Value module implements From<DateTime<Utc>>, preserving Unix
milliseconds as Value::Timestamp. Existing C++ scalar Timestamp construction did
not supply the corresponding native C++ clock-input capability.

`teaql/core/chrono_value.hpp` now provides
`value_from_utc(destination, instant)` for standard system-clock time points with
signed integral seconds, milliseconds, microseconds or nanoseconds. This named
factory adapts the original conversion trait to the C++ allocation-domain API.
It uses C++20 chrono and adds no date/timezone/runtime dependency. Existing
Date/DateTime decomposition and Timestamp parsing/conversion remain separate.

Fractional milliseconds use mathematical floor, including negative instants:
minus one nanosecond produces Timestamp(-1). Duration casts that truncate toward
zero would produce a different value. Seconds conversion checks signed bounds
before multiplication; no native signed overflow or compiler 128-bit integer is
required. Native millisecond points preserve the full i64 scalar range, including
points outside the original chrono calendar's representable dates. That is an
explicit C++ input-domain extension, not a claim that Rust DateTime represents
those dates. Floating duration representations and other clocks require explicit
caller conversion; the source input likewise is an integral UTC instant.

The 64-input oracle constructs actual original chrono DateTime<Utc> objects and
calls original Value::from. Every input is representable in that calendar;
seconds/milliseconds/microseconds use broad signed dates, and nanoseconds include
full i64 bounds. Sub-millisecond negative/positive boundaries compare complete
Value Debug, Timestamp milliseconds and the original Value JSON projection.
All 64 observations match live original Rust on standalone native C++.

The native default-PMR-disabled contract checks floor, exact scale, rejected
seconds overflow and explicit Global retention after actual User destruction.
The scalar result stores the selected destination resource without allocating
characters or consuming a new object ID. Managed allocation still goes through
the caller's Context factory; Global business allocation remains User-delegated.
Native raw borrows keep the existing expiry contract.

Standalone native, ASan/UBSan with leak detection and MIPS/QEMU ownership
contracts all completed successfully. The cross-built MIPS probe also matches all
64 live original observations. Each standalone build finished before execution;
these checks link existing static archives, not a full fresh consumer rebuild. The public core.hpp facade now exposes the factory, and chrono_value_contracts /
chrono_value_rust_parity are registered. After the previous full build passed
212/212 and reached its terminal result, the final integration targets were
rebuilt. ASan/UBSan with leak detection and MIPS/QEMU each pass the enlarged
14/14 selection, and the named MIPS UTC probe matches all 64 live original
observations. Full native CTest completed with 215/215 checks passed. The named native UTC
probe matches all 64 live original observations, as does the MIPS probe. No active build was restarted for this addition.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin chrono_value_oracle --locked --offline
python3 tests/parity/generate_chrono_value.py \
  --rust /home/philip/shared-cargo-target/debug/chrono_value_oracle
python3 tests/parity/compare_chrono_value.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/chrono_value_oracle
```

For MIPS pass `--cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'`. The native contract
is `teaql_core/tests/chrono_value_tests.cpp`. This capability does not complete every Value/
DateTime/API/serialization/source-test gate, stage 2 or the six-stage objective.
Physical-router execution remains unverified.

## Integrated acceptance on 2026-10-07

All final builds and tests completed successfully:

| Configuration | Result |
| --- | --- |
| Native Debug, all consumers and full CTest | 215/215 checks passed. |
| ASan/UBSan with leak detection | Fourteen selected checks passed 14/14. |
| OpenWrt MIPS and QEMU 74Kf | The same fourteen checks passed 14/14. |

The selection includes registration, BaseEntityData diagnostics, Entity Debug,
owned Entity/wrappers, default-method sequences, UTC conversion, existing Entity
expression, SafeExpression, SmartList and Context reference contracts. Named
native and MIPS probes match live original Rust: registration 42/42, parsing
1,481/1,481, Debug 289/289 inputs (5,202 outputs), defaults 93/93 and UTC 64/64.
Every selected target finished building before its tests ran. The source lock,
476-file pinned formatter, public declaration and 58-group inventory checks pass.
No production dependency or generator changes were introduced. Physical-router
execution and complete remaining stage/API/privacy/source-test acceptance are
still unproven; this checkpoint does not complete the six-stage objective.
