# Invalid wire-naming compatibility exception (Rust 5.0.6)

Status: approved by the user on 2026-10-24. Preserve explicit C++ rejection of
unsupported `json_field_naming` configuration.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

The [original inspection](parity/generated_wire_mappings_profile_inspection_5_0_6.json.gz)
records successful Rust generation with empty wire strings and no diagnostic
for an invalid profile. The shared model helper rejects the unsupported value,
but the Rust template suppresses its getter failure. C++ propagates the explicit
configuration error rather than emitting empty mappings.

The C++ contract retains `camelCase`, `snake_case`, `PascalCase` and existing
default/blank-profile behavior. Unsupported profiles fail generation with the
`Unsupported json_field_naming` diagnostic and identify the invalid value.
There is no fallback spelling or successful empty-map claim. This exception is
limited to the invalid naming configuration; all valid own-field mappings,
inheritance boundaries, aliases, JSON decoding and Context allocation contracts
remain subject to their existing source comparisons.

The [exception manifest](compatibility_exceptions_5_0_6.json) binds the approval
to the exact references and original inspection hash. Retain the original
successful empty-wire artifacts and the C++ rejection as different outcomes.
The permanent generator test additionally exercises three unsupported values;
the existing test covers all three supported profiles.

This settles R08/AP03's behavior decision. Complete G12/G14 signoff, fresh Java
results and the remaining phase requirements retain their own acceptance gates.
The earlier approved JSON payload exception remains independent.
