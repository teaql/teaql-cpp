# Private memory row pipeline: Rust 5.0.6

Status: focused native/module qualification. The whole
test-only `memory.rs` source remains pending review.

## Original observations

The [original receipt](parity/memory_pipeline_original_5_0_6.json.gz) runs an
appended observer against the byte-preserved locked `memory.rs`, using its actual
`MemoryDataService::fetch_all`. It observes eighteen named queries over three
rows. Results encode every Value using Rust Debug, retaining signed/unsigned
types, missing fields and explicit NULL; errors retain their full display text.

The [initial native comparison](parity/memory_pipeline_initial_comparison_5_0_6.json.gz)
matches 7/18 and records eleven differences: Soundex/GBK projection, missing
projection fields, unsupported scalar expressions/functions, projection source
identity, ascending/descending GBK ordering, mixed signed/unsigned HAVING,
non-boolean HAVING and GBK over aggregate output. Its comparator exits 1.

## Repair

The private C++ helper now runs its own row pipeline using the private evaluator
qualified in the [filter checkpoint](memory_source_upgrade_audit_5_0_6.md#filter-repair-checkpoint).
Production runtime and generated APIs are unchanged.

- Ordinary queries order rows, apply offset/limit, then project. All expressions
  in an ordinary projection read the original input row. Missing field selections
  are omitted; a missing column expression produces NULL.
- Ordering processes keys in reverse priority with stable ordering. Field absence
  differs from an expression that evaluates to NULL. Expression errors become
  absent sort keys, as in the reference. Incomparable values compare equal.
- This test helper uses stable insertion sorting to avoid implicit heap scratch
  storage; it has quadratic worst-case cost. This is not a production sort
  implementation or a performance-parity claim. Larger and partially ordered
  input families remain to be qualified.
- Slice bounds are compared before conversion to `size_t`, including an offset
  of `UINT64_MAX`. Slicing precedes ordinary projection errors.
- Aggregate projections read the progressively updated aggregate row. All groups
  finish projection before HAVING starts. Aggregate queries retain the reference
  behavior of ignoring ordinary ordering/slicing at this stage.

The [repaired native observation](parity/memory_pipeline_native_repaired_5_0_6.json.gz)
matches 18/18. Native CTests pass 3/3: the new pipeline gate, the sixteen-case
filter gate and the retained original caller fixture. This is broader focused
evidence, not complete coverage of ordering, scalar values, aggregate arithmetic,
relation aggregation, mutation semantics or the full private source.

The [module observation](parity/memory_pipeline_modules_repaired_5_0_6.json.gz)
also matches 18/18. The earlier sixteen-filter comparison and module original
caller test pass. Canonical verification checks 1,111 consumer/fixture bodies;
affected native and module target builds report no work remaining. Module path
and locked-source provenance checks also pass.
Pinned formatting and format-check pass for 1,779 handwritten C++ files.
The [CTest checkpoint](parity/memory_pipeline_ctest_checkpoint_5_0_6.json.gz)
retains the actual native/module CTest logs, current source hashes and verified
original-build cleanup. No source disposition is promoted by this checkpoint.

## Reproduction and artifact lifecycle

```bash
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_memory_pipeline_probe
python3 tests/parity/diagnose_memory_pipeline_5_0_6.py \
  --cpp build_native_current/teaql_memory_pipeline_probe
```

To obtain fresh original observations, run
`scripts/observe_memory_pipeline_5_0_6.py --output <new-receipt.json.gz>`.
The runner verifies the locked upstream and preserves original bodies in an
owned temporary runtime view. Its Cargo target is isolated from the shared
`CARGO_TARGET_DIR`, with incremental compilation and debug information disabled.
The successful original run removed 160,994,972 bytes after its child exited;
the receipt verifies that the temporary directory no longer exists. Dependency
downloads and stable CMake preset builds remain available. No MIPS/device or
sanitizer execution is claimed for this test-helper semantic change.
