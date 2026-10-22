# Generated JSON input investigation (Rust 5.0.6)

Status: five boundary contracts qualified; combined payload qualified under an approved exception. Complete source parity remains pending.

The experimental `generated_json_inputs` Rust observer, C++ probe and replay
scripts are retained under `tests/parity/`. The probe now has native and module
build targets. Only the five trust-boundary/alias cases have a retained corpus
and native/sanitizer CTest registration (`generated_json_boundaries_rust_parity`).
The default six-case replay still includes the unresolved combined payload and
has no expected-results corpus. Use `--boundaries-only` explicitly to replay the
five independent contracts; it must not stand in for full input parity.

The initial original-runtime execution rejected the combined SampleRecord input
with `JSON_ENTITY_INPUT: invalid field type: payload`. The failure receipt is
[retained here](parity/generated_json_inputs_initial_original_5_0_6.json.gz).
Investigate the generated Rust field type and metadata before changing the
fixture or interpreting this as a C++ runtime defect. The remaining input
scenarios have not been qualified by this observer.

After resolving the original observation, generate and review the corpus,
compile the C++ probe, compare its outcomes, and register the completed contract.
This investigation does not increase accepted source or capability counts.

## Original-runtime diagnosis

The generated Rust `SampleRecord` declares `payload: Option<serde_json::Value>`
with `#[teaql(large_text)]`. The locked macro's `derive_impl.rs` overrides its
inferred JSON metadata with `DataType::LargeText`. Context JSON decoding therefore
rejects an object before generated hydration. A string passes that metadata
check, but `mapping.rs` requires `Value::Json` to hydrate the generated JSON field
and rejects the resulting `Value::Text`. NULL is accepted. The retained C++
metadata declares `DataType::Json`, so this is an unresolved generated contract
mismatch, not evidence that the general Context JSON parser is wrong.

The initial alias input also used the wrong metadata name: `Record` declares
`type_field` with column alias `legacy_type`. The canonical model name `type`
is not the JSON decoder's metadata name in either retained SDK. Both observers
now use `type_field`, including loaded-state and duplicate-alias checks.

The independent-process diagnostic retains subsequent observations even when
an earlier original case fails. It never rewrites a failed case as accepted or
produces the expected-results corpus used by the qualification replay:

```bash
python3 tests/parity/diagnose_generated_json_inputs_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_json_inputs_5_0_6 \
  --cpp build_native_current/teaql_generated_json_inputs_probe \
  --output docs/parity/generated_json_inputs_diagnostic_comparison_5_0_6.json.gz
```

The initial independent original observations are retained in
[the diagnostic receipt](parity/generated_json_inputs_diagnostic_original_5_0_6.json.gz).
They include the invalid initial alias fixtures; later observations must not
relabel those failed attempts as successful.

## Native comparison results

The [comparison receipt](parity/generated_json_inputs_diagnostic_comparison_5_0_6.json.gz)
records exact equality for five inputs: eleven rejection outcomes, two empty
relationship target checks, and alias batches at 1, 100 and 10000 rows. Alias
observations include shared snapshot identity, distinct omitted-field shapes,
NULL versus omission, and duplicate-alias rejection. The combined input still
differs: C++ accepts and roundtrips the JSON payload while Rust rejects it.

These are native diagnostic observations only. They do not qualify the complete
JSON test source, replace module/lifetime gates, or authorize a generator change.
Resolve the LargeText/JSON contract before claiming full input parity. No runtime
or generated SDK implementation was changed by this investigation.

## Final scoped gate and approved exception

The user approved preserving C++ JSON support on 2026-10-21; the normative
[compatibility exception](json_payload_compatibility_exception_5_0_6.md) resolves
the intended behavior without changing either generated SDK. Earlier diagnostic
statements above describe the investigation before that decision.

The [final receipt](parity/generated_json_boundaries_installed_v2_5_0_6.json.gz)
passes five exact original comparisons and the full expected C++ combined
roundtrip in native, canonical modules and focused sanitizer. Native/sanitizer
each pass two CTests. Generated hashes, canonical consumers, no-work builds and
captured input/reference guards pass. No leak scan or MIPS run was requested.
The full JSON test-source checklist remains pending.
