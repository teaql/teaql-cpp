# Numeric, time and auxiliary-value source provenance

This cohort binds fifteen retained comparisons against Rust 5.0.6 runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a` remains unchanged.

| Existing comparison | Complete observations |
| --- | ---: |
| Unified log-buffer clone sharing | 1 |
| SystemTime arithmetic | 1,232 |
| SystemTime Result/Error diagnostics | 7,056 |
| Chrono-to-Value conversion | 64 |
| SystemTime range and metadata | 254 |
| Decimal operations | 963 |
| Extended Decimal operations | 8,056 |
| Inline Decimal formatting | 484 |
| Auxiliary core/dependency diagnostics | 94 |
| Decimal scale | 1,148 |
| Decimal parsing | 8,576 |
| Decimal numeric conversion | 6,058 |
| Decimal foundation operations | 4,280 |
| Decimal wire format | 3,008 |
| Timestamp public API | 108 |
| Total | 41,382 |

The [registry](../tests/parity/numeric_time_driver_contracts_5_0_6.json)
and [dedicated project](../tests/parity/rust_numeric_time_driver_source_5_0_6/Cargo.toml)
compile nine unchanged Rust observer bodies. Seven Decimal matrices share the
original Decimal driver; inline formatting retains the TeaQL Decimal driver.
The other seven targets observe buffer sharing, standard time arithmetic and
errors, execution metadata, Chrono values, auxiliary diagnostics and Timestamp.

The [input selector](../scripts/numeric_time_driver_inputs_5_0_6.py) checks
each complete corpus file hash and takes every input in source order. It does
not select expected results, rewrite outputs or reconstruct an undeclared
historical input generator. Each fresh Rust executable independently produces
all results. Both source and C++ output must equal the retained expected values
with strict JSON types and exact record counts.

The replay copies the complete **610-file** runtime archive and **30**
source/input/project files into an owned temporary directory. Cargo metadata
must contain exactly this project and the six archived TeaQL packages.
Registry package names, versions, sources and checksums must belong to the
reference Cargo.lock. The build uses `--offline --locked --bins`, debug assertions
and no incremental compilation. Original source projects, observer bodies,
corpora and shared verification tools remain unchanged.

The first build failed because the dedicated manifest omitted the Decimal
observer's direct `num-traits` dependency. The
[failed execution](parity/numeric_time_driver_initial_compile_failure_5_0_6.json.gz)
and [initial preparation](parity/numeric_time_driver_initial_preparation_5_0_6.json.gz)
are retained separately. The manifest now declares the reference-locked
`num-traits 0.2.19`; this is a Rust verification dependency, not an added C++
runtime dependency. The failed temporary build tree was removed after Cargo
terminated. Its incomplete receipt is not semantic acceptance.

The [fresh execution](parity/numeric_time_driver_execution_5_0_6.json.gz)
matches all **41,382** source/corpus/native results.
[Binding validation](parity/numeric_time_driver_bindings_5_0_6.json.gz)
and [nineteen negative controls](parity/numeric_time_driver_controls_5_0_6.json.gz)
pass. Substituted or missing source, targets, local packages, dependency locks
and input payloads; failed/unlocked/partial builds; missing/extra output; and
Boolean/number confusion are rejected.

The [registered gate](parity/numeric_time_driver_gates_5_0_6.json.gz) passes
**17/17** affected entries and **2/2** previous SQLite-provider verification
entries, including its retained nineteen controls. Exact JUnit identities,
fixture requirements, no skips and stable execution inputs are recorded.

These are native64 Unix Debug observations. SystemTime platform ranges,
formatter behavior and Decimal panic boundaries retain their recorded scope.
The single buffer case proves shared clone storage only; compiling the complete
log-model observer does not establish complete logging or privacy acceptance.
No new Release, MIPS32, application-policy or generated graph-Save claim follows.

The [source partition](parity/numeric_time_driver_partition_5_0_6.json.gz)
now covers **213 disjoint corpora / 249,862 observations** across separate epochs.
**92 other direct registrations** remain, distinct from the already covered
historical 107-entry declared-driver worklist. The locator has **918 tests /
305 direct bindings / 189 custom dispatchers**; those counts are enumeration,
not a fresh full-suite run.

All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. The successful source/build tree and registered-test scratch
were removed after terminal exit, reclaiming **242,714,366 measured bytes**.
Complete results, source identities, dependency metadata and commands remain;
stable CMake profiles and shared downloads are retained.

Complete metadata, remaining direct/custom provenance, protected execution and
G13/G14/G17/P0-P7 exits remain open: **14 qualified / 3 partial**. The separate
column-attribute compatibility decision remains pending. No fresh full native,
imported-module, sanitizer, MIPS or physical-device acceptance is claimed.
