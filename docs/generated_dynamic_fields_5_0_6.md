# Generated indexed dynamic-field carriers

Current G16 qualification is recorded in the [complete acceptance audit](dynamic_storage_acceptance_5_0_6.md). The text below preserves this earlier checkpoint and its original remaining-work boundaries.

Recorded: 2026-10-12. Runtime baseline:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Frozen generator producer:
`45e1572c4956ca963410d8ea436ead3c8857087a`. The independent cpp_support workspace
retains its original ancestry and previous edits.

## Runtime before emission

`GeneratedDynamicFields` centralizes the indexed carrier behavior emitted by
Rust `teaql-macros/src/derive_impl.rs`. Installation validates canonical layout
identity and definition owner before changing root values or load state. Dynamic
update/delete require an indexed snapshot and the model's canonical layout before
calling EntityRuntimeState's existing mutation methods. Existing selected values
can change without changing immutable availability identity. Newly selected codes
and delete transitions derive a new snapshot through the explicit Context pool.
Failures preserve the existing carrier where source does. The helper introduces
no dependency or hidden ownership domain.

The generated Model base now advertises indexed dynamic loading, exposes its
runtime-owned values/pending intent and delegates install/update/delete to that
helper. Native fields, snapshots and mutations remain distinct from durable
extension values. Application methods are `dynamicFieldValues`,
`updateDynamicField`, `deleteDynamicField` and `hasPendingDynamicMutations`;
required kernel trait methods retain snake_case. Generated Q requests provide
ref-qualified `selectDynamicFields(selection)` delegates to EntityQuery. Purpose,
comment, Context binding and execution stay owned by the existing runtime.

The support template and CppCoreGenerator were changed in the independent workspace.
All five committed fixture sets were regenerated, including implementation hashes
and model-aware Assist. No generated header was patched by hand. The original
Rust templates and original generator checkout are unchanged.

## Qualification scope

The regenerated Product model and Q factory execute real SQLite dynamic list and
stream queries, NULL/delete saves, authoritative version increments and sparse
extension selection. Tests check layout/owner and non-indexed rejection, immutable
snapshot preservation, availability transitions and independent copied row values.
The same test source is consumed through both ordinary headers and module imports.

Eighty direct original/C++ cases compare a frozen generated Rust School carrier
with regenerated C++ Product adapters through an explicit model-independent
projection. Values, `#code` availability, snapshot retention/transitions, clone
isolation, pending intent and common errors match for absent/NULL/typed-NULL,
Unicode/long values, invalid type/code and layout/owner/state failures. Native
per-model field positions and names are outside this projection; they are not
rewritten or normalized. This corpus proves shared carrier behavior, not complete
model/configuration or generator parity.

Generated source tests assert thin helper delegation, current Q APIs and Assist;
selected Java tests pass 11/11. Five fixture sets (55 files) reproduce byte for
byte using `scripts/generate_examples.py --check`. Final gates pass: native 5/5, targeted ASan/UBSan 5/5 with leak scanning disabled
(`ASAN_OPTIONS=detect_leaks=0`), and modules 3/3 including actual generated dynamic
queries/saves through `import teaql_provider_sqlite`. Live original comparisons
pass 80/80. Formatting checks 804 files; 1,406 catalog exports, seven facade
archives and active paths (116 production units) verify. These counts retain
separate provenance and do not qualify all in-scope capabilities.

All generator capability records remain unqualified. Full readonly/presentation
contracts, generated Checker/Fix configurations, inheritance, constants, complete
query/aggregate/pagination/JSON/E/assembly surfaces and the full P0-P7 acceptance
matrix still require work. This checkpoint adds the missing generated carrier
adapters over qualified runtime mechanics; it does not reduce the goal.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
python3 scripts/generate_examples.py --check
cmake --build --preset native --target teaql_generated_dynamic_field_tests \
  teaql_generated_dynamic_carrier_probe teaql_generated_api_tests \
  teaql_generated_checker_tests teaql_generated_query_expression
ctest --test-dir build_native_current --output-on-failure \
  -R '^(generated_dynamic_field_contracts|generated_dynamic_carrier_rust_parity|generated_api_contracts|generated_checker_contracts|example_generated_query_expression)$'
```

Build the original public probe from `tests/parity/rust_field_layout/Cargo.toml`
with `--offline --locked --bin generated_dynamic_carrier_5_0_6`,
`CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc` and
`CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506`. Replay/regenerate with
`generate_generated_dynamic_carrier.py --rust <rust-probe> --cpp <cpp-probe>`.

Run `CppGenerationTest,CppFieldLayoutMetadataTest` in the independent generator
reactor with `-Dsurefire.failIfNoSpecifiedTests=false`; earlier module build/tool
and generation requirements remain unchanged. No new C++ runtime dependency,
MIPS run or physical-device evidence is claimed.

For lifetime checks, build the five native targets with `--preset sanitize` and
run the same CTest selection in `build_sanitize_current` using
`ASAN_OPTIONS=detect_leaks=0`. Build `teaql_module_contracts`,
`teaql_module_generated_dynamic_field_tests` and `teaql_generated_query_expression`
with `--preset modules_native`; run the three `module_import_contracts`,
`module_generated_dynamic_field_contracts` and `module_generated_query_expression`
tests in `build_modules_native`. Archive ownership verification uses
`scripts/verify_module_symbols.py --build-dir build_modules_native`.
