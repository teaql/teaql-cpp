# Complete original relation enhancement execution

Status: all 754 original/native and original/module outcomes match at the real runtime/executor boundary.
This is not an acceptance or source-disposition promotion.

The [final combined original receipt](parity/relation_enhancement_complete_combined_original_5_0_6.json.gz)
executes 754 inputs through the locked Rust 5.0.6 runtime. The first 377 preserve
all inputs from the existing 266-case enhancement corpus and 111-case variant
corpus. The second 377 set explicit comment/purpose on query objects, changing no
other input. Existing corpora and earlier observations remain unchanged.

## Actual boundary

An append-only test module in `data_service/relation.rs` calls the original
aggregate, object-group and child-query enhancement methods. It constructs the
actual RuntimeModule, Context and EntityDataService. Original method bodies and
request preparation are preserved; no reconstructed EntityDataService or
child-fetch implementation substitutes for runtime execution. A recording
QueryExecutor receives prepared requests, returns input-selected compact rows,
and can suspend or fail. Its guard records completion/cancellation drops.
Capabilities are counted by the actual executor method.

The observer retains parents, prepared queries, request trace/comment/purpose,
executor calls, drops, capabilities, errors and cancellation. Shared fixture
parsers only construct inputs and serialize outputs; their hashes are retained.
The future is dropped before partial rows and counters are inspected. This is
execution at the real runtime/executor boundary, not physical SQLite evidence.

## Outcomes

| Inputs | Success | Failure | Cancelled | Executor calls | Capability calls |
| --- | ---: | ---: | ---: | ---: | ---: |
| unchanged | 41 | 295 | 41 | 76 | 0 |
| explicit_intent | 183 | 135 | 59 | 355 | 0 |

Unchanged inputs include 168 missing-comment and 75 missing-purpose rejections.
The explicit-intent matrix reaches successful aggregate, group and child-query
execution, while preserving intentional missing descriptors/relations, executor
failures and cancellation. In particular, `IgnoredTarget` is rejected when it
has no trusted metadata; the observer does not silently retarget such queries.
Successful simplified-SPI observations cannot substitute for these boundaries.

The two initial 377-case receipts are diagnostic history. They inherited an
unused fixed-zero capability field from the older shim driver. The final combined
observer replaces that placeholder with actual counting; only the final receipt
is suitable as the new differential reference. Do not relabel the diagnostic
capability fields as measured values.

## Reproduction and cleanup

```sh
source scripts/toolchains/activate.sh
python3 scripts/observe_relation_enhancement_complete_5_0_6.py --combined --output <fresh-receipt.json.gz>
```

The runner records original source prefixes, locked dependencies, observer and
fixture hashes, input-corpus hashes, actual inputs, terminal commands and output.
It owns and reclaims its isolated source/build directory after compiler and test
processes terminate. All three observations confirm cleanup; total removed:
489833842 bytes. Compact receipts remain in the repository.

## C++ replay and regression gates

The separate `relation_enhancement_complete_probe` retains the historical input
parser and output projections but replaces the child-fetch shortcut with
`EntityDataServiceAccess::fetch_all`. Its recording QueryExecutor receives actual
prepared requests. Neither runtime implementation nor generated source changes.

The [native receipt](parity/relation_enhancement_complete_native_initial_5_0_6.json.gz)
and [module receipt](parity/relation_enhancement_complete_modules_5_0_6.json.gz)
each match 754/754 full outputs. Both retain identical request errors, partial
parents, ordered requests, trace/intent, executor drops and capability counts.
A [negative comparator control](parity/relation_enhancement_complete_negative_5_0_6.json.gz)
changes one request error: 753/754 match and exit 1 confirms rejection. Its
throwaway executable was removed; its intentionally failing receipt is retained.

Native CTest `relation_enhancement_complete_rust_parity` passes 1/1. Canonical
module verification checks 1,124 consumer bodies; module paths and baseline checks
pass. Pinned format/check passes for 1,791 handwritten C++ files. Stable native/module preset directories are
reused; no additional build tree is retained. No sanitizer or MIPS run is claimed
for this test-only extension.

```sh
cmake --build --preset native --target teaql_relation_enhancement_complete_probe -j 2
ctest --test-dir build_native_current -R '^relation_enhancement_complete_rust_parity$' --output-on-failure
cmake --build --preset modules_native --target teaql_module_relation_enhancement_complete_probe -j 2
python3 tests/parity/compare_relation_enhancement_complete_5_0_6.py --cpp build_modules_native/modules/teaql_module_relation_enhancement_complete_probe
```

## Remaining work

The older callback-level probes remain evidence for their own scope. This new
replay closes the identified full-runtime observation gap for these retained
R09/R10 inputs; it does not establish every source responsibility or physical
SQLite behavior. Complete the relation source review and final regression union
before promoting its disposition. Counts remain 111 accepted / 11 pending /
2 excluded_external; P0-P7 remain incomplete.
