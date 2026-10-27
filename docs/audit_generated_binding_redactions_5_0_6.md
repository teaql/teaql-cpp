# Generated audit metadata through binding redaction

Three source-derived audit metadata inputs now reach the actual Rust and C++
`SqlIntentRedactions::from_bindings` implementations through their generated
entity descriptors. All **42 generated field policies** and their safe/debug
binding capture lists match. This is a finite G14/AP05 consumer join, not SQL
execution, log-sink publication or complete AP02/G17/P0-P7 acceptance.

The reference commits are Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## What executes

The [three generated domains](audit_delegated_nested_callers_5_0_6.md) cover an
ignored cast followed by a real `name` mask, a delegated cast that replaces the
entity identity and mask, and a nested function followed by entity/table/service
overrides. Each original Rust and generated C++ observer obtains the actual
`Platform`, `Item` and `Child` descriptors. For every property, it classifies
the field, gives that policy and a distinct synthetic text binding to
`from_bindings`, then reads the capture lists with `allow_plaintext=false` and
`allow_plaintext=true`.

| Generated property | Effective policy | Safe capture | Plaintext-enabled capture |
| --- | --- | --- | --- |
| First domain: `Item.name` | Masked | Synthetic name value | Empty |
| All three domains: `Child.name` | Masked | Synthetic name value | Empty |
| All three domains: `Child.secret` | Credential | Synthetic secret value | Synthetic secret value |
| Override domains: `Witness.name` | Plain | Empty | Empty |

Across all 42 property bindings, **7** safe captures and **3** forced captures
in the plaintext-enabled path agree. The verifier compares each policy to the
original source and checks its expected capture behavior, so matching but
incorrect capture lists cannot pass solely because they agree. The `safe`
and `plaintext-enabled` names describe arguments to the redaction SPI; no file
sink is opened by this gate. A synthetic `SELECT ?` string is provenance input
to `from_bindings`, not an executed statement.

## Reproducible execution and ownership

The [current receipt](parity/audit_delegated_nested_binding_redactions_5_0_6.json.gz)
records three fresh original and C++ generations, independent Java source
compilation, three original Rust entity builds/runs and the generated C++
observer build/run. All **526** input hashes remain unchanged, every command
exits successfully, all **9** descriptor objects and **42** binding rows match
under the strict JSON comparator, and the owned **174,786,739-byte** build
directory is removed. Native execution disables default PMR and destroys each
`UserContext`, checking zero live user bytes.

The new observer sources are
[Rust](../tests/parity/audit_binding_rust_main_5_0_6.rs)
and [C++](../tests/parity/audit_binding_cpp_observer_5_0_6.hpp).
They are selected only by `--binding-redactions`; existing descriptor-only
compiler comparisons keep their previous mode. The parser, runtime, generated
SDKs and module exports are unchanged. The pinned C++ formatter and check pass
for all 1,810 handwritten files.

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_compiler_5_0_6.py \
  --binding-redactions \
  --corpus tests/parity/audit_delegated_nested_caller_models_5_0_6.json.gz \
  --cases nested_item_cast_and_mask delegated_cast_and_overrides \
          nested_signature_and_overrides \
  --record docs/parity/audit_delegated_nested_binding_redactions_5_0_6.json.gz
```

Complete modeled configurations, actual SQL/provider execution, privacy
publication, the column-attribute compatibility decision and G13/G14/G17/P0-P7
signoff remain open at **14 qualified / 3 partial** capabilities.
