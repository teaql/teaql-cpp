# Registry driver source provenance

Status: all seven targets of the existing Registry source project now have fresh,
complete source/corpus/native bindings, covering **27,257 observations**.
Cumulative bounded source qualification covers **72 direct corpora / 106,987
observations**. G13/G14/G17 and P0-P7 remain open; status stays
**14 qualified / 3 partial**.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Complete project and actual results

The [registry](../tests/parity/registry_driver_contracts_5_0_6.json) maps every
explicit target of `rust_registry_source_5_0_6` to its exact driver, corpus,
compiled project, native executable and registered test.

| Responsibility | Observations |
| --- | ---: |
| Runtime registry | 321 |
| Registry Debug scope | 18 |
| Entity decoder | 198 |
| Typed JSON graph decoder callbacks | 24,576 |
| Runtime graph | 464 |
| Schema runtime | 384 |
| SmartList registry behavior | 1,296 |
| Total | 27,257 |

The [fresh replay](parity/registry_driver_execution_5_0_6.json.gz) builds all seven
actual Rust targets offline with reference-locked dependencies, then compares
every original result to the retained corpus and current C++ output. Complete
raw stdout, ordered payload hashes, source/binary identities, resolved Cargo
metadata, original/prepared manifests and dependency locks are retained. No
corpus input, expected value or historical provenance label changes. Five
historical corpora retain their bytes and input order.

The JSON graph matrix qualifies typed registry callbacks, descriptor/layout
validation, callback copies, list/option edges, child views and weak graph state.
Its scope does not become complete JSON text parsing or authorization to save.

## Original source and test enumeration

The [replay driver](../scripts/replay_registry_drivers_5_0_6.py) uses the existing
byte-preserved runtime view. All **67 original source/test/vector files** retain
their bodies. Exact existing observation code is appended separately to
`entity_runtime.rs` and `registry.rs`; both appendices and their input hashes
are checked. The prepared project changes path locations and redirects only
its runtime dependency to this owned view.

The view's manifest is reconstructed from actual metadata. It preserves the
existing [external telemetry scope](external_telemetry_scope_5_0_6.md) for the
two disabled exporter development dependencies, introducing no new exclusion of
core runtime or telemetry SPI responsibilities. Every resolved registry version
and checksum matches the reference lock. All **466 tracked reference Rust,
manifest and lock files**, plus the clean checkout, are checked before and after
execution. Debug assertions remain enabled; incremental compilation is disabled
and debug information reduced in one owned target.

The actual locked `registry.rs` has **zero embedded unit tests**. Cargo enumerates
273 runtime-library tests and no `registry::` test; enumeration is not recorded
as 273 passing tests. Behavioral evidence here comes from the 27,257 actual
source/native observations and the separate native invariant programs. Controls
reject both an invented Registry unit-test count and a missing `--list` mode.

The [initial setup failure](parity/registry_driver_setup_failure_5_0_6.json.gz)
records incorrectly inferred compressed filenames for two historical corpora.
It occurs before compilation; the dependent launch cannot find the absent
registry. The corrected registry checks the actual files and their original
hashes. This is setup evidence, rather than a runtime defect or passing replay.

## Permanent verification and CTest

The [validator](../scripts/verify_registry_driver_contracts_5_0_6.py) checks the
complete source view, both exact appendices, metadata-derived manifest, prepared
target set, original-driver declarations, historical inputs, dependency locks,
test enumeration, full cardinality and strict JSON types. It runs without
building Rust. Existing comparisons execute current native binaries; recorded
binary hashes do not freeze future C++ implementations.

The [31 controls](parity/registry_driver_controls_5_0_6.json.gz) reject 29 effective
corruptions and two actual legacy locks. They cover changed original prefixes,
self-consistently rehashed appendices, missing observer inputs, unreviewed
dependency omissions, substituted or extra targets, partial or misrepresented
test enumeration, missing or duplicated rows, missing members, Boolean/number
confusion and missing/changed original-driver declarations. Untouched bindings
pass first.

Permanent host checks `parity_registry_driver_contracts` and
`parity_registry_driver_controls` preserve this contract. Each existing comparison
adds the source fixture while preserving other requirements. The
[native gate](parity/registry_driver_gates_5_0_6.json.gz) passes **13/13**: seven
comparisons, four existing Registry/lifetime programs and two source checks.
Selecting only JSON graph decoding automatically runs the source fixture and
passes **2/2**, with exact JUnit names and no failures, errors or skips. Native
and modules configuration both pass; this is not a new imported-module test run.

The [dispatcher proof](parity/registry_driver_dispatcher_5_0_6.json.gz) executes
the actual invariant-control command without fabricated C++ output substitution.
All **21 current source/comparison/admission/inventory checks**, pinned format
and its check, locked references and module paths pass in the
[guard receipt](parity/registry_driver_guards_5_0_6.json.gz).

## Remaining scope and cleanup

The [current locator](parity/registry_driver_inventory_5_0_6.json.gz) records
**897 native tests / 305 direct active-baseline entries / 179 custom
classifications**. These are locator facts rather than a full 897-test execution.
The seven new bindings are disjoint from the previous 65, leaving **233 direct
entries** and **68 of 107 declared-driver actual-project/view responsibilities**
to reconcile. Each custom classification retains its own actual CLI, source,
dependency, observer, normalization and protected-execution requirements.

The frozen 893-test full native execution retains its original inputs and parser
identities. Production C++, producer implementations, SDKs, module exports and
all corpus values remain unchanged. Complete metadata grammar, provenance,
protected execution and every G13/G14/G17/P0-P7 exit remain required.

Successful replay removes **303,132,369 bytes** of owned outputs after all
compilers and executables terminate. The
[cleanup receipt](parity/registry_driver_cleanup_5_0_6.json.gz) records exact
task-owned replay, gate, inventory, dispatcher and guard directories. Stable
presets, shared caches, databases, sources and retained evidence are preserved.
The [milestone](parity/registry_driver_milestone_5_0_6.json.gz) joins only this
finite qualification.
