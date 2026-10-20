# Nested audit metadata: actual generated Rust/C++ differences

Status: the shared generator now interprets sequential audit settings and keeps
effective runtime masks separate from model inheritance. The parser-level gate
matches **526/526** observations and the complete generated/compiler gate passes
**7/7** SDK pairs. All **78** existing SDKs reproduce without changing generated
content. G14/G17 and P0-P7 remain open. This work is independent of the qualified
[borrowed-root adapter](generated_runtime_state_bridge_5_0_6.md).

The initial witness below is historical defect evidence: seven complete SDK
pairs compiled and executed, but only **2/7** complete descriptor/privacy results
matched. Its five differences and complete sources remain immutable.

## Source and complete execution

The locked macro's `parse_container_attrs` walks nested metadata in order. A
callback can update the output before a later nested-meta error is ignored.
Looking only at the first audit-length literal misses both subsequent changes
and the distinction between parse termination and a recognized ignored value.

The [compiled witness](parity/audit_nested_metadata_compiled_initial_5_0_6.json.gz)
uses the unchanged frozen producer, actual generated entity sources, original
derive/runtime and exact reference dependency identities. It also generates
and compiles the current C++ counterparts against completed native archives.
Every source and native compiler exits zero. All **21** descriptors and **98**
field-policy tuples are retained, together with complete generated sources,
manifests, locks and diagnostics. This observes actual runtime metadata rather
than only parsing an isolated string.

| Audit value supplied to the model | Actual source behavior | Initial C++ behavior |
| --- | --- | --- |
| `17, audit_value_max_len=19` | Item limit becomes19 | Limit stays17 |
| `17, audit_value_max_len_int=19` | Item limit becomes19 | Limit stays17 |
| `17, audit_value_max_len=18446744073709551616` | The recognized later integer assignment clears the limit on overflow | Limit stays17 |
| `17, audit_value_max_len=false` | A recognized but inapplicable expression preserves17 | Matches |
| `17, other=0, audit_value_max_len=19` | Unknown metadata leaves its value unconsumed; parsing stops and preserves17 | Matches |
| `17, other, audit_value_max_len=19` | An unknown flag consumes no value, so parsing continues and assigns19 | Limit stays17 |
| `17, audit_mask_fields=""` | Item's runtime mask list becomes empty; its name field policy is Plain | Existing masks remain; the name field policy is Masked |

The distinction is not simply “last number wins.” A fix must preserve ignored
expressions, recognized assignments that clear the option, and the exact point
at which nested parsing stops. The existing lexical-rejection discrepancies in
[the literal review](generated_audit_literal_5_0_6.md) remain separate unfinished
work; this new successful-compilation matrix does not qualify them.

## Model blueprint versus effective runtime attributes

The mask case changes only the Item descriptor in these complete results.
Platform and Child descriptors remain equal across implementations. The frozen
generator resolves model inheritance before the Rust derive interprets the
injected container attributes. Mutating ObjectDescriptor's model mask attributes
while resolving Item would also change subsequent child generation and invent
a source behavior. The correction therefore needs a separate effective runtime
metadata result over the existing model blueprint.

Injected mask strings are interpreted by the macro, rather than sent back through
the generator's model-field name resolver. Source split/trim/order and declared
mask semantics must remain distinct from model-level mask inheritance and field
resolution. The witness records the observed policy difference; it does not
authorize a new privacy policy or accept arbitrary string normalization.

## Initial implementation requirements

Extend the shared generator metadata interpreter before changing emitted
adapters. It must represent sequential effective updates rather than only an
optional first literal, preserve the original inheritance/model objects, and
account for the source's lexical/error boundary. Then qualify source-derived
positive/negative controls, actual full SDK/descriptor execution and restricted
artifact reproduction before accepting the affected G14 responsibilities.

Only seven named cases were added to the existing development auditor. The
[before archive](parity/audit_nested_metadata_before_5_0_6.json.gz) preserves its
entire previous body. No producer source, runtime C++ body or checked-in generated
SDK changed in this witness. It introduces no dependency, database integration
or new native CMake registration. Source status remains **24 accepted / 98
pending / 2 excluded_external**; G14/G17 and P0-P7 remain open.

```bash
source scripts/toolchains/activate.sh
TMPDIR=/dev/shm python3 scripts/audit_generated_privacy_contracts_5_0_6.py \
    --cases nested_limit_override nested_integer_override \
            nested_overflow_override nested_boolean_preserves \
            nested_unknown_value_stops nested_unknown_flag_continues \
            nested_mask_override \
    --output /tmp/audit_nested_metadata_fresh.json.gz
```

The command records mismatches and exits zero unless `--require-match` is used.
Its terminal 2/7 summary is defect evidence, not a passing parity gate.

## Shared interpreter correction

The producer's `CppRustLiteralMetadata` now returns the effective audit limit and
an optional raw mask override. An absent override retains model-resolved masks;
an explicitly empty list clears only the generated runtime descriptor. The
adapter never mutates ObjectDescriptor or changes a child's model blueprint.
`CppCoreGenerator` emits the resulting descriptor through the existing runtime
metadata API; no runtime allocation, privacy algorithm or public API changes.

Recognized later integer/string assignments can replace or clear a limit.
Ignored expressions preserve the previous value. Unknown flags can continue to
the next metadata item; an unknown assigned value stops the source callback.
The `_int` spelling accepts integer expressions only. Raw mask lists preserve
source order, case and duplicates, with Rust string whitespace trimming rather
than lexer whitespace or Java's identifier/trim rules.

The [first repaired parser run](parity/audit_metadata_sequences_initial_repair_5_0_6.json.gz)
retains **11** rejection differences among **526** observations. After the
numeric-suffix/radix and unterminated-comment checks,
[the repaired run](parity/audit_metadata_sequences_repaired_5_0_6.json.gz) matches
all **514** valid-source outputs exactly and agrees on rejection for **12**
source-invalid inputs. Duplicate historical stimuli remain in the count. Full
language-specific errors and rejection stages are retained; rejection agreement
does not mean Rust compiler errors and Java generation errors are identical.

The checked-in observer imports the complete unchanged locked `attr.rs` and
checks its dependency registry identities against the Rust reference. Its Java
counterpart calls the actual compiled producer helper. Neither observer copies
the metadata interpretation algorithm. Reproduce with:

```bash
source scripts/toolchains/activate.sh
TMPDIR=/dev/shm python3 scripts/verify_audit_metadata_sequences_5_0_6.py \
    --require-match --output /tmp/audit_metadata_sequences_fresh.json.gz
```

This finite matrix does not establish the complete Rust expression/token grammar,
every other container-attribute override, target-width handling on MIPS, or
governed SQL/publication. AP02 and the remaining AP01-AP12 responsibilities retain
their full scope until separately reconciled.

## Complete generated descriptor verification

The [repaired SDK/compiler comparison](parity/audit_nested_metadata_compiled_repaired_5_0_6.json.gz)
passes **7/7** original/native pairs, retaining all **21** complete descriptors
and **98** field-policy tuples. Every Rust and C++ compiler exits zero before
execution. The original model text, complete generated Rust files and complete
runtime outputs are identical to the initial witness. All five reproduced
differences are removed, including Item's empty raw mask and Plain name policy;
Child's original inherited mask result is preserved.

These actual compiled pairs are distinct from the parser-only observations.
The native observer disables default PMR allocation and calls the existing
EntityDescriptor and SqlIntentRedactions APIs. It does not execute governed
business queries or assert publication parity for every logging configuration.

The [terminal Java gate](parity/audit_metadata_java_5_0_6.json.gz) passes **86/86**
tests across the full C++ generation, field-layout, JSON-input, literal and new
sequence test classes. The new generated fixture checks that an empty effective
mask does not mutate parent model attributes or the child's inherited mask.
It also retains centralized pluralization controls for `order_status`,
`category`, `person` and `inventory`.

## Existing artifacts and runtime consumers

The [restricted reproduction](parity/audit_metadata_sdk_refresh_5_0_6.json.gz)
generates every existing SDK into an independent temporary directory and requires
the complete file set and every generated byte to match before staging any
write. All **78** SDKs / **3,457** generated files match. Only producer
fingerprints in their manifests/provenance are refreshed; **2,201** protected
historical artifacts remain unchanged. The new `--refresh-provenance` mode
rejects every generated-code or guidance change, including changes that the
separate historical borrowed-root `--apply` mode permits.

The selected native regression passes **4/4**: generated API, audit-event,
private generated JSON execution and real file-sink privacy. Imported modules
pass **3/3**: the audit-event, private-query and file-sink counterparts. This
retains the existing complete 1,728-query privacy corpus and six sink controls
per profile. Runtime bodies and public APIs are unchanged; the earlier
55-positive/141-negative compiler result remains historical, while the seven
new SDK pairs above are freshly compiled. No full-suite, sanitizer or MIPS run
is claimed for this Java-only interpretation change.

The [terminal record](parity/audit_metadata_sequences_acceptance_5_0_6.json.gz)
joins these bounded gates. Source status stays **24 accepted / 98 pending /
2 excluded_external**. Complete G14/G17 and P0-P7 acceptance remains open.
