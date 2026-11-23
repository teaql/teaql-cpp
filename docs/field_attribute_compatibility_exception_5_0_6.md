# Column attribute injection compatibility exception

Approved by the user on 2026-11-20. Preserve the explicit C++ rejection of
unescaped column literals that inject derive attributes in the locked Rust
generator.

The reference remains Rust runtime 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The two complete fixtures `column_attribute_alias` and `column_attribute_shape`
insert an unescaped quote into a column literal. The Rust template interpolates
it into a generated derive attribute. The original generated entity modules
compile; their executed descriptors resolve the column as `chosen`, with maximum lengths
100 and 7 respectively. C++ refuses both inputs before publishing generated
files with `Unescaped quote in original Rust column literal`.

The [executed original/refusal receipt](parity/field_attribute_after_reverse_types_property_exception_5_0_6.json.gz)
and [ordinary valid-model control](parity/field_attribute_after_reverse_types_valid_control_5_0_6.json.gz)
remain unchanged. Their recorded `semantic_accepted: false` describes the actual
nonmatching execution. The approval is separate from that historical diagnostic.

The exception permits only this explicit refusal boundary. Properly escaped
column strings and ordinary column/length metadata keep their qualified
behavior. It does not authorize silently dropping attributes, accepting
arbitrary invalid models or inventing a different SQL identifier. The existing
quoted-foreign-key loading exception concerns a different defect and remains
independent.

See the [source and diagnostic history](field_attribute_boundary_5_0_6.md) and
the machine-readable scope in
[`compatibility_exceptions_5_0_6.json`](compatibility_exceptions_5_0_6.json).
Approval resolves the behavior choice; complete generated metadata, protected
consumer integration and final P0-P7 qualification still require their evidence.

The [compiled-reader continuation](compiled_column_boundary_5_0_6.md) enforces
the same refusal when trusted compiled metadata is supplied. It also records
the distinct complete-SDK reference refusal in sample-data code; the earlier
entity-module result must not be relabeled as complete-SDK compilation.
