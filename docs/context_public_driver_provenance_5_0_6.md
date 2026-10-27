# Context capability source provenance

This cohort binds seventeen complete retained corpora to Rust 5.0.6 runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a` remains unchanged.

| Existing comparison | Complete observations |
| --- | ---: |
| Context locks and recording provider | 222 |
| Audit event/privacy projection | 104 |
| I18n diagnostics | 60 |
| I18n catalog and rendering | 4,575 |
| Telemetry model diagnostics | 110 |
| After-commit error models | 20 |
| Request intent and trace envelopes | 4,571 |
| Telemetry lifecycle | 1,032 |
| Context logging implementation | 327 |
| Runtime log formatting | 425 |
| Business-ID service | 859 |
| Runtime business-ID allocation | 2,150 |
| Context resources and configuration | 410 |
| Public facet/support execution | 1,178 |
| Facet intent and log boundaries | 36 |
| Purposed-query diagnostics | 46 |
| Context data store | 251 |
| Total | 16,376 |

The [registry](../tests/parity/context_public_driver_contracts_5_0_6.json)
and [dedicated project](../tests/parity/rust_context_public_driver_source_5_0_6/Cargo.toml)
declare **sixteen unchanged Rust observer bodies**. The two public facet
matrices share their retained support observer. Every corpus input remains
in its original order.

The [hash-checked input selector](../scripts/context_public_driver_inputs_5_0_6.py)
selects every declared input without selecting expected results. Fresh Rust
executables produce the observations independently. Complete source and C++
output must each match the retained expected values with strict JSON types
and exact record counts. No corpus or observer body is rewritten.

The owned source build preserves the full **610-file runtime archive** and
**41 copied source/input/project files**. Actual Cargo metadata must contain
exactly this project and the six archived TeaQL packages. All registry package
names, versions, sources and checksums must belong to the reference Cargo.lock.
The build uses `--offline --locked --bins`, debug assertions, no incremental
compilation and an owned temporary target tree. Direct Tokio and async-trait
dependencies are test-only and use the reference-locked versions. No C++
runtime dependency is added.

The [fresh execution](parity/context_public_driver_execution_5_0_6.json.gz)
matches all **16,376 source/corpus/native observations**.
[Binding validation](parity/context_public_driver_bindings_5_0_6.json.gz) and
[twenty negative controls](parity/context_public_driver_controls_5_0_6.json.gz)
pass. They reject missing/substituted sources, targets, packages, locks and
payloads; failed/unlocked/partial builds; missing/extra results; Boolean/number
confusion; and an incorrect project/runtime source root. The audit-projection
corpus retains its explicit project-relative `.reference/teaql-rs` locators.
The source archive is independently checked against the locked repository.

The [registered gate](parity/context_public_driver_gates_5_0_6.json.gz) passes
**19/19 affected entries** and **2/2 previous core-model verification entries**,
including its nineteen controls. Exact JUnit identities, fixture requirements,
unchanged execution inputs and no skips are retained. Source-mutation checks
remain serial. Neither shared source verification nor its negative-control
helper changes in this cohort.

Existing observer limits remain explicit:

- The Context logging observer compiles the original private logging
  implementation unchanged against its documented field/resource shell.
  Resource assembly and complete Context event dispatch retain their separate
  live-runtime and append-only bridge evidence.
- Runtime log formatting retains the original observer's human-time display
  projection. Other historical header/executable-name adapters keep their
  earlier scope; this cohort introduces no new output normalization.
- Audit event projection proves the retained event/privacy module observations,
  including invocation provenance and bootstrap projection. It does not prove
  complete configured governance or protected-sink delivery.
- The locking provider is a recording fixture. No external lock/cache service,
  cloud integration or new provider support is established.
- Facet and purposed-query observers call the unchanged public runtime helpers
  with their recording executor. They retain their intent/error boundaries;
  compiling them is not whole generated-model or graph-Save acceptance.
- Error-model cases cover public after-commit error construction and diagnostics.
  Commit queues, transaction delivery and retry behavior retain separate gates.
- I18n, telemetry, business-ID and Context observations remain native64 Unix
  Debug scope. External telemetry adapters and other architectures are separate.

The [source partition](parity/context_public_driver_partition_5_0_6.json.gz)
now covers **264 disjoint corpora / 276,863 observations** across separate source
epochs. **41 other direct registrations** remain, distinct from the already
covered historical 107-entry declared-driver worklist. The locator contains
**922 tests / 305 direct bindings / 191 custom dispatchers**. These counts are
enumeration rather than a fresh complete integration run.

All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Successful source/build outputs and registered-test scratch
were removed after terminal exit, reclaiming **339,923,435 measured bytes**.
Compact complete results, source identities, commands and dependency metadata
remain. Stable CMake profiles and shared downloads are retained.

Complete metadata, remaining direct/custom provenance, protected execution and
G13/G14/G17/P0-P7 remain open: **14 qualified / 3 partial**. The separate
column-attribute compatibility decision remains pending. No fresh full native,
imported-module, sanitizer, MIPS or physical-device acceptance is claimed.
