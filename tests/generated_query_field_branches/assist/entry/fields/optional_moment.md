# Field Assist: entry.optional_moment

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalMoment()` / `updateOptionalMoment(const std::optional<Timestamp>&)`; changed `changedOptionalMoment()`; evaluation `evalOptionalMoment()`; expression `getOptionalMoment()`.
- Scalar predicates: `withOptionalMomentIs`, `withOptionalMomentIsNot`, `withOptionalMomentGreaterThan`, `withOptionalMomentGreaterThanOrEqualTo`, `withOptionalMomentLessThan`, `withOptionalMomentLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withOptionalMomentBetween(lower, upper)`, `withOptionalMomentIn(std::span<const Timestamp>)`, `withOptionalMomentNotIn(std::span<const Timestamp>)`, `withOptionalMomentIsNull()`, `withOptionalMomentIsNotNull()`.
- Projection/order/group: `selectOptionalMoment()`, `orderByOptionalMomentAsc()`, `orderByOptionalMomentDesc()`, `groupByOptionalMoment()`.
- Group/count: `groupByOptionalMoment()`, `groupByOptionalMomentAs(alias)`, `groupByOptionalMomentWithFunction(alias, function)`, `countOptionalMoment()` and `countOptionalMomentAs(alias)`.
- Scalar aggregate: `sumOptionalMoment()` uses alias `"sum_optional_moment"`; `sumOptionalMomentAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalMoment()` uses alias `"avg_optional_moment"`; `avgOptionalMomentAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalMoment()` uses alias `"min_optional_moment"`; `minOptionalMomentAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalMoment()` uses alias `"max_optional_moment"`; `maxOptionalMomentAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withOptionalMomentIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalMomentIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
