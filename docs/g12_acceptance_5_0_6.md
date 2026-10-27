# G12 acceptance audit

G12 is qualified after [complete R01-R18 post-approval acceptance](approved_exception_capability_acceptance_5_0_6.md). The following sections preserve their earlier source/integration scopes; the invalid-profile decision and final signoff are now complete.

Recorded combined integration passes native 51/51 and modules 40/40 at its
frozen checkpoint. The later
[Context dynamic schema repair](context_dynamic_schema_lifecycle_5_0_6.md) changes
shared runtime orchestration and has its own current selected execution gates;
the earlier 51/40 outcome is not relabeled as a post-repair full combined run.
That earlier checkpoint preceded the final G12 signoff. Complete P0-P7
acceptance remains open.
The user approved R08 explicit rejection on2026-10-24; see the
[bounded contract](invalid_wire_naming_compatibility_exception_5_0_6.md).
Rust remains `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and the generator remains
`45e1572c4956ca963410d8ea436ead3c8857087a`. Earlier 5.0.5 evidence is historical.

## Finite source responsibilities

The [current reconciliation](parity/g12_current_responsibility_reconciliation_5_0_6.json.gz)
preserves all eighteen responsibility groups, their exact source anchors, 28
complete locked source bodies and 22 primary evidence records. It follows the
[source review](g12_source_responsibility_review_5_0_6.md); a source inventory or
passing existing examples alone cannot establish acceptance.

The [driver review](parity/g12_generator_driver_review_5_0_6.json.gz) adds the
previously unindexed RustLibGenerator, RustRuntimeGenerator and
RustSampleDataGenerator to the locked source ledger, which now has 42 entries.
It checks EntityDomain wrapping, library/runtime dispatch, view/value-object
exclusion, depth/name ordering and root/constant/business sample partitions.
The source sample template uses reverse-relation presence rather than traversing
the sorted reverse list. No new runtime or generator algorithm is introduced.

R04 now has its own [qualified startup evidence](generated_business_environment_startup_5_0_6.md).
The remaining supported responsibilities must retain their actual source
outcomes, ownership and current combined execution evidence. Historical broad
questions are reconciled against later concrete checkpoints rather than treated
as an indefinite request for more models.

## Approved invalid-profile behavior

R08 retains an actual invalid-profile difference. The frozen Rust generator
returns success and empty wire strings for an invalid `json_field_naming`;
current C++ rejects it. The [original inspection](parity/generated_wire_mappings_profile_inspection_5_0_6.json.gz)
contains both real generation outcomes. Three valid profiles and their mapping
functions have separate source comparisons.

On2026-10-24 the user explicitly approved retaining C++ rejection as a bounded
compatibility exception. The original successful empty-wire artifacts remain
unchanged. The new permanent test checks three unsupported profiles; the fresh
[source/Java gate](parity/invalid_wire_profile_java_source_gate_5_0_6.json.gz)
passes8252 source comparisons and115 Java tests. Formal G12/G14 acceptance
still requires its complete responsibility/current-gate review.

## Current integration selection

`scripts/verify_g12_integration_5_0_6.py` preserves the earlier 45 native and
34 module selections, then adds the qualified key/environment/initializer and
inherited startup contracts. Native also includes the existing generic I18n
controls. Module coverage now includes the same generated catalog/Checker test
body and the complete 82-outcome generated catalog comparison; these were
absent from the earlier imported selection.

Both current builds finish successfully: 48 native targets and 38 module targets.
The new import target uses the original handwritten test body and existing
GraphFixState/GraphFixActivation exports, without project declaration includes
or production body changes. Its source-root macro supplies the actual generated
catalog path. The [new imported preflight](parity/g12_generated_i18n_module_preflight_5_0_6.json.gz)
passes both added tests, including the full 82-outcome catalog comparison.
The later forty-test combined imported selection also passes.

The selected tests are 51 native and 40 imported checks. The current native
selection is terminal and all 51 checks pass: the whole bootstrap passes in
168.14 seconds under its unchanged timeout, then the remaining fifty tests pass
in 763.08 seconds. Native executable hashes are unchanged after execution.
The imported whole bootstrap passes in 157.24 seconds under its unchanged
timeout, then the remaining thirty-nine tests pass in 729.69 seconds.
Both complete selections are terminal, including post-execution executable
hash checks. The [combined terminal record](parity/g12_final_integration_terminal_5_0_6.json.gz)
preserves the previous six ledger questions before their reconciliation.
Each profile first runs
the complete unchanged 2,304-case bootstrap comparator alone under its original
timeout. The remaining complete tests run with two workers. Profiles execute
sequentially after both builds terminate, avoiding the previously observed
bootstrap contention without partitioning a corpus or raising its timeout.

Records retain exact current registrations, commands, executable hashes and
terminal output:

- [Native integration](parity/g12_final_native_5_0_6.json.gz).
- [Module integration](parity/g12_final_module_5_0_6.json.gz).
- [Before checkpoint](parity/g12_integrated_acceptance_before_5_0_6.json.gz).

A record with `build_complete` proves compilation/linkage only; `complete` stays
false until its entire selected execution finishes. A live process or tool
handle is polled rather than restarted because an observation timed out.

No production C++ body, generated SDK, external integration or dependency changes
in this checkpoint. Existing targeted sanitizer evidence retains its own scope;
this test-registration/source-review work does not require full ASan/UBSan,
MIPS/QEMU or physical-router execution.

## Reproduction

Use fresh record paths for a new build/execution audit; preserve previous failed
or partial records. Build both profiles before executing the staged selections.

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_g12_integration_5_0_6.py --profile native --stage build --record /tmp/g12_native.json.gz
python3 scripts/verify_g12_integration_5_0_6.py --profile modules_native --stage build --record /tmp/g12_module.json.gz
python3 scripts/verify_g12_integration_5_0_6.py --profile native --stage test --record /tmp/g12_native.json.gz
python3 scripts/verify_g12_integration_5_0_6.py --profile modules_native --stage test --record /tmp/g12_module.json.gz
```

Audit every responsibility against its current evidence and resolve R08 before
changing the capability status. G08/G13-G17 and the independent
phase/release exits remain required even if the G12 selection passes.

The [current source audit](g12_current_source_audit_5_0_6.md) completes that
finite R01-R18 review. It independently accounts for every passing name in the
historical 51/40 selections and verifies all 28 locked source bodies and 22
primary records. Five strict provenance checks pass. Five earlier broad-observer
families also freshly replay 347/347 original outcomes under exact reference
registry identities; the old workspace's 46 dependency differences remain
recorded. These are source/evidence checks, not current C++ execution. R08,
the running full native epoch and formal capability promotion remain open.
