# Field Assist: receipt.receipt_number

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `receiptNumber()` / `updateReceiptNumber(const ReceiptNumber&)`; changed `changedReceiptNumber()`; evaluation `evalReceiptNumber()`; expression `getReceiptNumber()`.
- Scalar predicates: `withReceiptNumberIs`, `withReceiptNumberIsNot`, `withReceiptNumberGreaterThan`, `withReceiptNumberGreaterThanOrEqualTo`, `withReceiptNumberLessThan`, `withReceiptNumberLessThanOrEqualTo`. Arguments: `const ReceiptNumber&`.
- Range/set/null predicates: `withReceiptNumberBetween(lower, upper)`, `withReceiptNumberIn(std::span<const String>)`, `withReceiptNumberNotIn(std::span<const String>)`, `withReceiptNumberIsNull()`, `withReceiptNumberIsNotNull()`.
- Projection/order/group: `selectReceiptNumber()`, `orderByReceiptNumberAsc()`, `orderByReceiptNumberDesc()`, `groupByReceiptNumber()`.
- Group/count: `groupByReceiptNumber()`, `groupByReceiptNumberAs(alias)`, `groupByReceiptNumberWithFunction(alias, function)`, `countReceiptNumber()` and `countReceiptNumberAs(alias)`.
- Scalar aggregate: `sumReceiptNumber()` uses alias `"sum_receipt_number"`; `sumReceiptNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgReceiptNumber()` uses alias `"avg_receipt_number"`; `avgReceiptNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minReceiptNumber()` uses alias `"min_receipt_number"`; `minReceiptNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxReceiptNumber()` uses alias `"max_receipt_number"`; `maxReceiptNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withReceiptNumberContaining(text)`, `withReceiptNumberStartingWith(text)`, `withReceiptNumberEndingWith(text)`.

Null alias: `withReceiptNumberIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withReceiptNumberIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
