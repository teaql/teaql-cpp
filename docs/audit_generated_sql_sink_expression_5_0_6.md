# Generated audit expression through SQLite and log sinks

A set of three source-derived audit expressions now passes through fresh locked
Rust and C++ generation into compiled entity descriptors, an audited SQLite
Save, safe audit projection, an actual SQLite query, safe SQL diagnostics and
ordinary/debug file sinks. This joins three AP02 metadata inputs to bounded AP05/AP07/AP08/AP09
consumers. It does not close G14, G17 or P0-P7.

The locked Rust runtime is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`;
the original generator is `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Model and generated descriptors

The [probe model](../tests/parity/inheritance_audit_mask_expression_5_0_6.xml)
is an exact copy of the existing inheritance graph model with one School
attribute added. Its `_audit_value_max_len` text is the
`cast_audit_value_max_len_mask` input from the locked-source nested-item corpus:
`17, audit_value_max_len=17 as usize, audit_mask_fields="name"`. The existing
School `name` mask remains. No original model or checked-in SDK is edited.

Fresh complete producer builds generate **44 original Rust files** and **95 C++
files**. Both compiled observers report the same School, Academy and
ResearchAcademy descriptors (**3/3**). School has effective audit length `17`,
mask `name`, `school_data` and the SQLite service. The inherited descriptors
retain their own expected null length and `name` mask. The C++ observer destroys
its `UserContext` and checks zero live user bytes.

Both observers pass a synthetic 21-character `name` value through the actual
`RawAuditEvent::build_safe_event` implementation with the generated School
descriptor. The output matches exactly: masked and truncated, 17 characters,
with `_audit_mask_fields` and `_audit_value_max_len` reasons. The synthetic
raw value is absent from the safe field. The persisted Save below independently
checks that projection through the application safe event sink.

The verifier also derives two temporary models from the same unchanged
inheritance model and locked nested-item source corpus. It substitutes only the
School `_audit_value_max_len` declaration, generates complete SDKs for each
model, and checks the actual safe field rather than treating metadata as the
consumer result:

| Locked source case | Effective length | Safe `name` output | Truncated |
| --- | ---: | --- | --- |
| `cast_audit_value_max_len_mask` | 17 | `PR*****...*****RY` | Yes |
| `sequence_boundary_9` | 19 | `PR******...******RY` | Yes |
| `audit_value_max_len_0_then_assignment` | 21 | `PR*****************RY` | No |

The last case assigns the length twice; the final assignment governs the
generated descriptor and persisted safe event. The School name mask comes from
the unchanged model in all three cases. Every safe field retains raw length
`21`, mask provenance and the correct output length; only the truncated cases
carry `_audit_value_max_len` as a truncation reason. All three pairs also pass
the same twelve Save outcomes and twelve fresh-process query file-sink cases.

## Audited persistence and safe event delivery

Both generated School facades load row `7`, update its `name` to the public
synthetic value and invoke audited ledger Save through the SQLite provider.
The graph variants also load Enrollment `21`, update its `note`, and explicitly
include its pending mutation in the School save boundary. The twelve
original/C++ result pairs match exactly:

| Save mode | Result | SQLite readback | Safe event sink |
| --- | --- | --- | --- |
| Success | Save succeeds | New name, version `4` | One `School` update |
| Write trigger rejects update | Save fails | Original name, version `3` | No event |
| Safe sink returns an error | Save reports error | New name, version `4` | One attempted update |
| Application policy denies `School.saveGraph` | Policy error | Original name, version `3` | No event |
| Loaded version becomes stale | Optimistic lock conflict | Original name, externally raised version `4` | No event |
| Application policy explicitly approves `School.saveGraph` | Save succeeds | New School name, version `4`; Enrollment unchanged | One approved `School` update |
| School and Enrollment graph save | Save succeeds | School and Enrollment each reach version `4` with new values | One `School` and one `Enrollment` update |
| Application policy denies the two-operation graph plan | Policy error | Both rows retain original values and version `3` | No event |
| Application policy explicitly approves the two-operation graph plan | Save succeeds | Both rows reach version `4` with new values | Two approved entity updates |
| School write trigger rejects the graph update | Save fails | Both rows retain original values and version `3` | No event |
| Enrollment version becomes stale | Optimistic lock conflict | School stays at version `3`; Enrollment's external version is `4` | No event |
| School version becomes stale | Optimistic lock conflict | School's external version is `4`; Enrollment stays at version `3` | No event |

The successful and sink-error School events contain the same safe `name` field as the
direct projection: masked, truncated from 21 to 17 characters, with both
source reasons. The raw value is absent from that field. The sink-error case
also proves that an audit delivery error after commit must not be treated as a
rolled-back write in this route.

The four policy modes each record one review of `School.saveGraph`. The two
single-entity plans contain one operation; both graph plans contain two.
The approved modes install an explicit approval provider. The single-entity
safe event reports `Approved`; the approved graph Save emits `School` and
`Enrollment` events, both marked `Approved`. The unapproved graph Save also
persists both changed rows and emits two entity events. Denying the whole
graph plan leaves both rows unchanged and emits no event.

The locked Rust executor sorts update batches by entity name, and the C++
executor uses an ordered map for the same batches. Enrollment therefore
precedes School in this fixture. A School write trigger fails the later update;
both implementations report the specific fixture error and leave the earlier
Enrollment change uncommitted. Staling School's version similarly leaves
Enrollment unchanged, while staling Enrollment prevents the first update.
Both implementations name the expected entity in each optimistic conflict.
All three graph failures emit no event.
Other modes have no application Policy. This covers one explicit
pending-mutation composition, not arbitrary relation graphs or custom sink
combinations. The database row retains its full business value; the audit
projection is the privacy boundary checked here.

## Actual query and publication

The generated School facade executes a `name LIKE` query against SQLite using
the public synthetic value `PRIVATE-SCHOOL-CANARY`. Original Rust and C++
return the same `[[7, 3, null]]` row and matching full query, safe SQL and
trace records. The SQL uses parameter markers; safe diagnostics mask the
binding, redact the marked intent text and label the rendered statement
`MASKED; NOT REPLAYABLE`.

The [first file-sink matrix](parity/generated_audit_default_model_matrix_5_0_6.json.gz)
starts a fresh process for each of six settings in each language: unset
permission, invalid permission, exact permission, missing debug destination,
ordinary endpoint off and query logging disabled. All **12/12** pass. Ordinary
files and Context SQL records never contain the canary. The debug file contains
it only when the exact acknowledgement, explicit debug destination, enabled
ordinary endpoint and query logging are present together. These file checks
cover the generated query. The audited Save's application safe-event sink is
checked separately above. Credential, arbitrary custom-sink and other
publication configurations have independent requirements.

## Reproduction and scope

The [first complete receipt](parity/generated_audit_default_model_current_5_0_6.json.gz)
records all **41** successful source, generation, build and execution steps;
**532** unchanged input hashes and unchanged dependency JAR hashes. Its strict
original/C++ comparator checks compiled descriptors, twelve audited Save modes and
safe event delivery, direct projection and full query records. The owned
**397,069,078-byte** build and the sink verifier's **24,468-byte**
temporary file set were removed after success. The Rust query observer and C++
query observer are existing probes adapted only inside the owned build; the
descriptor and Save observers are retained as small source fixtures. Production
runtime, producer implementation, checked-in SDKs and module exports are
unchanged. A preceding compile-only failure from an ambiguous C++
`DataServiceExecutor` name is retained in its
[diagnostic receipt](parity/generated_audit_save_sql_sink_current_5_0_6.json.gz);
the generated type is explicitly qualified in the successful run.
An intermediate C++ observer constructor error is retained in the
[second diagnostic receipt](parity/generated_audit_graph_approval_current_5_0_6.json.gz).

The [length-19 receipt](parity/generated_audit_sequence_boundary_current_5_0_6.json.gz)
and [its sink matrix](parity/generated_audit_sequence_boundary_matrix_5_0_6.json.gz),
plus the [length-21 receipt](parity/generated_audit_last_assignment_current_5_0_6.json.gz)
and [its sink matrix](parity/generated_audit_last_assignment_matrix_5_0_6.json.gz),
each contain **41** successful steps, **44/95** freshly generated files,
**532** unchanged input hashes, **12** exact Save pairs and **12** file-sink
cases. Their owned builds of **397,133,450** and **397,123,929** bytes and both
**24,468-byte** sink workspaces were removed. Across the three independently
generated models, this is **36** Save pairs and **36** file-sink cases. Their
receipts retain the exact source case, derived model hash and original/C++
observations separately. All three were produced by the same verifier source
SHA-256. None changes the retained model fixture.

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_audit_sql_sinks_5_0_6.py \
  --record docs/parity/generated_audit_default_model_current_5_0_6.json.gz \
  --sink-record docs/parity/generated_audit_default_model_matrix_5_0_6.json.gz

python3 scripts/verify_generated_audit_sql_sinks_5_0_6.py \
  --source-case sequence_boundary_9 \
  --record docs/parity/generated_audit_sequence_boundary_current_5_0_6.json.gz \
  --sink-record docs/parity/generated_audit_sequence_boundary_matrix_5_0_6.json.gz

python3 scripts/verify_generated_audit_sql_sinks_5_0_6.py \
  --source-case audit_value_max_len_0_then_assignment \
  --record docs/parity/generated_audit_last_assignment_current_5_0_6.json.gz \
  --sink-record docs/parity/generated_audit_last_assignment_matrix_5_0_6.json.gz
```

The verifier requires fresh receipt paths on rerun. Complete AP02 grammar,
other modeled configurations and graph/approval combinations, arbitrary
sinks, the pending column-attribute compatibility decision and full
G13/G14/G17/P0-P7 acceptance remain open: **14 qualified / 3 partial**
capabilities.
