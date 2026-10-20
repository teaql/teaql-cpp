# P2 inheritance acceptance: Rust 5.0.6

Status: G04/P2 qualified with the documented language/ownership adaptations.
Default inheritance generation and all required scoped gates have passed. This record
addresses G04/P2; complete P0-P7 and the other capabilities remain separate.

The runtime and producer are the exact commits in upstream.lock.json and the
independent cpp_support workspace's cpp_generator.lock.json. The original
producer checkout and retained Rust generated sources are unchanged.

## Requirements and evidence

| P2 requirement | Authoritative evidence |
| --- | --- |
| Distinct parent/child model/table identities and flattened fields | 48 generated model/layout observations; 36 parent metadata API observations; branching, a fieldless intermediate, compatible redeclaration and third-level descendants |
| Type matching/grouping, child queries and enhancements | 1,944 query constructions and 144 SQLite outcomes; source missing-discriminator-column errors are preserved |
| Source/result type separation | 702 list/one/stream outcomes, plus the typed-return application contracts |
| Audited inherited creation/update/delete and graph save | 1,350 save outcomes and 2,313 forward/reverse/nested graph observations, including real commits, conflicts, rollback and database readback |
| Inherited optional/boolean/clock checks and evidence | 375 variant observations, including 135 audited saves |
| Shared inherited Business-ID type and governed behavior | Five adapted source helper models, 60 actual SQLite creation outcomes and 20 update/retry outcomes; source binding defects and adaptation are explicit |
| Streaming and cleanup | 432 outcomes covering no polling, early cancellation, chunk decode errors, exhaustion and executor reuse; startup/cleanup timing adaptation is retained |
| JSON and inherited view/lifetime integration | 428 source/native inherited JSON observations, existing inherited graph lifetime controls and five new shared-Business-ID ownership controls |
| Normal generation, accurate Assist and reproduction | Four inherited workspaces generate without qualification flags; 20 workspaces/240 files reproduce; 41 unrelated complete C++ contract SDKs regenerate with identical bytes |

The nine existing inheritance observer drivers execute unchanged in the isolated
rust_inheritance_closure_5_0_6 workspace. Its registry versions and checksums
match the original runtime Cargo.lock. All **7,736** recorded/source/native
observations agree without normalization. These comparisons preserve each
observer's documented projections, including explicit trigger/conflict
classifications and operational-clock predicates. The Business-ID adapter has
its own **80** exact source/native and source/module observations.

Native inheritance regression passes **18/18**; module regression passes
**10/10**. The new five-model ownership test previously passed native/module and
ASan/UBSan **1/1**, with leak scanning disabled. Default PMR is disabled in the
ownership controls; actual UserContext deletion invalidates E borrows, while
explicit Global copies retain their own nested text. No memory/runtime algorithm
changed when the generation guard was removed.

Selected Java regression passes **40/40** across CppGenerationTest,
CppFieldLayoutMetadataTest and CppJsonInputMetadataTest. Tests cover normal
inheritance, shared Business-ID declaration, inherited masks, retained scalar and
checker mappings, legacy-entry equivalence and the still-guarded constants path.
Canonical module verification covers **521** bodies, **1,479** exports and seven
acyclic package dependencies.

## Producer binding defect disposition

The untouched inherited Business-ID Rust artifact still fails with twenty
undefined shared-type/slot references. Adding only imports/slot carriers to a
derived copy exposes four private tuple-field errors; that copy also makes the
tuple field crate-private. Original method, Checker, runtime and persistence
algorithms are retained. The paired execution qualifies this explicit language
binding adaptation, not untouched Rust artifact compilation. See
[the binding experiment](inherited_business_binding_bridge_5_0_6.md).

C++ normal generation emits one ancestor identifier type and binds the field
adapter to the actual descendant. It does not reproduce the Rust producer's
invalid name/visibility bindings. This difference is retained in the acceptance
contract and the original failure record rather than hidden by editing upstream.

## Boundaries

SQLite source queries can fail when the model has no discriminator column.
Streaming remains scalar-only for relation/aggregate enhancement. C++ Context
deletion and poll/error delivery retain their documented ownership/protocol
adaptations. Parent API validation does not claim full malformed KSML diagnostic
parity. Generated constants, broader aggregate/facet/readonly configurations and
full assembly/Assist qualification retain their respective capability entries.
No external provider, GPU adapter or physical-router acceptance is implied.

The old --qualify-inheritance CLI flag and Java constructor argument remain
compatible development entry points; they no longer gate inheritance or alter
output. Constants now also use normal generation; see the separate
[P3 acceptance](constants_acceptance_5_0_6.md).

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
python3 scripts/verify_inheritance_closure_5_0_6.py
python3 scripts/verify_inherited_business_binding_bridge_5_0_6.py \
    --cpp build_native_current/teaql_inherited_business_governed_probe \
    --updates-cpp build_native_current/teaql_inherited_business_update_probe
python3 scripts/verify_generator_parity.py
```

These commands require successfully built native observer targets. See the
[inheritance matrix](inheritance_qualification_matrix_5_0_6.md),
[reference-locked replay](parity/inheritance_closure_source_5_0_6.json) and
[contract SDK reproduction](parity/p2_contract_sdk_reproduction_5_0_6.json).
The [terminal acceptance record](parity/inheritance_acceptance_5_0_6.json)
records the gate results and retained original-artifact boundary.
