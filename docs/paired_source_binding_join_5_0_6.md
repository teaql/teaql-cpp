# Paired source and native execution bindings for Rust 5.0.6

This SQ02 continuation follows the [named custom-comparison join](custom_comparison_binding_join_5_0_6.md)
and [generator-child repair](generator_child_optimized_checks_5_0_6.md).
It preserves both locked reference commits, existing Rust execution receipts,
runtime functions and generated artifacts. Capability status remains
**14 qualified / 3 partial**.

## Complete paired command inventory

The [registered verifier](../scripts/verify_paired_source_bindings_5_0_6.py)
joins all **35 source cohorts** to their actual native execution commands using
the unchanged, hash-pinned custom-comparison manifest. Each cohort retains one
source fixture owner and one native peer. A native peer must use the expected
Python interpreter, source script, contract selection, executable/build paths
and execution arguments; `--verify-only` cannot replace native execution.

Query and dynamic JSON share a source script but select different contracts.
The public customization cohort names three native executables explicitly.
Four cohorts also require the CTest build directory. Other comparisons happen
to share the `_source_rust_parity` name suffix; source script identity and
fixture requirements distinguish the actual paired verifier commands.

The [ordinary](parity/paired_source_binding_5_0_6.json.gz) and
[optimized](parity/paired_source_binding_optimized_5_0_6.json.gz) catalogs agree
on all 35 pairs. Eight controls reject a missing/duplicate peer, source-only
execution, a wrong build path, a missing source requirement/setup, a swapped
query contract and an undeclared extra peer. These checks qualify command and
execution-mode bindings; actual native execution is a separate obligation.

## Native execution continuation

The [coordinator receipt](parity/paired_source_binding_native_5_0_6.json.gz)
records the actual paired native commands, their source prerequisites, three
binding indexes and the configuration recipe. Both ordinary and optimized
selections pass **74/74** with exact registry/JUnit membership and no failures,
errors or skips. All **11,963 input hashes**, **115 evidence files**, both
references and **707 native ELF files** remain unchanged. The two selections
take 1,209.94 and 1,232.48 seconds respectively. Original Rust execution is
explicitly reused; the paired commands execute current native consumers.

The subsequent [ordinary P0 gate](parity/paired_source_binding_p0_5_0_6.json.gz)
passes **1/1** on the same frozen source inputs and reference commits. The
current registry has **1,101 tests / 619 Python entry files**, with zero direct
assertions in those entries. This is selected execution, not a full current
native epoch.

Both coordinators remove their owned JUnit directories after their children
exit: **43,017 bytes** in total. The four paired receipts and role inventory
retain **579,477 bytes**. No disposable build directory, copied binary or Rust
dependency checkout is created.

## Finite remaining SQ02 inventory

The [current Python registration inventory](parity/python_dispatcher_roles_5_0_6.json.gz)
contains **716 command registrations / 619 entry files** within the
**1,101-test** registry. It separates 305 direct comparisons, 160 named custom
comparisons, thirteen outside-directory comparisons and 35 paired source/native
commands: **513 comparison registrations** in total. Source, control and
invariant checks account for the other **203 Python registrations**.
The registry also contains 385 non-Python commands.

This role inventory retains exact commands, script hashes and fixture
properties. Docstrings are discovery hints, and these counts do not accept
the remaining checks or identify missing runtime features. Reuse unchanged
qualified evidence when reviewing each dependency; inspect actual child calls,
dynamic loading and executable strings rather than treating every historical
script reference as execution. Record an explicit disposition for every
remaining registration and executed acceptance boundary before SQ02 signoff.

Full current native execution, G14 metadata, G17 governed execution and all
P0-P7 exit criteria remain separate requirements. No sanitizer or MIPS profile
is required by this Python/CMake-only continuation.
