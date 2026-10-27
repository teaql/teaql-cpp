# Core-model source provenance

This cohort binds 34 complete retained corpora to the locked Rust 5.0.6 runtime
at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a` remains unchanged.

| Existing comparison | Complete observations |
| --- | ---: |
| Graph planning and scope propagation | 336 |
| Scoped comments and trace tokens | 242 |
| Request-owned trace and nested batches | 932 |
| Data-service model constructors | 40 |
| FieldLayout constructors and lookups | 44 |
| LoadedSnapshot geometry and updates | 427 |
| Dynamic-field providers with SQLite | 72 |
| Dynamic metadata and checked row merge | 176 |
| Original snapshots and shared ledger views | 56 |
| School Checker helper diagnostics | 274 |
| Retained School relation helpers | 128 |
| Value presentation | 785 |
| Dynamic search | 509 |
| Value conversion | 722 |
| Value formatting | 848 |
| Presentation API | 675 |
| Core graph access | 240 |
| Metadata descriptor boundaries | 80 |
| Naming and trace primitives | 162 |
| Readonly schema hash and diagnostics | 69 |
| SmartList API | 1,291 |
| Graph shape and presentation defaults | 200 |
| CompactRow API | 401 |
| LIKE expression construction | 32 |
| Expression API boundaries | 162 |
| Safe-expression empty semantics | 132 |
| Mutation commands and value types | 240 |
| Query helper validation | 318 |
| Query purpose and trace boundary | 48 |
| Readonly query schema builder | 480 |
| Complete SelectQuery structural state | 124 |
| Row extension and panic boundaries | 300 |
| Dynamic-field error diagnostics | 48 |
| Dynamic search Clone/default semantics | 32 |
| Total | 10,625 |

The [registry](../tests/parity/core_model_driver_contracts_5_0_6.json)
and [dedicated project](../tests/parity/rust_core_model_driver_source_5_0_6/Cargo.toml)
compile **27 unchanged Rust observer bodies**. The original comprehensive
FieldLayout program also contains generated model calls. Its complete body
is retained, with the unmodified archived School library and the retained
business-ID model declared as dependencies. The latter's source/model hashes
are checked against its original generator provenance. Compiling this model
does not add a new claim about its unexecuted branches.

The first build exposed the missing model dependencies. The
[failed execution](parity/core_model_driver_initial_compile_failure_5_0_6.json.gz)
and [initial preparation](parity/core_model_driver_initial_preparation_5_0_6.json.gz)
are retained separately, including the exact observer manifest, registry,
input selector, lock and CMake contents before correction. The failed tree
was removed after Cargo terminated. Its incomplete receipt is not acceptance.
The corrected build includes the original model dependencies and also consumes
the retained Checker, dynamic-provider and generated-relation matrices.

The [input selector](../scripts/core_model_driver_inputs_5_0_6.py) checks
each immutable corpus file hash, then selects every input in source order.
Expected values are neither selected as input nor rewritten. Fresh independent
Rust binaries produce all observations; complete source and C++ output must
each equal the retained corpus with strict JSON types and exact record counts.

The replay copies all **610 runtime archive files** and **93 source/input/
project/model files** to an owned temporary tree. Cargo metadata must contain
exactly the project, six archived TeaQL packages, archived School package and
declared business-ID package. Every registry package name, version, source
and checksum must belong to the reference Cargo.lock. Compilation uses
`--offline --locked --bins`, debug assertions and no incremental compilation.
No Rust/C++ observer body, generated artifact or corpus is edited. The shared
source verifier remains unchanged.

The [fresh execution](parity/core_model_driver_execution_5_0_6.json.gz) matches
all **10,625 source/corpus/native observations**.
[Binding validation](parity/core_model_driver_bindings_5_0_6.json.gz) and
[nineteen negative controls](parity/core_model_driver_controls_5_0_6.json.gz)
pass. Missing/substituted archive/model sources, targets, packages, locks and
payloads; failed/unlocked/partial builds; missing/extra output; and
Boolean/number confusion are rejected.

The first negative-control run stopped because its setup assumed the first
matrix contained a Boolean value. Both graph-plan and scoped-comment outputs
are Boolean-free; the next request-model matrix contains real Boolean fields.
The [failed control attempt](parity/core_model_driver_initial_control_failure_5_0_6.json.gz)
is retained, with the [exact control-driver correction](parity/core_model_driver_control_fix_5_0_6.json.gz).
The control now searches the complete matrices for an actual Boolean to replace
with its numeric value; a corpus set without any Boolean fails explicitly.
This preserves the mandatory type-confusion rejection. The source/corpus/native
outputs remain unchanged. The previous numeric/time gate also exercises the
earlier path with a Boolean in its first matrix.

The [registered gate](parity/core_model_driver_gates_5_0_6.json.gz) passes
**36/36 affected entries** and **2/2 previous numeric/time verification
entries**, including its nineteen controls. Exact JUnit names, fixture
requirements, stable inputs and no skips are recorded. Source-mutation checks
remain serial.

These observations retain their original scopes:

- Query and mutation matrices prove public structural, validation and value-type
  behavior. They do not establish SQL persistence, authorization or whole Save.
- FieldLayout and LoadedSnapshot cases cover indexed/named geometry, immutable
  changes and readonly metadata. Durable field values, generated hydration and
  complete cache ownership retain their separate requirements.
- Graph-planning inputs disable Timestamp descriptors and values. The source's
  current-time rendering branch is not used by this matrix.
- Checker cases use the archived School helper methods and exact diagnostics
  across the retained naming profiles. Relation helpers retain explicit loaded,
  NULL and empty semantics; they are not a new generated graph-Save gate.
- Dynamic-provider cases perform actual in-memory SQLite storage through the
  registered executor. Private batch conversion, audit and graph orchestration
  remain separate.
- Native64 Unix Debug observations, platform-width hash/metadata limits and
  panic/formatter behavior retain their declared boundaries.

The [source partition](parity/core_model_driver_partition_5_0_6.json.gz)
now covers **247 disjoint corpora / 260,487 observations** across separate source
epochs. **58 other direct registrations** remain, distinct from the already
covered historical 107-entry declared-driver worklist. The locator contains
**920 tests / 305 direct bindings / 190 custom dispatchers**. Enumeration does
not establish complete current execution.

All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Successful source/build outputs and registered-test scratch
were removed after terminal exit, reclaiming **397,396,555 measured bytes**.
Complete results, commands, source identities and dependency metadata remain;
stable CMake profiles and shared downloads are retained.

Complete metadata, remaining direct/custom provenance, protected execution and
G13/G14/G17/P0-P7 exits remain open: **14 qualified / 3 partial**. The separate
column-attribute compatibility decision remains pending. No fresh full native,
imported-module, sanitizer, MIPS or physical-device acceptance is claimed.
