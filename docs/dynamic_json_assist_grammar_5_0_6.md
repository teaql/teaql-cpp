# Dynamic JSON application guidance

This G12 checkpoint replaces an incomplete generated query summary with the
complete value/operator and ordering/paging rules used by Rust 5.0.6. It preserves
the G10 runtime acceptance and does not qualify all assembly, sample-data or
progressive Assist responsibilities. G12 and P0–P7 remain active.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Actual guidance defect

The old per-entity Assist said that strings become contains predicates and
two-element numeric arrays become ranges. This omitted null sentinels, the
precedence of IN over BETWEEN, nonnumeric ranges, fallback equality and object
conversion. Following that summary could produce a different condition from
the one intended by application code.

The producer now emits one shared **Dynamic JSON value and control rules**
section, linked from every entity's `filterWithJson(text)` guidance. This avoids
repeating the complete grammar in each entity section. Field/relationship names
remain canonical model names; application methods remain lowerCamelCase.

| Responsibility | Guidance and actual source behavior |
| --- | --- |
| Null sentinels | Exact `"__is_null__"` and `"__is_not_null__"` strings compare ASCII case-insensitively before trimming. Surrounding spaces select an ordinary contains predicate instead. |
| Text and scalars | Other strings select contains after value trimming; numbers and Booleans select equality. |
| Array precedence | A first string or object selects IN, even for two elements. Other two-element arrays select BETWEEN, including Boolean/NULL shapes. This is not a numeric-type check. |
| Fallback arrays | Other array lengths select equality using the first converted element. Empty arrays fail with the original missing-value error; they do not silently remove a filter. |
| Conversion | Objects recursively convert `id`, or NULL when absent. Nested arrays remain list values. Conversion does not ensure an operand is meaningful for a particular field or SQL operator. |
| Ordering | Bare self-field strings mean descending. Objects use `field` and Boolean `useAsc`, default false. Arrays process object entries only. Unknown/dotted ordering fields do nothing; existing orders remain. |
| Paging | Nonnegative integer controls apply in fixed order: offset, size, page size, page number. Page size overrides size. A positive page uses explicit page size, existing limit or ten, with saturating offset arithmetic. Page zero and invalid control types are ignored. |
| Composition and authority | Filters/orders accumulate; non-objects do nothing. Underscore-prefixed keys are reserved. Unknown fields add no restriction. Recognized relation paths preserve their live child query even with an unknown tail. Input does not supply intent, install trusted metadata or authorize writes. |

The last distinction matters for application validation: ignoring an unknown
filter field is the locked query behavior, not evidence that a filter was
enforced. This query language is separate from presentation JSON decoding and
its unknown-key rejection.

## Source and regression evidence

The [pre-change record](parity/dynamic_json_assist_grammar_before_5_0_6.json.gz)
preserves complete locked core/request-template bodies, current shared native
translation, both producer files, the prior ledger and every file hash in all
77 SDK fixture destinations. Prior G10 records and historical corpora are intact.

The [fresh observations](parity/dynamic_json_assist_grammar_observations_5_0_6.json.gz)
reproduce all **872** unchanged original/native helper outcomes. Another **17**
literal guide witnesses compare full original/native operator, converted value,
expression and error results. They include spaced sentinels, two strings, two
objects, Boolean/NULL ranges, equality fallback, empty-array failure, recursive
IDs and an object with an unrecognized operator key. No local operator evaluator
stands in for the original implementation.

The [Java regression record](parity/dynamic_json_assist_grammar_java_5_0_6.json.gz)
retains the intended failing test on the old producer and **69/69** subsequent
successful tests. The application-guidance regression requires the complete
rules and canonical controls, exactly one shared section and its entity links.
The producer edit adds only a guidance helper, its call and the summary
replacement; every other production Java byte is preserved.

The [regeneration record](parity/dynamic_json_assist_grammar_regeneration_5_0_6.json.gz)
records actual Java generation, pinned formatting and strict reproduction of
the 24 ordinary workspaces, 50 contract SDKs and three identity fixtures.
Its bounded whole-SDK proof permits only the shared section, linked summary and
updated provenance fingerprints. Generated C++ code must remain byte-identical;
generated artifacts are never edited by hand.

The [native integration record](parity/dynamic_json_assist_grammar_native_5_0_6.json.gz)
retains the completed build and **4/4** passing checks: helper contracts,
complete helper-source replay, generated JSON-source replay and generated
Context contracts. The build starts after all SDK writes and their complete
code-preservation proof; remaining strict reproductions use temporary directories.

The [terminal record](parity/dynamic_json_assist_grammar_terminal_5_0_6.json.gz)
reconciles the exact former ledger question with these observations and gates.
No production C++, generated C++ implementation, dependency, frozen source or
historical corpus changes. This guidance-only checkpoint requires no additional
sanitizer, module, MIPS, full-suite or physical-device claim.

G10 owns the complete translation and execution evidence. This change adds
accurate discovery guidance over that accepted runtime. The remaining G12
requirements still include the full model assembly/customization review,
dependency-aware sample-data responsibilities and other action/field/tool Assist
workflows. Independent storage, privacy, shared load state and request trace
requirements remain in their own ledger entries.
