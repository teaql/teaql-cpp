# Field Assist: child_record.label

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `label()` / `updateLabel(const std::optional<String>&)`; changed `changedLabel()`; evaluation `evalLabel()`; expression `getLabel()`.
- Scalar predicates: `withLabelIs`, `withLabelIsNot`, `withLabelGreaterThan`, `withLabelGreaterThanOrEqualTo`, `withLabelLessThan`, `withLabelLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withLabelBetween(lower, upper)`, `withLabelIn(std::span<const String>)`, `withLabelNotIn(std::span<const String>)`, `withLabelIsNull()`, `withLabelIsNotNull()`.
- Projection/order/group: `selectLabel()`, `orderByLabelAsc()`, `orderByLabelDesc()`, `groupByLabel()`.
- Group/count: `groupByLabel()`, `groupByLabelAs(alias)`, `groupByLabelWithFunction(alias, function)`, `countLabel()` and `countLabelAs(alias)`.
- Scalar aggregate: `sumLabel()` uses alias `"sum_label"`; `sumLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgLabel()` uses alias `"avg_label"`; `avgLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minLabel()` uses alias `"min_label"`; `minLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxLabel()` uses alias `"max_label"`; `maxLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withLabelContaining(text)`, `withLabelStartingWith(text)`, `withLabelEndingWith(text)`.

Null alias: `withLabelIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withLabelIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withLabelNotContaining(value)`.

Text predicate: `withLabelNotStartingWith(value)`.

Text predicate: `withLabelNotEndingWith(value)`.

Text predicate: `withLabelSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
