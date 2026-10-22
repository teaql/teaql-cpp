# Private memory source acceptance: Rust 5.0.6

Status: accepted for the locked 5.0.6 private test-oracle responsibility.
This accepts one source entry, not the full runtime or P0-P7 migration.

## Scope

This review covers the complete 1,097-line locked
`teaql-runtime/src/memory.rs`, including its nine runtime-root callers and its
own typed-LIKE provenance test. It is the cfg(test)-only reference data service,
not the production in-memory engine, SQLite provider, graph-save runtime or a
claim of complete generator parity.

The [responsibility review](memory_complete_responsibility_review_5_0_6.md) maps
all function families, the metadata extension point, error/service diagnostics,
shared state and typed result wrappers. D01 diagnostics and D02 query-time
arithmetic failure have their own original observations and implementation
reviews. D03 requires the fresh source/candidate execution collected here.

## Semantic evidence

Twelve direct reference families cover 3,409 outcomes: filter 16, pipeline 18,
aggregates 208, grouping/identity 1,459, mutation 1,144, relation shapes 160,
entry points 43, valid sorting 60, recursive filters 272, Decimal failure 12,
Debug output 12 and Debug contention/failure 5. The original corpus is retained
with byte-preserved source provenance; final candidate replay uses the current
helper. These finite observations support the responsibility review rather than
replace its branch and abstraction audit.

The [fresh original replay](parity/memory_source_final_original_5_0_6.json.gz)
executes all nine `memory_data_service_*` runtime tests and
`both_memory_evaluators_use_unchanged_typed_like_patterns`. The controller derives
caller identities from the locked source and checks the exact terminal passed
names. The original view has no appended observer for this final replay. It
preserves all source bodies and locked registry identities.

The existing C++ `typed_like_matches_raw_patterns` function checks all six
positive/negative contain, prefix and suffix pairs through both the private
service and public evaluator. It runs alongside all nine caller ports in the
native and canonical-module executable; it is not substituted by an unrelated
LIKE comparison.

## Final execution and disposition

The [native final receipt](parity/memory_source_final_native_5_0_6.json.gz) passes
18/18 CTests, including all twelve comparison families. The [module final receipt](parity/memory_source_final_modules_5_0_6.json.gz)
passes all 3,409 comparisons, the original caller/typed-LIKE executable and four
metadata/lifetime/complexity/binding controls. All family receipts bind the same
current helper hash. Canonical verification checks 1,122 consumer/fixture bodies.
The unchanged C++ inputs retain the successful 1,790-file pinned format/check.

The [sanitizer final receipt](parity/memory_source_final_sanitize_v2_5_0_6.json.gz)
passes 6/6 focused metadata, contention/failure, mutation, grouping and entry/lifetime
CTest gates. The first sanitizer receipt recorded successful terminal tests but
its controller expected an older CTest summary spelling. The v2 validation checks
all six retained passed lines and exit zero without rerunning the successful tests;
it links the original receipt hash. It is a corrected evidence check, not a test fix.

The [acceptance receipt](parity/memory_source_acceptance_5_0_6.json.gz) binds the
review, locked source, C++ helper and completed execution evidence. The source
ledger moves from pending_review to accepted: **111 accepted / 11 pending /
2 excluded_external**. Capability counts remain unchanged; full generator phases
and other pending source entries are not accepted by this decision.

## Explicit adaptations and limits

- GlobalContext/UserContext own the growing arena and all nested service data.
  Checked service/metadata borrows do not reproduce Arc lifetime extension.
  Shared service state is explicit; metadata is obtained through MetadataStore,
  including custom implementations, with checked domain retention.
- Version arithmetic follows the selected Rust debug-test profile's checked
  failures. C++ exceptions preserve the observed panic boundary and lock poison
  state. This is not a promise about Rust release-profile integer wrapping.
- Decimal aggregate addition failure occurs outside the lock and leaves storage
  usable. Diagnostic formatter failure poisons storage only while holding the
  data-field guard. Panic/exception normalization is limited to the documented
  tagged boundaries, with original raw failures retained.
- Service diagnostics take the original typed metadata reference explicitly,
  preserving the source's conditional Debug capability after interface erasure.
  Identity is checked before output. This is raw test diagnostic formatting,
  not production privacy or logging configuration.
- Context-backed stable merge sorting preserves valid-order semantics and
  O(n log n) comparison cost. Non-total comparator inputs retain the original
  standard-library precondition boundary; a sampled permutation is not a new
  ordering guarantee.
- Relation result indexing uses Context-owned ordered identity buckets, preserving
  the source's row order and separate grouping/identity key responsibilities.
- Thread contention tests access only the mutex from the worker. They do not
  authorize cross-thread UserContext allocation. No fresh MIPS, physical-router,
  TSan or leak-scanning result is claimed.

## Reproduction

```bash
source scripts/toolchains/activate.sh
python3 scripts/replay_private_memory_source_5_0_6.py --output <new-original-receipt.json.gz>
ctest --test-dir build_native_current \
  -R '^(private_memory_.*|original_runtime_memory_service_contracts|original_runtime_optimistic_conflict_output_contracts)$' \
  --output-on-failure
```

Build the corresponding `teaql_memory_*_probe`, `teaql_memory_metadata_tests`
and `teaql_original_runtime_memory_service_tests` targets before replay. Canonical
module probes use the same source bodies and the registered modules-native
preset targets. The final receipts identify exact commands and helper hashes.

The original runner removed 160,879,927 bytes of owned view/build outputs and
confirmed absence after child exit. Stable build presets are reused. Original,
failed and corrected compact receipts remain available; no source or user data
is removed. Source acceptance does not promote any unrelated capability or
complete P0-P7.
