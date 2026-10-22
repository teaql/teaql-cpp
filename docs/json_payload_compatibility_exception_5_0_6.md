# JSON payload compatibility exception (Rust 5.0.6)

Status: approved by the user on 2026-10-21. Preserve C++ JSON support and record
this exception explicitly; do not reproduce the reference defect.

## Scope and evidence

The locked generator is `45e1572c4956ca963410d8ea436ead3c8857087a` and the runtime
is Rust v5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The retained
`sample_fields_runtime` model declares `payload="jsonMe()?"`. Rust generates
`Option<serde_json::Value>` together with `#[teaql(large_text)]`.
`RustFieldDescriptorNaming.rustFieldAttr` emits that attribute for LARGE_TEXT
SQL storage or `isText`; the runtime macro overrides the JSON type with
`DataType::LargeText`. Context accepts only strings for that metadata, whereas
the generated field conversion accepts only `Value::Json` for non-null values.

The [original diagnostic](parity/generated_json_inputs_diagnostic_original_5_0_6.json.gz)
records acceptance of NULL, rejection of an object during Context input
validation, and rejection of both JSON-encoded and plain strings during typed
hydration. The [native comparison](parity/generated_json_inputs_diagnostic_comparison_5_0_6.json.gz)
retains the combined input's Rust failure and C++ successful roundtrip. Original
source and generated artifacts remain unchanged.

## C++ contract

A generated JSON-valued field keeps `DataType::Json` for runtime decoding and
its generated JSON C++ type. A text-oriented physical SQL representation does
not turn its runtime JSON descriptor into `LargeText`. Nullable fields accept
JSON NULL; non-null supported JSON values retain their JSON structure through
hydration and serialization. This exception does not relax unknown-field,
metadata identity, Context lifetime, graph, or mutation authority checks.

The exception is limited to this generated JSON/text-metadata conflict. It does
not authorize ignoring other metadata discrepancies or changing the locked
Rust baseline. Runtime `LargeText` fields that are actually strings retain their
existing contract.

## Qualification and reporting

Keep the failing original observation as evidence. Verify C++ behavior with
explicit expected JSON and roundtrip assertions, alongside the unaffected
exact Rust comparisons. Label the combined generated payload as an approved
compatibility exception, never as an exact Rust match. Approval settles the
intended behavior; it does not by itself satisfy the remaining compiler,
module, lifetime, generated reproduction or complete source acceptance gates.
