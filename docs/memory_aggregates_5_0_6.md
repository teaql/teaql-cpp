# Private memory aggregate checkpoint: Rust 5.0.6

Status: focused native, module and sanitizer comparisons pass. Full `memory.rs`
source acceptance remains pending.

## Source observations

The [original receipt](parity/memory_aggregate_original_5_0_6.json.gz) invokes
the locked test-only `MemoryDataService::fetch_all` through an appended observer.
Every original body remains byte-identical. Sixteen datasets exercise twelve
aggregate functions plus a separate `COUNT(*)`, yielding 208 unique observations.
Results preserve Value Debug types and full error text.

Datasets cover empty input, NULL, absent fields, signed and unsigned integers,
mixed integers, Decimal, floating point, text, singleton and paired booleans,
TypedNull, unsigned sum overflow, signed sum underflow and NaN/Infinity. Functions
are Count, Sum, Avg, Min, Max, Stddev, StddevPop, VarSamp, VarPop, BitAnd, BitOr
and BitXor. Count uses field `v`; the thirteenth operation uses `*`.

The [initial C++ comparison](parity/memory_aggregate_initial_comparison_5_0_6.json.gz)
matches 185/208, retaining all 23 differences and a failing comparison exit code.

## Repairs in the private C++ helper

- MIN/MAX select the first non-NULL value without attempting a comparison.
  Subsequent values use the source's typed partial ordering. Text works, a single
  Bool or TypedNull is retained, and a later incomparable value produces the
  source's error identifying that later value.
- Integer SUM preserves the source's narrowing behavior. An unsigned accumulator
  records the low 64 bits; the exact Decimal accumulation determines the sign
  while retaining the source's existing Decimal conversion/arithmetic path.
  Negative integer results use `std::bit_cast<int64_t>`. This reproduces Rust's
  integer casts without C++ signed overflow or implementation-defined narrowing.
  `UINT64_MAX + 1` returns `U64(0)`; `INT64_MIN - 1` returns `I64(INT64_MAX)`.
- Bit aggregates distinguish out-of-range U64 from unsupported input types,
  preserving the exact error message.
- Statistical aggregates validate and convert input before checking whether
  enough samples exist. A singleton invalid input remains an error rather than
  silently returning NULL.

These are private test-oracle semantics. Public runtime bodies, generated APIs,
database providers and production aggregate behavior are unchanged. In particular,
this checkpoint does not recommend wrapping SUM as a general application policy.

## Verification

- [Native comparison](parity/memory_aggregate_native_repaired_5_0_6.json.gz): 208/208.
- [Canonical module comparison](parity/memory_aggregate_modules_repaired_5_0_6.json.gz): 208/208.
- [ASan/UBSan comparison](parity/memory_aggregate_sanitize_repaired_5_0_6.json.gz):
  208/208, empty diagnostic stderr, leak scanning disabled. This focused check
  covers the new integer boundary handling; it is not a full sanitizer run.
- Native CTests: 4/4, comprising aggregate, row-pipeline, filter and original
  memory-service callers. Module original callers: 1/1, with the 18 pipeline
  and 16 filter observations also compared successfully. Sanitizer CTest: 1/1.
- Canonical verification: 1,112 consumer/fixture bodies.
- Pinned formatting and format-check: 1,780 handwritten C++ files. Module path
  and locked-source provenance checks pass; affected native/module/sanitizer
  builds report no work remaining. The [CTest checkpoint](parity/memory_aggregate_ctest_checkpoint_5_0_6.json.gz)
  retains actual test logs, current-source identities and verified cleanup.

The ordinary pipeline/filter regressions remain qualified separately. The
matrix is not exhaustive: Date/Timestamp extrema, broader mixed numeric kinds,
Decimal overflow/panic boundaries, grouping key identity/order, relation
aggregation and mutation helpers still require source-based review and evidence.
No source disposition or full P0-P7 phase is promoted.

## Reproduction and cleanup

```bash
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_memory_aggregate_probe
python3 tests/parity/diagnose_memory_aggregate_5_0_6.py \
  --cpp build_native_current/teaql_memory_aggregate_probe
```

Fresh original observations use
`scripts/observe_memory_aggregate_5_0_6.py --output <new-receipt.json.gz>`.
Existing receipts cannot be overwritten. The runner checks the locked source,
uses an owned temporary runtime view and Cargo target, disables incremental
compilation/debug information, and removes outputs after the child exits.
The successful run removed 160,929,158 bytes and verified directory removal.
Shared dependency caches and stable CMake preset directories are retained.
