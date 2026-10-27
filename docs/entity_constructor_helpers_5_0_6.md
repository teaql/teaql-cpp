# Entity construction and inherited scalar helpers: Rust 5.0.6

The later [installed CLI source-binding checkpoint](entity_constructor_cli_source_binding_5_0_6.md)
rebuilds the original Rust observer and verifies all 512 outcomes against
fresh C++ execution. The bounded evidence does not change the wider
qualification status below.

This checkpoint repairs a generated constructor discrepancy and adds G01/G13
evidence. It does not complete G01/P1 or the wider generator goal.

## Original private method access

The Rust observer crate imports six original inheritance_variants entity files
through path attributes. Its small exported functions call those entities'
crate-private runtime_new methods. Original files, method bodies, derive macros,
metadata and runtime remain unchanged. The wrappers grant test access only;
they do not reconstruct the constructor algorithm.

The workspace uses the original runtime lock's registry versions/checksums.
The actual original factories construct Document, fieldless IntermediateRecord,
Invoice, Receipt and third-level SpecialInvoice using a supplied runtime state.
The observer additionally reconstructs loaded rows through actual generated
from_compact_row and recursively attaches them to that state.

## Constructor bug and repair

The first 256-input comparison found 128 differing new-object cases. Required
createTime/updateTime/dateTime fields receive Timestamp::now() in the original
Rust runtime_new; C++ generated constructors instead used zero-valued Timestamp.
All differences were the three before/after/clone views of created/updated time.
The [original differences](parity/entity_constructor_helpers_5_0_6_initial_gaps.json.gz)
remain retained.

CppCoreGenerator now consults the authentic RustFieldDescriptorNaming default
expression and emits Timestamp::now() for those required Timestamp fields.
Optional timestamps remain empty and copy constructors preserve source values.
Row decoding retains its separate field-decoding defaults. The change adds no
new clock implementation, Context capability, dependency or duplicate checker.

The expanded observers share the existing fixed process-wall-clock fixture,
retaining exact timestamps. Monotonic time remains unchanged. Constructor clocks
are distinct from the graph-fix operation clock; constructing an object does not
record fix evidence or mark its fields loaded.

## Scalar and snapshot matrix

All **512/512 original/native outputs match without normalization**. Six field
paths span five concrete models: inherited Document/IntermediateRecord name,
Invoice amount, Receipt status and SpecialInvoice amount/reference. Inputs use
new construction and loaded row reconstruction, two wall clocks, no mutation,
typed update, self update and ledger-only overlays.

Decimal cases retain zero, negative scale, seven fractional digits and the source
96-bit maximum. Text cases retain empty text, trim-sensitive whitespace, Unicode
and embedded NUL. Incompatible/null/typed-null/boolean ledger operands are actual
overlays, preserving conversion fallback rather than inventing setter signatures.

Complete outputs retain ordinary default values, getters, Value/NotLoaded eval,
changed presence/value, availability, original snapshots, independent stored
clone fields with shared ledger observations, and ordered pending field names.
Child-only helpers are actual generated methods. Neither observer issues SQL.

## Verification and artifact controls

Java **44/44**, native **1/1**, module **1/1**, ASan/UBSan **1/1**, and existing
generated API/optional/inherited/example regression **4/4** pass. Sanitizer leak
scanning is disabled. Native observers disable default PMR, explicitly copy
their complete results into Global, destroy stack PMR values, delete UserContext
and only then serialize Global results.

Twenty workspaces/240 files regenerate and reproduce. Forty-four separate SDKs
also strictly reproduce. One timestamp candidate SDK required an initializer
change. Its recording mode first applies only the known required-Timestamp
initializer replacement to the retained model header, formats that expected
result with pinned clang-format, and requires byte equality with real generated
output. Every other file/body stays identical; source/historical artifacts stay
unchanged. See [SDK regeneration](parity/constructor_timestamp_sdk_regeneration_5_0_6.json).

The [terminal record](parity/entity_constructor_helpers_5_0_6.json) records current
profiles and provenance. Canonical verification covers 545 bodies; pinned C++
format/check covers 1,372 files. No full-suite, MIPS or physical-device result is
claimed. Remaining source/interface/configuration closure follows the
[G01 matrix](entity_helper_requirements_5_0_6.md).

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_entity_constructor_helpers_probe teaql_sample_fixed_clock
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
  CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_entity_constructor_helpers_5_0_6/Cargo.toml
python3 tests/parity/compare_entity_constructor_helpers_5_0_6.py \
  --cpp build_native_current/teaql_entity_constructor_helpers_probe \
  --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/entity_constructor_helpers_506 \
  --clock build_native_current/libteaql_sample_fixed_clock.so
ctest --preset native --output-on-failure -R '^entity_constructor_helpers_rust_parity$'
```
