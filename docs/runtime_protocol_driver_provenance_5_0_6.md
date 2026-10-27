# Private runtime protocol source provenance

This cohort binds seven complete retained corpora to locked Rust 5.0.6 runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a` remains unchanged.

| Existing comparison | Primary observations |
| --- | ---: |
| After-commit audit queue and enrichment | 99 |
| Relation key snapshots | 64 |
| Private shared service types | 96 |
| Physical statement trace identity | 1,638 |
| Base query execution protocol | 4,096 |
| Mutation protocol with Debug assertions | 8,832 |
| Mutation protocol with wrapping counters | 8,832 |
| Total | 23,657 |

The [registry](../tests/parity/runtime_protocol_driver_contracts_5_0_6.json)
declares six unchanged observer modules and two actual Rust test profiles.
The [source engine](../scripts/runtime_protocol_driver_provenance_5_0_6.py)
uses the existing byte-checked runtime view. All **610 archive files** and
**24 copied source/input files** are retained unchanged in the owned tree.
All **67 original runtime source/test/vector files** retain their original
bytes; observers are appended only to `commit_audit.rs` and the data-service
`base.rs`, `helpers.rs` and `types.rs` files.

The compiled source hashes must equal each complete original file plus its
declared append text. Marker substitutions supply only owned input/fixture
paths. Every observer byte, original prefix, generated manifest and omitted
dependency declaration is verified. Only the two disabled external-exporter
development dependencies are omitted, as required by the existing runtime
view contract. All original source and test bodies remain intact.

Actual Cargo metadata must contain exactly the owned runtime view and archived
core, SQL, data-service and macro packages. Registry package names, versions,
sources and checksums must belong to the reference lock. The generated runtime
manifest is independently reconstructed from original Cargo metadata. The
compiled lock must account for every resolved registry package.

Both Rust **Debug and Release test executables** are built with
`cargo test --offline --locked --lib --no-run --message-format=json`, with
`--release` only for Release. Actual compiler artifacts, executable hashes,
test listings and `debug_assertions` flags remain separate. The six Debug
observers and the Release mutation observer execute their selected tests
with one test thread.

The C++ Release-named entry qualifies the **header-defined wrapping counter
under `NDEBUG`**. Its actual compile command is checked against the ordinary
probe's command. Both probes link the retained native runtime configuration;
this is not acceptance of a complete C++ Release runtime.

Every corpus input is selected in retained order. The query protocol additionally
replays **1,024 existing query controls** before its 4,096 primary inputs, and
checks their complete source/native outputs against the original corpora.
The shared service-type observer retains its input/output envelope, with exact
input echo verification before selecting the declared output field. The audit
observer retains its original file-based input/output protocol. Other observers
retain their exact JSON markers. No expected value or observation is normalized.

The [fresh execution](parity/runtime_protocol_driver_execution_5_0_6.json.gz)
matches all **23,657 primary source/corpus/native observations** and all
**1,024 legacy query observations**. The five original audit tests execute
alongside the audit observer. The original physical-statement identity test
also passes separately.

[Binding validation](parity/runtime_protocol_driver_bindings_5_0_6.json.gz) and
[26 negative controls](parity/runtime_protocol_driver_controls_5_0_6.json.gz)
pass. They reject missing/changed archive files, original tests, compiled
prefixes, appended observers, manifests, dependency locks, payloads and output;
unlocked/failed builds; incorrect profile assertions or missing Release;
substituted test executables/listings; incorrect C++ `NDEBUG` flags;
missing original regressions; and Boolean/number confusion.

The [registered gate](parity/runtime_protocol_driver_gates_5_0_6.json.gz)
passes **9/9 affected entries** and **2/2 previous generated-model checks**,
including their 27 controls. Exact JUnit identities, fixture requirements,
stable execution inputs and no skips are retained. Source-mutation controls
remain serial.

The [source partition](parity/runtime_protocol_driver_partition_5_0_6.json.gz)
covers **297 disjoint corpora / 317,194 primary observations** across separate
epochs. The 1,024 legacy query controls are additional replay evidence and are
not counted twice. **Eight other direct registrations** remain, distinct from
the already covered historical 107-entry declared-driver worklist.
The locator has **926 tests / 305 direct bindings / 193 custom dispatchers**;
enumeration does not establish full current integration.

All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Owned source/build outputs and registered-test scratch were
removed after terminal exit, reclaiming **342,368,010 measured bytes**.
Complete compact results, commands, source identities, compiler artifact metadata and
dependency metadata remain. Stable CMake profiles and shared downloads remain.

Complete metadata, remaining direct/custom provenance, protected execution and
G13/G14/G17/P0-P7 remain open: **14 qualified / 3 partial**. Context event bridges,
loaded-graph protocols and the remaining direct cases retain their separate
requirements. No fresh full native, imported-module, sanitizer, MIPS or
physical-device acceptance is claimed.
