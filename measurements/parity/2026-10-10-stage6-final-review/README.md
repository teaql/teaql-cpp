# Stage 6 final acceptance evidence

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
All six in-scope stages are accepted on 2026-10-10 within the documented
C++ ownership, owner-thread and supported-protocol adaptations.

| Profile | Build tasks | Complete CTest | Seconds | Live original examples |
| --- | ---: | ---: | ---: | ---: |
| Native Debug | 44 namespace consumer refresh | 388/388 | 187.25 | 2/2 |
| ASan/UBSan Debug | 411 final refresh | 388/388 | 171.81 | 2/2 |
| OpenWrt musl MIPS MinSizeRel, SQLite3.53.2/QEMU74Kf | 351 final refresh | 387/387 | 173.24 | 2/2 |

All compiler/linker and dependent test chains terminate with exit zero before
outputs are inspected or used. Each profile has 427 nonempty compiled dependency
records, independent compiled kernel layers and no-work pre/post-test builds.
The source snapshot lists 1,266 unchanged production/example/test files. Counts
and path registration are supporting evidence, not complete semantic proof.

`completion-requirements.json` reconciles all 22 requirements. The module ledger
has 55 accepted groups, zero partial, two external exclusions and one unpublished
reference helper. Complete implementing body/default/trait/protocol/ownership
reviews are referenced by `leaf-review-scope.json` and the accepted module map.

`runtime-root-test-review.json` preserves each of the 75 original root contracts
and their direct C++ evidence. Both root production regions, generated wildcard
and all published module bindings are reviewed. Four missing public namespaces
have concrete before/after compilation and a qualified type/function/dispatch
contract. Unchanged Rust passes 224 runtime tests, including all 75 roots; the
single ignored subprocess fixture is invoked by its parent privacy test.
All 451 source-test dispositions are reviewed: 441 in scope, five non-SQLite
backend cases and five external cases, with complete registration verification.

Examples execute actual entity/query/graph runtime and use handwritten models.
Each profile matches the live original business rows and recorded original SQL
provenance; runtime_examples_context_contracts additionally checks actual C++
SQL audit/purpose provenance. The generator remains unchanged. Production
scope stays C++20 standard facilities and SQLite, with independent interfaces.

Final sanitizer environment: `ASAN_OPTIONS=detect_leaks=0`,
`UBSAN_OPTIONS=halt_on_error=1`. Leak scanning is disabled and is not verified by
this full suite. Checked references enforce their resolver contract; arbitrary
escaped native pointers remain borrows and cannot be used after Context close.

MIPS uses OpenWrt GCC13.3 musl, the 74K soft-float profile and QEMU74Kf. One
native64-only raw FromSql Debug test is excluded by an existing CMake platform
gate; structured errors and provider contracts remain registered on MIPS.
`registration-difference.json` records the reason. The earlier SQLite3.40.1
profile is historical; the final archive and profile fingerprints are retained.
Physical WNDR4300 execution is explicitly deferred by the user. QEMU results
make no hardware claim. GPU adapters and external/non-SQLite implementations
remain outside migration scope.

Reproduce with the existing profile directories (or their recorded CMake/toolchain
configuration):

```sh
cmake --build build-root-integration -j4 && ctest --test-dir build-root-integration -j4 --output-on-failure
cmake --build build-sanitize -j4 && ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-sanitize -j4 --output-on-failure
cmake --build build-wndr4300-memory -j4 && ctest --test-dir build-wndr4300-memory -j4 --output-on-failure
```

Raw logs and input fixtures are evidence data. Plans, contracts, status and
explanatory measurements remain English.
