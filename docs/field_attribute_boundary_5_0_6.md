# Column literal and derive-attribute boundary

This is a diagnostic finding against runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator commit
`45e1572c4956ca963410d8ea436ead3c8857087a`. It does not qualify a new
compatibility exception or complete G14.

The locked `RustFieldDescriptorNaming.rustFieldAttr` inserts the configured
column into a Rust string literal without escaping embedded quotes. The Rust
derive parser then consumes the resulting attribute tokens. A repeated `column`
overwrites the first column, and an inserted `max_length` changes the descriptor.
This crosses the boundary between a column value and generated program syntax.

Two complete model inputs in
[`field_attribute_models_5_0_6.json.gz`](../tests/parity/field_attribute_models_5_0_6.json.gz)
demonstrate the result. XML uses `&quot;` to represent each quote below.

| Configured column value | Actual Rust column | Actual Rust maximum length |
| --- | --- | --- |
| `first",column="chosen` | `chosen` | 100 |
| `first",max_length=7,column="chosen` | `chosen` | 7 |

Both the ordinary Item field and the inherited Child field have these outcomes.
The original generator succeeds, the generated entity modules compile against
the locked runtime, and execution reads the actual derived descriptors. This
probe compiles the original entity modules, not the entire original SDK.

Current C++ generation rejects both inputs with
`IllegalArgumentException: Unescaped quote in original Rust column literal`.
The diagnostic controller requires that exact refusal and still builds and runs
the original entities. Its receipt explicitly records `semantic_accepted: false`;
successful diagnostic execution is not a matching C++ result. No production
generator or runtime behavior was changed for this finding.

The [diagnostic receipt](parity/field_attribute_diagnostic_5_0_6.json.gz)
retains the complete models, original generated and observer sources, commands,
refusal output, exact reference revisions and unchanged input hashes. A separate
[ordinary-model control](parity/field_attribute_default_control_5_0_6.json.gz)
runs both original and C++ compilation/execution successfully after adding the
optional refusal mode. Their owned build directories were removed: 165,591,717
and 168,359,496 measured bytes respectively. Shared caches and stable build
presets were retained.

## Current candidate verification

The [fresh property-descriptor replay](parity/field_attribute_candidate_property_exception_5_0_6.json.gz)
uses the current compiler controller, property observer and generator sources.
Both locked models compile and execute with the original generator. The actual
Item and inherited Child `name` fields have column `chosen`; their maximum
lengths are **100** for the repeated-column case and **7** for the injected
length case. Their other fields remain in the complete 12-property-per-model
observations. C++ generation rejects both models with the exact unescaped-quote
diagnostic. The receipt deliberately retains `semantic_accepted=false`.

The [fresh valid-model control](parity/field_attribute_candidate_valid_control_5_0_6.json.gz)
compiles and executes original and C++ generated entities under the same source
epoch. All **24 property descriptors** match exactly, including column, type,
nullability and length/precision/scale. The
[candidate boundary verifier](../tests/parity/verify_field_attribute_exception_5_0_6.py)
checks current source hashes, generated file contents, both actual original
descriptor results, exact C++ refusal and the valid-model equality. Its
[control receipt](parity/field_attribute_exception_controls_5_0_6.json.gz)
rejects **13/13** corruptions. The [focused Java gate](parity/field_attribute_candidate_java_gate_5_0_6.json.gz)
passes **8/8** tests after rebuilding model and generator together. This is a
reviewable candidate boundary, not an approved compatibility exception. The
three probe-owned build directories were removed after recording approximately
503 MB of disposable output; stable presets and shared caches remain.

The compatibility decision is pending. The proposed boundary is to preserve
explicit C++ rejection rather than interpret column contents as additional
attributes. Existing approved JSON, quoted-foreign-key and invalid wire-naming
exceptions do not authorize this separate case. Valid escaped column literals
remain covered by [the earlier column metadata checks](field_column_model_metadata_5_0_6.md).

## Current-source requalification

The preceding candidate receipt became stale when the shared compiler
controller changed after that source epoch. The old verifier correctly rejected
its input hash. Fresh [diagnostic execution](parity/field_attribute_current_property_exception_5_0_6.json.gz)
rebuilds the complete original and C++ generator Java sources and compiles both
original entity models. Item and inherited Child again derive column `chosen`
with maximum lengths 100 and 7 respectively; current C++ generation rejects
each input with the exact unescaped-quote diagnostic. This diagnostic has
`semantic_accepted=false`.

A separate [current valid-model execution](parity/field_attribute_current_valid_control_5_0_6.json.gz)
generates and runs both languages and matches all 24 property descriptors.
Both receipts bind 524 unchanged inputs and remove their owned build trees
after execution (166,418,924 and 169,186,703 bytes). The
[current default verifier](../tests/parity/verify_field_attribute_exception_5_0_6.py)
checks those receipts and rejects [13/13 controlled corruptions](parity/field_attribute_current_exception_controls_v2_5_0_6.json.gz).
The earlier receipts remain historical. User approval of this separate
compatibility exception is still pending, so G14/AP02 and P0-P7 remain open.
