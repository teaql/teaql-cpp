# DataType scalar capability acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The source DataType enum has ten unit variants and derives Debug, Clone, Copy,
PartialEq and Eq. The C++ enum already preserves the variants and ordinary value
copy/comparison, but its public diagnostic formatter previously accepted only a
containing Value::TypedNull. A standalone debug_string(DataType) failed to compile.

## Repair

The public DebugWriter now has a DataType overload that emits exactly the source
unit-variant name. Width, precision, numeric radix and padding do not change the
unit name. The containing TypedNull writer delegates to that overload, retaining
its outer tuple and inherited indentation without allocating a temporary wrapped
Value diagnostic. The core facade exposes the same overload through the existing
write_debug/debug_string entries; no parallel formatter abstraction is added.

The name table contains only immutable literal facts, not Context-owned strings,
a cache, singleton or new lifetime. Native out-of-range enum values reject before
indexing; those values cannot arise from safe Rust construction and are not new
valid source variants. DataType has no source serde derive, and no new wire
protocol is introduced.

## Current evidence

The negative compilation fixture reproduces the missing standalone capability.
The repaired native probe matches all ten original variants, 180 exact Debug
outputs and 100 pairwise equality results. The unchanged original oracle calls
actual derived formatting in nine modes for both DataType and Value::TypedNull.
It copies no formatting algorithm. Ordinary C++ enum value copying remains
independent of Context storage; formatted output uses the explicit destination.

The native default-PMR-disabled contract passes, checking facade access, literal
unit formatting under numeric/padding options, no extra object reservation,
invalid native variant rejection and complete Global output after actual source
User destruction. Existing model/Entity Debug corpora remain relevant to the
TypedNull delegation change.

Named datatype_debug_contracts and datatype_debug_rust_parity are registered.
Integrated acceptance completed on 2026-10-07 after all builds finished:

| Configuration | Result |
| --- | --- |
| Native Debug, all rebuilt consumers and full CTest | 217/217 checks passed. |
| ASan/UBSan with leak detection | Seven selected checks passed 7/7. |
| OpenWrt MIPS/QEMU 74Kf | The same seven selected checks passed 7/7. |
| Named native/MIPS probes against live original Rust | All ten inputs, 180 formatter outputs and 100 equality results match in each. |

The selected checks are datatype_debug_contracts, datatype_debug_rust_parity,
model_debug_contracts, model_debug_rust_parity, entity_debug_contracts,
entity_debug_rust_parity and chrono_value_contracts. Original row/list and Entity
Debug corpora therefore continue passing after the TypedNull delegation change.
The source lock, 478-file pinned formatter, public declarations and 58-group
inventory checks pass; those structural checks alone are not behavioral proof.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin datatype_debug_oracle --locked --offline
python3 tests/parity/generate_datatype_debug.py \
  --rust /home/philip/shared-cargo-target/debug/datatype_debug_oracle
cmake --build build --target teaql_datatype_debug_tests teaql_datatype_debug_probe
ctest --test-dir build --output-on-failure \
  -R '^(datatype_debug_contracts|datatype_debug_rust_parity)$'
python3 tests/parity/compare_datatype_debug.py --cpp build/teaql_datatype_debug_probe \
  --rust /home/philip/shared-cargo-target/debug/datatype_debug_oracle
```

For MIPS add `--cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'`. This scalar repair
retains all source variants and responsibilities but does not establish complete
Value/Decimal/API/source-test, stage-2 or all-stage acceptance. Physical-router
execution remains unverified. No generator or production dependency changed.
