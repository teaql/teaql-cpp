# Mutation wrapper and command Debug acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-core/src/mutation.rs`. Public derived diagnostics are a model capability,
separate from execution, snapshot/mutation roles and safe log projection.

## Complete source mapping

mutation_debug.hpp supplies ten overloads through core.hpp, reusing DebugWriter,
Value diagnostics and the existing public Trace formatter. No production source
variant, role, constructor, equality operator, dependency or lifetime changes.

| Source type | Preserved diagnostic contract |
| --- | --- |
| MutationValues | Distinct named tuple containing the ordered field map; intentionally supplied mutation input. |
| GeneratedValues | Distinct named tuple containing the ordered field map; provider-generated output. |
| EntitySnapshot | Distinct named tuple containing the ordered field map; previously persisted values. |
| MutationKind | Insert, Update, Delete and Recover unit variants. |
| InsertCommand | entity, values and trace_chain in source declaration order. |
| UpdateCommand | entity, id, expected_version, values, trace_chain and old_values; None/Some(empty)/Some(populated) snapshots remain distinct. |
| BatchInsertCommand | entity, batch_values and trace_chains; empty and nested trace lists remain visible. |
| BatchUpdateCommand | entity, batch_ids, batch_expected_versions, batch_values, update_fields, trace_chains and batch_old_values. Separate lengths and duplicate update fields are preserved. |
| DeleteCommand | entity, id, expected_version, soft_delete and trace_chain. Formatting does not choose the deletion policy. |
| RecoverCommand | entity, id, required expected_version and trace_chain. |

Options propagate through tuple/map/list/optional/struct boundaries. Compact and
pretty output, lower/upper hex, pretty hex, width/precision, Unicode fill and
alignment, explicit sign and zero padding use the common formatter. Raw fields
and audit comments remain complete; these functions are not safe SQL log
projections. Invalid C++ MutationKind values fail explicitly, an extension for
states the original Rust enum cannot represent.

## Original and ownership evidence

A previous correct public-core syntax-only fixture failed at missing write_debug
for MutationValues. With the production header it now compiles. The unchanged
original derived Debug supplies all expectations: 120 inputs and 9,810 exact
formatted outputs, recorded in deterministic gzip without discarding fields.
All thirteen Value variants, NaN/negative zero, Decimal scale, recursive JSON and
containers, control/Unicode/NUL keys, empty/default models, optional versions,
three old-snapshot states, hard/soft deletion and irregular batch lengths are
observed. Every command and all four unit kinds appear independently.

The initial standalone native probe matches all 120 original inputs. The native
ownership contract passes with default PMR disabled, qualified facade access and
actual User deletion before reading retained Global batch data and complete raw
output. It checks nested map/list/string/snapshot/trace allocator propagation;
formatting consumes no managed identities and never normalizes the command.
The final transparent-map lookup refinement is included in the completed named acceptance below.

## Integrated acceptance pending

mutation_debug_contracts and mutation_debug_rust_parity are registered. The full
native build and regression completed successfully with 228/228 checks. Fresh
ASan/UBSan with leak detection and OpenWrt MIPS/QEMU builds each pass all eight
selected checks: new mutation diagnostics, existing mutation API/equality,
query/Trace diagnostics, core models and Context references. Fresh named native
and MIPS probes match all 120 live original inputs and 9,810 exact outputs. The
previous 226-test checkpoint is superseded for this header change.

This implementation does not establish full mutation execution/governance/log
parity, generic external serialization diagnostics, all six stages or physical
router execution. Keep the three field-map roles and every command distinct.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin mutation_debug_oracle --locked --offline
python3 tests/parity/generate_mutation_debug.py \
  --rust /home/philip/shared-cargo-target/debug/mutation_debug_oracle
python3 tests/parity/compare_mutation_debug.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/mutation_debug_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'.

## Selected integrated checkpoint

Fresh ASan/UBSan with leak detection and OpenWrt MIPS/QEMU builds completed
successfully and each passed all eight selected checks, including the final
transparent-map lookup refinement in the ownership test. Fresh named MIPS public
formatting matches all 120 live original inputs and 9,810 exact outputs. The full native rebuild/regression also completed with 228/228 checks; its
fresh named probe matches all 120 live original inputs. All three original
process handles returned successful terminal results.

## Current 5.0.6 whole-source continuation

The preceding checkpoints remain historical 5.0.5 evidence. The complete current
[MU01-MU14 review](core_mutation_source_review_5_0_6.md) now accepts the
291-line mutation source. All 2,048 original/native/imported outcomes per
frontend, one positive/fourteen precise negative source compiler controls and
native 5/imported 5 pass. Four complete canonical consumers are permanently
imported; native model assertions cover all six role conversion/assignment
directions. Existing function bodies and all production implementations remain
unchanged. Whole execution, governance, privacy and G13/G14/G17/P0-P7 retain
their separate obligations. No historical full or sanitizer/MIPS result is
relabeled as a new run.
