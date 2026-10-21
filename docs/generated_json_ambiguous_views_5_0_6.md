# Generated JSON ambiguous views and inferred keys

Status: both contracts qualified in native, canonical modules and focused
ASan/UBSan; the complete JSON test source remains pending.

This checkpoint adds two actual-source observations for the still-pending
`teaql-runtime/src/context/json/tests.rs` review. It uses unchanged generated
SampleRecord/SampleLink models from the retained sample_fields_runtime domain in
both languages, rather than substituting a locally implemented graph resolver.
No generator or generated artifact is modified.

The locked Rust observer is
`tests/parity/rust_field_layout/src/bin/generated_json_ambiguous_5_0_6.rs`.
The C++ counterpart is `tests/parity/generated_json_ambiguous_probe.cpp`.
The replay script asserts the original outcomes independently and compares the
complete retained output without normalization.

## Ambiguous identity lookup

A SampleLink has first_record_id and second_record_id both equal to 7. Its first
forward projection loads SampleRecord.label; its second projection loads only
identity. Each explicit relation must return its own view with the corresponding
loaded-field state. Identity-only lookup for SampleRecord 7 must return no entity,
because identity cannot choose a projection. Root JSON and both separately read
child JSON values are observed, preserving both views exactly.

This directly exercises the responsibility of the original
`two_relations_to_equal_identity_keep_distinct_views_and_refuse_ambiguous_lookup`
test using a retained generated domain with two forward edges to one target type.
The original JsonChild/GraphParent naming and nullability fixture remains a
separate part of whole-test-source consolidation.

## Contradictory inferred keys

A SampleRecord has two first_links projections with the same SampleLink identity.
Both have the same explicit first_record_id, but their second_record projections
infer IDs 7 and 8. Neither input supplies second_record_id directly. Decoding must
reject conflicting native values for the repeated identity. The complete error
comes from the actual Context decoder and is compared unchanged across languages.

This closes the missing generated control identified for
`inferred_foreign_keys_cannot_conflict_across_views_of_one_identity`; it does not
turn one successful source case into complete JSON test-source acceptance.

## Verification evidence

Native, canonical imported-module and focused ASan/UBSan compare both original
outputs successfully. Native/sanitizer CTest also passes the retained corpus.
Original and C++ generated files match their manifests throughout; no-work builds,
canonical-body verification and captured-input/reference guards pass. Leak
scanning remains disabled. The complete receipt is
`parity/generated_json_ambiguous_installed_5_0_6.json.gz`.
Source/capability totals remain unchanged until a whole acceptance unit is proven.
