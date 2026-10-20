# Inherited Business-ID custom Checker and governed Save: Rust 5.0.6

All **450/450 adapted-source/native observations match**. The matrix extends
G02/G03 with actual public audited saves across Document, IntermediateRecord,
Invoice, Receipt and third-level SpecialInvoice. It does not complete P1 or the
full generator goal.

## Source integrity and adaptation

The original 43-file generated domain remains unchanged. The observer uses the
accepted [inherited binding adaptation](inherited_business_binding_bridge_5_0_6.md):
four shared-type imports, the exact parent BusinessIdSlot rebound to each child
carrier, and crate-private visibility for the shared identifier tuple field.
No entity, Checker, request, runtime or persistence algorithm is replaced.
The original compilation failures remain source dispositions; adapted execution
is not untouched artifact acceptance.

The new observer is independent of the earlier governed/update observers and
their corpora. Its Cargo workspace retains the reference registry versions and
checksums. [The verifier](../scripts/verify_inherited_business_extensions_5_0_6.py)
reproduces the exact binding specification, compiles offline with the lock and
compares fresh source/C++ execution. The
[source record](parity/inherited_business_extensions_source_5_0_6.json) retains
every original/derived file fingerprint and the explicit adaptation.

## Complete business-operation observations

The five models combine Create/Update, ordinary/denying/fixing custom logic,
allowed/denied whole-plan MutationPolicy, valid/blank/overlength names and
normal/trigger-failed writes. Updates also exercise optimistic conflict after
a concurrent version change. Update inputs first persist an ordinary seed,
then install custom logic and Policy before the second save.

Each actual generated Checker performs predefined fixes and loaded-update
checks before calling its concrete business callback. Custom logic calls the
generated required/min/max helpers, can append a finding and stage a name repair.
Earlier findings remain errors even when a later repair changes the field.
Default checkers for the other model types remain registered.

The result contains ordered callbacks and preexisting findings, allocated ID/root
observations, immutable Policy plans, exact non-clock changed values, clock-field
presence, complete snake-case diagnostics, ordered fix evidence, saved values and
authoritative database rows. Operational timestamp values are projected by
presence and creation-time retention, not normalized to an invented value.
The fixed BusinessClock/key controls ID generation; it does not replace the
graph operation clock.

The corpus has 450 callbacks, 100 immutable plan reviews, 600 findings and
1170 fix-evidence records. Twenty operations commit; 350 stop with Checker
findings, 50 with Policy denial, 20 through SQL triggers and 10 with stale versions.
Success retains the ID and creation time on updates. Denial/trigger/conflict paths
preserve the actual seed or empty table rather than reporting simulated rollback.

## Ownership and verification

Native observers use explicit Context allocation for models, custom logic,
registry adapters, Policy, keys and clocks. Default PMR is disabled. The result
is copied into Global, stack PMR values leave scope, UserContext is deleted and
only then is the retained output serialized.

Native regression passes 4/4; the new module and ASan/UBSan selections each pass
1/1. Leak scanning is disabled. The
[checkpoint record](parity/inherited_business_extensions_5_0_6.json) retains
terminal results and fingerprints. No production/runtime/generator function,
dependency, full-suite, MIPS or physical-device result changes.

The [G02/G03 matrix](checker_behavior_requirements_5_0_6.md) now has direct
inherited/custom Business-ID governed-save evidence in addition to the existing
80 creation/update/retry comparisons. Source/helper closure, full customization
review and the other capabilities' integration obligations remain independent.

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_inherited_business_extensions_probe
python3 scripts/verify_inherited_business_extensions_5_0_6.py
ctest --preset native -R '^inherited_business_extensions_rust_parity$'
```
