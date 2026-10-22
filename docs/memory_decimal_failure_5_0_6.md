# Private memory aggregate failure boundary: Rust 5.0.6

Status: native, canonical module and focused sanitizer comparisons pass.
Complete private-source acceptance remains pending. No execution body changes
are needed for the observed boundary.

## Original behavior and C++ adaptation

The original service copies rows under its storage mutex, releases that lock,
and only then evaluates aggregates. Decimal addition overflow therefore unwinds
the query without poisoning the storage lock. This differs from version
arithmetic failure during a mutation while that lock is held.

The [original observer](parity/memory_decimal_failure_original_5_0_6.json.gz)
executes unchanged private Rust bodies and catches actual panics outside the
service. Six datasets are queried with SUM and AVG: positive and negative
Decimal extrema added to themselves, MAX plus one, MAX plus MIN, intermediate
MAX-plus-one overflow followed by minus one, and out-of-range/nonfinite floating
conversion. There are twelve observations and eight actual addition panics.
The cancellation case distinguishes final mathematical range from overflow at
an intermediate addition. Floating conversion failure follows the source's
Decimal-zero fallback.

The C++ probe catches only `std::overflow_error` whose exact message is
`Addition overflowed`. It emits a tagged `panic` field solely for this established
Rust-panic/C++-exception adaptation; any other exception fails the executable.
This preserves the core Decimal exception contract rather than converting query
failure into an executor error or weakening comparison. Original raw panic
output remains in the receipt. The native normalized comparison matches 12/12.

## Service remains usable

Every observation checks the complete stored rows after the attempted aggregate,
then replaces them with a valid row, repeats the aggregate, inserts another row
and repeats it again. Thus both read and write availability are checked after
failure, including successful SUM/AVG over replacement data. No stale query
scratch storage or poisoned-lock state becomes a new persistent service state.
The candidate disables the default PMR resource.

This closes D02's service-boundary question for Decimal addition overflow and
the observed conversion fallback. It is not an additional proof of every Decimal
arithmetic operation; those remain covered by the separate core Decimal review.
Mutation-overflow poisoning keeps its distinct contract and evidence.

## Executed gates

The [native comparison](parity/memory_decimal_failure_native_5_0_6.json.gz),
[module comparison](parity/memory_decimal_failure_modules_5_0_6.json.gz) and
[sanitizer comparison](parity/memory_decimal_failure_sanitize_5_0_6.json.gz) each
match 12/12. The [checkpoint](parity/memory_decimal_failure_checkpoint_5_0_6.json.gz)
records native/sanitizer CTests (one each), 1,120 canonical consumer/fixture
checks and locked source/path verification. ASan leak scanning is disabled;
UBSan stops on undefined behavior. All probes use the same unchanged helper. Pinned formatting/check passes for
1,788 handwritten files; affected builds finish with no work remaining.

## Reproduction and cleanup

```bash
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_memory_decimal_failure_probe
ctest --test-dir build_native_current \
  -R '^private_memory_decimal_failure_rust_parity$' --output-on-failure
```

The original runner removed 160,993,029 bytes of owned view/build outputs after
completion and verified cleanup. Stable preset build directories are reused.
No production body, generated artifact or ownership domain changed. Debug
responsibilities and final whole-source consolidation remain open; no MIPS or
physical-device execution is claimed.
