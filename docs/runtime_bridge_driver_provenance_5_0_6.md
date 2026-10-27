# Runtime access-bridge source provenance

This cohort binds Context events, typed relation aggregates and loaded graph
reconstruction to locked Rust 5.0.6 runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a` remains unchanged.

| Existing comparison | Complete observations |
| --- | ---: |
| Complete Context event dispatch | 1,088 |
| Private typed relation-aggregate helper | 146 |
| Loaded graph reconstruction and shared identity | 3,568 |
| Total | 4,802 |

The [registry](../tests/parity/runtime_bridge_driver_contracts_5_0_6.json)
retains the complete original driver bodies for all three comparisons.
The [bridge source engine](../scripts/runtime_bridge_driver_provenance_5_0_6.py)
copies the complete **610-file archive** and **19 source/input files** into
an owned temporary tree. All **67 runtime source/test/vector files** retain
their complete original prefixes.

The Context access bridges are read from the existing builder's literal
`APPENDS` declaration. The typed-aggregate delegation comes from its existing
builder's literal `BRIDGE`. The
[loaded-graph access bridge](../tests/parity/runtime_loaded_graph_access_bridge_5_0_6.rs)
delegates directly to the original private reconstruction method. These
declarations append to `context.rs`, `lib.rs` and `data_service/resolved.rs`;
no original implementation or test body is replaced.

Each original driver, including its unused main and all fixture functions,
is wrapped in its own test module. A crate alias and test entry are appended.
Only the loaded-graph fixture module's relative path is redirected to the
owned copy of the same unchanged file. The generated source hashes must
equal the complete original bytes plus the exact declared append text.
Bridge constants, module wrappers and path substitutions are independently
reconstructed during validation.

Actual Cargo metadata must contain exactly the owned runtime view and the
archived core, SQL, data-service and macro packages. Registry package names,
versions, sources and checksums must belong to the reference Cargo.lock.
The runtime manifest is reconstructed from original metadata; only the two
disabled external-exporter development dependencies are omitted under the
existing source-view contract. No runtime feature or C++ dependency is added.

The actual Rust Debug test artifact, executable hash, test listing and
`debug_assertions` flag are recorded. Sensitive-plaintext diagnostic opt-in
is removed from this fixture process, preserving the original Context
driver's declared test setup. Each complete original corpus is consumed in
order, with exact source/corpus/native counts and JSON types.

The [fresh execution](parity/runtime_bridge_driver_execution_5_0_6.json.gz)
matches all **4,802 source/corpus/native observations**.
[Binding validation](parity/runtime_bridge_driver_bindings_5_0_6.json.gz) and
[23 negative controls](parity/runtime_bridge_driver_controls_5_0_6.json.gz)
pass. Missing/changed archive bodies, compiled prefixes, original tests,
bridge constants, observer modules, dependencies, test artifacts, payloads
and output are rejected. Specific checks reject a dropped fixture-path
adaptation, substituted driver wrapper and incorrect bridge constant.

The [registered gate](parity/runtime_bridge_driver_gates_5_0_6.json.gz)
passes **5/5 affected entries** and **2/2 previous protocol checks**, including
their 26 controls. Exact JUnit identities, fixture requirements, unchanged
execution inputs and no skips are retained. Mutation checks remain serial.

The Context matrix retains its original event projection and fixture sink
behavior; it does not establish a durable outbox or complete configured
authorization. The typed-aggregate observer calls the original private helper
through its unchanged delegation, with all historical and explicit-intent
controls retained. The graph matrix reconstructs loaded compact rows and
shared identities; its executor fixture rejects unexpected database access.
Full application graph Save, external providers and other integration scopes
remain separate. No new output normalization is introduced.

The [source partition](parity/runtime_bridge_driver_partition_5_0_6.json.gz)
covers **300 disjoint corpora / 321,996 primary observations** across separate
epochs. **Five other direct registrations** remain, distinct from the already
covered historical 107-entry declared-driver worklist. The locator contains
**928 tests / 305 direct bindings / 194 custom dispatchers**; enumeration is
not complete current integration.

All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Owned source/build outputs and registered-test scratch were
removed after terminal exit, reclaiming **191,755,956 measured bytes**.
Complete compact results, commands, source identities, compiler artifact
metadata and dependency records remain. Stable CMake profiles and shared
downloads are retained.

Complete metadata, remaining direct/custom provenance, protected execution and
G13/G14/G17/P0-P7 remain open: **14 qualified / 3 partial**. No fresh full native,
C++ Release runtime, imported-module, sanitizer, MIPS or physical-device
acceptance is claimed.
