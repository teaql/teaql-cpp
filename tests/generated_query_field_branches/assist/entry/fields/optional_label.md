# Field Assist: entry.optional_label

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalLabel()` / `updateOptionalLabel(const std::optional<String>&)`; changed `changedOptionalLabel()`; evaluation `evalOptionalLabel()`; expression `getOptionalLabel()`.
- Scalar predicates: `withOptionalLabelIs`, `withOptionalLabelIsNot`, `withOptionalLabelGreaterThan`, `withOptionalLabelGreaterThanOrEqualTo`, `withOptionalLabelLessThan`, `withOptionalLabelLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalLabelBetween(lower, upper)`, `withOptionalLabelIn(std::span<const String>)`, `withOptionalLabelNotIn(std::span<const String>)`, `withOptionalLabelIsNull()`, `withOptionalLabelIsNotNull()`.
- Projection/order/group: `selectOptionalLabel()`, `orderByOptionalLabelAsc()`, `orderByOptionalLabelDesc()`, `groupByOptionalLabel()`.
- Group/count: `groupByOptionalLabel()`, `groupByOptionalLabelAs(alias)`, `groupByOptionalLabelWithFunction(alias, function)`, `countOptionalLabel()` and `countOptionalLabelAs(alias)`.
- Scalar aggregate: `sumOptionalLabel()` uses alias `"sum_optional_label"`; `sumOptionalLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalLabel()` uses alias `"avg_optional_label"`; `avgOptionalLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalLabel()` uses alias `"min_optional_label"`; `minOptionalLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalLabel()` uses alias `"max_optional_label"`; `maxOptionalLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalLabelContaining(text)`, `withOptionalLabelStartingWith(text)`, `withOptionalLabelEndingWith(text)`.

Null alias: `withOptionalLabelIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalLabelIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withOptionalLabelNotContaining(value)`.

Text predicate: `withOptionalLabelNotStartingWith(value)`.

Text predicate: `withOptionalLabelNotEndingWith(value)`.

Text predicate: `withOptionalLabelSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
