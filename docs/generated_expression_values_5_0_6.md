# Generated E Value/Null qualification against Rust 5.0.6

Status: partial G11/P6 follow-up. Runtime reference is
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; retained producer reference is
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Original observations

The corpus captures **108 actual original generated E observations**, with no
result normalization. The producer manifest, model hash and all original file
hashes are verified. The Rust oracle calls the retained generated E interfaces,
model constructors/mutation methods, and the unmodified runtime graph builder.

- 48 nullable Profile.note before/after observations cover NULL, empty/nonempty
  text, whitespace, quoted Unicode data and a 4 KiB string. Each checks eval
  presence/value, legitimate-null unwrap rejection, eager/lazy/default fallbacks
  and callback count, including explicit updates to NULL or text.
- 24 selected reverse to-one views cover an explicitly empty Profile relation
  and a present Profile with NULL/text note. Empty relation traversal propagates
  Null to the field; a present NULL field also resolves to Null. These states
  remain distinct from omitted/unloaded relationships covered by the diagnostic
  corpus.
- 36 selected Orders lists cover zero/one/two rows, first/indexed access, in-range
  and out-of-range indexes, scalar values/defaults and lazy callback count. Both
  true and false stored `is_loaded` flags are supplied to the original graph
  installation API. The empty selected list is a present Value with size zero;
  missing list elements propagate Null rather than NotLoaded.

The C++ probe uses the same generated models and actual Context-owned graph
installation, with the default PMR resource set to `null_memory_resource`.
These are runtime/E tests; they do not execute SQL or prove provider behavior.

## Restored graph-view availability

Before the fix, **18/108** observations differed. Every differing case installed
an explicit selected Orders view whose stored SmartList flag was false. Rust
`Person::eval_orders` reads `RelationHandle::state` and returns Value for both
Loaded and Empty. The runtime graph determines that state from the installed
view and its contents, independently of `SmartList::is_loaded`. C++ previously
checked only the payload flag and incorrectly emitted a missing-preload error.

`GeneratedRelationAccess::list` now accepts an unmarked list when it is exactly
the installed graph view for the owner's entity/ID/relation. It retains the
stored flag and uses the existing checked entity/list borrow. Other unmarked
list objects do not gain availability merely because the owner has a selected
view. The function signature, generated interfaces and layer responsibilities
remain unchanged; there is no implicit database fetch or new ownership.

The lifetime regression test constructs an installed empty unmarked view,
checks Value/size zero and Null propagation through entity/scalar traversal,
rejects an unrelated unmarked list, then actually deletes UserContext and
requires the captured list expression to reject access before reading its data.

## Verification and remaining scope

Completed gates: native **6/6** (three E gates plus generated relation/entity
helper regressions), selected ASan/UBSan **2/2** with leak scanning disabled,
**108/108** live original/C++ value observations and **196/196** original
structured diagnostic observations. Pinned formatting checks **930 handwritten
C++ files**. Source lock, generator ledger, module path/export and whitespace
checks pass. Generator and module interfaces were not changed; Java,
regeneration, module builds and MIPS/QEMU are not repeated for this checkpoint.


The native E gates compare this corpus and the existing 196 original diagnostic
observations, plus Context binding/lifetime tests. The selected ASan/UBSan gates
cover this corpus and the lifetime tests with leak scanning disabled. The
existing SQLite archive is not instrumented; this corpus performs no SQLite I/O.
Further entity/relation helpers receive targeted native regression checks.

```sh
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_generated_expression_values_probe teaql_generated_expression_diagnostics_probe teaql_runtime_expression_tests
ctest --test-dir build_native_current --output-on-failure -R '^(runtime_entity_expression_contracts|generated_expression_diagnostics_rust_parity|generated_expression_values_rust_parity)$'
python3 tests/parity/compare_generated_expression_values.py --cpp build_native_current/teaql_generated_expression_values_probe --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_expression_values_5_0_6
```

Broader generated model configurations, malformed diagnostic-string presentation
and the full P6 Assist/sample/extension assembly qualification remain open.
This follow-up does not claim full G11/P6 or overall migration completion.
