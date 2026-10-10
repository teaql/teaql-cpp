# Structured model Debug acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Original CompactRow and SmartList derive Debug. Typed Value text alone did not
provide their complete structural diagnostic capability; the public
`teaql/core/debug.hpp` boundary now formats their actual fields and recursive
facets without copying the models or introducing a formatter dependency.

`debug_string(destination, model, options)` returns a PMR string in an explicit
destination. `DebugWriter` borrows that output and exposes structural, tuple,
sequence, text and scalar writing for custom element formatters. The generic
SmartList overload accepts a callback `(DebugWriter&, const T&)`; it imposes no
Clone or assignment requirement. Default element writing covers Value, rows
and supported primitive/string/Decimal/date/timestamp types. It is exported
through the core umbrella without merging core and runtime responsibilities.

## Original formatting contracts

- Row fields are columns and values in their original projection order,
  retaining duplicate names and escaped Unicode/control characters.
- List fields appear in source order: data, total_count, aggregations, summary,
  facets, is_loaded. Recursive facet values retain their complete row/list
  shapes, optional totals and loaded state.
- Compact and alternate/pretty structures retain source spaces, indentation,
  empty containers, tuple shapes and trailing commas. Custom multiline writes
  inherit the active structural indentation.
- Lower/upper hexadecimal Debug propagates to integer fields. Negative signed
  values use their source-width unsigned representation; alternate hex includes
  the source 0x prefix, including uppercase digits.
- Width, precision, fill, alignment, explicit plus and zero padding propagate
  to source scalar formatters. String/enum/date diagnostics and JSON's own Debug
  follow their distinct source behavior. JSON numbers/bools do not inherit the
  outer numeric flags. Decimal delegates to the already-validated Decimal
  formatter; float precision uses fixed correctly-rounded standard conversion.
- NaN suppresses the plus sign even when requested, and retains source padding.
  This behavior was identified by the unchanged source oracle and repaired.

These are raw diagnostic facilities, like the source Debug trait. They do not
replace the independent safe SQL/audit/log projection or invent a model serde
protocol. CompactRow/SmartList have no source-derived serde protocol.

## Evidence and ownership

The corpus records 169 unchanged original public formatting calls, each returning
nine modes for a row and complete list: compact, pretty, lower/upper hex, pretty
lower/upper hex, right-aligned width/precision, Unicode-centered fill with plus
and precision, and plus/zero padding. This is 3,042 exact formatted outputs.
Inputs cover every Value variant, nested Json/object/list content, signed and
unsigned extrema, non-finite/negative-zero/large/small floats, Decimal scales,
control/Unicode text, duplicate columns, explicit zero/maximum totals and loaded
state. Expectations come from original derive-generated Debug and standard
formatting, not a copied formatter algorithm.

Native tests disable default PMR, format non-copyable/non-assignable custom
elements exactly once in order, and check owner-local output, unchanged source
data and no additional independently managed object IDs. Explicit Global copies
remain complete after actual source User deletion. Invalid Unicode fills are
rejected. Borrowed models must remain live throughout formatting; raw pointer
access is not intercepted. Output and scratch remain subject to the destination
arena's existing byte limits. Formatting failure may leave partial output;
there is no universal callback transaction guarantee.

## Reproduction and remaining scope

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin model_debug_oracle --locked --offline
python3 tests/parity/generate_model_debug.py \
  --rust /home/philip/shared-cargo-target/debug/model_debug_oracle
cmake --build build --target teaql_model_debug_tests teaql_model_debug_probe
ctest --test-dir build --output-on-failure -R '^model_debug_'
python3 tests/parity/compare_model_debug.py --cpp build/teaql_model_debug_probe \
  --rust /home/philip/shared-cargo-target/debug/model_debug_oracle
```

On 2026-10-07 the complete rebuilt native suite passed 200/200. The two dedicated
contracts passed 2/2 on native, ASan/UBSan with leak detection and OpenWrt MIPS
under QEMU. All 169 observations matched live original Rust on native and MIPS.
Pinned upstream, root export-name coverage, inventory, clang-format 18.1.8 and
git diff whitespace checks passed. These checks do not establish generic
formatting for every other model, all formatter-flag/value combinations, or
complete core/runtime/API/privacy/source-test acceptance. The six-stage goal
remains broader than this capability repair. Physical-router execution remains
unverified; MIPS testing uses the established OpenWrt toolchain and QEMU.
