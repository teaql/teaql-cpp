# Runtime service cursor boundaries, 2026-10-09

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

The full runtime-service review identifies unconditional native overflow
rejection during continuous cursor registration. In the pinned source,
`original_offset + rows.len() as u64` checks overflow in Debug and wraps in
Release. Direct public EntityDataService calls observe sixteen explicit offset
scenarios independently in both actual original build profiles. SmartList
compatibility controls do not execute cursor registration.

Before repair, native Debug matches16/16 and MIPS MinSizeRel matches10/16.
The six mismatches affect rows/owned/outer aggregate entries at u64::MAX and
sequential cached-seek registration. All165 existing public query observations
still match live original Rust on both profiles. See before.json and complete
negative-control output logs. No corpus is weakened to accommodate the gap.

The native next_offset check now matches the build profile, retaining checked
Debug failure and defined unsigned Release wrapping. The probe supplies offsets
directly and independently reports purpose validation and execution panic; all
arithmetic and orchestration remain inside the actual runtime under test.
CTest selects the corresponding recorded original expectation via its configured
build profile. Full consumer/integration validation is pending.

Original binaries must finish building successfully before any observations:

```sh
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" cargo build --manifest-path tests/parity/rust-values/Cargo.toml --locked --offline --bin entity_query_oracle
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" cargo build --manifest-path tests/parity/rust-values/Cargo.toml --locked --offline --release --bin entity_query_oracle
python3 tests/parity/compare_entity_query_boundaries.py --cpp build-root-integration/teaql_entity_query_probe --rust build/rust-original-roundtrip/debug/entity_query_oracle --profile debug
python3 tests/parity/compare_entity_query_boundaries.py --cpp build-wndr4300-memory/teaql_entity_query_probe --rust build/rust-original-roundtrip/release/entity_query_oracle --profile release '--cpp-emulator=/usr/bin/qemu-mips;-cpu;74Kf'
```

The complete runtime service responsibility is reviewed in
[the API contract](../../../docs/runtime-data-service-api-contract.md).
Physical-router testing remains explicitly deferred. Generator and production
dependency scope are unchanged; this numerical behavior is independent of
Context allocation quotas and lifetime contracts.


## Completed target checkpoint

The changed entity-query header/probe consumers finish69 tasks on MIPS and70 on
ASan/UBSan before their51-check selections. Both post-test builds report no
work. MIPS/QEMU passes51/51 in29.45 seconds, including all16 corrected Release
boundary inputs; a fresh actual original Release build also matches16/16 live.
ASan/UBSan passes51/51 in37.43 seconds with leak scanning disabled. Separate
four-task generic/foreign-child rebuilds pass3/3 in0.40 seconds on MIPS and
0.23 seconds on sanitizer, preserving non-Clone completion and cleanup contracts.
The complete332-task native build has finished and its376-check regression is
running. The newly discovered private typed-aggregate helper remains an
independent source gap; this checkpoint does not accept the full service group.
