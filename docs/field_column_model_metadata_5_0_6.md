# Field-column model input against Rust 5.0.6

Status: FC01-FC04 are reconciled through actual original-source and current
native observations, permanent Java regression and scoped gates. Complete
G14, SQ02 and P0-P7 signoff remain separate.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. This review follows the
[default container continuation](default_model_literal_metadata_5_0_6.md).
It checks field-column input before assuming the container's rendering fix also
belongs in the field decoder.

## Responsibilities and actual observations

| ID | Responsibility |
| --- | --- |
| FC01 | Parse the model language before interpreting generated Rust literals. The locked `BaseDomainManager.typeRefer` pattern does not match these multiline inline definitions. Their column setting is absent and the default member name is used. |
| FC02 | Preserve the earlier model parser's escape and delimiter handling. `Parser.splitToPair` consumes backslashes; `parseAttributes` splits whitespace and braces. A raw model spelling that resembles a Rust escape does not necessarily reach Rust as that escape. |
| FC03 | Preserve the resulting column as a JSON input alias through ordinary and inherited generated entities. Alias input, canonical input and omitted input retain accepted status, loaded state and actual payload separately. |
| FC04 | Keep physical SQL identifiers independent of JSON input aliases. The approved C++ quoted-FK behavior is unchanged. This cohort executes JSON hydration without executing SQL or authorizing persistence. |

The [eight model inputs](../tests/parity/column_model_inputs_5_0_6.json.gz)
have these actual Item and inherited Child columns:

| Raw inline column value | Original generated column |
| --- | --- |
| `business_name` | `business_name` |
| `business` + CR + `name` | `name` |
| `business` + CRLF + `name` | `name` |
| `business\rname` | `businessrname` |
| `business\nname` | `businessnname` |
| `business\x5fname` | `businessx5fname` |
| `business\u{5f}name` | `businessu` |
| `business` + backslash + CRLF + spaces + `name` | `name` |

These are observations of the locked model grammar, rather than proposed
application conventions. Platform retains its ordinary `name` column. The
source parser's behavior explains why adding CR removal to
`CppJsonInputMetadata.column` is unnecessary for this cohort. The proposed
production change was never installed; all producer implementation files and
all generated SDKs remain unchanged.

## Original execution and observer correction

The [complete original generation and compilation receipt](parity/column_model_current_5_0_6.json.gz)
retains all eight models, complete original entity sources, resolved dependency
identities, Rust observer sources and raw stdout. Every original generation,
build and execution succeeds. Each model installs its three generated entity
types through the actual RuntimeModule and decodes three input views per type.

That receipt remains **incomplete overall**: the C++ observer incorrectly used
`operator->` on TeaQL's Result type. The two-case preliminary receipt and the
unused candidate's eight-case receipt retain the same observer failure. Their
compiler errors are probe defects, not runtime defects or passing native gates.

The [corrected native continuation](parity/column_model_current_native_5_0_6.json.gz)
rehydrates the exact archived generated C++ files, checks their hashes against
both source epochs, and changes only the Result accessor and owned include
paths in the observer before pinned formatting. It reuses the hash-checked
original execution; it does not claim another Rust run. Fresh C++ compilation
and execution match **8 models / 24 columns / 72 JSON hydration outcomes**.
Default PMR allocation is disabled, every UserContext is actually destroyed,
and the global live-user byte count returns to zero.

## Permanent regression

The new source-owned Java resource records all 24 column values from the actual
original output, including the model grammar's defaulting and escape behavior.
The existing column test class parses those complete models through the real
model manager and checks ordinary and inherited fields. Its
[negative control](parity/column_model_regression_controls_5_0_6.json.gz)
passes five tests with the current helper; a member-only column adapter fails
exactly the new test. Existing direct literal and C++ view tests remain active.

The [installed source gate](parity/column_model_installed_metadata_5_0_6.json.gz)
passes **63,918/63,918** original/current metadata outcomes and **134/134** Java
tests. The new model resource is an explicit fingerprinted input. These full
model observations are separate from the audit-value corpus count.

The [native result controls](parity/column_model_native_controls_5_0_6.json.gz)
reject 29 type, shape and payload corruptions of the actual output. Ordinary
Python equality accepts the Boolean/number diagnostic controls; that is a
counterexample to weak equality, not a claim of an archived verifier defect.

The [scoped gate](parity/column_model_gates_5_0_6.json.gz) passes two existing
native JSON boundary/column tests, two imported-module hydration/column tests,
all seventeen source guards, pinned format/check and locked-reference/path
checks. This is a scoped current run, rather than another complete native,
sanitizer or MIPS run.

The [exact input delta](parity/column_model_installed_delta_5_0_6.json.gz)
checks four regression/corpus/verifier changes before status reconciliation.
All producer implementation files remain unchanged. Byte/hash checks explicitly
reuse the preceding installed 80-SDK generation: 3,560 generated files, 2,467
protected artifacts and all manifests retain their identities. The isolated
unused candidate's SDK replay retains its own epoch and is not substituted
for current production evidence. All 1,813 C++ inputs retain the preceding
full-native identities. Historical receipts keep their original source/parser
identities; the later regression adds verification rather than changing them.

The [milestone](parity/column_model_milestone_5_0_6.json.gz) joins the finite
qualification and the [cleanup receipt](parity/column_model_cleanup_5_0_6.json.gz)
verifies removal of exact probe-owned outputs. Failed observer receipts remain
preserved, and unmeasured removed bytes are reported separately.

## Remaining scope

This cohort qualifies the named inline field-column inputs and actual JSON
consumers. It does not accept every field configuration, complete delegated
Rust grammar, protected publication or whole provenance. Source status remains
122 accepted / 0 pending / 2 external exclusions; capabilities remain
**14 qualified / 3 partial**. Complete G13/G14/G17 and every P0-P7 exit remain
required. Approved JSON, quoted-FK and invalid wire-profile exceptions retain
their existing scope.
