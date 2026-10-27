# Business-ID runtime configuration qualification

Status: 27 original/C++ execution scenarios and 54 actual Context rounds match.
Shared-kernel native/module/ASan-UBSan selections each pass five checks. Generated
filename delegation is reproduced and its final native/module/lifetime/compiler
selection passes. Complete G12 assembly surfaces remain under qualification.
This is G12/P6 configuration work against Rust 5.0.6 and generator 45e. It does
not close the wider generator or runtime qualification plan.

The successful manual-key ownership checkpoint below is not complete source
startup acceptance. A fresh full-template review confirms the missing automatic
key-environment step and its typed failure ordering. Follow
[business_runtime_environment_alignment_5_0_6.md](business_runtime_environment_alignment_5_0_6.md)
for this required G12 repair. Historical observations and the explicit manual
path remain preserved; an environment reader can also use managed UserContext
ownership.

That [source-shaped startup repair](generated_business_environment_startup_5_0_6.md)
is now qualified separately. Its automatic-key observations do not relabel the
manual-key evidence below; R04 closure does not complete G12 or P0-P7.

## Original public assembly and persistence

The unchanged generated Business-ID library supplies `service_runtime_from_env`,
`service_runtime` and `service_runtime_from_pool`. The independent observer calls
these actual entries, inspects the database before explicit schema initialization,
initializes schema, performs a public audited Receipt save and reads complete
modeled ID/name/number/tenant/version fields through generated queries. Repeated
pool entries share the actual SQLite connection; environment/configuration entries
open fresh in-memory connections. Contexts are dropped after every round.

The [original observations](parity/business_runtime_configuration_original_5_0_6.json)
cover 27 scenarios and 54 Context rounds: three public entries, one/two/three
rounds and key versions 1, 2 and the maximum u32. All 54 actual saves succeed.
The fixed business clock is reflected in the generated identifier's business
date; this model has no created/updated timestamp fields, so no invented accessor
or synthetic timestamp evidence is used.

Eighteen further source-only observations retain missing/short/nonhex keys and
zero/invalid/overflow key versions across all three entries. Their complete
Display/Debug/source and original stderr are retained. The [build record](parity/business_runtime_configuration_source_build_5_0_6.json)
uses 129 registry name/version/checksum identities matching the frozen reference.
The original generated library, model, provenance and prior observer locks are
unchanged. Every source compiler/linker exits before its executable runs.

## Explicit C++ key and lifetime adaptation

The agreed C++ contract installs a typed application-owned key provider through
UserContext after runtime infrastructure assembly and before schema/business
operations. It does not introduce the original environment-key reader or a
hidden owner. The observer supplies synthetic version/bytes through the actual
key/provider APIs, calls generated environment/configuration/checked-connection
entries, then executes the same schema/save/query operations. This is the
existing ownership adaptation in [generated_business_ids_5_0_6.md](generated_business_ids_5_0_6.md).
Original environment failures are kept separately; the comparator never rewrites
them into successful C++ startup or claims identical key-configuration APIs.

For connection reuse, the C++ application requests a Global connection through
its UserContext and explicitly rebinds access for each subsequent Context. Each
round destroys its UserContext and requires zero live User bytes. Observation
objects are explicitly deep-copied into Global before close, with default PMR
allocation disabled. The query error is the actual RuntimeError payload carried
by the existing typed data-service envelope, without reconstructing or rewriting
its text or Debug. The comparison requires every recorded successful/failure
outcome, identifier, persisted row and version to match.

## Retained observer failures

The [initial source build](parity/runtime_configuration_models_original_initial_build_failure_5_0_6.json.gz)
used the wrong extension-trait syntax and omitted the Entity trait import. The
[next source build](parity/runtime_configuration_models_observer_field_failure_5_0_6.json.gz)
attempted timestamp accessors absent from the actual model. Corrections change
only the observer; no original API or fixture is patched.

The [initial C++ syntax record](parity/business_runtime_configuration_probe_initial_syntax_failure_5_0_6.json.gz)
used the wrong typed error envelope and passed duplicate PMR allocator arguments.
The [first execution failure](parity/business_runtime_configuration_observer_assembly_order_failure_5_0_6.json.gz)
installed the key before its required BusinessIdService existed. The observer
now follows the already documented assembly/key/schema order. These are test
construction failures, not evidence of runtime parity or reasons to change
runtime key ownership. C++ execution and imported/lifetime gates remain pending.

## Shared filename assembly follow-up

The configuration work initially left the old filename adapter's provider setup
body intact while the new entries used shared assembly. The provider/ID/executor
step is now extracted as `bind_generated_sqlite_runtime`; complete assembly
applies metadata/checker/behavior registrations and delegates to that step.
The upcoming thin filename adapter will retain its existing module-install-before-
connection-open ordering, then call the same binding helper. This prevents two
emitted provider assembly algorithms without changing existing filename/setup
semantics. Shared runtime qualification precedes the generated replacement.

The [shared-kernel checkpoint](parity/business_runtime_configuration_shared_kernel_checkpoint_5_0_6.json.gz)
records terminal native, imported and instrumented builds/execution. All 54 saves
and complete queried rows match the unchanged original, with the established
typed-key ownership adaptation. Leak scanning is disabled. This checkpoint
precedes the generated filename replacement and does not qualify full G12/P6/P7.

## Generated filename delegation checkpoint

The shared runtime binding extraction is followed by exactly one filename
adapter replacement per emitted contract SDK. The [controlled reproduction](parity/runtime_binding_contract_reproduction_5_0_6.json)
records 30 replacements across all 44 contract fixtures. The recording helper
requires the exact module-install / metadata-install / direct-filename-open /
shared-bind sequence, restores the prior function body, and requires every other
byte after pinned formatting to remain identical. Fourteen rejected-model
observations remain unchanged; no original Rust or historical C++ fixture is
rewritten. Complete workspace reproduction and final execution remain pending.

The [Java checkpoint](parity/runtime_binding_java_terminal_5_0_6.json.gz) passes
62 regressions, including source environment naming, plain/Business-ID binding
selection and preserved filename registration/open order. Earlier assertions
that expected provider/ID/executor algorithms in generated bodies are retained
in their failure records; they now require shared delegation. Actual executor
registration and query behavior are verified through the runtime selection,
not by asserting that duplicate setup text remains emitted.

## Final generated binding selection

The [terminal generated selection](parity/runtime_binding_final_execution_5_0_6.json.gz)
passes native 12/12, imported modules 8/8, selected ASan/UBSan 8/8 and four
positive/seven negative compiler fixtures. Pinned formatting applies/checks all
1,478 handwritten C++ files. The new filename fixture proves that a literal
`?mode=ro.db` suffix remains part of the filename, schema initialization stays
explicit, and failed native open leaves the module installed without an executor,
as before. Actual User destruction returns all User bytes. Existing Q/E and
custom Checker examples, real Business-ID contracts, bootstrap and inherited
lifetime checks pass alongside the complete 27-scenario/54-round source comparison.

The generated artifacts reproduce across 24 workspaces (288 files) and all 44
contract fixtures; exactly 30 emitted filename functions are replaced while
every other generated function remains unchanged. This is a qualified binding
responsibility, not complete G12. The [assembly qualification matrix](runtime_assembly_qualification_matrix_5_0_6.md)
identifies missing standalone registries, LocalSchemaProvider and type aliases,
with broader model/customization/sample/Assist and P7 requirements still active.

## Fresh original configuration replay

The [current locked-source replay](parity/business_runtime_configuration_original_replay_current_5_0_6.json.gz)
rebuilds the unchanged generated Rust Business-ID observer at runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a`. It executes all **27**
retained runtime/persistence scenarios (**54** Context rounds) against the
original and C++ probes, and replays the **18** original environment failures
without treating C++ explicit-key setup as the same API. Both receipt steps
exit zero. The one-off Cargo target used 254,885,349 bytes and was removed.

This verifies the retained observations on the current host; broader runtime
configuration, model customization, G13 and P0-P7 acceptance remain open.
