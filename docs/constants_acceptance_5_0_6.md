# P3 constants and candidates acceptance: Rust 5.0.6

Status: G05/P3 qualified with the documented language/ownership adaptations.
Default constant generation and all required scoped gates have passed.
This record addresses G05/P3. The remaining generator phases and capabilities
retain their independent obligations.

The runtime and producer references are the exact commits in upstream.lock.json
and the independent cpp_support workspace's cpp_generator.lock.json. Original
producer/reference checkouts, generated Rust artifacts and historical corpora
remain unchanged.

## Requirements and evidence

| P3 requirement | Evidence and acceptance boundary |
| --- | --- |
| Typed candidate predicates, model updates and E reads | 388 original/native observations include 36 audited SQLite changes; another 376 scalar SQLite outcomes cover eight complete compilable domains. Nine generated candidate SDKs and 76 instantiated methods include the independent native authentic-bool compiler control. Literal null and SQL NULL remain distinct. |
| Root/constant create, reconcile, no-op and identifiers | 99 public ensure-schema cases, 36 scalar/catalog variants and 504 identifier-model scenarios preserve exact rows, versions, ID floors, active roots and errors. Fixed zero follows source allocation and is not idempotent. Explicit, saturated, hash-fallback and missing identity cases retain their metadata. |
| Retry, failure order and governed writes | The original private retry classifier has 85 observations. Public initialization adds 768 controlled second-connection scenarios, including five-attempt exhaustion, rival repair and recovery. Another 960 module/policy/approval/audit scenarios preserve immutable plans, safe events and committed-delivery failure boundaries. |
| Scalar and configured fields | Numeric/catalog and 1,728 temporal scenarios cover field initialization, unchanged drifted clocks and independent process/business clocks. Six accepted configured-scalar domains add 576 bootstrap/Q/E scenarios. Temporal Checker/clock replacement adds 480 cases with bootstrap and ordinary-save callbacks, fixes and early/late rejection. |
| Root/constant dependency topology | Ten complete parser/producer/compiler fixtures and 480 SQLite/Q/E scenarios preserve declaration order, reference seed identity 1, required self-reference, foreign-key failures and load-state results. Another 2,880 topology/Checker/Behavior cases cover ordered, late-target and required-self-reference domains with all four module factories. |
| Invalid metadata and bindings | Ten ID fixtures, thirteen metadata fixtures, nine scalar candidate fixtures, nine configured-type fixtures and three opaque-carrier fixtures retain source success/error dispositions. Native generation rejects negative IDs, collisions, unusable constant roots and unsupported carriers before emitting an SDK. Shared malformed XML, cycles and depth-limit failures stay explicit. |
| Context ownership, registration and asynchronous cleanup | Twenty active native bootstrap-save scenarios cover five commit positions, resume, failure, cancel and actual User deletion, with joined host work and no busy statement/transaction at cleanup. Another 384 resource scenarios cover typed/named replacement, wrong-type shadowing, embedded-NUL keys, nested allocation and expired access/copy rejection. |
| Normal generation and accurate guidance | All twenty workspaces generate without qualification flags. Default DomainGenerationService and legacy constructor output agree. Assist documents declaration order, zero-ID behavior, retry, governance, committed audit errors and source binding limits only for models with constants. Strict regeneration retains every code byte across 44 contract SDKs. |

Each row composes its named source and execution checkpoints; it does not turn
an uncompiled Rust domain into a runtime oracle. The
[qualification matrix](constant_qualification_matrix_5_0_6.md) links the retained
evidence and the [terminal record](parity/constants_acceptance_5_0_6.json)
records default-generation gates and provenance.

## Source and C++ adaptation decisions

Invalid constant-only/multiple-root models have no usable ordinary Domain Root.
Negative identity names, duplicate/colliding candidates and unsupported scalar
carriers can produce invalid source output. C++ rejects those configurations
before output rather than emit an incomplete or ill-typed SDK. Malformed XML,
dependency cycles and depth limits use the shared parser contracts.

Original boolean equality methods, unescaped JSON candidate literals and optional
Timestamp/object-reference seed assignments can fail Rust compilation. Those
complete original artifacts and diagnostics remain retained. C++ has independent
well-typed bindings and compiler controls; there is no unchanged-source runtime
comparison for an artifact that cannot compile. These are explicit language
binding dispositions, not successful Rust execution or a claim of universal
model/configuration parity.

Resource replacement Drop is immediate in Rust and deferred to arena destruction
in C++. The resource comparator checks both timing contracts and equal final
destructor totals. C++ asynchronous cleanup tests use a controlled scheduling
barrier with actual SQLite writes; they do not invent an original deferred-provider
oracle. These are the existing approved Context ownership/protocol adaptations.

Source initialization retains declaration order and reference identity 1.
Late target constants can therefore fail a foreign-key check. Zero fixed IDs can
allocate another row on a later bootstrap. Temporal bootstrap does not silently
replace drifted create/update timestamps. These source behaviors remain in the
accepted contract and generated guidance.

## Reference dependency replay

Six unchanged observer files execute in a separate Cargo workspace with the
original registry versions and checksums. All 4,939 source observations match
their existing corpora, then all seven C++ comparators pass. Isolated manifest-only
projects also reproduce the 17 scalar/root and four optional-Timestamp errors
with those dependencies. See [the source closure](constant_source_closure_5_0_6.md).
This improves dependency evidence without editing old projects or observations.

## Default entry and terminal verification

Normal generation reproduces twenty workspaces/240 files. Selected Java tests
pass 42/42; native regression passes 13/13 and module regression passes 9/9.
Strict regeneration covers 44 contract SDKs with every code byte unchanged.
Thirty genuinely generated Assist files changed; the initial instruction-change
record is retained separately from the final strict replay. Canonical verification
covers 526 bodies, 1,479 exports and seven packages; pinned format/check covers
1,362 C++ files. The optional-Timestamp check retains four original E0308 errors
and independently compiles the current complete C++ binding.

## Remaining scope

The old --qualify-constants flag and constructor parameter remain compatible
development entry points; they no longer gate constants or change output.
Normal constant generation keeps all metadata validation. Applications use
UserContext ensure_schema and generated Q/E/mutation APIs over the same shared
runtime mechanics. No runtime function changed when the guard was removed.

General runtime assembly, sample-data configuration and full Assist acceptance
remain G12; broader cross-capability/model semantic acceptance remains G13.
No external database provider, GPU adapter or physical-device acceptance is
included. Targeted earlier sanitizer ownership/cleanup evidence retains its
own scope and disabled leak scanning; no new memory algorithm was introduced.

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
python3 scripts/verify_constant_closure_5_0_6.py
python3 scripts/verify_constant_failure_closure_5_0_6.py
python3 scripts/verify_constant_temporal_source_failure.py --check
python3 scripts/verify_generator_parity.py
```

Observer and compiler commands require successfully built native targets.
The terminal record names the native/module selections and Java tests; it does
not claim a fresh complete native-suite or MIPS run.
