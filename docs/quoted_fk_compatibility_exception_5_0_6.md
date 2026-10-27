# Quoted foreign-key compatibility exception (Rust 5.0.6)

Status: approved by the user on 2026-10-24. Preserve correct C++ relationship
loading and record the reference discrepancy explicitly.

The locked runtime is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
is `45e1572c4956ca963410d8ea436ead3c8857087a`. The approval covers the same
quoted/raw column identity defect in relationship attachment and direct typed
foreign-key decoding; it does not change either reference.

## Confirmed reference behavior

The generated Line descriptor stores its foreign-key column as the quoted SQL
token `"order"`. SQLite returns the physical column label `order`. Reference
attachment compares those different strings and produces zero or empty
relationship results. Current C++ descriptors use the physical name for row
lookup and quote it through the SQL dialect when constructing SQL.

The [relationship witness](parity/aggregation_quoted_key_current_witness_5_0_6.json.gz)
retains identical SQL alongside different attached values. Of 7,122 relationship
observations, 6,576 match and 546 expose this defect. The
[raw-metadata source control](parity/aggregation_raw_metadata_bridge_results_5_0_6.json.gz)
matches all 7,122 C++ observations by changing only the public source metadata
input. That experiment remains a metadata control, not an unchanged-source
execution claim.

The [direct decoder witness](parity/generated_member_alias_quoted_witness_5_0_6.json.gz)
retains twelve scalar/object/NULL inputs and five differences. The
[preservation control](parity/generated_member_alias_quoted_preservation_5_0_6.json.gz)
confirms that the later scalar member-alias correction preserves these outcomes.
Availability and decoded values are checked separately; NULL is not NotLoaded.

## C++ contract and bounds

Use actual physical column names for typed row lookup, relation keys, aggregate
attachment and availability aliases. SQL quoting belongs to SQL construction.
Keep canonical model/property identities and ordinary field-type validation.
A quoted SQL token is not automatically another physical column alias.

The approval covers this identifier-representation discrepancy. Other query,
filter, grouping, ordering, NULL/type, policy, privacy, ownership and error
contracts retain their independent requirements. It does not permit arbitrary
zero/empty source results to be replaced or unknown result differences to pass.

The [exception manifest](compatibility_exceptions_5_0_6.json) binds both locks,
the immutable evidence hashes and the exact 546 relationship case indices.
Current acceptance must compare every complete C++ observation with its retained
explicit expectation and preserve all original observations. Report ordinary
source matches and approved exceptions separately. A raw-metadata control is
additional support; it does not overwrite the frozen Rust results.

The [current CLI source binding](relation_analysis_cli_source_binding_5_0_6.md)
enforces that split for the actual registered relationship-analysis command.
The current debug adapter rewrites only the exact generated `order_id`
`PropertyDescriptor` context. It retains all **424** approved relationship
descriptor changes and **189** dynamic-JSON descriptor changes while preserving
ordinary query text with the same column marker. The
[post-edit compatibility replay](parity/quoted_fk_scoped_adapter_compatibility_5_0_6.json.gz)
checks the complete **6,576 + 546** relationship split, eight invalid manifest
variants and three corrupted result variants. The
[dynamic-JSON adapter controls](parity/dynamic_json_adapter_scope_post_edit_5_0_6.json.gz)
check **18** boundaries, including a formerly accepted ordinary-text rewrite.
The **15/15** selected CTests pass across relation, dynamic-JSON execution,
policy and logging. This is scoped current-source evidence; the preceding
complete native epoch predates these comparator edits.

This settles AG07 and the additional quoted-FK portion of LS10. Complete G08/G15
responsibility signoff and their required current gates remain separate work;
approval alone is not capability or P0–P7 completion.
