# BaseEntityData parse and diagnostic acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The audit covers original public parsing, map conversion, Clone/equality and
EntityError message/Display responsibilities. It does not claim complete Entity
or stage acceptance.

## Original contract and repair

`BaseEntityData::from_values_map` validates ID before version. Missing or untyped
Null roles become zero. ID accepts U64 and nonnegative I64; version accepts I64.
TypedNull never substitutes for untyped Null. Other fields become dynamics;
`to_values_map` includes ID and version and copies dynamics. Clone remains
structural, including non-reflexive NaN equality.

Original failure messages use the optional lookup's Debug representation:
`invalid id field: Some(I64(-1))`, for example. EntityError Display then prefixes
`BaseEntity: `. C++ previously used the Value's JSON projection, losing both
optional and variant/type information. Its current source implementation now
uses the existing Value Debug kernel and retains the complete original message.
This is a raw core diagnostic, not a safe audit/log projection; safe projections
retain their independent privacy responsibilities.

## Direct evidence

The 1,481-input corpus calls unchanged original methods. Its role matrix combines
missing, Null, all thirteen Value variants, all ten TypedNull DataTypes, full
signed/unsigned integer boundaries, nonfinite floating values, nested content,
and invalid role pairs. Additional valid parses vary dynamic payloads, including
NaN and empty keys. Observations retain complete parsed ID/version, every
dynamic and reconstructed map value, Clone equality, or complete error entity,
message and Display. JSON is a test observation envelope; neither BaseEntityData
nor EntityError receives an invented serde protocol.

The negative control against the earlier library matched only 100/1,481 inputs:
all successful parses matched, but every invalid role's diagnostic differed.
The repaired production source matches all 1,481 observations against live Rust
in a standalone native probe. The native ownership contract runs with usable
default PMR disabled, checks exact typed failures and role precedence, preserves
the borrowed input map, and explicitly copies both successful results and errors
into Global before actually deleting User. Complete retained strings use Global.

The standalone probes compile the current `src/core/entity.cpp` directly and
link existing static libraries, resolving its definitions before archive lookup.
They do not mutate the active Ninja build. The prior Decimal build preceded this source repair; a subsequent complete
consumer rebuild included it and the shared-header repairs. Both named parse
checks and the final full regression completed successfully. Standalone results
above are complemented by the integrated acceptance recorded below.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin base_entity_api_oracle --locked --offline
python3 tests/parity/generate_base_entity_api.py \
  --rust /home/philip/shared-cargo-target/debug/base_entity_api_oracle
python3 tests/parity/compare_base_entity_api.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/base_entity_api_oracle
```

The native generic contract is `tests/base_entity_api_tests.cpp`. Cross probes
use the same comparator, with
`--cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'` for MIPS. The original source and
generator remain unchanged, and no production dependency is added. Physical
router execution and the remaining Entity API/Debug/default/source-test gates
remain unverified.

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
