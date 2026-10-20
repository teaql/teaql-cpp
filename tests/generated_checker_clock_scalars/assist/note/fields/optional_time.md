# Field Assist: note.optional_time

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalTime()` / `updateOptionalTime(const std::optional<String>&)`; changed `changedOptionalTime()`; evaluation `evalOptionalTime()`; expression `getOptionalTime()`.
- Candidate filters: `withOptionalTimeIsTimeCreateFunctionNow()` and `withOptionalTimeIsNotTimeCreateFunctionNow()`; exact value `time(createFunction=now`.
- Candidate filters: `withOptionalTimeIsUpdateFunctionNow()` and `withOptionalTimeIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalTimeIs`, `withOptionalTimeIsNot`, `withOptionalTimeGreaterThan`, `withOptionalTimeGreaterThanOrEqualTo`, `withOptionalTimeLessThan`, `withOptionalTimeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalTimeBetween(lower, upper)`, `withOptionalTimeIn(std::span<const String>)`, `withOptionalTimeNotIn(std::span<const String>)`, `withOptionalTimeIsNull()`, `withOptionalTimeIsNotNull()`.
- Projection/order/group: `selectOptionalTime()`, `orderByOptionalTimeAsc()`, `orderByOptionalTimeDesc()`, `groupByOptionalTime()`.
- Group/count: `groupByOptionalTime()`, `groupByOptionalTimeAs(alias)`, `groupByOptionalTimeWithFunction(alias, function)`, `countOptionalTime()` and `countOptionalTimeAs(alias)`.
- Scalar aggregate: `sumOptionalTime()` uses alias `"sum_optional_time"`; `sumOptionalTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalTime()` uses alias `"avg_optional_time"`; `avgOptionalTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalTime()` uses alias `"min_optional_time"`; `minOptionalTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalTime()` uses alias `"max_optional_time"`; `maxOptionalTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalTimeContaining(text)`, `withOptionalTimeStartingWith(text)`, `withOptionalTimeEndingWith(text)`.

Null alias: `withOptionalTimeIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalTimeIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withOptionalTimeNotContaining(value)`.

Text predicate: `withOptionalTimeNotStartingWith(value)`.

Text predicate: `withOptionalTimeNotEndingWith(value)`.

Text predicate: `withOptionalTimeSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
