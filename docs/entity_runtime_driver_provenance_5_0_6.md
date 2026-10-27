# Entity-runtime driver source provenance

Status: all seven targets of the existing entity-runtime source project now
have complete fresh source/corpus/native bindings, covering **4,226 observations**.
The cumulative bounded source scope is **65 direct corpora / 79,730 observations**.
G13/G14/G17 and P0-P7 remain open; capability status stays **14 qualified / 3 partial**.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Complete project and observations

The [registry](../tests/parity/entity_runtime_driver_contracts_5_0_6.json)
maps every explicit target of `rust_entity_runtime_source_5_0_6` to its actual
driver, C++ executable and registered comparison. No existing corpus or expected
value changes.

| Responsibility | Observations |
| --- | ---: |
| Private identity, graph sharing and ledger composition | 241 |
| Public constructors, derives and state model | 252 |
| Dynamic ledger state | 147 |
| JSON graph views and option identity | 2,304 |
| Consuming loaded-view transfer | 512 |
| Equality, including dynamic floating-point values | 338 |
| Complete dynamic Debug output | 432 |
| Total | 4,226 |

The [fresh execution](parity/entity_runtime_driver_execution_5_0_6.json.gz)
builds all seven actual Rust targets offline with reference-locked dependencies,
then compares every original observation to the retained corpus and current C++
output. Complete raw stdout, ordered payload hashes, source/binary identities,
Cargo metadata and original/prepared manifests and locks are retained. All
**21/21 original entity-runtime tests** also pass. Their complete expected names
are independently derived from the locked source's three test modules.

The identity corpus originally names an isolated-module driver. The current
compiled driver preserves its body after the pinned import and uses the existing
crate-root accessors. That adaptation is checked explicitly rather than claiming
the two driver files have the same hash. The dynamic ledger corpus uses its two
original source hashes instead of a driver declaration; its compiled target and
driver are recorded separately. The other five declared drivers retain exact
metadata identities. Both historical input corpora retain their bytes and order.
Old manifest/lock labels are preserved as historical provenance; the new actual
compilation project and dependencies have their own recorded evidence.

## Source view and dependency boundary

The [replay](../scripts/replay_entity_runtime_drivers_5_0_6.py) uses the existing
byte-preserved runtime view. All **67 original source/test/vector files** remain
unchanged, including the source prefix before the appended observation code.
The exact three existing observer/accessor inputs are assembled in recorded order.
No original implementation, test body or dependency algorithm is substituted.

The prepared project changes path locations and redirects only its runtime
dependency to the owned view. The view's manifest is reconstructed from actual
Cargo metadata. Its existing omission of the two disabled external-exporter
development dependencies retains the
[standing telemetry scope](external_telemetry_scope_5_0_6.md); this introduces no
new exclusion of core runtime or telemetry SPI responsibilities. All registry
versions and checksums match the reference lock. All **466** tracked reference
Rust/manifest/lock files and the clean reference checkout are checked before and
after execution.

One owned target serves the complete replay. Incremental compilation is disabled
and debug information reduced; debug assertions and required tests remain active.
Successful replay removes **299,500,509 bytes** of owned outputs after every
compiler and executable terminates. Stable presets and shared caches remain.

## Permanent verification and registration

The [validator](../scripts/verify_entity_runtime_driver_contracts_5_0_6.py)
checks the retained source view, exact appendix, metadata-derived manifest,
prepared target set, all original test names, source/dependency identities,
complete cardinality and strict JSON values. It runs without compiling Rust.
Existing comparisons still execute current C++ binaries; archived binary hashes
do not freeze future C++ implementation changes.

The [negative controls](parity/entity_runtime_driver_controls_5_0_6.json.gz)
reject **27 effective corruptions and two actual legacy locks**. These include
self-consistently rehashed appendices, missing original source/observer inputs,
unreviewed omitted dependencies, substituted compilation targets, partial unit
tests, missing or duplicated observations, omitted members and Boolean/number
confusion. Dynamic source-hash declarations have their own corruption controls.
Untouched bindings pass first.

Permanent host checks `parity_entity_runtime_driver_contracts` and
`parity_entity_runtime_driver_controls` preserve this contract. All seven existing
comparisons require the source fixture. The
[native gate](parity/entity_runtime_driver_gates_5_0_6.json.gz) passes **19/19**:
seven comparisons, ten existing runtime/lifetime contracts and two source checks.
Selecting only equality automatically adds the source fixture and passes **2/2**.
JUnit names are exact, with no failures, errors or skips.

The [dispatcher check](parity/entity_runtime_driver_dispatcher_5_0_6.json.gz)
executes the actual invariant control command. It does not fabricate a C++ probe
argument or count the invariant as a substituted-output experiment. All
**19 current source/comparison/admission/inventory checks**, pinned format/check
and reference/module paths pass in the
[guard receipt](parity/entity_runtime_driver_guards_5_0_6.json.gz).

## Remaining scope

The [current locator](parity/entity_runtime_driver_inventory_5_0_6.json.gz)
records **895 native tests / 305 direct active-baseline entries / 178 custom
classifications**. The seven new bindings are disjoint from the prior 58. They
leave **240 direct entries** and **72 of 107 declared-driver actual-project/view
responsibilities** to reconcile. Static custom classifications remain locators;
each keeps its actual CLI, dependency, observer and normalization obligations.

The frozen 893-test full native execution retains its original inputs. Current
19-test runtime and 19-test source selections are scoped continuations, not a
new full 895-test execution. Production C++, generator implementations, SDKs,
module exports and corpus values remain unchanged. Complete metadata grammar,
protected execution, SQ02 provenance and every P0-P7 exit remain required.

The [milestone](parity/entity_runtime_driver_milestone_5_0_6.json.gz) joins this
finite qualification. The [cleanup receipt](parity/entity_runtime_driver_cleanup_5_0_6.json.gz)
tracks exact task-owned replay, gate, dispatcher and inventory outputs; retained
evidence and shared/stable build outputs are preserved.
