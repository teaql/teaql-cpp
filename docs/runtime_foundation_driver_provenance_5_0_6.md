# Runtime foundation driver source provenance

Status: eight declared public-runtime corpora have fresh source/native bindings.
Together with the preceding groups, this covers **49 direct corpora / 67,261
observations**. Complete SQ02/GP13, G14/G17 and P0-P7 remain open; capability
verdicts stay **14 qualified / 3 partial**.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Exact public-runtime callers

The [registry](../tests/parity/runtime_foundation_driver_contracts_5_0_6.json)
binds the complete declared Context/Checker/Policy/Event/reference-model group
to its current Cargo targets and actual CTest names. Private access bridges,
generated-model carriers and other runtime/SQL callers retain independent work.

| Corpus | Complete observations |
| --- | ---: |
| Context defaults | 75 |
| Context model | 234 |
| Checker execution | 1,362 |
| Check model | 220 |
| Policy model | 177 |
| Mutation Policy | 276 |
| Event model | 112 |
| Reference model | 229 |
| Total | 2,685 |

The [fresh replay](parity/runtime_foundation_driver_execution_5_0_6.json.gz)
compiles five existing exact-lock source projects against the unchanged original
libraries. Source targets, manifests, locks, local dependencies, compiler identity,
ordered input payloads, complete source/native stdout and binary hashes are
retained. Every original result matches both its existing expected value and
current C++ output. Historical input relationships remain explicit; no expected
data is rewritten and no private observation shim is substituted.

The [generic replay driver](../scripts/replay_declared_drivers_5_0_6.py) preserves
the preceding producer's build/execute order while accepting an explicit registry.
Offline/locked Cargo builds use one owned target with reduced debug information
and disabled incremental compilation. Debug assertions stay active. All 466
reference Rust/manifests/lock files and replay inputs remain unchanged; the clean
reference HEAD/tag is checked before and after execution.

## Permanent source and negative checks

The [generic validator](../scripts/verify_declared_driver_contracts_5_0_6.py)
checks exact source/dependency/target identities and full raw observations without
running Rust. It checks matrix cardinality, historical input equality, payload
hashes and strict JSON value types. Current C++ execution stays with the existing
comparators; historical binary hashes do not freeze future implementation.

[Twenty controls](parity/runtime_foundation_driver_controls_5_0_6.json.gz) pass:
18 effective source/compilation/dependency/raw-output corruptions reject, as do
the two retained early workspace locks. Untouched bindings pass first. The
[control runner](../tests/parity/verify_declared_driver_controls_5_0_6.py) accepts
the registry and execution record explicitly, preserving the previous core
cohort's exact producer and verifier bytes.

Permanent checks `parity_runtime_foundation_driver_contracts` and
`parity_runtime_foundation_driver_controls` are registered for the native host.
Each of the eight comparisons requires its source fixture. The
[native gate](parity/runtime_foundation_driver_gates_5_0_6.json.gz) passes **10/10**
checks with exact JUnit membership and no failures, errors or skips. Selecting
Mutation Policy alone adds and successfully runs the source check first (**2/2**).
The [dispatcher replay](parity/runtime_foundation_driver_dispatcher_5_0_6.json.gz)
executes the actual source invariant rather than fabricating a C++ output wrapper.

## Remaining scope and preservation

The [current locator](parity/runtime_foundation_driver_inventory_5_0_6.json.gz)
records 891 native tests, 305 direct active-baseline entries and 176 custom
registrations. After these eight bindings, **256 direct entries** and **85 of the
107 declared-driver entries** retain independent provenance requirements. These
overlapping worklists do not exclude source views, protected callers, metadata,
normalization, partial-result checks or whole current integration.

The [preceding snapshot](parity/runtime_foundation_before_5_0_6.json.gz) preserves
registration, dispatcher and plan documents, including the actual native build
configuration. Production C++, generated SDKs, generator sources, dependencies,
module exports and memory/lifetime bodies are unchanged.

The replay removed **231,221,004 bytes** of its owned source-build outputs after
compiler/linker and probe completion. The
[cleanup receipt](parity/runtime_foundation_driver_cleanup_5_0_6.json.gz) also
tracks exact temporary gate/inventory directories. Stable presets and compact
full-output evidence remain. Routine source checking does not rebuild Rust or
run Java, sanitizer, MIPS or device profiles.

The [milestone](parity/runtime_foundation_driver_milestone_5_0_6.json.gz) joins
this finite group while preserving all complete capability and P0-P7 exits.
