# Field Assist: entry.day

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `day()` / `updateDay(const Date&)`; changed `changedDay()`; evaluation `evalDay()`; expression `getDay()`.
- Scalar predicates: `withDayIs`, `withDayIsNot`, `withDayGreaterThan`, `withDayGreaterThanOrEqualTo`, `withDayLessThan`, `withDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withDayBetween(lower, upper)`, `withDayIn(std::span<const Date>)`, `withDayNotIn(std::span<const Date>)`, `withDayIsNull()`, `withDayIsNotNull()`.
- Projection/order/group: `selectDay()`, `orderByDayAsc()`, `orderByDayDesc()`, `groupByDay()`.
- Group/count: `groupByDay()`, `groupByDayAs(alias)`, `groupByDayWithFunction(alias, function)`, `countDay()` and `countDayAs(alias)`.
- Scalar aggregate: `sumDay()` uses alias `"sum_day"`; `sumDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDay()` uses alias `"avg_day"`; `avgDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDay()` uses alias `"min_day"`; `minDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDay()` uses alias `"max_day"`; `maxDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withDayIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withDayIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Date predicates: `withDayBefore(value)`, `withDayAfter(value)` and `withDayBetween(from, to)`.

[Model/action index](../../../ASSIST.md)
