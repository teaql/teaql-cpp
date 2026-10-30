# Relation probe source-test boundary closure

Status: all 32 native and module contract cases pass; existing 432-case differential regressions pass in both profiles.

The original Rust 5.0.6 planner test
`single_parent_probe_does_not_delegate_to_a_multi_parent_provider_batch` checks
parent counts 0, 1, 2 and 32, explicit thresholds 0 and 32, and a provider without
small-parent probe capability. The [fresh full-source replay](parity/relation_source_original_tests_5_0_6.json.gz)
passes that test and the two adjacent planner/telemetry tests unchanged.

The older 432-case dispatch corpus does not contain parent count 32 or explicit
threshold 32. It remains unchanged. A new C++ contract enters the production
`fetch_relation_rows` operation through the existing recording child-fetch probe.
It covers every original batching stimulus and an additional 32-parent explicit
threshold boundary, independently varying capture mode and compact mode: 32
cases in total. The fixture duplicates every parent identity and includes an
absent-key row, so observed dispatch counts also exercise deduplication.

The contract checks callback counts, exact ordered returned probe indices,
completion counts, capability lookup counts, selected-plan telemetry, parent
cardinality, target entity, nested-load removal and scalar/partition selection.
In particular, a default 32-parent probe-capable route batches behind one callback
when capture is disabled and issues 32 callbacks when capture is enabled. An
explicit threshold of 32 keeps per-parent dispatch even with capture disabled.
A singleton retains scalar dispatch. Empty identity input performs no callbacks.

These are source-derived C++ integration assertions, not new original Rust output
comparisons. The callback boundary does not establish physical SQLite statement
counts. Production implementation, signatures, ownership and generated code are
unchanged; no sanitizer or MIPS result is claimed.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_relation_execution_probe -j 2
ctest --test-dir build_native_current -R '^(relation_probe_source_boundaries|relation_execution_rust_parity)$' --output-on-failure
cmake --build --preset modules_native --target teaql_module_relation_execution_probe -j 2
python3 tests/parity/check_relation_probe_boundaries_5_0_6.py --cpp build_modules_native/modules/teaql_module_relation_execution_probe
python3 scripts/verify_module_consumers.py --build-dir build_modules_native
```

The checker verifies the locked original source and successful original-test
receipt, checks executable identity before/after execution, and can retain its
inputs/outputs with `--output <fresh-receipt.json.gz>`. The CTest forwards the
configured emulator argument without claiming cross-target validation.

The [native receipt](parity/relation_probe_boundaries_native_5_0_6.json.gz)
and [module receipt](parity/relation_probe_boundaries_modules_5_0_6.json.gz)
record all 32 passing cases per profile. The existing 432-case original/native and
original/module comparisons pass. Native CTest passes 2/2; canonical validation
checks 1,123 consumer bodies and module paths pass. Stable preset directories are reused; no
new disposable checkout/build tree is retained. Complete relation-source review
and P0-P7 acceptance remain open.

## Current original-receipt admission repair

The boundary checker previously required three expected original test names
but did not pin the complete original-test receipt. A
[process-local negative control](parity/relation_source_receipt_false_acceptance_5_0_6.json.gz)
replaced one name with `fabricated::test::not_in_original_source` while keeping
the three-entry count; the installed checker still passed the actual 32-case
C++ probe. Neither the original receipt nor source worktree was changed.

The checker now requires the complete original receipt SHA-256
`c105efaa134ea2eaa49a597594e2cf4cae60aacd1c52a30af0e08742238937c1`
before reading its claims. A permanent CTest control executes the unchanged
receipt successfully and rejects the same altered test identity through the
installed checker. Native configure and stable no-work build pass; the source
boundary and rejection control pass **2/2**. The registry has **1,063** tests.
The existing imported-module relation probe also passes all **32** boundary
cases and the same altered-receipt rejection control without a module rebuild.

The preceding 1,062-test epoch was deliberately interrupted after **785 passed
tests**, before `generated_api_type_errors` finished. Its
[terminal record](parity/current_full_native_1062_bound_terminal_5_0_6.json.gz)
has CTest exit `-2` and no JUnit; its [frozen input snapshot](parity/current_full_native_1062_bound_inputs_5_0_6.json.gz)
remains separate. The controller and compiler children exited, the owned
wrapper directory was removed, and 1,326,475,088 bytes of its remaining
compiler scratch were removed only after confirming no child held them. This
checkpoint is not a full native verdict. Complete relation-source, G13 and
P0-P7 acceptance remain open.

## Optimized-Python source boundary

The [pre-edit counterexample](parity/relation_source_optimized_preedit_witness_5_0_6.json.gz)
retains a process-local altered original-test name. The checker accepted it
under `python -O` while ordinary Python rejected it. The
[seventeen-assertion rewrite](parity/relation_source_assert_rewrite_5_0_6.json.gz)
changes only the checker and its receipt control to unconditional checks; their
whole ASTs match the transformation. The new optimized receipt control runs
the same valid and altered original receipt through the actual 32-case C++
probe. A separate provenance guard verifies the exact before/after sources and
whole AST transformation. Current native CTests pass **4/4** for the boundary,
ordinary control, optimized control and rewrite provenance. That checkpoint
had 1,068 registered tests. The current 1,072-test registry adds separate
direct private-key/classifier verifier and P0 inventory checks. It still has
no full native epoch. The preceding 1,066-test hardened-controller epoch
retains its own frozen inputs. Complete G13/SQ02 and the remaining phase exits
remain open.
