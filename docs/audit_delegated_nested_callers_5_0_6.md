# Generated callers for delegated and nested audit metadata

Three locked Rust 5.0.6 metadata inputs now execute through complete original
generated entities and the current generated C++ facades. All **9 entity
descriptors** and **42 field-privacy classifications** match. This joins three
previously parser-only cases to actual generated model callers; it does not
qualify complete AP02 grammar, governed SQL execution or G14/P0-P7.

The reference commits remain runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Selected source effects

The [three-case input](../tests/parity/audit_delegated_nested_caller_models_5_0_6.json.gz)
records each selection's exact source corpus name and hash. Its expected parser
effects come from the permanent original-source corpora; the compiled gate
independently observes complete generated descriptors and privacy policies.

| Case | Item descriptor observed in both implementations | Inherited Child check |
| --- | --- | --- |
| Nested item cast and later mask | Limit `17`; mask `name`; the `display_name` binding is `Masked` | Inherits the masked `name`; `secret` remains `Credential` |
| Delegated cast and overrides | Entity `Witness`; limit `73`; mask `safe` is an unknown field and `name` stays `Plain` | Retains its separate inherited `name` mask and `Credential` secret |
| Nested function and overrides | Entity `Witness`, table `witness_data`, service `local`; limit `113`; mask `safe` leaves `name` `Plain` | Retains its own SQLite service and inherited mask/credential policies |

The service replacement above is a descriptor result. This gate does not
register or execute a `local` SQL service. It also does not equate an unknown
mask name with authorization or sensitive-data protection.

## Executed gate and limits

The [compiler receipt](parity/audit_delegated_nested_callers_5_0_6.json.gz)
records all 524 input hashes and the generated source hashes.
It compiles the complete original and producer Java sources independently,
generates three full Rust/C++ domains, compiles and executes all three original
Rust entity observers, then compiles and executes one native observer over the
generated C++ facades. Every child process exits successfully; the strict
JSON comparator reports no differences. Native execution disables default
PMR allocation and destroys each `UserContext`, checking zero live user bytes.

The generated domains contain 26 original files and 52 C++ files each. These
are retained as hashes and source text in the compressed receipt; the owned
**174,397,986-byte** build directory was removed after all children exited.
The implementation parser, runtime, committed generated SDKs and module
exports remain unchanged. The separate permanent
[79,346-case source gate](audit_delegated_nested_checkpoint_5_0_6.md) and
prior full native **1,058/1,058** pass keep their original input epochs.

The focused command is:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_compiler_5_0_6.py \
  --corpus tests/parity/audit_delegated_nested_caller_models_5_0_6.json.gz \
  --cases nested_item_cast_and_mask delegated_cast_and_overrides \
          nested_signature_and_overrides \
  --record docs/parity/audit_delegated_nested_callers_5_0_6.json.gz
```

Complete modeled configurations, protected sinks, persistence, column-attribute
compatibility and all P0-P7 exits remain open. Capability status remains
**14 qualified / 3 partial**.

The later [binding-redaction continuation](audit_generated_binding_redactions_5_0_6.md)
rebuilds these three domains and checks all 42 generated field policies through
the original/C++ `from_bindings` implementations. It remains distinct from SQL
execution and log-sink publication.
