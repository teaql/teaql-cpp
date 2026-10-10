# Migration acceptance gates

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

All six stages are accepted within the documented C++ ownership, owner-thread
and supported-protocol adaptations. No in-scope source group remains partial:
55 accepted, two external exclusions and one unpublished reference helper.
Complete production/default/trait/protocol/ownership reviews accompany the
source-test evidence; locators and passing counts alone are not the verdict.

The final native and ASan/UBSan suites pass 388/388; matched SQLite3.53.2
OpenWrt musl MIPS/QEMU passes 387/387. All profiles match both live original
examples, have 427 nonempty dependency records, retain independent compiled
layers and report no work before/after tests. Final sanitizer leak scanning is
disabled. The single native64-only raw FromSql Debug comparison has a recorded
platform scope; structured target error/provider contracts remain covered.

All 451 original source tests have individually reviewed dispositions: 441
covered, five non-SQLite backend cases and five external cases. Runtime-root
review includes both production regions, all four public module namespaces
and all 75 original root fixture/assertion contracts. The 22-requirement final
audit and reproducible logs are stored in
[final evidence](../measurements/parity/2026-10-10-stage6-final-review/README.md).

Physical WNDR4300 execution is explicitly deferred by the user and is not
claimed by QEMU evidence. Entity-generator changes, GPU adapters and external/
non-SQLite integrations remain outside scope. Arbitrary escaped native pointers
remain explicit borrows and must not be used after Context close. No open
in-scope migration requirement is inferred from these explicit limitations.
