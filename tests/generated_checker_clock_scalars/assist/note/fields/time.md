# Field Assist: note.time

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `time()` / `updateTime(std::string_view)`; changed `changedTime()`; evaluation `evalTime()`; expression `getTime()`.
- Candidate filters: `withTimeIsTimeCreateFunctionNow()` and `withTimeIsNotTimeCreateFunctionNow()`; exact value `time(createFunction=now`.
- Candidate filters: `withTimeIsUpdateFunctionNow()` and `withTimeIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withTimeIs`, `withTimeIsNot`, `withTimeGreaterThan`, `withTimeGreaterThanOrEqualTo`, `withTimeLessThan`, `withTimeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withTimeBetween(lower, upper)`, `withTimeIn(std::span<const String>)`, `withTimeNotIn(std::span<const String>)`, `withTimeIsNull()`, `withTimeIsNotNull()`.
- Projection/order/group: `selectTime()`, `orderByTimeAsc()`, `orderByTimeDesc()`, `groupByTime()`.
- Group/count: `groupByTime()`, `groupByTimeAs(alias)`, `groupByTimeWithFunction(alias, function)`, `countTime()` and `countTimeAs(alias)`.
- Scalar aggregate: `sumTime()` uses alias `"sum_time"`; `sumTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgTime()` uses alias `"avg_time"`; `avgTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minTime()` uses alias `"min_time"`; `minTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxTime()` uses alias `"max_time"`; `maxTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withTimeContaining(text)`, `withTimeStartingWith(text)`, `withTimeEndingWith(text)`.

Null alias: `withTimeIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withTimeIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withTimeNotContaining(value)`.

Text predicate: `withTimeNotStartingWith(value)`.

Text predicate: `withTimeNotEndingWith(value)`.

Text predicate: `withTimeSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
