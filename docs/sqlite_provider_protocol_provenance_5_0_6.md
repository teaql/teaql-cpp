# SQLite provider and diagnostic source provenance

This cohort binds eleven retained comparisons against runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a` remains unchanged.

| Existing comparison | Complete observations |
| --- | ---: |
| Dynamic-field SQLite storage | 123 |
| Storage/schema/selection boundaries | 34 |
| Provider business IDs | 198 |
| Provider internal IDs | 261 |
| Synchronous transactions | 183 |
| Provider transport and streaming | 798 |
| SQLite value bindings | 1,084 |
| Provider error models | 563 |
| Native diagnostic formatting | 5,810 |
| Simple SQLite errors | 62 |
| FromSql errors | 36 |
| Total | 9,152 |

The [registry](../tests/parity/sqlite_provider_protocol_contracts_5_0_6.json)
and [dedicated project](../tests/parity/rust_sqlite_provider_protocol_source_5_0_6/Cargo.toml)
declare seven unchanged original Rust targets. The storage and boundary inputs
share the original storage driver. Business-ID, internal-ID, synchronous
transaction and transport matrices share their original provider driver.
Each error/binding observer retains its own complete body.

All input lists are selected in source order from their immutable corpus
declarations by the [hash-checked input selector](../scripts/sqlite_provider_protocol_inputs_5_0_6.py).
Historical expected values are not selected as inputs or rewritten. Fresh
untouched Rust executables independently produce current results. The complete
source and native outputs must each equal the retained expected values with
strict record counts and JSON types.

The source replay preserves all 610 runtime archive files and 27 copied
source/input/project files. Actual Cargo metadata must contain exactly the owned
project and six archived TeaQL packages. Every registry name/version/source/
checksum must belong to the reference lock. Compilation uses
`--offline --locked --bins`, debug assertions and an owned temporary target tree.
Original source projects, locks, observers and corpora remain unchanged.
Neither the shared verifier nor its existing control driver needs modification.

Storage, ID, transaction and transport cases perform actual in-memory SQLite
operations. Error and native diagnostic matrices establish exact model/formatting
behavior rather than additional database-service integrations. Their native64,
Unix diagnostic and checked-arithmetic scopes remain explicit. The separate
checked decoding wrapper is not replaced or requalified by these eleven matrices.
No result normalization, case reduction or claim of new MIPS32/Release behavior
is introduced.

The [fresh source execution](parity/sqlite_provider_protocol_execution_5_0_6.json.gz)
matches all **9,152** source/corpus/native results. The 27 copied inputs and
610 archive files remain unchanged after execution, as do all retained corpus
contents and hashes. The
[binding receipt](parity/sqlite_provider_protocol_bindings_5_0_6.json.gz) and
[nineteen negative controls](parity/sqlite_provider_protocol_controls_5_0_6.json.gz)
pass. Missing/substituted sources, targets, packages, locks and payloads;
failed/unlocked/partial builds; missing/extra output; and Boolean/number confusion
are rejected.

The [registered gate](parity/sqlite_provider_protocol_gates_5_0_6.json.gz) passes
**13/13** affected CTest entries and **2/2** previous SQL-tool checks. Exact JUnit
identities, complete fixture closure, unchanged execution inputs and no skips
are retained. Source mutation checks remain serial. Earlier original unit suites
and source acceptance reviews retain their separate execution epochs.

The [current partition](parity/sqlite_provider_protocol_partition_5_0_6.json.gz)
covers **198 disjoint corpora / 208,480 observations** across separate source
epochs. **107 other direct registrations** remain; this is a different category
from the already covered historical 107-entry declared-driver worklist. The
native locator has **916 tests / 305 direct bindings / 188 custom dispatchers**,
which is enumeration rather than complete current integration.

All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. The owned source/build tree was removed after terminal exit,
reclaiming **252,934,032 measured bytes**; registered-test scratch is also removed.
Compact complete results, commands, source identities and dependency metadata
remain. Stable CMake profiles and shared downloads are retained.

These observations remain distinct from application-owned authorization,
generated graph Save, protected-sink publication and complete current integration.
Complete metadata, other direct/custom provenance and G13/G14/G17/P0-P7 retain
their remaining requirements: **14 qualified / 3 partial**. The column-attribute
compatibility decision remains pending. No fresh full native, imported-module,
sanitizer, MIPS or physical-device acceptance is claimed.
