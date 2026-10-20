# Ordinary indexed dynamic query decoding (Rust 5.0.6)

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
`teaql-runtime/src/data_service/resolved.rs` and its registry decoder delegate.
This continuation covers the successful carrier branch missing from the earlier
unsupported-carrier and validated-intent guard observations.

## Caller fixture

A handwritten indexed `Item` in each language delegates layout construction,
loaded-state creation, dynamic snapshot merging and runtime-state installation to
the corresponding runtime. It supplies only the Entity hooks needed by that
runtime. It is not generated code or a replacement decoder implementation.

A ready dynamic provider returns definitions for text field `a` and integer field
`b`. The four payload modes are a text value, explicit NULL, an empty map and a
provider error. The provider deliberately omits `b`: its definition alone must not make
`#b` loaded. Explicit NULL for `a` differs from an absent `a` payload. The observations include scalar schemas, loaded flags for both
scalar and dynamic fields (`#a` and `#b`), installed dynamic values, root presence, exact provider
requests and ordered Behavior/Policy callbacks.

The 288 inputs cross those payload modes with the earlier 72 incoming-selection,
callback-selection, empty-root and executor-failure controls. Every input performs
two ordinary typed reads. Provider readiness is intentional: pending-provider
cancellation retains its separate tests and is not proved by this matrix.

## Scope and verification

The [final installed receipt](parity/resolved_dynamic_loaded_installed_5_0_6.json.gz)
records fresh original-source execution, 288/288 matching observations in each
frontend, native5/module5, canonical module consumers and no-work rebuilds.
Source and both frontends satisfy the explicit 30-hydrated-result/40-provider-call
assertions. Retained tests cover the 72 intent guards, 288 callback-selection
cases, 192 typed schemas and Context cleanup. No additional production change
was needed after the earlier intent-guard repair. Formatting, baseline and module
path checks pass. No fresh sanitizer, MIPS or physical-device claim is made.

Historical fixture records follow. The first fixture used `_result` as a fixed field
and omitted `version`; the core rejected that layout before successful decoding.
Its [source receipt](parity/resolved_dynamic_loaded_invalid_layout_source_5_0_6.json.gz)
and [288 observations](parity/resolved_dynamic_loaded_invalid_layout_cases_5_0_6.json.gz)
are retained as invalid-fixture evidence, not acceptance. The corrected carrier
uses fixed `id`/`version` geometry and keeps `_result` as a readonly dynamic
property. The verifier now rejects any caught execution panic in this suite. This fixture does not establish enhanced relation
loading, ID-set/continuous pages, generated-carrier parity, live SQLite storage,
whole-source acceptance or completion of P0–P7.

An intermediate [source replay](parity/resolved_dynamic_loaded_unprefixed_source_5_0_6.json.gz)
observed bare `a`/`b` names, which correctly have no fixed field slots. Its
[observations](parity/resolved_dynamic_loaded_unprefixed_cases_5_0_6.json.gz)
matched both native and module probes but did not prove dynamic selection bits.
The first prefixed assertion incorrectly expected every defined field to become
loaded. The [failed assertion receipt](parity/resolved_dynamic_loaded_selection_assertion_failure_5_0_6.json.gz)
retains the original observations: `DynamicFieldBatch::into_values` delegates to
`DynamicFieldValues::from_batch`, which derives availability from returned keys.
The corrected assertions distinguish a returned NULL from an omitted field.
The final suite observes `#a`/`#b` and asserts 30 hydrated results, exact installed
values, provider-call counts, empty results and error ordering. A readonly
`_result` property still does not acquire a fixed field slot. The existing direct dynamic-row
loader matrix separately qualifies batch validation and partial merges.
