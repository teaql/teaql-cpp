# Field Assist: entry.optional_day

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalDay()` / `updateOptionalDay(const std::optional<Date>&)`; changed `changedOptionalDay()`; evaluation `evalOptionalDay()`; expression `getOptionalDay()`.
- Scalar predicates: `withOptionalDayIs`, `withOptionalDayIsNot`, `withOptionalDayGreaterThan`, `withOptionalDayGreaterThanOrEqualTo`, `withOptionalDayLessThan`, `withOptionalDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withOptionalDayBetween(lower, upper)`, `withOptionalDayIn(std::span<const Date>)`, `withOptionalDayNotIn(std::span<const Date>)`, `withOptionalDayIsNull()`, `withOptionalDayIsNotNull()`.
- Projection/order/group: `selectOptionalDay()`, `orderByOptionalDayAsc()`, `orderByOptionalDayDesc()`, `groupByOptionalDay()`.
- Group/count: `groupByOptionalDay()`, `groupByOptionalDayAs(alias)`, `groupByOptionalDayWithFunction(alias, function)`, `countOptionalDay()` and `countOptionalDayAs(alias)`.
- Scalar aggregate: `sumOptionalDay()` uses alias `"sum_optional_day"`; `sumOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDay()` uses alias `"avg_optional_day"`; `avgOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDay()` uses alias `"min_optional_day"`; `minOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDay()` uses alias `"max_optional_day"`; `maxOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withOptionalDayIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalDayIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Date predicates: `withOptionalDayBefore(value)`, `withOptionalDayAfter(value)` and `withOptionalDayBetween(from, to)`.

[Model/action index](../../../ASSIST.md)
