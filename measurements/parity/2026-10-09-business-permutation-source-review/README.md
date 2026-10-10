# Complete Business ID permutation source-test review

The four original business_id_permutation.rs test bodies and helper inputs
were compared against the C++ port on 2026-10-09. Every ten-column golden CSV
row and both full business-ID/code assertions are preserved. Fixture bytes
match the pinned source exactly (SHA-256
0b9fe148d139c2c545550f12fa14229f0311e8f6349bc4dea9ae6ee921c9e563).

The retained-range contract runs all 20,000 sequences twice, reconstructing
key and scope for the second encode, and checks exact equality, six characters,
uppercase/digit alphabet and uniqueness. All domain exhaustion, key-version,
blank namespace and invalid-allocation-range inputs and error codes remain.
No locally generated C++ result is substituted for an original golden value.

The unchanged pinned Rust run completes successfully: 4/4 in 65.64 seconds.
The C++ fixture uses explicit arena storage with default PMR disabled. This
file defines no additional public production API or standalone wire model;
complete runtime IDs/clocks/services retain their separate source group.
Fresh C++ native/sanitizer/MIPS checks and final integration are still required.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" cargo test \
  --manifest-path .reference/teaql-rs/Cargo.toml -p teaql-runtime \
  --test business_id_permutation --locked --offline
```

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
