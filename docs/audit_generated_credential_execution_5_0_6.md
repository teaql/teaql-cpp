# Generated Credential field through SQLite and log sinks

This finite G14/G17 checkpoint carries a generated `School.password` field
through a real SQLite `password LIKE` query and ordinary/debug file sinks.
The locked references are Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and original generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. It extends the earlier
[four mask-state configurations](audit_generated_binding_policy_execution_5_0_6.md)
without changing or relabeling their receipts.

The verifier creates an owned copy of the inheritance model. It adds
`password="string()"` to School, leaves `_audit_mask_fields="name"`, and uses
the locked nested-item corpus's last-assignment audit-length input. The
generated School descriptor reports limit 21, `name` policy `Masked`, and
`password` policy `Credential` in both languages. The added field is modeled
in the temporary original and C++ generations; neither checked-in generated
SDK nor generator implementation is edited.

The task-owned SQLite fixture adds the `password` column to School and its
inherited tables and sets row 7 to `PRIVATE-PASSWORD-CANARY`. This value differs
from the existing `PRIVATE-SCHOOL-CANARY` in `name` and query intent. The
separation prevents `name` masking from serving as the sole explanation for
the protected password binding. Both generated queries execute the same
`password LIKE` filter and return `[[7, 3, null]]`. Their complete Context SQL
records match. The actual ordinary and authorized debug files contain
`password LIKE '[REDACTED]' /* masked */`; neither contains the password
canary. Exact acknowledgement, missing debug destination, endpoint-off and
logging-disabled controls execute in fresh processes.

The initial execution also passes twelve exact original/C++ audited SQLite
Save outcomes for the existing `name` mutation. Its direct and delivered
safe-audit value remains masked to length 21. That receipt does not claim a
password mutation audit route; the separate continuation below executes it.

The [complete execution receipt](parity/generated_audit_credential_field_current_v3_5_0_6.json.gz)
records 41 successful steps, twelve Save pairs, a matching generated query,
44 original Rust and 98 C++ generated files, 533 unchanged input hashes,
unchanged dependency hashes and removal of the 397,952,593-byte owned build.
The [file-sink receipt](parity/generated_audit_credential_field_matrix_v3_5_0_6.json.gz)
records all twelve original/C++ fresh-process cases and removal of the
24,772-byte temporary log set. The earlier successful
[same-canary pilot](parity/generated_audit_credential_field_current_5_0_6.json.gz)
is kept as preliminary evidence; its common value with `name` does not provide
the independent-binding control used in the final run. The
[pre-format independent run](parity/generated_audit_credential_field_current_v2_5_0_6.json.gz)
passed with the same value separation, but its C++ metadata observer was
subsequently formatted; the final run pins the current source bytes.

## Audited password mutation

The [password Save continuation](parity/generated_audit_credential_save_current_v2_5_0_6.json.gz)
uses the same generated model and independent fixture but adapts only owned
observer copies to call `School.updatePassword` / `School.update_password`.
It changes row 7 from `PRIVATE-PASSWORD-CANARY` to
`UPDATED-PASSWORD-CANARY`. The application audit reason and readback select
refer to `password`, and the event sink observes its `password` field. The
checked-in original and C++ observer sources stay unchanged.

All twelve Save modes match exactly between locked Rust and C++:
successful single-entity and School/Enrollment graph saves persist the new
password at version 4; write failure, policy denial and child conflict leave
the initial password at version 3; a stale School version leaves the initial
password with the external version 4. The safe-event delivery-error route
reports an error after commit while the new password and attempted event
remain. Approved plans retain their separate approval evidence.

Every delivered School `password` safe field has value `[REDACTED]`, raw
length 23, output length 10, `masked=true`, no truncation and mask reason
`_audit_mask_fields`. The original `build_safe_audit_field` treats credential
field names as forced masks even though this model's explicit mask list only
contains `name`; C++ matches that behavior. The observed password safe field
does not expose the updated value. The receipt records 41 successful steps, 533 unchanged
inputs and removal of the 397,886,500-byte owned build. Its separate
[twelve-case query file-sink receipt](parity/generated_audit_credential_save_matrix_v2_5_0_6.json.gz)
retains the independent password-binding controls and removes the 24,772-byte
temporary log set. The first
[diagnostic attempt](parity/generated_audit_credential_save_current_5_0_6.json.gz)
matched source/C++ Save results but incorrectly expected `masked=false`;
it remains a failed assertion, not a pass.

## Mutation SQL file publication

The [final mutation-sink execution](parity/generated_audit_credential_mutation_sinks_current_v2_5_0_6.json.gz)
reruns the generated password update in four fresh processes: Rust and C++
with the plaintext acknowledgement unset, then each with its exact value and
an explicit SQL debug destination. The same twelve Save pairs still match,
and the [query file-sink matrix](parity/generated_audit_credential_mutation_query_matrix_v2_5_0_6.json.gz)
still passes twelve cases.

All four ordinary files, and both authorized debug files, contain the identical
safe UPDATE statement:

```sql
UPDATE school_data SET password = '[REDACTED]' /* masked */, version = 4 WHERE id = 7 AND version = 3
```

Neither the initial `PRIVATE-PASSWORD-CANARY` nor the updated
`UPDATED-PASSWORD-CANARY` appears in any of those files. With acknowledgement
unset, neither language writes a debug file. The verifier checks
the complete UPDATE line so an unrelated redaction marker cannot satisfy the
mutation assertion. This checkpoint establishes the stated SQL and privacy
invariants; it does not assert byte identity for every emitted log line.

The final receipt records **45** successful steps, four mutation file cases,
**533** unchanged inputs and removal of its **397,975,998-byte** owned build
and logs. The preceding [successful exploratory run](parity/generated_audit_credential_mutation_sinks_current_5_0_6.json.gz)
found the same UPDATE text, but its verifier checked only a generic redaction
marker; the final run adds the exact SQL assertion.

## Current receipt admission

The independent [receipt verifier](../tests/parity/verify_generated_credential_mutation_receipt_5_0_6.py)
now checks that all 533 execution inputs and 59 resolved Java dependencies
still match the current source epoch. It binds the 45 terminal commands,
compiled generated descriptors, twelve exact Save pairs, four mutation files
and the hashed twelve-case query-file receipt. It separately checks the
committed database state after delivery failure, absence of precommit audit
events, forced safe-event masking and exact masked SQL in ordinary and
authorized debug files.

Its [admission receipt](parity/generated_credential_mutation_receipt_controls_v4_5_0_6.json.gz)
rejects **14/14** controlled corruptions, including changed source hashes,
altered Save readback, unmasked safe fields, leaked file values and a debug
file without acknowledgement. The check is registered as
`generated_credential_mutation_receipt_contracts`; the selected native
CTest passes **1/1** under the **1,059-test** registry. This is current-source
admission of the retained generated execution, not a new SQL run or complete
G14/G17/P7 acceptance.

To reproduce from the repository root, activate the locked toolchain and
choose fresh receipt paths:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_audit_sql_sinks_5_0_6.py \
  --source-case audit_value_max_len_0_then_assignment \
  --mask-declaration credential_field \
  --save-field password \
  --record /tmp/audit_credential_run.json.gz \
  --sink-record /tmp/audit_credential_sinks.json.gz
```

This proves one modeled credential field, one SQLite query shape, the specified
password and name Save paths, six query sink settings per language and two
mutation sink settings per language. Arbitrary credential fields, custom
sinks, full AP02/G14/G17 and P0-P7 acceptance remain open.
