# Complete presentation data sources: Rust 5.0.6

Status: complete `teaql-core/src/web.rs` (508 lines) and `xls.rs` (214 lines)
accepted under WP01-WP10 and the existing explicit C++ adaptations. Native
**3/3** and imported modules **1/1** pass. Both frontends replay all **785**
unchanged protocol observations. The preceding **675** complete API observations
are reused only after verifying their current input/executable hashes. The
actual original Web unit test also passes. Source status advances to
**24 accepted / 98 pending / 2 excluded_external**; G13/P7 remains open.

## Preserved models rather than external components

These files define portable data models, builders, projection and JSON behavior.
They do not implement a Web server, UI renderer or spreadsheet engine. Their
core concepts remain in the edge runtime even though external integrations are
excluded. The complete old/current Rust files are identical, and their C++
headers/implementations are unchanged from the current HEAD. Source equality
supports continuity; the complete responsibility review and current evidence
below provide acceptance.

## Complete source responsibilities

| ID | Original source contract | Mapping and evidence |
| --- | --- | --- |
| WP01 | Three constants and all seven model layouts | `style`, `actionList`, `1.001`, WebStyle, WebAction, WebResponse, XlsBlock, XlsBlockBuildContext, XlsPage and XlsWorkbook retain authentic fields, widths and wire keys. `requestURL` is preserved rather than inferred from ordinary camel casing. |
| WP02 | WebStyle defaults, three factories/setters, JSON and three binding paths | All three optional fields remain distinct from present empty strings. Entity binding calls the entity's dynamic override; base and Record bindings retain their own targets. The complete binding matrix and native override/retention contract exercise that dispatch. |
| WP03 | WebAction defaults, fifteen factories including common actions, nine setters plus optional-warning setter, JSON and bindings | Exact action strings/order and warning None/Some(empty) remain unchanged. The two private append routines preserve absent values, append to an existing JSON array, or retain an existing non-array as the first array element. Action entity binding directly mutates base data, unlike Style's override dispatch. |
| WP04 | All WebResponse constructors, projections, data/count/facet builders and JSON | success/fail/empty_list, records, single entity, entity iterators and SmartList preserve their distinct contracts. Single-entity projection requires allocator-aware copying; genuine owning iteration supports non-Clone entities. Named move-reference and value iterators remain owning. SmartList totals/taken facets remain distinct from data length; with_data/push recompute length while an explicit count override is retained. |
| WP05 | All XlsBlock fields, construction, region/span/value/property/style builders, queries and JSON | Span uses source saturating subtraction before coordinate addition. Reversed bounds are retained, contains is inclusive, property insertion replaces matching keys, and styles are recursive owner-local boxes. Existing native tests cover self-style snapshots and checked coordinate/navigation overflow. |
| WP06 | Complete context/page/workbook navigation and lookup | Constructor coordinates clamp at zero; raw deserialization preserves supplied negatives. next increments x, new_line resets x to zero, next_line restores start_x, and to_block uses current coordinates. add/push methods preserve order and duplicates; block_at/page choose the first match and report absence. |
| WP07 | Source Clone, equality, Default and Debug roles | Allocator-aware destination copies replace ordinary Clone; structural equality ignores owner identity. Explicit-owner constructors retain only source-defined defaults. WebResponse does not gain a claimed Rust Default trait. Complete raw Debug output, including nested boxes/JSON and nine formatting modes, stays separate from safe runtime diagnostics. |
| WP08 | Complete source JSON derive behavior | Required, optional, omitted, NULL, empty, unknown, sequence and numeric-bound cases retain original outcomes. Empty block properties may be omitted/defaulted; required fields do not gain defaults merely because a C++ constructor initializes them. In particular, a NULL block value can be omitted on serialization and then fail required-field decoding; universal roundtrip success is not invented. |
| WP09 | Context allocation and explicit retention | Native default-PMR-disabled contracts copy responses, nested JSON/maps/keys, actions, workbooks, boxed styles and raw Debug output into Global. They read the retained copies after actual source User destruction and check released User blocks. Borrowed input ranges retain their copy constraints; models do not extend Context lifetime. |
| WP10 | Complete original test and existing language adaptations | The original Web constructor/wire-shape test passes against the exact reference. XLS has no test module in this source file; existing direct navigation/workbook ports supplement the two full matrices. The current typed-JSON errors, explicit destinations, member-name collision mappings and checked arithmetic boundary retain the documented C++ contract. |

## Current evidence and deliberate reuse

The [native receipt](parity/presentation_source_native_5_0_6.json.gz) rebuilds and
runs presentation behavior, complete model ownership/API contracts and the
785-case protocol comparison. The [module receipt](parity/presentation_source_module_5_0_6.json.gz)
rebuilds the canonical imported protocol observer and runs the complete matrix.
Each also executes the actual original protocol observer with reference-lock
registry versions/checksums and compares all complete outputs without normalization.
Both end with a no-work build of their selected targets.

The separately qualified 675-case API matrix includes all seven derived model
types, full constructor/binding/range results and the six non-Clone owning
iterator controls. Its preceding native/module receipts are reused after every
recorded input and executable hash is verified. It is not described as a second
fresh replay. Its 145 trimming cases remain evidence for the separate
[helper acceptance](serde_source_acceptance_5_0_6.md), rather than additional Web/Xls
responsibilities. The [acceptance receipt](parity/presentation_source_acceptance_5_0_6.json.gz)
binds both matrices, source bodies, previous dispositions and current gates.

## Existing C++ boundaries

These are JSON-value adapters rather than a new general-purpose Serde framework.
Native structured decode failures preserve success/failure and field roles;
external Serde error strings and non-JSON formats are not newly claimed.
Consuming Rust builders map to existing mutable C++ builders. Setter/field name
collisions use established set_ names, and the context page factory is for_page.
Authentic wire identifiers and binding responsibilities remain unchanged.

Rust debug arithmetic panic maps to checked C++ overflow_error rather than
signed C++ undefined behavior. Span's saturating_sub and each remaining checked
addition/subtraction retain their separate order. Recovering a consumed Rust
value after panic is not a promise about a mutable C++ object after an exception.
The existing documented adaptation is retained rather than silently normalizing
coordinates, bounds, counts, missing data or errors.

The [historical whole-surface review](presentation_api_acceptance.md) and its
initial owning-iterator defect/repair remain historical. Its sanitizer/MIPS/full
suite totals are not relabeled as new runs. Current C++ bodies and lifetime
mechanics are unchanged, so this source-only qualification uses relevant native
and imported checks without repeating sanitizers or architecture profiles.

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_presentation_sources_5_0_6.py \
    --profile native --record /tmp/presentation_source_native.json.gz
python3 scripts/verify_presentation_sources_5_0_6.py \
    --profile modules_native --record /tmp/presentation_source_module.json.gz
```

No C++/SDK/compiler input, dependency or test registration changes. Native
registry736/imported-body875/export1506 remain structural counts. The separate
generated compiler-contract process is still running; it is not accepted by
this review. All other pending source, model, governance and P0-P7 obligations
remain open.
