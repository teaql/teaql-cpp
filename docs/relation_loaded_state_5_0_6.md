# Relation attachment loaded state (Rust 5.0.6)

Status: focused regression reproduced and repaired; native and module checks pass.
This does not close the complete relation source review.

## Source contract

The locked runtime is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
In `teaql-runtime/src/data_service/relation.rs`, relation attachment preserves a
SmartList sidecar when a facet was requested or a related row has nested relation
results. It constructs that sidecar through `SmartList::new`, which sets
`is_loaded = true` in `teaql-core/src/list.rs`. To-one attachment truncates the
result to its first row. Nested relation states remain independent.

The C++ attachment path constructed an empty SmartList, appended rows, and copied
it into the parent without marking it loaded. `CompactRow::set_loaded_relation`
retains that flag unchanged. The repair explicitly marks the completed attachment
loaded before storing it; allocation domains and ownership are unchanged.

## Regression and original observations

The initial four source-derived combinations reproduced the missing loaded flag:
native `dynamic_relation_contracts` failed with
`attached relation is loaded independently of descendant load state` (CTest exit 8).
After the shared runtime repair, both native and module contracts passed.

The expanded `attached_relation_loaded_state` fixture covers 24 combinations:

- To-one and to-many relations.
- No nested sidecar, an unloaded nested sidecar, or a loaded nested sidecar.
- Zero or two matching rows.
- No facet request or a facet request.

An append-only observer calls the actual private Rust `attach_relation_rows`
method in a full source-preserving runtime view. Its executor panics on any I/O.
The observation records sidecar presence, loaded state, row count and each nested
state. This verifies empty many-side facet results, identity-only to-one fallback,
to-one truncation and independent nested states. It does not execute the facet
query or qualify unrelated hydration methods or physical SQLite behavior.

The [original receipt](parity/relation_loaded_state_original_5_0_6.json.gz)
contains all 24 results, observer/source hashes, preserved-source provenance and
terminal commands. Its disposable source view/build was removed: 160,950,750
bytes, with cleanup confirmed. The comparator rejects changed source provenance,
incomplete case sets, reordered cases and changed executable identity.

The [negative control](parity/relation_loaded_state_negative_5_0_6.json.gz)
alters one originally true loaded flag to false. It records 23/24 matches and
`parity_passed: false`; the controller observed exit 1. Its temporary executable
was removed. This receipt is intentionally failing evidence, not acceptance.

## Verification

With the pinned toolchain activated:

```sh
python3 scripts/observe_relation_loaded_state_5_0_6.py --output <fresh-original-receipt.json.gz>
cmake --build --preset native --target teaql_dynamic_relation_tests -j 2
ctest --test-dir build_native_current -R '^(dynamic_relation_contracts|relation_loaded_state_rust_parity)$' --output-on-failure
python3 tests/parity/compare_relation_loaded_state_5_0_6.py --cpp build_native_current/teaql_dynamic_relation_tests
cmake --build --preset modules_native --target teaql_module_dynamic_relation_tests -j 2
ctest --test-dir build_modules_native -R '^module_dynamic_relation_contracts$' --output-on-failure
python3 tests/parity/compare_relation_loaded_state_5_0_6.py --cpp build_modules_native/modules/teaql_module_dynamic_relation_tests
python3 scripts/format_cpp.py
python3 scripts/format_cpp.py --check
python3 scripts/verify_upstream.py
python3 scripts/verify_module_paths.py
python3 scripts/verify_module_consumers.py --build-dir build_modules_native
git diff --check
```

The comparator uses the retained original receipt and verifies its source binding;
a fresh observer run uses a new output path without overwriting historical evidence.
Native tests pass 2/2 and the [native comparison](parity/relation_loaded_state_native_v2_5_0_6.json.gz)
matches 24/24. The [module comparison](parity/relation_loaded_state_modules_5_0_6.json.gz)
also matches 24/24, and its contract CTest passes 1/1. Canonical verification
checks 1,122 consumer bodies. The native v2 receipt follows addition of the
standard emulator forwarding argument to CTest; the earlier native receipt is
retained and also passes 24/24. No cross-target execution is claimed.
Formatting/check covers 1,790 handwritten files. No sanitizer or MIPS run is
claimed for this Boolean-state repair. Stable preset directories were reused.

Source totals remain 111 accepted / 11 pending / 2 excluded_external. Full
relation-source qualification and P0-P7 completion remain open.
