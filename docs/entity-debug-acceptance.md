# Entity model Debug acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Original EntityError and BaseEntityData derive structural Debug. Their C++
counterparts now provide `write_debug` overloads and use the existing
`debug_string(destination, model, options)` interface through
`teaql/core/entity_debug.hpp`. The core.hpp umbrella now exposes this header. `entity_debug_contracts` and
`entity_debug_rust_parity` are registered; fresh consumer acceptance has completed successfully.

## Preserved responsibilities

EntityError retains both `entity` and `message`; its Debug remains distinct from
Display. BaseEntityData retains `id`, `version` and the source field name
`dynamic`, despite C++'s public storage rename to `dynamic_values`. Every dynamic
key and Value variant remains visible. Field order, map order, empty maps,
quoted strings and escaping match the derived source implementation.

Nested formatting inherits ordinary/pretty, lower/upper hexadecimal, alternate,
width, precision, alignment, Unicode fill, sign and zero-padding behavior.
Overloads operate on borrowed immutable models without requiring another copy.
ADL also finds the overloads through the existing default SmartList element
formatter. Arbitrary handwritten entity types retain the separate custom
borrowed formatter API; no Debug capability is invented for source types that
do not implement it.

These are raw source diagnostics. They retain untrusted text and credentials
exactly where the source Debug does. They do not replace SQL/audit privacy
projections or become a new safe logging entry point. Neither source type gains
an invented serde protocol.

## Independent acceptance on 2026-10-07

The original-call corpus covers 289 inputs, both derived types and nine formatter
modes: 5,202 exact output strings. IDs include zero, one and maximum u64;
versions include minimum/maximum i64, negative and zero. Dynamic values cover all
thirteen Value variants and every TypedNull DataType, nested JSON/list/object,
Decimal, nonfinite floats, empty/Unicode keys and escaped text. The empty/default
model and empty error fields are checked separately.

| Evidence | Result |
| --- | --- |
| Standalone native probe against live original Rust | 289/289 inputs, all 5,202 outputs match. |
| OpenWrt MIPS/QEMU probe against live original Rust | The same 289/289 inputs and 5,202 outputs match. |
| Native default-PMR-disabled ownership contract | Passed. |
| Standalone ASan/UBSan ownership contract, leak detection | Passed. |
| Standalone MIPS/QEMU ownership contract | Passed. |

The ownership contract checks complete raw shapes, inherited pretty/hex fields,
SmartList ADL dispatch, immutable source models and unchanged object reservation
count for formatting scratch. Explicit Global destination output survives actual
source User destruction with the Global allocator. Probes compile the new public
header and link existing static archives. This is scoped evidence, not a full
fresh native/sanitizer/MIPS consumer-rebuild claim.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin entity_debug_oracle --locked --offline
python3 tests/parity/generate_entity_debug.py \
  --rust /home/philip/shared-cargo-target/debug/entity_debug_oracle
python3 tests/parity/compare_entity_debug.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/entity_debug_oracle
```

For MIPS add `--cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'`. The native contract
is `tests/entity_debug_tests.cpp`. Named targets are registered alongside the
entity registration and BaseEntityData diagnostic checks. The final full native and selected sanitizer/MIPS integration has finished.
No generator or production dependency changed. Remaining Entity/default/API/
source-test gates and physical-router execution are not established here.

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
