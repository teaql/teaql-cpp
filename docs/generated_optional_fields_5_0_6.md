# Optional generated fields and Checker extensions

This P1 checkpoint uses the active Rust 5.0.6 runtime and producer locks. It
supplements G01, G02, G03, G12 and G13; complete capability qualification remains
pending. Existing handwritten examples and earlier source evidence are retained.

## Model and source authority

`tests/generator/optional_types.xml` defines one Reading entity with optional
text, bool, decimal, double, float, integer, date, timestamp and JSON fields.
The independent C++ generator emits `tests/generated_optional_types`, which is
the sixth reproducible C++ fixture family. The clean original producer emits
`tests/generator/parity/optional_types_rust`: 13 untouched source/configuration
files and a provenance record with model/file hashes and both locked commits.
`scripts/verify_generator_parity.py` checks this fourth original fixture family.

The original entity and Checker compile unchanged using the controlled
`rust_types_entities` path-import harness. The complete original generated
crate fails for three source reasons:

- Its runtime sample code calls `update_recorded_at(Timestamp::now())`, although
  the entity method correctly accepts `Option<Timestamp>`.
- Its two boolean convenience methods call missing `with_active_is` and
  `with_active_is_not` methods.

These source failures remain visible. The harness does not repair or qualify
the original request/runtime scaffolds. Future assembly work must distinguish
source behavior from these producer errors; no generated Rust is hand-edited.

## Direct entity and Checker comparison

The additional corpus contains **1,548 matching original/C++ observations**.
It covers absent/NULL/value rows, typed updates, explicit NULL updates,
ledger-only overlays, cloned views and immutable original values for every
optional field. Floating updates use the corresponding typed value on both
sides; optional string setters retain raw ledger operands and trim stored text.
JSON None, Some(JSON null), false and nested values remain distinct. NULL,
typed-null and incompatible ledger overlays do not replace optional storage.

Each case also calls the original generated ReadingChecker and its generated
C++ counterpart with Create, Update or Unknown status. Complete diagnostics are
compared in camelCase, snake_case and PascalCase wire profiles, retaining order,
location, rule, message and operand data. Sparse updates reject missing fields
even when those fields are optional. Fully loaded optional NULL is legitimate.

The custom Checker logic reads a typed resource from UserContext. It records
how many predefined diagnostics exist when the hook runs, optionally emits a
required-field diagnostic and optionally fixes text. Both implementations call
the hook exactly once, after predefined checks. A business fix does not retract
the earlier sparse-update diagnostics. No extra caller-supplied service or
metadata argument is introduced.

The existing 1,624 entity/relationship comparisons remain a separate corpus.
The optional corpus is gzip-compressed with a fixed timestamp; outputs are
compared exactly without normalization.

## Qualified naming adaptation

The `text` field exposed a C++ casing bug: the model adapter renamed the qualified
runtime call `detail::EntityRuntimeAccess::update_text` to `updateText` because
that token also named the generated domain setter. The adapter now preserves
qualified external/runtime identifiers and converts methods qualified by
generated model types. Its qualifier check scans only adjacent tokens rather
than copying/scanning the entire preceding source for every method.

Java regression assertions cover both the camelCase application setter and
the unchanged snake_case runtime call. All fixture artifacts are regenerated;
none are manually patched. No runtime behavior or dependency was added for this
naming fix.

## SQLite, expressions and ownership

`generated_optional_types_tests.cpp` uses the generated runtime assembly, Q/E and
the shared audited save service. It checks real SQLite persistence of optional
false/zero, decimal/date/timestamp, a present JSON null, and subsequent explicit
SQL NULL mutations. Sparse projections retain NotLoaded. Generated stream
decoding preserves NULL/zero availability and exhausts its cursor.

An entity expression expires after actual UserContext deletion. Explicit copies
into Global retain both populated nested optional values and loaded NULL after
the source Context is deleted. Nested JSON uses the destination allocator. The
test runs with the default PMR resource disabled. The same source is consumed
through native named modules and through the normal headers.

## Verification

The direct optional comparison passes 1,548/1,548. The new SQLite/lifetime test
passes in native, targeted ASan/UBSan (leak scanning disabled) and named-module
profiles, 1/1 in each. Java generation/layout tests pass 12/12. The original
optional fixture reproduces all 14 files including provenance. All six C++
fixture sets reproduce their 66 files. Both entity-helper corpora replay through
native CTest, 2/2. The 1,406 module exports and seven package/source placement
contracts verify; these structure checks are not semantic acceptance.
Compiler contracts pass 3 positive and 14 negative cases, including optional
timestamp setter/predicate rejection and the preserved existing Q/E and
Business-ID type contracts. Pinned formatting passes for 813 current C++ files.

Reproduce the direct optional comparison with:

```sh
source scripts/toolchains/activate.sh
python3 scripts/generate_entity_references.py --check --fixtures optional_types
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin generated_entity_helpers_5_0_6
cmake --build build_native_current --target teaql_generated_entity_helpers_probe
python3 tests/parity/compare_generated_optional_fields.py \
  --cpp build_native_current/teaql_generated_entity_helpers_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_entity_helpers_5_0_6
```

Inherited/candidate configurations, complete governed save source comparisons,
business workspace scaffolds and later P2-P7 features remain open. These tests
do not establish complete generator parity.
