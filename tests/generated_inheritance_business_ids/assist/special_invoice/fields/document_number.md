# Field Assist: special_invoice.document_number

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `documentNumber()` / `updateDocumentNumber(const DocumentNumber&)`; changed `changedDocumentNumber()`; evaluation `evalDocumentNumber()`; expression `getDocumentNumber()`.
- Scalar predicates: `withDocumentNumberIs`, `withDocumentNumberIsNot`, `withDocumentNumberGreaterThan`, `withDocumentNumberGreaterThanOrEqualTo`, `withDocumentNumberLessThan`, `withDocumentNumberLessThanOrEqualTo`. Arguments: `const DocumentNumber&`.
- Range/set/null predicates: `withDocumentNumberBetween(lower, upper)`, `withDocumentNumberIn(std::span<const String>)`, `withDocumentNumberNotIn(std::span<const String>)`, `withDocumentNumberIsNull()`, `withDocumentNumberIsNotNull()`.
- Projection/order/group: `selectDocumentNumber()`, `orderByDocumentNumberAsc()`, `orderByDocumentNumberDesc()`, `groupByDocumentNumber()`.
- Group/count: `groupByDocumentNumber()`, `groupByDocumentNumberAs(alias)`, `groupByDocumentNumberWithFunction(alias, function)`, `countDocumentNumber()` and `countDocumentNumberAs(alias)`.
- Scalar aggregate: `sumDocumentNumber()` uses alias `"sum_document_number"`; `sumDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDocumentNumber()` uses alias `"avg_document_number"`; `avgDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDocumentNumber()` uses alias `"min_document_number"`; `minDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDocumentNumber()` uses alias `"max_document_number"`; `maxDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withDocumentNumberContaining(text)`, `withDocumentNumberStartingWith(text)`, `withDocumentNumberEndingWith(text)`.

Null alias: `withDocumentNumberIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withDocumentNumberIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
