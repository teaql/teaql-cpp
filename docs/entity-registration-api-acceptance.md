# Entity registration dispatch audit

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Status: the production dispatcher/default repair is applied, both named
checks are registered, and final native/sanitizer/MIPS integration passed.

## Original contract and observed gap

`TeaqlEntity::register_into` is a trait method with a default implementation that
registers `Self::entity_descriptor()`. Implementations can override it.
`register_entities!` calls that method for each named type, in order, including
repetitions. It does not inline the default body or guarantee one descriptor per
entity. Store failure leaves already performed registrations intact and prevents
later hooks from running.

The earlier C++ `register_entities<T...>` directly registered every descriptor,
bypassing `T::register_into`. A custom hook registering two descriptors and a
silent hook registering none expose the mismatch. Across six sequences and seven
store limits, that negative control matched only 12/42 unchanged original calls.
These are dispatch/ordering/failure mismatches, not allocator adaptations.

## Concrete repair verified independently

Dispatch each type to its `register_into(store)` when present; descriptor-only
C++ models retain the implicit default. Change `EntityDefaults<T>::register_into`
to register `T::entity_descriptor` directly instead of invoking the dispatcher,
which would otherwise recurse. Preserve left-to-right pack evaluation.
The empty C++ pack remains an explicit no-op extension; the Rust macro requires
at least one type.

An isolated copy of the current header with those two changes matches all 42
live original observations. The source crate and original macro/methods are
unchanged. A separate native generic contract exercises inherited defaults,
overrides, silent hooks, duplicates, descriptor-only models, empty packs and
store failure. It disables usable default PMR and checks explicit Global copying
of every registered name after actual destruction of the source User.
These standalone probes link existing native libraries and prove the proposed
header behavior, not installed production or full-regression acceptance.

Repository fixtures:

- `tests/parity/rust-values/src/bin/entity_registration_api_oracle.rs` calls the
  original macro and trait implementations directly.
- `tests/parity/entity_registration_api_cases.json` retains all 42 inputs and
  observations, including attempted calls, applied names and completion.
- `tests/parity/entity_registration_api_probe.cpp` exercises the C++ dispatcher.
- `tests/entity_registration_api_tests.cpp` contains the native ownership and
  generic capabilities contract.

The concrete repair is now applied, with both named checks registered and
rebuilt integration completed. This audit does not alone complete downstream
Entity/module/source-test acceptance.

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
