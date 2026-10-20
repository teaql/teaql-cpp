# Field Assist: entry.moment

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `moment()` / `updateMoment(const Timestamp&)`; changed `changedMoment()`; evaluation `evalMoment()`; expression `getMoment()`.
- Scalar predicates: `withMomentIs`, `withMomentIsNot`, `withMomentGreaterThan`, `withMomentGreaterThanOrEqualTo`, `withMomentLessThan`, `withMomentLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withMomentBetween(lower, upper)`, `withMomentIn(std::span<const Timestamp>)`, `withMomentNotIn(std::span<const Timestamp>)`, `withMomentIsNull()`, `withMomentIsNotNull()`.
- Projection/order/group: `selectMoment()`, `orderByMomentAsc()`, `orderByMomentDesc()`, `groupByMoment()`.
- Group/count: `groupByMoment()`, `groupByMomentAs(alias)`, `groupByMomentWithFunction(alias, function)`, `countMoment()` and `countMomentAs(alias)`.
- Scalar aggregate: `sumMoment()` uses alias `"sum_moment"`; `sumMomentAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgMoment()` uses alias `"avg_moment"`; `avgMomentAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minMoment()` uses alias `"min_moment"`; `minMomentAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxMoment()` uses alias `"max_moment"`; `maxMomentAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withMomentIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withMomentIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
