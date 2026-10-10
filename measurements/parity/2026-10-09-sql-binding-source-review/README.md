# Complete SQL binding source-test review

Baseline: Rust v5.0.5 at 62940ae2b88e7acc32763853b8b4685a01c7e90a.

The complete binding_tests.rs body was compared with the handwritten C++ port
on 2026-10-09. All ten original responsibilities and their inputs are retained:
legacy undeclared/empty declarations; logical-field/credential precedence;
scalar/BETWEEN/IN sibling scope; subquery descriptor scope; search and unknown
literals; all five raw taint variants; four guarded mutations; both batch
policy vectors; exception restoration and all 64 independently owned workers.
Exact expected policy vectors, canaries and parameter-count assertions were
compared directly against original source assertions. The fixture's dialect
has the same SQLite kind, quoted identifier and anonymous placeholder.

The fresh unchanged original command completes successfully and passes 10/10:

```sh
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" cargo test \
  --manifest-path .reference/teaql-rs/Cargo.toml -p teaql-sql \
  --locked --offline binding_tests -- --nocapture
```

Each C++ compiler worker owns its own arena and joins before local storage
is destroyed. The shared dialect is stateless; it introduces no TLS/shared
Context data. The source file adds no public production API or independent
serialized model; production compiler and privacy responsibilities retain
their separate acceptance groups. Fresh C++ native/sanitizer/MIPS verification
and final integrated registration are still required for this source group.

The selected MIPS build finishes, then reports no work. All four source/ID
checks pass in 21.73 seconds, including the full permutation range and
2,150 strategy observations. Native/sanitizer integration remains pending.

The fresh sanitizer build finishes before the selected four checks run.
ASan/UBSan passes4/4 in 77.73 seconds, with leak scanning disabled; the full
20,000-sequence range is retained. Complete native 353 integration is pending.

## Source-file acceptance

Native353/353 passes in 88.45 seconds after a complete consumer build and
corrected fixture refresh; post-test and follow-up builds report no work.
All 416 compiled dependency records are nonempty. The fresh sanitizer/MIPS
source/ID selections pass 4/4 and original Rust source tests pass. Complete
body/input/assertion review covers the entire test-only source file; it has no
additional production public surface or independent serialized protocol. This
accepts this source-test group only. Production runtime/API/privacy groups,
complete Stage 6 and physical-router execution remain independent gates.
