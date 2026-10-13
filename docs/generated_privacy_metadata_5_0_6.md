# Generated audit metadata against the 5.0.6 producer

Status: G14/P1 scoped metadata comparison passes; complete qualification remains open. Runtime
reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; producer:
`45e1572c4956ca963410d8ea436ead3c8857087a`. Complete runtime privacy and G14
acceptance are not established by metadata emission checks.

## Resolve inherited masks at their declaring model

The original `ObjectDescriptor.getAuditMaskFields()` resolves each model's own
mask names against its predefined fields. It inherits only the parent's resolved
fields. `FieldDescriptorNaming.normalizedModelFieldName` normalizes case,
camelCase boundaries, punctuation and repeated separators for this lookup;
it does not change emitted model keys. The Rust entity template receives the
resolved model keys through `ObjectDescriptorNaming.canonicalAuditMaskFields`.

The C++ adapter previously collected raw names recursively before resolving the
child's fields. A parent mask for nonexistent `later_secret` therefore became
effective when a child introduced that field. The original producer discards
that name at the parent boundary. C++ now follows the same boundary, while a
child or grandchild can explicitly declare its own mask for an available field.

The regression uses parent `secret_value`, unknown parent `laterSecret`, child
`later_secret` and `child_secret`, and an explicit grandchild `LATER_SECRET` mask.
It failed for Child before the fix and passes afterward. The full generator
regression passes **28/28**. No runtime policy bypass or implicit ownership
domain was added.

## Original model and template observations

`scripts/verify_generated_privacy_metadata.py` builds the unchanged original
producer and the independent C++ implementation in their respective workspaces.
It creates temporary models and output trees, calls the original descriptor
methods, renders the actual original Rust templates, and compares the C++
descriptor emission. It does not patch generated files or relabel older corpora.

All **12** models / **48** entity metadata records match, including a fresh `--check` replay of the original template mask and audit-limit attributes. The matrix includes absent and unknown masks, duplicates, punctuation/case
normalization, three inheritance levels, ID/version and relationship fields,
default table/service identity and declared audit value limits. Original model
keys and storage keys are recorded separately. C++ descriptors name their
storage members, including relationship ID members; the comparison obtains
that mapping from the original `RustFieldDescriptorNaming` rather than guessing
that `platform` and `platform_id` are equivalent. The original template must
emit exactly the recorded model mask keys, in the original field order.

The corpus records both baseline commits, the complete model inputs and hashes,
and the original source hashes. Reproduce it with:

```sh
python3 scripts/verify_generated_privacy_metadata.py
python3 scripts/verify_generated_privacy_metadata.py --check
```

This checks resolved metadata and emitted descriptors. It does not execute the
Rust derive macro or governed SQL/audit/log operations. Explicit audit-limit
validation failures, nondefault storage identities, broader model families and
complete runtime privacy/provenance remain open. All 18 generated workspaces / 216 files were refreshed and reproduce after the fix. The complete native build exits successfully; 11 maintained native fixture contracts pass after request-intent and root-bootstrap repairs. Full native CTest remains a separate failing integration gate, including historical 5.0.5 corpus rejection. These compiler/fixture results do not establish complete runtime G14 privacy acceptance.
