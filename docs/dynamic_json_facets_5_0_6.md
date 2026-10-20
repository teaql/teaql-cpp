# Simultaneous readonly/persistent JSON queries and generated facets

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This qualifies the recorded Order/Person composition and its source limitations.
It does not close complete G08/G10/G16/G17 or the full P5/P7 requirements.

## Actual original and C++ behavior

The existing SQLite fixture and trusted DatabaseDynamicFieldsProvider are used
through both real runtimes. Root Order requests always include the trusted
readonly `_computed = id + 7` property alongside empty, all or typed `title`
persistent selection. JSON criteria are applied after those trusted controls.
The root projection may be full or minimal.

Customer facets use the original generated Order-to-Person relation. They vary
ordinary, readonly, persistent, combined readonly/persistent and deliberately
wrong persistent type, plus a no-facet control. Include-all and relation-restricted
facets, allow/restrict/slice/deny policies, list/one/stream/count/exists/page and
empty/untrusted-override/matched/missing JSON criteria cross the root controls.
Logging is enabled for this matrix; the preceding persistent matrix separately
qualifies independent logging switches. No new local filtering, SQL execution or
hydration implementation is used.

All **6,336/6,336** actual original/native observations agree. There are **3,672**
successes, **1,584** policy denials and **1,080** matching SQLite subquery errors.
Full query descriptions, root ID/version/load states, persistent Value and
readonly Value Debug, compact facet rows/read metadata, before/after policy calls,
SELECT authorizer observations and safe SQL diagnostics are retained.

There are **1,104** loaded root title snapshots (**504** Text, **600** NULL) and
**1,656** root readonly snapshots. Simultaneous root loading is successful;
unselected fields remain unavailable. **720** returned lists/pages contain facet
maps, with **960** compact facet rows and **384** readonly facet values. One
executes facets before returning its optional entity; stream/count/exists retain
the original boundary and do not execute or attach facets.

**10,344** policy calls and **7,416** safe SQL records match exactly. None of the
**1,584** denied root requests reaches a SELECT authorizer callback. The JSON
attempts to replace `_computed`, `#title`, field definitions or selection cannot
install trusted definitions, SQL snippets or runtime capabilities.

## Source limitations are preserved, not labeled successes

The original generated facet helper fetches CompactRow lists. It does not run
the typed entity dynamic-field loader. Even a trusted persistent facet selection
therefore has no persistent read sidecar in the observed facet rows. The wrong
type child control is likewise not validated by a hydration step that never
executes. All **960** compact facet rows retain this original absence; it is not
converted into loaded NULL and is not proof of persistent facet hydration.
Readonly facet SQL projections still execute and appear in compact rows.

With include-all false, the original helper retains the outer readonly property
while forming the relation membership subquery. Clearing ordinary projections
does not clear that computed property, so affected SQLite attempts fail with
`sub-select returns 2 columns - expected 1`. The **1,080** exact original/C++ errors
are recorded as failures, including safe SQL diagnostics. They do not establish
successful restricted-facet execution for that query shape. Earlier ordinary
restricted-facet evidence remains separate.

The [source review](parity/dynamic_json_facets_source_review_5_0_6.json) retains
original outer/facet execution excerpts, complete file hashes and C++ mappings.
No production/runtime/generator function changed to manufacture these outcomes.
This source-based boundary is separate from the public recursive facet helper
and from any proposed future correction to upstream behavior.

## Original dependency replay and ownership

The entire retained 1,184-case generated-facet observer and its driver were copied
byte for byte into the independent reference-registry project. Its **1,184/1,184**
original/current-C++ results match the unchanged historical corpus. The
[copy record](parity/dynamic_json_facets_original_observer_copy_5_0_6.json)
checks original and copy hashes. Older producer/runtime dependencies and
historical observations are not silently replaced.

C++ disables the default PMR allocator. Provider, definitions, requests, results,
policy observations and SQLite resources belong to UserContext. Scope exit
reclaims the Context; existing Context/facet integration controls separately
exercise explicit User deletion, foreign binding rejection and Global copies.
The new observer does not claim every deferred replacement/cancellation scenario.
The existing exact quoted-Line descriptor Debug adaptation remains; no SQL,
result, error or parameter rewriting is added.

Safe Context logs are compared, but this ordinary generated domain has explicit
empty audit-mask lists. The matrix does not establish private-field plaintext
acknowledgement, credential redaction or actual sensitive-sink behavior. Those
privacy responsibilities remain explicit obligations of G14/G17 and final
integration rather than inferred from ordinary safe-log equality.

## Reproduction and remaining acceptance

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_dynamic_json_facets_probe teaql_generated_facets_probe
python3 scripts/verify_dynamic_json_facets_5_0_6.py
ctest --preset native --output-on-failure -R 'dynamic_json|^generated_facets_(rust_parity|context_contracts)$'
```

The new import target/test are `teaql_module_generated_dynamic_json_facets_probe`
and `module_generated_dynamic_json_facets_rust_parity`. The separate sanitizer
profile uses the native target/test names and disables leak scanning.
Native JSON/facet integration passes **13/13**, the new import check **1/1**
and the new ASan/UBSan check **1/1**, with leak scanning disabled. Format/check
covers **1,445** C++ files; **661** canonical consumer bodies and **1,485**
module exports verify.

See the [terminal record](parity/dynamic_json_facets_5_0_6_terminal.json) for
completed gates and raw outputs. Wider inherited/constant/candidate facet and
protected privacy configurations, complete cache/async responsibilities and final
integration remain in the [G10 matrix](dynamic_json_qualification_matrix_5_0_6.md)
and [full parity plan](cpp_generator_parity_plan.md). No full-suite, Java,
generation, MIPS or physical-device acceptance is inferred.
