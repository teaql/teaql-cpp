# Field Assist: priority.instant

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `instant()` / `updateInstant(const Timestamp&)`; changed `changedInstant()`; evaluation `evalInstant()`; expression `getInstant()`.
- Candidate filters: `withInstantIsValue20010101t010203()` and `withInstantIsNotValue20010101t010203()`; exact value `2001-01-01T01:02:03`.
- Candidate filters: `withInstantIsValue20030303t030405()` and `withInstantIsNotValue20030303t030405()`; exact value `2003-03-03T03:04:05`.
- Scalar predicates: `withInstantIs`, `withInstantIsNot`, `withInstantGreaterThan`, `withInstantGreaterThanOrEqualTo`, `withInstantLessThan`, `withInstantLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withInstantBetween(lower, upper)`, `withInstantIn(std::span<const Timestamp>)`, `withInstantNotIn(std::span<const Timestamp>)`, `withInstantIsNull()`, `withInstantIsNotNull()`.
- Projection/order/group: `selectInstant()`, `orderByInstantAsc()`, `orderByInstantDesc()`, `groupByInstant()`.
- Group/count: `groupByInstant()`, `groupByInstantAs(alias)`, `groupByInstantWithFunction(alias, function)`, `countInstant()` and `countInstantAs(alias)`.
- Scalar aggregate: `sumInstant()` uses alias `"sum_instant"`; `sumInstantAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgInstant()` uses alias `"avg_instant"`; `avgInstantAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minInstant()` uses alias `"min_instant"`; `minInstantAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxInstant()` uses alias `"max_instant"`; `maxInstantAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withInstantIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withInstantIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
