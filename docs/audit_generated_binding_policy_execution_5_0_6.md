# Generated audit binding policies through SQLite and publication

This checkpoint joins complete, freshly generated inheritance domains to the
locked Rust 5.0.6 runtime and the C++ implementation. It observes the generated
School descriptor, direct safe-audit projection, twelve audited SQLite Save
outcomes, one actual `name LIKE` query, Context SQL diagnostics, and six fresh
processes per language writing ordinary and debug log files. The four model
configurations are separate finite witnesses; they do not qualify all audit
metadata grammar, arbitrary sinks, or complete G14/G17/P0-P7.

The references are Rust runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and original generator `45e1572c4956ca963410d8ea436ead3c8857087a`.
The School audit-length input comes from the locked nested-item source corpus.
For `Unknown`, the earlier
[complete-model source corpus](../tests/parity/default_model_literal_final_inputs_5_0_6.json.gz)
supplies the pattern in which an unknown container assignment stops parsing
before the modeled mask. This execution model adapts that pattern to the
existing `school_data` table and explicitly parsed `sqlite` service, so the
result can execute on the supported provider. The verifier asserts the source
pattern before generating either language.

| Model declaration | Effective School mask state | `name` SQL policy | Direct safe-audit field | Governed Save event field | Ordinary SQL log binding | Authorized debug binding |
| --- | --- | --- | --- | --- | --- | --- |
| `_audit_mask_fields="name"` | Declared `name`; limit 17 | `Masked` | Masked to 17 characters | Same masked value | Protected | Synthetic canary visible |
| `_audit_mask_fields=""` | Declared empty; limit 21 | `Plain` | Synthetic canary visible | Synthetic canary visible | Synthetic canary visible | Synthetic canary visible |
| Attribute omitted from a complete model | Generated as declared empty; limit 21 | `Plain` | Synthetic canary visible | Synthetic canary visible | Synthetic canary visible | Synthetic canary visible |
| Partial container parse before the mask | Undeclared; no parsed limit | `Unknown` | Synthetic canary visible | `[REDACTED]` | Protected | Protected |

The ordinary missing model attribute is **not** an undeclared descriptor in
this generator: its complete template emits `audit_mask_fields=""`. Both
languages therefore classify its `name` binding as `Plain`. Conversely, the
partial parse retains a valid table and SQLite service but never reaches the
mask or audit length, yielding `audit_mask_fields_declared=false` and
`Unknown`. The model still contains `_audit_mask_fields="name"`; raw model
intent does not override the parsed descriptor.

The direct safe-audit call intentionally has no diagnostic-redaction source.
Consequently, its `Unknown` field remains raw in both reference and C++.
The governed Save attaches binding-derived diagnostic redactions; the safe
event sink then receives `[REDACTED]`, retaining raw length 21 and output
length 10. Its field-level `masked` flag remains false and has no mask reason,
because the replacement comes from diagnostic redactions rather than the
configured audit mask. This is an important publication boundary: a caller
must not treat the direct projection alone as proof that every
unknown-provenance value is safe to publish. The SQLite query's safe Context
record and ordinary file mask `Unknown` even with the exact plaintext
acknowledgement. The authorized debug file is created but keeps that binding
protected. `Masked` debug output requires the exact acknowledgement and
enabled destinations. Logging-disabled
and endpoint-off controls produce no ordinary output. The database query
itself still uses parameter binding and returns `[[7, 3, null]]` in every model.

## Receipts and reproduction

| Configuration | Complete execution receipt | Actual file-sink receipt |
| --- | --- | --- |
| Named mask | [Original checkpoint](parity/generated_audit_default_model_current_5_0_6.json.gz) | [Original matrix](parity/generated_audit_default_model_matrix_5_0_6.json.gz) |
| Explicit empty mask | [Complete run](parity/generated_audit_empty_mask_current_v3_5_0_6.json.gz) | [File matrix](parity/generated_audit_empty_mask_matrix_v3_5_0_6.json.gz) |
| Omitted model attribute | [Complete run](parity/generated_audit_absent_mask_current_v2_5_0_6.json.gz) | [File matrix](parity/generated_audit_absent_mask_matrix_v2_5_0_6.json.gz) |
| Partial parse, `Unknown` | [Complete run](parity/generated_audit_partial_unknown_current_v3_5_0_6.json.gz) | [File matrix](parity/generated_audit_partial_unknown_matrix_v3_5_0_6.json.gz) |

Each complete receipt records 41 successful steps, twelve matching original/C++
Save outcomes and twelve file-sink cases. The observations use public synthetic
canaries. Source and generated-output hashes are retained, and task-owned
temporary builds and logs are removed after successful verification. The
full verifier is [reproducible](../scripts/verify_generated_audit_sql_sinks_5_0_6.py).
It creates an owned temporary Rust observer manifest to add a direct dependency
on the already locked `teaql-data-service` crate; the checked-in manifest,
Cargo lock, original generator and generated SDKs are unchanged.

From the repository root, activate the locked toolchain, then run the verifier
with a fresh receipt pair. For the `Unknown` witness:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_audit_sql_sinks_5_0_6.py \
  --source-case audit_value_max_len_0_then_assignment \
  --mask-declaration partial_unknown \
  --record /tmp/audit_unknown_run.json.gz \
  --sink-record /tmp/audit_unknown_sinks.json.gz
```

The other declaration choices are `named`, `empty` and `absent`; `named`
defaults to the source length-17 case, while the others use the length-21 case
above. The script rejects existing receipt paths to prevent overwriting evidence.

Two failed diagnostic receipts are preserved: the initial
[omitted-mask assumption](parity/generated_audit_absent_mask_current_5_0_6.json.gz)
incorrectly expected `Unknown` for a complete generated model, while the
[first partial-parse assertion](parity/generated_audit_partial_unknown_current_5_0_6.json.gz)
incorrectly equated direct projection with the governed Save event. Their
source observations caused the explicit boundaries above; neither is relabeled
as a successful run.

Complete AP02/G14 metadata and privacy publication, arbitrary model inputs,
custom sinks, remaining G17 governance joins and P0-P7 exits remain open.
