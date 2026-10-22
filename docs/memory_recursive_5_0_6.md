# Private memory recursive evaluation: Rust 5.0.6

Status: focused native and canonical-module qualification. Full private
memory-source acceptance remains pending. No execution body is changed here.

## Source responsibilities

The locked evaluator evaluates Binary operands left then right, and Between
operands in value/lower/upper order. AND stops at the first false result, OR at
the first true result, and either propagates an encountered error immediately.
NOT recursively evaluates its child. Empty AND is true and empty OR is false.
Only Boolean values and untyped NULL have truthiness; scalar integer values
produce an error. A missing column evaluates to untyped NULL.

Subquery filters return an unsupported-expression error when reached; a previous
short-circuiting operand can prevent that evaluation. Scalar evaluation does not
implicitly execute Boolean expression trees. Function arity is checked before
argument evaluation. These are source semantics, not SQL three-valued logic or
the public in-memory engine contract.

## Qualification

The [original observation](parity/memory_recursive_original_5_0_6.json.gz) invokes
private `eval_filter` directly through an appended observer with all original
source bodies preserved. The [C++ comparison](parity/memory_recursive_native_5_0_6.json.gz)
reaches the private evaluator through a one-row `MemoryDataService::fetch_all`.
All 272 outcomes match, including exact errors. The
[canonical module comparison](parity/memory_recursive_modules_5_0_6.json.gz) also
matches 272/272. The [execution checkpoint](parity/memory_recursive_checkpoint_5_0_6.json.gz)
records the native CTest, 1,118 canonical consumer/fixture checks and locked
source/path verification. Pinned formatting/check pass for 1,786 handwritten
C++ files; affected native/module builds report no work remaining.

The observations cover:

- Eight operand forms crossed in both positions with AND, OR, NOT(AND) and
  NOT(OR), yielding 256 combinations. Operands include true, false, NULL, a
  non-Boolean integer, a missing column, an invalid Soundex argument, GBK of a
  Boolean and an IN subquery.
- Sixteen boundaries cover empty trees, nested NOT, Binary and Between error
  precedence, invalid Boolean-as-scalar reads under IS NULL/GBK/Soundex, NULL
  under IS NOT NULL/Soundex, GBK arity preceding a bad argument and NOT IN
  subquery rejection.

The candidate disables the default PMR resource. Its runtime path returns errors
through the executor category; an unexpected category is a test failure. This
checkpoint does not expand the supported API or repair a difference: existing
private evaluation already matched these source responsibilities.

## Reproduction and boundary

```bash
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_memory_recursive_probe
ctest --test-dir build_native_current -R '^private_memory_recursive_rust_parity$' --output-on-failure
```

Original observations use `scripts/observe_memory_recursive_5_0_6.py` with a new
receipt path. Its temporary view and Cargo target are removed after execution;
cleanup removed 160,921,028 bytes. Shared downloads and stable CMake builds remain. No additional sanitizer or MIPS
run is required for this test-only evidence addition; no fresh result is claimed
for those profiles.

Whole-source consolidation must still review Value comparison/aggregate branches
against their supporting core contracts, retained caller and typed-LIKE evidence,
and all explicit lifetime and panic adaptations. Passing these expression trees
alone does not accept the source or a complete P0-P7 phase.
