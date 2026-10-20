# Field Assist: note.timestamp

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `timestamp()` / `updateTimestamp(const Timestamp&)`; changed `changedTimestamp()`; evaluation `evalTimestamp()`; expression `getTimestamp()`.
- Candidate filters: `withTimestampIsDateTimeCreateFunctionNow()` and `withTimestampIsNotDateTimeCreateFunctionNow()`; exact value `dateTime(createFunction=now`.
- Candidate filters: `withTimestampIsUpdateFunctionNow()` and `withTimestampIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withTimestampIs`, `withTimestampIsNot`, `withTimestampGreaterThan`, `withTimestampGreaterThanOrEqualTo`, `withTimestampLessThan`, `withTimestampLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withTimestampBetween(lower, upper)`, `withTimestampIn(std::span<const Timestamp>)`, `withTimestampNotIn(std::span<const Timestamp>)`, `withTimestampIsNull()`, `withTimestampIsNotNull()`.
- Projection/order/group: `selectTimestamp()`, `orderByTimestampAsc()`, `orderByTimestampDesc()`, `groupByTimestamp()`.
- Group/count: `groupByTimestamp()`, `groupByTimestampAs(alias)`, `groupByTimestampWithFunction(alias, function)`, `countTimestamp()` and `countTimestampAs(alias)`.
- Scalar aggregate: `sumTimestamp()` uses alias `"sum_timestamp"`; `sumTimestampAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgTimestamp()` uses alias `"avg_timestamp"`; `avgTimestampAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minTimestamp()` uses alias `"min_timestamp"`; `minTimestampAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxTimestamp()` uses alias `"max_timestamp"`; `maxTimestampAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withTimestampIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withTimestampIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
