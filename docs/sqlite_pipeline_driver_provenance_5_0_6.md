# SQL and SQLite pipeline source provenance

This cohort binds eighteen retained comparisons against runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a` remains unchanged.

| Existing comparison | Complete observations |
| --- | ---: |
| Query-log discovery without query I/O | 79 |
| SQLite chunk capacity | 858 |
| Lease-before-binding order | 15 |
| Lease/binding and poisoned-lock precedence | 72 |
| Asynchronous scheduling | 183 |
| Asynchronous transaction leases | 144 |
| Explicit-intent SQL execution | 2,216 |
| Dialect schema helpers | 262 |
| IN-list construction | 216 |
| Grouped/partitioned SQL | 146 |
| Projection logging scope | 8 |
| Schema SQL construction | 1,305 |
| Mutation SQL construction | 2,505 |
| Dialect mutation hooks | 48 |
| Dialect query construction | 2,206 |
| Binding and privacy provenance | 1,169 |
| SQL value/renderer boundaries | 552 |
| SQL model boundaries | 80 |
| Total | 12,064 |

The [registry](../tests/parity/sqlite_pipeline_driver_contracts_5_0_6.json)
and [dedicated project](../tests/parity/rust_sqlite_pipeline_source_5_0_6/Cargo.toml)
name fifteen unchanged Rust targets. Schema/helper/IN matrices share their
original schema driver; mutation/hook matrices share their mutation driver.
All selected corpus paths, registered native commands and input counts are
explicit. The bootstrap classifier and other SQLite storage/provider families
remain separate responsibilities and are not counted here.

Each current corpus is the immutable declaration of its complete input list.
The [input selector](../scripts/sqlite_pipeline_driver_inputs_5_0_6.py) verifies
the entire declared file hash and selects every `input` in order. It does not
return historical expected values, regenerate corpora or invent missing original
input-generation evidence. Fresh untouched Rust executables independently
produce all current results. The complete raw Rust and native outputs must each
match the retained expected values with strict JSON types and record counts.

The source replay copies 42 source/input/project files and the complete 610-file
runtime archive. Shared value/SQL/query-key helpers remain unchanged. The binding
observer imports the complete original binding module; no production function
is extracted or rewritten. Actual Cargo metadata must contain exactly the owned
project and six archived TeaQL packages. Every registry name/version/source/
checksum must belong to the reference lock. Compilation uses
`--offline --locked --bins`, debug assertions and an owned temporary target tree.
Original collector projects, locks and source fixtures remain untouched.

Scope follows the retained observers. The no-I/O logging driver panics if its
query method is invoked. Compiler/schema/dialect matrices establish construction,
callbacks, values and diagnostics, not live PostgreSQL/MySQL integration. Valid
external discriminator names remain authentic metadata. SQL executor scenarios
exercise their original transport SPI fixtures; they do not independently prove
generated graph Save, customer authorization or committed audit delivery.
The grouped/partitioned driver and SQLite transport/stream cases perform actual
in-memory SQLite execution.

Existing C++ capacity bounds and Context ownership/scheduling adaptations remain
explicit. Poisoned-lock controls retain their original checked Debug scope;
native64 maximum-width cases do not establish new MIPS32 or Release behavior.
No result normalization or input reduction is introduced. Other provider/storage,
generated-model, protected-publication and complete integration duties retain
their separate gates.

The [fresh source execution](parity/sqlite_pipeline_driver_execution_5_0_6.json.gz)
matches all **12,064** source/corpus/native results. All 42 copied input files
and 610 archive files remain unchanged after execution. The
[binding receipt](parity/sqlite_pipeline_driver_bindings_5_0_6.json.gz) and
[nineteen negative controls](parity/sqlite_pipeline_driver_controls_5_0_6.json.gz)
pass: missing or substituted sources, targets, dependencies, locks and payloads;
failed/unlocked/partial builds; missing/extra output; and Boolean/number confusion
are rejected.

The [registered native gate](parity/sqlite_pipeline_driver_gates_5_0_6.json.gz)
passes **20/20** affected CTest entries and **2/2** previous document-tool checks.
Exact JUnit identities, complete fixture closure, unchanged input hashes and
absence of skips are retained. The previous document control selection still
exercises twenty effective checks. Original unit-test suites and prior broader
source reviews retain their separate execution epochs.

The [disjoint current partition](parity/sqlite_pipeline_driver_partition_5_0_6.json.gz)
covers **187 corpora / 199,328 observations** across separate source epochs.
**118 other direct registrations** remain. The native locator has
**914 tests / 305 direct bindings / 187 custom dispatchers**; enumeration is not
complete current integration or acceptance of every custom dispatcher.

All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. The owned source/build tree was removed after terminal exit,
reclaiming **312,700,681 measured bytes**; registered-test scratch is also removed.
Compact commands, complete results, source identities and dependency metadata
remain. Stable CMake profiles and shared downloads are retained.

Complete metadata, remaining direct/custom provenance, protected execution and
G13/G14/G17/P0-P7 retain their remaining requirements: **14 qualified / 3 partial**.
The column-attribute compatibility decision remains pending. This checkpoint
does not claim a fresh full native, imported-module, sanitizer, MIPS or physical
device execution.
