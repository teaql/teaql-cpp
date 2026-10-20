# Complete expression diagnostic framing

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the declared framing matrix passes; full G11/P6/P7 remains open.

## Removing the raw-field parser boundary

The earlier [text matrix](expression_diagnostic_text_5_0_6.md) retained original
panic strings but used field-label extraction during comparison. That fixture
excluded payloads containing the label delimiters themselves. The new comparator
compares complete original panic strings and does not parse their fields.

The original producer function and observer remain unchanged. The new adapter
consumes actual native diagnostic data and renderings, then reverses the already
documented presentation adaptations: ASCII method casing, nested `selectWith`
syntax and English human-message presentation. Root arguments and broken-point
markers are retained from the native rendering; they are not regenerated from
input to conceal a mismatch.

Getter tokens are consumed from the end in path order. Nested selections consume
their actual typed-placeholder/method tokens in order. This avoids replacing a
method-looking string inside root arguments or opaque field names. It also
distinguishes paths whose canonical spellings produce the same C++ method casing.
Missing tokens, unexpected fields or malformed native closing delimiters fail
the comparison rather than being repaired.

## Actual observations and controls

All **1200/1200** complete frames agree with unchanged original Rust. The original
**936** cases and raw expected outputs are retained verbatim as a prefix, and
**264** added inputs embed diagnostic labels and method-looking text in roots,
missing nodes and paths. Inputs include repeated/empty segments and both
`foo_bar` and `fooBar`. The raw source remains deliberately unescaped; native
JSON uses its existing safe serialization path.

The comparison has one positive and seven synthetic negative controls. Removing
a broken marker, changing root arguments/preload/severity/suggestion/English
message, or adding an unexpected native field must fail. The controls execute
inside the committed comparator. They are adapter checks, not additional runtime
or Rust observations. The
[control record](parity/expression_diagnostic_framing_5_0_6_adapter_controls.json)
retains the independently executed checks.

The [terminal record](parity/expression_diagnostic_framing_5_0_6.json) retains
actual CTest output, source hashes and live reproduction. Relevant native old/new
text checks pass **2/2**, the final comparator-control replay passes, and the
module consumer passes **1/1**. The existing 139 canonical consumer bodies verify.
No handwritten C++, generated artifact or lifetime behavior changed, so no
sanitizer, MIPS or additional compiler build was run for this test-only extension.

## Reproduction

Use the already qualified `teaql_expression_diagnostic_text_probe` and original
`expression_diagnostic_text_5_0_6` observer, then run:

```sh
python3 tests/parity/generate_expression_diagnostic_framing_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/expression_diagnostic_text_5_0_6 \
  --cpp build_native_current/teaql_expression_diagnostic_text_probe --check
python3 tests/parity/compare_expression_diagnostic_framing_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/expression_diagnostic_text_5_0_6 \
  --cpp build_native_current/teaql_expression_diagnostic_text_probe
ctest --test-dir build_native_current --output-on-failure \
  -R '^expression_diagnostic_(framing|text)_rust_parity$'
```

The module test is `module_expression_diagnostic_framing_rust_parity` and reuses
`teaql_module_expression_diagnostic_text_probe` in `build_modules_native`.

This resolves the declared source field-label delimiter comparison gap. It does
not introduce a general panic parser or automatic executable query repair.
Broader generated model/Assist/sample/extension qualification and all full
capability/phase exit criteria remain required.
