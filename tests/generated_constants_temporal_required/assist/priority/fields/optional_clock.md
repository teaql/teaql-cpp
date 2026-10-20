# Field Assist: priority.optional_clock

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalClock()` / `updateOptionalClock(const std::optional<String>&)`; changed `changedOptionalClock()`; evaluation `evalOptionalClock()`; expression `getOptionalClock()`.
- Candidate filters: `withOptionalClockIsValue084500()` and `withOptionalClockIsNotValue084500()`; exact value `08:45:00`.
- Candidate filters: `withOptionalClockIsValue000000()` and `withOptionalClockIsNotValue000000()`; exact value `00:00:00`.
- Scalar predicates: `withOptionalClockIs`, `withOptionalClockIsNot`, `withOptionalClockGreaterThan`, `withOptionalClockGreaterThanOrEqualTo`, `withOptionalClockLessThan`, `withOptionalClockLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalClockBetween(lower, upper)`, `withOptionalClockIn(std::span<const String>)`, `withOptionalClockNotIn(std::span<const String>)`, `withOptionalClockIsNull()`, `withOptionalClockIsNotNull()`.
- Projection/order/group: `selectOptionalClock()`, `orderByOptionalClockAsc()`, `orderByOptionalClockDesc()`, `groupByOptionalClock()`.
- Group/count: `groupByOptionalClock()`, `groupByOptionalClockAs(alias)`, `groupByOptionalClockWithFunction(alias, function)`, `countOptionalClock()` and `countOptionalClockAs(alias)`.
- Scalar aggregate: `sumOptionalClock()` uses alias `"sum_optional_clock"`; `sumOptionalClockAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalClock()` uses alias `"avg_optional_clock"`; `avgOptionalClockAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalClock()` uses alias `"min_optional_clock"`; `minOptionalClockAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalClock()` uses alias `"max_optional_clock"`; `maxOptionalClockAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalClockContaining(text)`, `withOptionalClockStartingWith(text)`, `withOptionalClockEndingWith(text)`.

Null alias: `withOptionalClockIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalClockIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withOptionalClockNotContaining(value)`.

Text predicate: `withOptionalClockNotStartingWith(value)`.

Text predicate: `withOptionalClockNotEndingWith(value)`.

Text predicate: `withOptionalClockSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
