# Private memory grouping and relation identity: Rust 5.0.6

Status: focused native/module qualification. Full private `memory.rs` acceptance
remains pending.

## Original evidence

The [successful original observer](parity/memory_grouping_original_v2_5_0_6.json.gz)
invokes the locked private MemoryDataService with byte-preserved runtime bodies.
It records 1,459 cases:

- 484 ordered pairs of 22 keys through grouped Count, plus one combined dataset
  containing every key and a row with an absent key.
- 968 parent/child key pairs through single and list relation aggregates.
- Six cases for mixed signed/unsigned keys, empty parents and missing parent keys,
  each in single and list modes. The latter cases deliberately omit child metadata
  to verify the source's early default-return path.

Keys cover NULL, TypedNull, booleans, signed/unsigned integers, floating point
including signed zero, Decimal scale, duplicate text, different Objects/Lists,
JSON, Date and Timestamp. Output preserves complete rows, Value Debug variants
and error text. Group order and the first representative key are observable.

The [first observer attempt](parity/memory_grouping_original_5_0_6.json.gz) failed
to compile because the observer used `teaql_core::Timestamp` instead of
`teaql_core::time::Timestamp`. This was an observer error, not a runtime defect.
Its failed evidence is retained; only the corrected v2 receipt qualifies results.

The [initial native comparison](parity/memory_grouping_initial_comparison_5_0_6.json.gz)
matches 1,430/1,459 and retains 29 differences with a failing exit code. A separate
[pre-repair snapshot](parity/memory_grouping_native_before_5_0_6.json.gz) retains
the complete native observations and original helper/executable identities.

## Repairs

The private helper now uses the source's grouping key, rather than Value Debug:
NULL and TypedNull share a key, Objects share `object`, Lists share `list`, and
signed integers retain their own prefix instead of becoming unsigned identities.
For remaining variants the source grouping key matches the existing graph-identity
key implementation, which is reused. This preserves source key order and Decimal
scale without changing the production graph-identity function.

Relation aggregation collects every present parent key, including NULL and
TypedNull. It no longer uses the production helper that removes NULL and
deduplicates identities. If no parent keys exist, each parent receives the default
zero or empty list before child metadata/execution is consulted. Relation lookup
itself still precedes that shortcut, as in the source.

The source intentionally has different grouping and relationship identity rules:
grouping distinguishes positive I64 and U64, while graph matching merges their
identities. The mixed-key cases preserve both behaviors. Public runtime bodies,
provider contracts and generated artifacts are unchanged.

## Verification and remaining scope

- [Repaired native](parity/memory_grouping_native_repaired_5_0_6.json.gz): 1,459/1,459.
- [Canonical module](parity/memory_grouping_modules_repaired_5_0_6.json.gz): 1,459/1,459.
- Native CTests: 5/5 for grouping, aggregate, pipeline, filter and original callers.
- Module comparisons retain 208 aggregate, 18 pipeline and 16 filter matches;
  the module original caller contract passes 1/1.
- Canonical verification checks 1,113 consumer/fixture bodies. Locked-source and
  module path checks pass.
- Pinned formatting/check pass for 1,781 handwritten files; affected native and
  module builds report no work remaining. The [CTest checkpoint](parity/memory_grouping_ctest_checkpoint_5_0_6.json.gz)
  retains actual test logs, current input identities and both verified cleanups.

This does not qualify every relation aggregate shape, multi-column grouping,
projection/predicate error precedence, concurrent helper use or mutation behavior.
The source disposition and full P0-P7 acceptance remain unchanged.

## Reproduction and artifacts

```bash
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_memory_grouping_probe
python3 tests/parity/diagnose_memory_grouping_5_0_6.py \
  --cpp build_native_current/teaql_memory_grouping_probe
```

Fresh source observations use
`scripts/observe_memory_grouping_5_0_6.py --output <new-receipt.json.gz>`.
Existing receipts cannot be overwritten. The owned temporary runtime view and
Cargo target are removed after child termination, with incremental compilation
and debug information disabled. The failed attempt removed 134,715,167 bytes;
the successful attempt removed 161,006,729 bytes. Both directories were checked
absent afterward. Shared downloads and stable CMake preset builds are retained.
No sanitizer, MIPS or physical-device result is claimed for this checkpoint.
