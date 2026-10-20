# Field Assist: priority.clock

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `clock()` / `updateClock(std::string_view)`; changed `changedClock()`; evaluation `evalClock()`; expression `getClock()`.
- Candidate filters: `withClockIsValue073000()` and `withClockIsNotValue073000()`; exact value `07:30:00`.
- Candidate filters: `withClockIsValue235959()` and `withClockIsNotValue235959()`; exact value `23:59:59`.
- Scalar predicates: `withClockIs`, `withClockIsNot`, `withClockGreaterThan`, `withClockGreaterThanOrEqualTo`, `withClockLessThan`, `withClockLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withClockBetween(lower, upper)`, `withClockIn(std::span<const String>)`, `withClockNotIn(std::span<const String>)`, `withClockIsNull()`, `withClockIsNotNull()`.
- Projection/order/group: `selectClock()`, `orderByClockAsc()`, `orderByClockDesc()`, `groupByClock()`.
- Group/count: `groupByClock()`, `groupByClockAs(alias)`, `groupByClockWithFunction(alias, function)`, `countClock()` and `countClockAs(alias)`.
- Scalar aggregate: `sumClock()` uses alias `"sum_clock"`; `sumClockAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgClock()` uses alias `"avg_clock"`; `avgClockAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minClock()` uses alias `"min_clock"`; `minClockAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxClock()` uses alias `"max_clock"`; `maxClockAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withClockContaining(text)`, `withClockStartingWith(text)`, `withClockEndingWith(text)`.

Null alias: `withClockIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withClockIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withClockNotContaining(value)`.

Text predicate: `withClockNotStartingWith(value)`.

Text predicate: `withClockNotEndingWith(value)`.

Text predicate: `withClockSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
