# Field Assist: priority.optional_instant

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalInstant()` / `updateOptionalInstant(const std::optional<Timestamp>&)`; changed `changedOptionalInstant()`; evaluation `evalOptionalInstant()`; expression `getOptionalInstant()`.
- Candidate filters: `withOptionalInstantIsValue20020202t020304()` and `withOptionalInstantIsNotValue20020202t020304()`; exact value `2002-02-02T02:03:04`.
- Candidate filters: `withOptionalInstantIsValue20040404t040506()` and `withOptionalInstantIsNotValue20040404t040506()`; exact value `2004-04-04T04:05:06`.
- Scalar predicates: `withOptionalInstantIs`, `withOptionalInstantIsNot`, `withOptionalInstantGreaterThan`, `withOptionalInstantGreaterThanOrEqualTo`, `withOptionalInstantLessThan`, `withOptionalInstantLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withOptionalInstantBetween(lower, upper)`, `withOptionalInstantIn(std::span<const Timestamp>)`, `withOptionalInstantNotIn(std::span<const Timestamp>)`, `withOptionalInstantIsNull()`, `withOptionalInstantIsNotNull()`.
- Projection/order/group: `selectOptionalInstant()`, `orderByOptionalInstantAsc()`, `orderByOptionalInstantDesc()`, `groupByOptionalInstant()`.
- Group/count: `groupByOptionalInstant()`, `groupByOptionalInstantAs(alias)`, `groupByOptionalInstantWithFunction(alias, function)`, `countOptionalInstant()` and `countOptionalInstantAs(alias)`.
- Scalar aggregate: `sumOptionalInstant()` uses alias `"sum_optional_instant"`; `sumOptionalInstantAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalInstant()` uses alias `"avg_optional_instant"`; `avgOptionalInstantAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalInstant()` uses alias `"min_optional_instant"`; `minOptionalInstantAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalInstant()` uses alias `"max_optional_instant"`; `maxOptionalInstantAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withOptionalInstantIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalInstantIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
