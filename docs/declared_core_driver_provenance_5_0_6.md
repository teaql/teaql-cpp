# Declared core-driver source provenance

Status: 14 embedded-driver corpora now have fresh source/native bindings in
addition to the preceding 27 baseline-only corpora. The cumulative bounded scope
is **41 corpora / 64,576 observations**. SQ02/GP13 and P0-P7 remain open; verdicts
stay **14 qualified / 3 partial**.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Resolve the actual compilation project

The [identity inventory](parity/embedded_driver_identity_inventory_5_0_6.json.gz)
finds 107 direct corpora with matching driver metadata. A matching file does not
identify its manifest, dependency versions, observation adapters or actual output.
The [target inventory](parity/embedded_driver_target_inventory_5_0_6.json.gz)
resolves 50 entries to explicit Cargo targets; implicit/prepared targets remain
unresolved. The [workspace record](parity/embedded_dependency_resolution_5_0_6.json.gz)
preserves two early locks with 46 and 44 registry differences from the reference.
The historical private workspace has no registry mismatch but still needs its
source-view/access-bridge qualification.

Many drivers later acquired separate exact-lock projects. The
[candidate selection](parity/embedded_core_driver_candidates_5_0_6.json.gz)
includes every explicit, lock-qualified core/data-service target in the
107-entry cohort whose manifest depends on neither runtime nor SQL. These 14
entries form a complete finite group. Other entries remain open.

## Complete source and native observations

The [registry](../tests/parity/declared_core_driver_contracts_5_0_6.json) binds
each corpus to its exact driver, Cargo target/project and actual CTest name.

| Source responsibility | Corpora | Observations |
| --- | ---: | ---: |
| Value ownership/API, requests, types and model diagnostics | 6 | 1,511 |
| Query API and diagnostics | 2 | 1,209 |
| Mutation API and diagnostics | 2 | 1,808 |
| Metadata primitives | 1 | 344 |
| Data-service models and executor defaults | 2 | 336 |
| Safe expression API | 1 | 448 |
| Total | 14 | 5,656 |

The [fresh execution](parity/declared_core_driver_execution_5_0_6.json.gz) builds
six existing source projects offline and locked in one owned target. It records
compiler identity, manifests, locks, resolved targets, complete original/native
stdout, ordered payload hashes and binary hashes. Every original output matches
its expected value and the current C++ result. Thirteen historical corpus hashes
and all their ordered inputs are checked. No corpus data or metadata is changed.

All 466 reference Rust/manifests/lock files are hashed. The clean reference is
verified before and after execution; recorded inputs remain unchanged. Reduced
debug information and disabled incremental compilation preserve debug assertions
and required checks.

The [validator](../scripts/verify_declared_core_drivers_5_0_6.py) checks the retained
record without running Rust: exact dependencies and local package locations,
compiled targets, source/historical inputs, complete cardinality, strict JSON
types and all source/native observations. Existing comparisons execute current
C++ binaries; archived binary hashes do not freeze future runtime changes.

## Controls and permanent native gates

[Twenty controls](parity/declared_core_driver_controls_5_0_6.json.gz) reject 18
effective source/dependency/raw-output corruptions and both actual early locks.
They cover missing/changed sources, unowned targets, unlocked/failed compilation,
manifest/lock changes, substituted targets/dependencies, missing matrices,
changed payloads, missing/duplicate rows, missing members and boolean/integer
substitution. All 14 untouched bindings pass first.

Permanent checks `parity_declared_core_driver_contracts` and
`parity_declared_core_driver_controls` preserve this contract. All 14 comparisons
require the source fixture. The [native gate](parity/declared_core_driver_gates_5_0_6.json.gz)
passes **16/16**, with exact JUnit names and no failures/errors/skips. Selecting
only request API automatically adds and runs its source fixture first (**2/2**).
The [dispatcher](parity/declared_core_driver_dispatcher_5_0_6.json.gz) executes the
actual source invariant without inventing C++ output replacements.

The [initial registration failure](parity/declared_core_driver_registration_failure_5_0_6.json.gz)
preserves an incorrectly inferred executor test name. The final registration uses
the actual `executor_defaults_scope_rust_parity` name. The dependent gate did not
run after failed configuration; source compilation was not repeated for this fix.

## Remaining scope and cleanup

The [locator](parity/declared_core_driver_inventory_5_0_6.json.gz) records 889 native
tests, 305 direct active-baseline bindings and 175 custom registrations requiring
review. This leaves **93** declared-driver entries and **264** other direct entries
to reconcile. These overlapping worklists add no exclusions. Complete custom
CLI/dependency/observer/normalization, G14 metadata and current whole integration
retain independent exits.

The [preceding snapshot](parity/corpus_source_before_declared_core_5_0_6.json.gz)
preserves registration, dispatcher and plan documents. Historical records and
obsolete locks remain intact. Production C++, generator/SDKs, dependencies and
module exports retain their bodies. The fresh run removed **119,817,666 bytes**
of owned outputs after process completion. The
[cleanup record](parity/declared_core_driver_cleanup_5_0_6.json.gz) also tracks gate
and inventory directories. Stable presets and full-output evidence remain.
Routine source validation invokes neither Rust builds nor Java/SAN/MIPS/device
execution.

The [milestone](parity/declared_core_driver_milestone_5_0_6.json.gz) accepts only
this finite group, preserving all whole-capability and P0-P7 requirements.
