# Temporal bootstrap extensions and clock replacement

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Status: 480 exact original/native outcomes pass. Native/module/ASan-UBSan
each pass 1/1 after successful builds; leak scanning is disabled. G05/P3 remains partial.

The complete unchanged constants_temporal_required Rust domain and generated C++
domain run actual public module, SQLite schema/bootstrap, typed query and audited
ledger-save methods. The new observer registers generated Priority Checker logic
and replaces an application-owned BusinessClock. It does not implement local
seeding, checking, persistence or reconciliation algorithms.

The expanded 480 scenarios cross empty/existing/corrupt data, four module factories,
none/observe/fix/deny/late-deny Checker modes, two directions of business-clock replacement,
two process wall clocks and one/two Context rounds. All 720 rounds use the existing
test-only wall-clock shim; monotonic clocks remain untouched. Business clocks
include -1 and a modern date, independently of positive advancing wall time.
The original observer's registry versions match the reference Cargo.lock.

Observations include complete before/after physical tables and ID spaces,
bootstrap SQL/bindings/intent, root identity, Fix evidence, Checker callback order,
clock values before/after replacement and actual subsequent audited-update rows
and diagnostics. No JSON/error/timestamp normalization is applied. Bootstrap can
invoke the registered Checker along its actual automatic-save path. The expanded
matrix records 840 bootstrap callbacks; none are fabricated by the observer.
Ordinary generated save also invokes the Checker. A custom fix can replace the
stored create timestamp and clock text. Deny mode can stop bootstrap before an
ordinary save is attempted; late-deny switches the User-owned logic after successful
bootstrap so ordinary-update rejection and unchanged readback are tested directly.
Replacing BusinessClock does not replace the operational seed/graph clock.

The C++ observer disables default PMR, stores observations in explicit Global
storage, registers callback/clock resources in UserContext and actually destroys
each UserContext before the next round. Context ownership does not depend on a
hidden singleton or shared owner. The clock shim is not a production dependency.

```sh
set -e
source scripts/toolchains/activate.sh
python3 tests/parity/compare_bootstrap_temporal_extensions_5_0_6.py \
    --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/teaql_bootstrap_temporal_extensions_506 \
    --cpp build_native_current/teaql_bootstrap_temporal_extensions_probe \
    --clock build_native_current/libteaql_sample_fixed_clock.so
```

Build both observers successfully before invoking the comparison. Ordinary CTest
uses the retained source corpus. See the [execution record](parity/bootstrap_temporal_extensions_5_0_6.json).

The final matrix observes 840 bootstrap and 360 ordinary-save callbacks, 240
bootstrap failures and 120 ordinary-update rejections. Canonical module consumers
verify 522 bodies; pinned format/check covers 1358 C++ files. No runtime algorithm,
MIPS profile or physical-device execution changed in this checkpoint.
