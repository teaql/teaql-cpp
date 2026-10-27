# Declared entity API driver source provenance

Status: all 28 declared field/entity API contracts are qualified for this finite
source-provenance cohort. All 11,022 raw original/corpus/native observations
agree. Complete SQ02, G13/G14/G17 and P0-P7 remain open; capability status stays
**14 qualified / 3 partial**.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Scope and actual compilation layout

The [contract registry](../tests/parity/entity_api_driver_contracts_5_0_6.json)
names all **28** remaining declared corpora originating in the legacy
`rust_field_layout` collector: **11,022** retained observations and **27**
distinct original driver files. JSON hydration and column-alias corpora share
one driver. Every target in the
[new source project](../tests/parity/rust_entity_api_source_5_0_6/Cargo.toml)
corresponds to one of these original drivers; no method bodies are extracted,
filtered or rewritten.

The cohort covers core/entity APIs, dirty/load/runtime graph state, graph
repository and planning models, logging, business/internal IDs, SQL types and
bindings, evaluation, inherited JSON and nullable/owned JSON graphs. The complete
legacy collector contains other probes and support modules; its remaining
responsibilities retain separate gates. This cohort does not qualify all of
that collector's targets or every possible semantic input.

The existing legacy manifest and lock contain registry versions outside the
locked baseline. They remain unchanged, including as an actual negative control.
The new project's lock is resolved from the reference lock and uses its exact
registry versions/checksums. The unchanged private-scope log-privacy module
requires a direct `sha2 = 0.10.9` declaration, already present in the reference
and legacy dependency sets. This is a Rust observation dependency.

Source execution uses a complete 610-file locked runtime archive, copied original
drivers, shared Rust fixtures and four unchanged generated Rust model libraries:
inheritance graph, dynamic JSON, member aliases and runtime sample fields. All
306 source/project files are copied into the owned layout. Includes, Cargo
manifests and generated bodies retain their original bytes. Actual metadata,
all executable targets, local dependency paths and the compiled lock are retained;
compilation uses `--offline --locked --bins`. No source appendix or dependency
omission is used. Copied source and archive hashes are checked again after
execution, before cleanup.

## Setup and failed attempts

The [setup diagnosis](parity/entity_api_driver_setup_diagnosis_5_0_6.json.gz)
retains the initial incorrect assumption that 28 corpora meant 28 executable
targets. The source-project creator stopped; dependent preparation was attempted
before its success had been checked and stopped on the missing directory. The
creator was corrected to emit each of 27 targets once. Successful preparation
was then checked before compilation.

The [first source attempt](parity/entity_api_driver_execution_5_0_6.json.gz)
records an unresolved direct `sha2` import in the unchanged log-privacy module.
Its complete owned build directory was removed after Cargo terminated. The
[corrected dependency preparation](parity/entity_api_driver_preparation_sha2_5_0_6.json.gz)
keeps every registry identity within the reference lock. Failed receipts are
retained; no corpus input, expected outcome, original driver, generated library
or production C++ implementation is changed by these setup fixes.

## Verification boundaries

The [replayer](../scripts/replay_entity_api_drivers_5_0_6.py) retains raw stdin
identities and full stdout/stderr. The
[validator](../scripts/verify_entity_api_driver_contracts_5_0_6.py) checks complete
source/corpus/native bindings without compiling Rust during routine CTest.
The [successful execution](parity/entity_api_driver_execution_final_5_0_6.json.gz)
and [binding receipt](parity/entity_api_driver_bindings_5_0_6.json.gz) distinguish
27 actual targets from 28 input matrices. All
[25 controls](parity/entity_api_driver_controls_5_0_6.json.gz) reject substituted
archives/models/sources, changed legacy identity, replaced manifests and locks,
unlocked or incomplete builds, wrong targets/dependencies/versions, incomplete
raw results and Boolean/number confusion. The actual legacy lock remains outside
the reference package set. The
[invariant dispatcher](parity/entity_api_driver_dispatcher_5_0_6.json.gz) executes
the registered control command without substituting a C++ probe.

The [native gate](parity/entity_api_driver_gates_5_0_6.json.gz) passes **30/30**
affected tests and **2/2** focused tests. Expected test names are computed from
the selected registrations and their declared fixture setup/cleanup closure;
JUnit must match that complete set with no failures, skips or duplicates. Every
existing CLI is preserved and all 28 comparisons require the new source fixture.
Native and modules configurations pass; no imported execution is repeated.
All [27 source guards](parity/entity_api_driver_guards_5_0_6.json.gz) pass, with
pinned format/check and baseline/module-path verification retained separately
in the same receipt.

## Cumulative scope and artifact lifecycle

The [inventory](parity/entity_api_driver_inventory_5_0_6.json.gz) locates
903 registered tests, 305 direct corpus registrations and 182 custom-dispatcher
entries. These are locator counts, not a fresh full 903-test run. Source bindings
now cover **139 unique corpora / 153,332 observations**, leaving **166 direct
registrations** and **3 of 107 declared-driver project/view responsibilities**:
mutation descriptor caching, mutation execution and SQL cached partitioning.
Complete remaining custom provenance and normalization remain separate work.

All 1,813 C++ inputs retain their frozen native893 identities. Producer
implementations, generated artifacts and module exports are unchanged; earlier
metadata source63,918/Java134 and artifact gates retain their own epochs.
The [cleanup report](parity/entity_api_driver_cleanup_5_0_6.json.gz) records exact
owned output removal after terminal compiler/test completion, including failed
attempts without inventing their unmeasured sizes. Compact receipts, stable
presets, source and shared caches remain. The
[milestone](parity/entity_api_driver_milestone_5_0_6.json.gz) seals input changes
and current evidence without promoting an incomplete capability or phase.

Existing JSON, quoted-FK and invalid-wire-profile compatibility decisions retain
their documented scopes. This source cohort does not expand those exceptions.
Original Rust unit-test execution, full current native integration, imported
execution, sanitizer/MIPS profiles, external services and physical-device
execution are not established by compilation or source hashing alone. Complete
metadata grammar/configuration, protected execution and remaining source/CLI
provenance retain their independent requirements.
