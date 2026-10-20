# Generated E scalar types and strict field decoding: Rust 5.0.6

Status: the 24 original decoder discrepancies are repaired. All 816 expanded
original/native cases and the declared integration gates pass.
G11/P6/P7 and complete generator acceptance remain open.

Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Original source and access adaptation

The observer imports complete unchanged generated entity, entity-expression,
list-expression and E files from the retained ordinary and optional type fixtures.
Its independent manifest uses reference dependency versions and registry checksums.
The full original workspaces retain their request/runtime compilation failures;
this expression-only entry does not repair or qualify those scaffolds.

Both original E files contain byte-identical scalar ValueExpression and diagnostic
functions. The test assembly binds these common helpers at crate root, aliases
the optional E entry and retains both complete original E files. No original
expression, diagnostic, conversion or entity body is copied into an observer
algorithm or edited. Original manifests and model/file fingerprints are checked.

## Actual gap and runtime repair

The [initial comparison](parity/expression_model_types_initial_5_0_6.json.gz)
retains all 120 actual original/native outcomes: 96 agree and all 24 TypedNull(Text)
inputs differ. Rust reports invalid fields; C++ previously supplied defaults or
optional NULL. An earlier observer failure on that decoder error is retained in
[the initial validation record](parity/expression_model_types_initial_validation_5_0_6.json.gz).

Rust's entity macro mapping distinguishes missing/untyped Null from TypedNull.
It also uses narrower field-specific conversions than general Value try methods.
The new shared GeneratedScalarCodec implements supported scalar field conversion
and allocator-aware defaults in runtime. Generated support functions delegate
through the same existing signatures. General Value conversions are unchanged.

The mapped branches include bool from Bool or integer zero/one; checked integer
conversions; Decimal from supported numeric variants using original display-to-parse
semantics; different f32/f64 accepted inputs and f32 range rejection; Date-only
values; Timestamp and signed/unsigned millisecond inputs; Text and JSON-only
values; optional wrapping and explicit allocation into the destination scope.

## Declared execution matrix

Person, Reading and Device supply 24 scalar access paths: mandatory and optional
text, bool, Decimal, f32, f64, signed integers, IDs, Date, Timestamp, JSON and
foreign-key storage. Each path executes eval, unwrap, eager/lazy/default NULL
fallbacks and records lazy callback counts. The original 120 inputs and outputs
remain a prefix of the expanded conversion matrix.

Additional inputs cross actual Value variants, numeric limits, unsigned timestamps,
non-finite/overflowing floating values and invalid field types. Decoder rejection
is observed before entering E; no rejected case counts as five expression reads.
Missing-load calls retain complete original panic frames. The comparator maps
only already documented C++ naming/English presentation and the exact generic
scalar-null error message; all raw original and native results are retained.

Native observations disable the default PMR resource. Every owning input/result
leaves scope before actual UserContext destruction, and live User arena bytes must
return to zero for each case. No provider is installed, and E introduces no SQL.

## Completed gates and retained evidence

The expanded matrix matches **816/816** actual original/native outcomes: **1520**
scalar resolutions and **512** field-decoder rejections. The complete original
120-input corpus and its original results remain unchanged as a checked prefix.
Successful JSON-null Values, optional NULL, unloaded fields and rejected TypedNull
remain distinct. Numeric kinds and floating negative zero compare strictly.

Relevant native integration passes **11/11**, targeted ASan/UBSan **2/2** with
leak scanning disabled, and the module-only matrix plus import contracts **2/2**. Three positive and
two negative expression-related compiler contracts pass. Both refreshed entity
helper binaries reproduce **3364/3364** unchanged original observations, including
recursive attachment and optional helpers. Java tests pass **53/53**.

All **23 workspaces / 276 files** and **44** complete contract SDKs strictly
reproduce. The [SDK report](parity/scalar_codec_sdk_regeneration_5_0_6.json) records
30 genuinely regenerated shared support headers; every other generated code file,
guidance file and original Rust artifact remains unchanged. Formatting checks
**1409** handwritten C++ files. The export catalog verifies **1483** declarations
across seven modules, and **603** canonical consumer/fixture bodies verify.

The [terminal record](parity/expression_model_types_5_0_6.json) retains actual
outputs, source hashes and validation scope. No MIPS or physical-device execution
is claimed. These gates qualify the declared decoder/E matrix and preserve old
contracts; they do not qualify every model extension or all generator phases.

## Reproduction

```sh
set -e
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
cargo build --offline --locked \
  --manifest-path tests/parity/rust_expression_model_types_5_0_6/Cargo.toml
cmake --build --preset native --target teaql_expression_model_types_probe
python3 tests/parity/compare_expression_model_types_5_0_6.py \
  --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/teaql_expression_model_types_506 \
  --cpp build_native_current/teaql_expression_model_types_probe
```

Remaining complete model/extension/Assist and P6 assembly responsibilities retain
their original owners and acceptance criteria. This matrix alone does not close
G11, complete model/configuration integration under G13 or the full migration goal.
