# P6 expression and application assembly phase acceptance: Rust 5.0.6

Status: **accepted for the P6 exit criteria** in the
[generator parity plan](cpp_generator_parity_plan.md). This phase joins the
qualified G11 structured E and G12 runtime assembly, sample-data and Assist
responsibilities. It does not accept G13 semantic qualification, G14 complete
metadata/configuration grammar, G17 governance/provenance integration or P7.

The locked generator is
`45e1572c4956ca963410d8ea436ead3c8857087a`; the locked Rust runtime is
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Exit-criterion review

| P6 requirement | Evidence and disposition |
| --- | --- |
| Structured E diagnostics and source values | The [G11 eleven-row acceptance](expression_acceptance_5_0_6.md) reconciles complete generator bodies with actual Value, Null and NotLoaded behavior; scalar/FK and forward/reverse relation reads; missing-load paths, root capture and the complete structured diagnostic fields. Its 2,416 original observations and the later [960-case identity requalification](generated_expression_identity_5_0_6.md) preserve actual source outcomes and the documented C++ presentation adaptation. Current E selections pass native 14/14 and modules 5/5. |
| No implicit SQL or implicit lifetime extension | G11 E adapters evaluate loaded graphs and register no provider. Missing and omitted fields remain NotLoaded; a legitimate Null alone permits the lazy fallback. Runtime checks Context identities before access, and ordinary wrappers expire after Context deletion. Explicit destination copies are required for retention. Native Context/diagnostic tests and imported E consumers pass in the current selection; earlier targeted ASan/UBSan and compiler gates retain their original scopes. |
| Model-driven registration and schema startup | [G12 R01–R18](g12_acceptance_5_0_6.md) separates library/runtime/sample dispatch, model descriptors, independent repositories/behaviors/checkers, passive module installation, SQLite provider and executor roles, conditional Business IDs and governed schema/bootstrap. The later [Context dynamic-schema repair](context_dynamic_schema_lifecycle_5_0_6.md) adds the missing provider phase before Business IDs. The **complete** G12 native 51 and module 40 selections were build-checked and re-executed after that repair; every selected name passed exactly once and executable hashes match the pre-execution records. |
| Dependency-aware samples across supported configurations | The [G12 source audit](g12_current_source_audit_5_0_6.md) reconciles scale, IDs, root/constant dependency order, required skips, reverse publication, configured/temporal/JSON/Business-ID recipes, actual audited Save, partial progress, retry and Info callbacks. Its five exact-reference families replay 347 unchanged original outcomes. The current 51/40 complete integration includes the unchanged 2,304-case bootstrap and actual ordinary, inherited, catalog, business, configured and failure/continuation sample paths. An execution result is not inferred from a recipe inventory. |
| Accurate generated guidance and unsupported paths | G11 Assist documents leaf-only diagnostic paths and actual selection/ownership rules. G12 R15–R18 covers seven model-aware action workflows, field guidance, canonical JSON wire keys, schema/customization instructions, locale/catalog installation and the external Tool boundary. Unsupported Tool operations are reported unavailable rather than emitted as working kernel APIs. The approved [invalid wire-profile exception](invalid_wire_naming_compatibility_exception_5_0_6.md) keeps C++ explicit rejection and original Rust empty-wire results separate. Generated application APIs remain lowerCamelCase, with `user_context` naming and explicit Context lifetimes. |

This phase decision combines complete source/body reviews, real original/native
outcomes, generated application execution and current native/imported-module
integration. It does not infer semantic agreement from a method list, a model
inventory or a clean build alone.

## Current execution and timeout correction

The complete post-repair G12 driver configured, built and checked registered
commands for 48 native targets and 38 module targets. It then ran the whole
bootstrap comparison separately and the remaining selected tests with two
workers. Native passed **51/51 selected** tests; the native preset additionally
ran eight source-contract checks, all passing. Its complete bootstrap took
285.22 seconds and all remaining CTests ended successfully. Modules passed
**40/40 selected** tests; its bootstrap took 243.29 seconds. The driver checked
the executable hashes again after each profile's tests.

The first current module attempt hit the old **180-second CTest timeout** on
the same complete 2,304-input bootstrap comparison. That failed record remains
unaltered. Running the imported comparator directly completed all
**2,304/2,304** source comparisons in 251.42 seconds. The native CTest already
allowed 600 seconds for this same matrix. The module CTest timeout was aligned
to **600 seconds**; no case, source expectation, comparator, executable or
semantic assertion was removed. A fresh full module build/test record passes.
The failed and successful module records contain identical selected tests,
targets, bootstrap command and executable hashes; only the effective timeout
changes from 180 to 600 seconds.

The [P6 gate receipt](parity/p6_phase_current_gates_5_0_6.json) binds the
terminal [native G12 record](parity/p6_g12_current_native_5_0_6.json.gz),
the preserved [module timeout](parity/p6_g12_current_module_5_0_6.json.gz),
the [successful module retry](parity/p6_g12_current_module_retry_5_0_6.json.gz)
and current G11 CTest logs to their hashes. It verifies that every G12 selected
name has exactly one passing terminal entry. The current E selection is native
**14/14** and modules **5/5**. The earlier G11 compiler, Java, generated-SDK,
source and sanitizer records retain their own scopes. The latest independent
producer replay after the AP02 parser change preserved all 80 active SDKs and
passed 80,185 source observations and 157 Java tests; it is not relabeled as a
new G11/G12 compiler run.

The G12 integration entry points are:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_g12_integration_5_0_6.py \
    --profile native --stage build --record /tmp/teaql_p6_g12_native_new.json.gz
python3 scripts/verify_g12_integration_5_0_6.py \
    --profile modules_native --stage build --record /tmp/teaql_p6_g12_module_new.json.gz
python3 scripts/verify_g12_integration_5_0_6.py \
    --profile native --stage test --record /tmp/teaql_p6_g12_native_new.json.gz
python3 scripts/verify_g12_integration_5_0_6.py \
    --profile modules_native --stage test --record /tmp/teaql_p6_g12_module_new.json.gz
```

Use new record paths; a failed or partial record is never overwritten. The
historical 51/40 checkpoint remains historical rather than being relabeled
as a post-repair pass.

## Remaining boundaries

P6 accepts the declared E, assembly, sample and guidance scope with the
documented language/ownership and invalid-profile dispositions. It does not
approve arbitrary model attribute grammar, all protected publication sinks,
complete governed mutation provenance or every source semantic locator.
G13/G14/G17, P1 and P7 remain active. No new MIPS/QEMU, GPU or physical-router
result is claimed.
