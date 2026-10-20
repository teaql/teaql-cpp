# G10 dynamic JSON acceptance review

G10 is qualified for all fourteen requirements in the
[qualification matrix](dynamic_json_qualification_matrix_5_0_6.md), with the
explicit ownership, representation and source-failure dispositions below. References
remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The [preserved previous ledger](parity/dynamic_json_ledger_before_acceptance_5_0_6.json.gz)
retains every prior question and the complete matrix. Historical partial records
are not relabeled. The dispositions below reconcile their later evidence with
the complete source responsibilities.

## Complete source and branch review

The [complete body review](parity/dynamic_json_complete_source_review_5_0_6.json.gz)
retains eight request-template methods, four field/relation templates and five
core helpers, plus current native runtime and producer implementations. The
earlier eight actual generated Person bodies remain unchanged. Native shared
translation and the field recipes still match that earlier reviewed runtime;
the new producer repair affects Assist spelling only.

| ID | Requirement and complete disposition |
| --- | --- |
| DJ01 | **Parse and failure behavior.** Parse with the shared JSON implementation; non-object input is a no-op, duplicates follow the parser's last-key rule, malformed input retains its original text in the format error. The original 1,181 inputs and later scalar/model matrices retain actual parse outcomes. Native checked diagnostic ownership is explicit below. |
| DJ02 | **Scalar conversion and operators.** Core helpers handle NULL, Boolean, signed/unsigned/floating numbers, trimmed strings, nested arrays and recursive object `id` extraction. Null sentinels are recognized before trimming. First-string/first-object arrays select IN before the two-element BETWEEN case; other shapes select equality. The unchanged 872 helper observations now reproduce with reference registry versions/checksums, alongside 5,088 scalar/optional/temporal SQLite outcomes. |
| DJ03 | **Self fields and foreign-key aliases.** Scalar storage names and object member/FK aliases map through trusted generated recipes. Unknown names contribute no filter. Original ordinary, twenty-field scalar and five-model constant/candidate/Business-ID matrices compare actual query state, rows, parameters and errors. JSON does not resolve candidate codes or allocate IDs. |
| DJ04 | **Inherited self fields and paths.** Recipes use the original ancestor-first predefined-field collection. The existing five-collection repair preserves 2,656 full original constructions, including the 852 retained initial omissions; physical ancestor conditions and nullable forward paths have separate execution evidence. |
| DJ05 | **Forward/reverse one/many and recursion.** Split at the first dot, create the source-shaped minimal live child request, recurse and use the actual forward/reverse join keys. Unknown root paths do nothing; a recognized relation retains its child live filter even with an unknown tail. The ordinary, cyclic, incomplete and valid inherited reverse-path cases preserve those distinctions. |
| DJ06 | **Ordering.** A bare self-field string orders descending; an object accepts a string `field` and Boolean `useAsc`; arrays process object entries only. Unknown fields, dotted paths and malformed types are ignored. Existing orders are appended, not replaced. Source construction and physical ordering/page matrices cover each branch. |
| DJ07 | **Pagination.** `_start` replaces offset, `_size` replaces limit, `_pageSize` then overrides limit. A positive `_page` uses explicit page size, otherwise the existing limit, otherwise ten; subtraction/multiplication saturate. Wrong value types and page zero do not become valid controls. Original boundary and sequential inputs preserve precedence and existing slices. |
| DJ08 | **Sequential composition.** Criteria/orders accumulate while offset and limit remain independent. Non-object/unknown inputs do not reset prior state. Complete original query snapshots, pre-slice/unlimited controls and repeated `filterWithJson` calls establish the transformations. No local evaluator or SQL builder is substituted. |
| DJ09 | **Constants, candidates and Business IDs.** The same translation applies to authentic model field maps. The 5,910 actual source/native outcomes retain five models' query/row/error/log behavior. The original deterministic startup test key performs no allocation; native trusted assembly remains its declared Context adaptation. |
| DJ10 | **Execution, Policy and cleanup.** Translation hands the request to the existing execution services. The 3,072 Policy and 2,240 one/stream outcomes preserve rewriting, denial before protected I/O, item errors, early close and actual subsequent reuse. G09 owns the complete eight-entry execution protocols and explicit native stream-startup disposition. |
| DJ11 | **Allocation, diagnostics and binding.** Parsed JSON, nested expressions and query state use the bound scope. Native tests disable default PMR, copy into User/Global explicitly, actually delete the source, reject expired borrows and mutate a rebound Global query without retaining source memory. Format-error `what()` is a safe fixed prefix; retained text requires a live borrow or explicit destination copy. |
| DJ12 | **Thin generation and accurate guidance.** Recipes/delegates retain the existing runtime behavior; application methods use lowerCamelCase and canonical model/wire names remain unchanged. The Assist repair below restores all five JSON control keys and the regression fails on the previous producer. Strict reproduction and the complete 77-SDK comparison require every C++ code byte to remain identical. |
| DJ13 | **Physical inherited execution.** The 4,236 unchanged-original/native cases retain ancestor filters, optional/missing/deleted advisors, valid reverse paths, compact rows/count/exists and safe SQL. Construction panics remain failures before SQL; they are not successful empty results. |
| DJ14 | **Protected-storage/Policy composition and current integration.** Separate 9,216 provider-missing and 9,216 real-provider matrices preserve authority and selected Text/NULL/readonly data. The 6,336 simultaneous readonly/persistent/facet cases retain absent compact facet sidecars and original two-column subquery errors. The 1,728 private-field cases, actual file sinks and existing hot-cache/deferred execution evidence preserve the downstream boundary. JSON cannot install definitions, replace Policy or convert input into trusted capabilities. Independent storage, audit and cache algorithms retain their own requirements. |

The five core helpers have no model-, credential-, cache- or audit-specific
branch. Field/relation recipes supply the modeled names and keys; query values
are converted by the shared helpers. G01/G04/G05/G06/G07/G09/G11 retain their
accepted entity, field, inheritance, constant, query, execution and expression
contracts. No additional unbounded product of those same generic bodies is
introduced as a replacement for the source requirements.

## Reproduced Assist defect and bounded repair

The producer wrote the correct JSON keys before a final inline-code naming pass.
That pass changed `_orderBy`, `_start`, `_size`, `_pageSize` and `_page` into
`Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Those are different wire keys:
the runtime ignores them, so following the generated guidance could silently
leave sorting or pagination unchanged.

The existing protocol-preservation set now also preserves those five canonical
wire keys. The existing Java dynamic-JSON generation test asserts each exact key
while retaining its camelCase application/delegation assertions. It fails on
`_orderBy` before the repair and passes with the complete producer regression.
An initial JUnit assertion-argument ordering error is retained separately from
that intended red test.

The [pre-repair snapshot](parity/dynamic_json_assist_before_5_0_6.json.gz) preserves
both producer files, complete guides/provenance and every file hash in 77 SDKs.
Regeneration uses the real Java producer and pinned formatter. The bounded SDK
proof permits only those five token corrections on `Dynamic JSON:` lines and
updated source/file fingerprints; all C++ code and other instructions must stay
byte-identical. Generated artifacts are not patched by hand.

## Current source and integration evidence

Eleven complete original matrices reproduce **50,879** outcomes with the frozen
runtime's registry versions/checksums. The independent helper build reproduces
another **872**, totaling **51,751** retained source observations. Original
outputs and corpora remain unchanged; the source replay applies no representation
adapter. Its partial first attempt retains the incorrect composition binary-name
failure and all completed observations; the corrected continuation resumes from
that exact prefix rather than restarting it.

Native JSON/core/Context/facet integration passes **18/18**, after rebuilding its
15 exact executable targets. Imported modules pass **11/11**, after rebuilding
nine consumers. These gates replay the same eleven matrices through the existing
comparators, preserving their narrowly declared quoted-Line descriptor adaptation.
The native core helper/allocator/search tests and deferred-facet cleanup remain
separate controls rather than extra original-observation counts.

The current application compiler gate passes **2 positive / 1 negative** JSON
contracts. The unchanged frozen SQLite LIKE privacy suite passes **5/5** with
reference dependencies. Twelve fresh-process original/native sink scenarios
preserve exact acknowledgement, separate debug endpoint, endpoint-off and logging
switches; ordinary output never contains the private canary.

The original helper source is reused byte-for-byte through one added bin target
in the existing reference-compatible observation workspace. No dependency, lock,
production algorithm or historical corpus changes. The independent producer passes **71/71 Java tests**, including the intended
pre-repair failure and subsequent successful canonical-key regression. Strict
reproduction passes **24 normal workspaces / 288 files, 50 contract SDKs and all
three identity fixtures**. Across all 77 SDKs, exactly **63** Assist files change;
every C++ code byte remains identical. Source fingerprints are updated only
through actual regeneration. Formatting checks 1,641 handwritten C++ files.

The [acceptance record](parity/dynamic_json_acceptance_5_0_6.json.gz) retains all
current gates, source identities and prior-question dispositions. The first
strict-contract preflight rejected an invalid `--check --report` combination
before generation; its corrected strict run passes without update/report flags.
No unsuccessful preflight is counted as a successful gate.

## Explicit boundaries and remaining capabilities

This is the generated `filterWithJson` language. It is separate from the runtime's
bounded dynamic-search grammar, typed presentation JSON decoding and mutation
authorization. Unknown query fields retain the original ignore behavior; this
does not weaken the presentation decoder's distinct unknown-key checks.

Native value wrappers and checked exceptions replace consuming Rust requests
and panic ownership. They do not claim that a failed moved Rust request can be
reused. Source privacy observations exclude unstable wall-clock fields and retain
complete stable payloads; file bytes with timestamps are retained with explicit
sink invariants rather than claimed byte-identical between processes.

The exact existing quoted-Line descriptor Debug adaptation changes only that
metadata spelling; SQL, result, error, property identity and join keys are not
normalized. The original generated schema failure is replayed separately on an
empty database. Query matrices use the declared existing SQL fixtures and make
no successful bootstrap claim.

G08 relationship analysis/facet/cache algorithms, G12 complete assembly/sample/
Assist workflows and G13-G17 model, metadata/privacy, indexed state, storage and
trace responsibilities remain intact. In particular, credential/audit sinks and
durable storage/readonly authority are not declared wholly qualified by testing
their JSON query entry. P5/P6/P7 and the overall migration remain active. No new
sanitizer, MIPS, external-provider or physical-device result is implied.
